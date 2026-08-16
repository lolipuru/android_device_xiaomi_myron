
#
# Copyright (C) 2026 The Android Open Source Project
#
# SPDX-License-Identifier: Apache-2.0
#

# Inherit from sm8650-common
$(call inherit-product, device/xiaomi/sm8850-common/common.mk)

# Get non-open-source specific aspects
$(call inherit-product, vendor/xiaomi/myron/myron-vendor.mk)

# Camera
PRODUCT_PACKAGES += \
    android.hardware.graphics.allocator-V1-ndk.vendor \
    vendor.qti.hardware.camera.offlinecamera-V2-ndk.vendor

# Properties
PRODUCT_COPY_FILES += \
    $(LOCAL_PATH)/configs/properties/odm_CN.prop:$(TARGET_COPY_OUT_ODM)/etc/odm_CN.prop \
    $(LOCAL_PATH)/configs/properties/odm_GL.prop:$(TARGET_COPY_OUT_ODM)/etc/odm_GL.prop

# Soong namespaces
PRODUCT_SOONG_NAMESPACES += \
    $(LOCAL_PATH)

# Overlays
PRODUCT_PACKAGES += \
    ApertureOverlayMyron \
    FrameworksResMyron \
    SettingsOverlayMyron \
    SystemUIResMyron
