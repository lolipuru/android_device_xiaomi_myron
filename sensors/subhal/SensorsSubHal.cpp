/*
 * Copyright (C) 2025 The LineageOS Project
 * SPDX-License-Identifier: Apache-2.0
 */

#include "SensorsSubHal.h"

#include <android-base/logging.h>
#include <android-base/stringprintf.h>
#include <dlfcn.h>
#include <time.h>
#include <unistd.h>

#include <algorithm>

using ::android::base::StringPrintf;
using ::android::hardware::sensors::V1_0::SensorFlagBits;
using ::android::hardware::sensors::V2_0::implementation::ScopedWakelock;
using ::android::hardware::sensors::V2_1::SensorInfo;
using ::android::hardware::sensors::V2_1::SensorType;
using ::android::hardware::sensors::V2_1::implementation::ISensorsSubHal;

namespace android {
namespace hardware {
namespace sensors {
namespace V2_1 {
namespace subhal {
namespace implementation {
namespace qsh_wrapper {

namespace {
constexpr auto kLibName = "sensors.qsh.so";
constexpr auto kRearLightType = "xiaomi.sensor.back_lux";
constexpr auto kTorchProp = "vendor.sensors.torch";
constexpr int64_t kTorchHoldoffNs = 1800LL * 1000000;

int64_t NowNs() {
    timespec ts;
    clock_gettime(CLOCK_BOOTTIME, &ts);
    return ts.tv_sec * 1000000000LL + ts.tv_nsec;
}
};  // anonymous namespace

SensorsSubHal::SensorsSubHal()
    : lib_handle_(dlopen(kLibName, RTLD_NOW), [](void* p) {
          if (p) dlclose(p);
      }) {
    if (!lib_handle_) {
        LOG(FATAL) << __func__ << ": dlopen " << kLibName << " failed, exiting";
    }

    auto get_sub_hal = reinterpret_cast<ISensorsSubHal* (*)(uint32_t*)>(
            dlsym(lib_handle_.get(), "sensorsHalGetSubHal_2_1"));
    uint32_t version;
    impl_ = get_sub_hal(&version);
}

Return<Result> SensorsSubHal::setOperationMode(OperationMode mode) {
    return impl_->setOperationMode(mode);
}

Return<Result> SensorsSubHal::activate(int32_t sensor_handle, bool enabled) {
    std::lock_guard<std::mutex> control_lock(control_mutex_);
    if (IsRearLight(sensor_handle)) {
        std::lock_guard<std::mutex> lock(mutex_);
        fw_rear_.active = enabled;
    } else {
        Result ret = impl_->activate(sensor_handle, enabled);
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = front_lights_.find(sensor_handle);
        if (it == front_lights_.end() || ret != Result::OK) return ret;
        it->second.active = enabled;
        it->second.has_lux = false;
    }
    return UpdateRearLight();
}

Return<Result> SensorsSubHal::batch(int32_t sensor_handle, int64_t sampling_period_ns,
                                    int64_t max_report_latency_ns) {
    std::lock_guard<std::mutex> control_lock(control_mutex_);
    if (IsRearLight(sensor_handle)) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            fw_rear_.sampling_period_ns = sampling_period_ns;
            fw_rear_.max_report_latency_ns = max_report_latency_ns;
            rear_batched_ = false;
        }
        return UpdateRearLight();
    }

    Result ret = impl_->batch(sensor_handle, sampling_period_ns, max_report_latency_ns);
    bool front = false;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = front_lights_.find(sensor_handle);
        if (it != front_lights_.end() && ret == Result::OK) {
            it->second.sampling_period_ns = sampling_period_ns;
            rear_batched_ = false;
            front = true;
        }
    }
    if (front) UpdateRearLight();
    return ret;
}

Return<Result> SensorsSubHal::flush(int32_t sensor_handle) {
    return impl_->flush(sensor_handle);
}

Return<void> SensorsSubHal::registerDirectChannel(const SharedMemInfo& mem,
                                                  ISensors::registerDirectChannel_cb _hidl_cb) {
    return impl_->registerDirectChannel(mem, _hidl_cb);
}

Return<Result> SensorsSubHal::unregisterDirectChannel(int32_t channel_handle) {
    return impl_->unregisterDirectChannel(channel_handle);
}

Return<void> SensorsSubHal::configDirectReport(int32_t sensor_handle, int32_t channel_handle,
                                               RateLevel rate,
                                               ISensors::configDirectReport_cb _hidl_cb) {
    return impl_->configDirectReport(sensor_handle, channel_handle, rate, _hidl_cb);
}

Return<void> SensorsSubHal::getSensorsList_2_1(ISensors::getSensorsList_2_1_cb _hidl_cb) {
    std::vector<SensorInfo> sensors;
    impl_->getSensorsList_2_1([&](const auto& list) { sensors = list; });

    std::lock_guard<std::mutex> lock(mutex_);
    float rear_max_range = 0;
    for (const auto& sensor : sensors) {
        if (sensor.typeAsString == kRearLightType && !(sensor.flags & SensorFlagBits::WAKE_UP)) {
            rear_handle_ = sensor.sensorHandle;
            rear_max_range = sensor.maxRange;
        }
    }
    if (rear_handle_ < 0) {
        LOG(WARNING) << "no " << kRearLightType << ", passing through";
    }
    for (auto& sensor : sensors) {
        if (sensor.type != SensorType::LIGHT || rear_handle_ < 0) continue;
        front_lights_[sensor.sensorHandle].wake_up = sensor.flags & SensorFlagBits::WAKE_UP;
        sensor.maxRange = std::max(sensor.maxRange, rear_max_range);
        LOG(INFO) << "fusing " << sensor.name << " with rear light sensor";
    }

    _hidl_cb(sensors);
    return Void();
}

Return<Result> SensorsSubHal::injectSensorData_2_1(const Event& event) {
    return impl_->injectSensorData_2_1(event);
}

Return<void> SensorsSubHal::debug(const hidl_handle& fd, const hidl_vec<hidl_string>& args) {
    impl_->debug(fd, args);
    if (fd.getNativeHandle() == nullptr || fd->numFds < 1) return Void();

    std::lock_guard<std::mutex> lock(mutex_);
    std::string out = StringPrintf("qsh_wrapper: rear 0x%x %s (fw: %d) lux=%.1f%s torch=%d\n",
                                   rear_handle_, rear_on_ ? "on" : "off", fw_rear_.active,
                                   rear_lux_, rear_valid_ ? "" : " (invalid)",
                                   IsTorchSuppressed());
    for (const auto& [handle, front] : front_lights_) {
        out += StringPrintf("  front 0x%x active=%d lux=%.1f sent=%.1f\n", handle, front.active,
                            front.lux, front.sent_lux);
    }
    write(fd->data[0], out.c_str(), out.size());
    return Void();
}

const std::string SensorsSubHal::getName() {
    return impl_->getName();
}

Return<Result> SensorsSubHal::initialize(const sp<IHalProxyCallback>& hal_proxy_callback) {
    hal_proxy_callback_ = hal_proxy_callback;
    return impl_->initialize(this);
}

Return<void> SensorsSubHal::onDynamicSensorsConnected(
        const hidl_vec<V1_0::SensorInfo>& sensor_infos) {
    return hal_proxy_callback_->onDynamicSensorsConnected(sensor_infos);
}

Return<void> SensorsSubHal::onDynamicSensorsDisconnected(const hidl_vec<int32_t>& sensor_handles) {
    return hal_proxy_callback_->onDynamicSensorsDisconnected(sensor_handles);
}

Return<void> SensorsSubHal::onDynamicSensorsConnected_2_1(
        const hidl_vec<V2_1::SensorInfo>& sensor_infos) {
    return hal_proxy_callback_->onDynamicSensorsConnected_2_1(sensor_infos);
}

void SensorsSubHal::postEvents(const std::vector<Event>& events, ScopedWakelock wakelock) {
    std::vector<Event> out, synth, synth_wake_up;
    out.reserve(events.size());
    {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto event : events) {
            if (rear_handle_ < 0) {
                out.push_back(event);
            } else if (event.sensorHandle == rear_handle_) {
                if (fw_rear_.active) out.push_back(event);
                if (event.sensorType == SensorType::META_DATA || !rear_on_) continue;
                rear_lux_ = event.u.data[0];
                rear_valid_ = true;
                // The front sensor is on-change, push the new fused value ourselves
                for (auto& [handle, front] : front_lights_) {
                    if (!front.active || !front.has_lux) continue;
                    float lux = FuseLux(front.lux);
                    if (lux == front.sent_lux) continue;
                    front.sent_lux = lux;
                    Event fused = event;
                    fused.sensorHandle = handle;
                    fused.sensorType = SensorType::LIGHT;
                    fused.timestamp = std::max(event.timestamp, front.timestamp + 1);
                    std::fill_n(fused.u.data.data(), fused.u.data.size(), 0.f);
                    fused.u.scalar = lux;
                    front.timestamp = fused.timestamp;
                    (front.wake_up ? synth_wake_up : synth).push_back(fused);
                }
            } else if (auto it = front_lights_.find(event.sensorHandle);
                       it != front_lights_.end() && event.sensorType == SensorType::LIGHT) {
                auto& front = it->second;
                front.lux = event.u.scalar;
                front.has_lux = true;
                front.timestamp = event.timestamp;
                front.sent_lux = FuseLux(front.lux);
                event.u.scalar = front.sent_lux;
                out.push_back(event);
            } else {
                out.push_back(event);
            }
        }
    }

    if (!out.empty()) hal_proxy_callback_->postEvents(out, std::move(wakelock));
    if (!synth.empty()) {
        hal_proxy_callback_->postEvents(synth, hal_proxy_callback_->createScopedWakelock(false));
    }
    if (!synth_wake_up.empty()) {
        hal_proxy_callback_->postEvents(synth_wake_up,
                                        hal_proxy_callback_->createScopedWakelock(true));
    }
}

ScopedWakelock SensorsSubHal::createScopedWakelock(bool lock) {
    return hal_proxy_callback_->createScopedWakelock(lock);
}

bool SensorsSubHal::IsRearLight(int32_t sensor_handle) {
    std::lock_guard<std::mutex> lock(mutex_);
    return rear_handle_ >= 0 && sensor_handle == rear_handle_;
}

Result SensorsSubHal::UpdateRearLight() {
    int32_t rear;
    bool need, do_batch;
    int64_t sampling_period_ns = INT64_MAX, max_report_latency_ns;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (rear_handle_ < 0) return Result::OK;
        rear = rear_handle_;

        bool front_active = false;
        for (const auto& [handle, front] : front_lights_) {
            if (!front.active) continue;
            front_active = true;
            sampling_period_ns = std::min(sampling_period_ns, front.sampling_period_ns);
        }
        need = front_active || fw_rear_.active;
        if (fw_rear_.active) {
            sampling_period_ns = std::min(sampling_period_ns, fw_rear_.sampling_period_ns);
        }
        max_report_latency_ns = front_active ? 0 : fw_rear_.max_report_latency_ns;
        do_batch = need && (!rear_batched_ || sampling_period_ns != rear_sampling_period_ns_);
        if (need == rear_on_ && !do_batch) return Result::OK;
    }

    Result ret = Result::OK;
    if (do_batch) ret = impl_->batch(rear, sampling_period_ns, max_report_latency_ns);
    bool do_activate;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (do_batch) {
            rear_sampling_period_ns_ = sampling_period_ns;
            rear_batched_ = ret == Result::OK;
        }
        do_activate = ret == Result::OK && need != rear_on_;
    }
    if (do_activate) ret = impl_->activate(rear, need);

    std::lock_guard<std::mutex> lock(mutex_);
    if (do_activate && ret == Result::OK) rear_on_ = need;
    if (!rear_on_) rear_valid_ = false;
    return ret;
}

float SensorsSubHal::FuseLux(float front_lux) {
    if (!rear_valid_ || IsTorchSuppressed()) return front_lux;
    return std::max(front_lux, rear_lux_);
}

bool SensorsSubHal::IsTorchSuppressed() {
    if (!torch_pi_) {
        torch_pi_ = __system_property_find(kTorchProp);
        if (!torch_pi_) return false;
    }
    uint32_t serial = __system_property_serial(torch_pi_);
    if (serial != torch_serial_) {
        torch_serial_ = serial;
        bool on = false;
        __system_property_read_callback(
                torch_pi_,
                [](void* cookie, const char*, const char* value, uint32_t) {
                    *static_cast<bool*>(cookie) = value[0] == '1';
                },
                &on);
        if (torch_on_ && !on) torch_off_ns_ = NowNs();
        torch_on_ = on;
    }
    return torch_on_ || NowNs() - torch_off_ns_ < kTorchHoldoffNs;
}

}  // namespace qsh_wrapper
}  // namespace implementation
}  // namespace subhal
}  // namespace V2_1
}  // namespace sensors
}  // namespace hardware
}  // namespace android

ISensorsSubHal* sensorsHalGetSubHal_2_1(uint32_t* version) {
    static ::android::hardware::sensors::V2_1::subhal::implementation::qsh_wrapper::SensorsSubHal
            sub_hal;
    *version = SUB_HAL_2_1_VERSION;
    return &sub_hal;
}
