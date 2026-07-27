#!/usr/bin/env -S PYTHONPATH=../../../tools/extract-utils python3
#
# SPDX-FileCopyrightText: 2026 The LineageOS Project
# SPDX-License-Identifier: Apache-2.0
#

import extract_utils.tools
from extract_utils.fixups_blob import (
    blob_fixup,
    blob_fixups_user_type,
)
from extract_utils.fixups_lib import (
    lib_fixups,
    lib_fixups_user_type,
)
from extract_utils.main import (
    ExtractUtils,
    ExtractUtilsModule,
)

namespace_imports = [
    'device/xiaomi/sm8850-common',
    'hardware/qcom-caf/sm8850',
    'hardware/xiaomi',
    'vendor/qcom/opensource/commonsys-intf/display',
    'vendor/xiaomi/sm8850-common',
]

lib_fixups: lib_fixups_user_type = {
    **lib_fixups,
}

blob_fixups: blob_fixups_user_type = {
    (
        'odm/etc/camera/snsc_bokeh_motiontuning.xml',
        'odm/etc/camera/snsc_enhance_motiontuning.xml',
        'odm/etc/camera/snsc_noface_motiontuning.xml',
        'odm/etc/camera/snsc_motiontuning.xml'
    ): blob_fixup()
        .regex_replace('xml=version', 'xml version'),
    (
        'vendor/lib64/libcameraopt.so',
    ): blob_fixup()
        .add_needed('libprocessgroup_shim.so'),
    (
        'odm/lib64/libMiEmojiEffect.so',
        'odm/lib64/libMiVideoFilter.so',
        'odm/lib64/libAncHumanPreviewBokeh.so',
        'odm/lib64/libarcsoft_beautyshot.so',
        'odm/lib64/libwa_widelens_undistort.so',
        'vendor/lib64/libMiPhotoFilter.so'
    ): blob_fixup()
        .clear_symbol_version('AHardwareBuffer_allocate')
        .clear_symbol_version('AHardwareBuffer_describe')
        .clear_symbol_version('AHardwareBuffer_lockPlanes')
        .clear_symbol_version('AHardwareBuffer_release')
        .clear_symbol_version('AHardwareBuffer_unlock')
        .clear_symbol_version('AHardwareBuffer_lock')
        .clear_symbol_version('AHardwareBuffer_isSupported'),
    (
       'odm/lib64/camera/components/com.qti.node.dewarp.so',
       'odm/lib64/hw/com.qti.chi.override.so',
       'odm/lib64/libcamximageformatutils.so',
       'odm/lib64/libchifeature2.so',
       'odm/lib64/vendor.qti.hardware.camera.offlinecamera-service-impl.so',
    ): blob_fixup()
        .replace_needed(
            'android.hardware.graphics.allocator-V1-ndk.so',
            'android.hardware.graphics.allocator-V2-ndk.so'
        ),
    (
       'vendor/lib64/vendor.xiaomi.hardware.camera.injection-V1-ndk.so',
       'vendor/lib64/vendor.xiaomi.hardware.camera.injection-client.so',
       'vendor/lib64/vendor.xiaomi.hardware.camera.injection-service.so',
    ): blob_fixup()
        .replace_needed(
            'android.hardware.camera.device-V1-ndk.so',
            'android.hardware.camera.device-V2-ndk.so'
        ),
    (
        'odm/lib64/camera/plugins/com.xiaomi.plugin.losslessjpeg.so'
    ): blob_fixup()
        .replace_needed(
            'libdng_sdk.so',
            'libdng_sdk-myron.so'
        ),
    (
        "odm/lib64/camera/dynamicplugins/com.xiaomi.plugin.mialgoallinone.so",
        "odm/lib64/camera/plugins/com.xiaomi.plugin.anchor.so",
        "odm/lib64/camera/preloadplugins/com.xiaomi.plugin.asd.so",
        "odm/lib64/camera/preloadplugins/com.xiaomi.plugin.offcamb2y.so",
        "odm/lib64/camera/preloadplugins/com.xiaomi.plugin.offcamformatconvertor.so",
        "odm/lib64/camera/preloadplugins/com.xiaomi.plugin.offcamheic.so",
        "odm/lib64/camera/preloadplugins/com.xiaomi.plugin.offcamjpeg.so",
        "odm/lib64/camera/preloadplugins/com.xiaomi.plugin.offcammfnr.so",
        "odm/lib64/camera/preloadplugins/com.xiaomi.plugin.offcammlawb.so",
        "odm/lib64/camera/preloadplugins/com.xiaomi.plugin.offcamyuveis.so",
        "odm/lib64/camera/preloadplugins/com.xiaomi.plugin.offcamyuvreprocess.so",
        "odm/lib64/camera/preloadplugins/com.xiaomi.plugin.offcamyuvsplit.so",
        "odm/lib64/camera/preloadplugins/com.xiaomi.plugin.offlineawbideal.so",
        "odm/lib64/camera/preloadplugins/com.xiaomi.plugin.offlineb2y.so",
        "odm/lib64/camera/preloadplugins/com.xiaomi.plugin.offlineehdr.so",
        "odm/lib64/camera/preloadplugins/com.xiaomi.plugin.offlineformatconvertor.so",
        "odm/lib64/camera/preloadplugins/com.xiaomi.plugin.offlinehdrraw2y.so",
        "odm/lib64/camera/preloadplugins/com.xiaomi.plugin.offlineheic.so",
        "odm/lib64/camera/preloadplugins/com.xiaomi.plugin.offlinei2y.so",
        "odm/lib64/camera/preloadplugins/com.xiaomi.plugin.offlinejpeg.so",
        "odm/lib64/camera/preloadplugins/com.xiaomi.plugin.offlinemfnr.so",
        "odm/lib64/camera/preloadplugins/com.xiaomi.plugin.offlinemlawb.so",
        "odm/lib64/camera/preloadplugins/com.xiaomi.plugin.offlinetintless.so",
        "odm/lib64/camera/preloadplugins/com.xiaomi.plugin.offlinetintlesshdr.so",
        "odm/lib64/camera/preloadplugins/com.xiaomi.plugin.offlineyuveis.so",
        "odm/lib64/camera/preloadplugins/com.xiaomi.plugin.offlineyuvreprocess.so",
        "odm/lib64/camera/preloadplugins/com.xiaomi.plugin.offlineyuvsplit.so",
        "odm/lib64/camera/preloadplugins/com.xiaomi.plugin.offlineyuvwarp.so",
        "odm/lib64/com.xiaomi.plugin.ecdengine.so",
        "vendor/bin/hw/vendor.qti.camera.provider-service_64",
        "vendor/lib64/camera/components/com.mi.node.fd.so",
        "vendor/lib64/camera/components/com.qti.node.fd.so",
        "vendor/lib64/com.xiaomi.stubv1.camx.so",
        "vendor/lib64/hw/camera.qcom.core.so",
        "vendor/lib64/libcamxdumpinforecorder.so",
        "vendor/lib64/libmicamera_hal_core.so",
        "vendor/lib64/libsimulation.so",
    ): blob_fixup()
        .replace_needed(
            'libtinyxml2.so',
            'libtinyxml2-v36.so'
        ),
}  # fmt: skip

module = ExtractUtilsModule(
    'myron',
    'xiaomi',
    blob_fixups=blob_fixups,
    lib_fixups=lib_fixups,
    namespace_imports=namespace_imports,
)

if __name__ == '__main__':
    utils = ExtractUtils.device_with_common(
        module, 'sm8850-common', module.vendor
    )
    utils.run()