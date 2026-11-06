/*
 * Copyright (C) 2025 The LineageOS Project
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <V2_1/SubHal.h>
#include <sys/system_properties.h>

#include <map>
#include <mutex>

namespace android {
namespace hardware {
namespace sensors {
namespace V2_1 {
namespace subhal {
namespace implementation {
namespace qsh_wrapper {

using ::android::hardware::sensors::V1_0::OperationMode;
using ::android::hardware::sensors::V1_0::RateLevel;
using ::android::hardware::sensors::V1_0::Result;
using ::android::hardware::sensors::V1_0::SharedMemInfo;
using ::android::hardware::sensors::V2_1::Event;
using ::android::hardware::sensors::V2_1::implementation::IHalProxyCallback;
using ::android::hardware::sensors::V2_1::implementation::ISensorsSubHal;

class SensorsSubHal : public ISensorsSubHal, public IHalProxyCallback {
  public:
    SensorsSubHal();

    // ISensors
    Return<Result> setOperationMode(OperationMode mode) override;
    Return<Result> activate(int32_t sensor_handle, bool enabled) override;
    Return<Result> batch(int32_t sensor_handle, int64_t sampling_period_ns,
                         int64_t max_report_latency_ns) override;
    Return<Result> flush(int32_t sensor_handle) override;
    Return<void> registerDirectChannel(const SharedMemInfo& mem,
                                       ISensors::registerDirectChannel_cb _hidl_cb) override;
    Return<Result> unregisterDirectChannel(int32_t channel_handle) override;
    Return<void> configDirectReport(int32_t sensor_handle, int32_t channel_handle, RateLevel rate,
                                    ISensors::configDirectReport_cb _hidl_cb) override;
    Return<void> getSensorsList_2_1(ISensors::getSensorsList_2_1_cb _hidl_cb) override;
    Return<Result> injectSensorData_2_1(const Event& event) override;

    // ISensorsSubHal
    Return<void> debug(const hidl_handle& fd, const hidl_vec<hidl_string>& args) override;
    const std::string getName() override;
    Return<Result> initialize(const sp<IHalProxyCallback>& hal_proxy_callback) override;

    // ISensorsCallback
    Return<void> onDynamicSensorsConnected(const hidl_vec<V1_0::SensorInfo>& sensor_infos) override;
    Return<void> onDynamicSensorsDisconnected(const hidl_vec<int32_t>& sensor_handles) override;
    Return<void> onDynamicSensorsConnected_2_1(
            const hidl_vec<V2_1::SensorInfo>& sensor_infos) override;

    // IHalProxyCallback
    void postEvents(const std::vector<Event>& events, ScopedWakelock wakelock) override;
    ScopedWakelock createScopedWakelock(bool lock) override;

  private:
    struct FrontLight {
        bool wake_up = false;
        bool active = false;
        bool has_lux = false;
        int64_t sampling_period_ns = 200000000;
        int64_t timestamp = 0;
        float lux = 0;
        float sent_lux = 0;
    };

    struct FrameworkRear {
        bool active = false;
        int64_t sampling_period_ns = 200000000;
        int64_t max_report_latency_ns = 0;
    };

    bool IsRearLight(int32_t sensor_handle);
    Result UpdateRearLight();
    float FuseLux(float front_lux);
    bool IsTorchSuppressed();

    std::unique_ptr<void, std::function<void(void*)>> lib_handle_;
    V2_1::implementation::ISensorsSubHal* impl_;
    sp<IHalProxyCallback> hal_proxy_callback_;

    std::mutex control_mutex_;
    std::mutex mutex_;

    std::map<int32_t, FrontLight> front_lights_;
    FrameworkRear fw_rear_;
    int32_t rear_handle_ = -1;
    bool rear_on_ = false;
    bool rear_batched_ = false;
    int64_t rear_sampling_period_ns_ = 0;
    bool rear_valid_ = false;
    float rear_lux_ = 0;

    const prop_info* torch_pi_ = nullptr;
    uint32_t torch_serial_ = 0;
    bool torch_on_ = false;
    int64_t torch_off_ns_ = INT64_MIN / 2;
};

}  // namespace qsh_wrapper
}  // namespace implementation
}  // namespace subhal
}  // namespace V2_1
}  // namespace sensors
}  // namespace hardware
}  // namespace android
