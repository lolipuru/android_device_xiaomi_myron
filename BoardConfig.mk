#
# Copyright (C) 2026 The Android Open Source Project
#
# SPDX-License-Identifier: Apache-2.0
#

DEVICE_PATH := device/xiaomi/myron
TARGET_OPT_IN_OSS_KERNEL := true

# Inherit from sm8650-common
include device/xiaomi/sm8850-common/BoardConfigCommon.mk

# Display
TARGET_SCREEN_DENSITY := 420

# Kernel modules
BOARD_VENDOR_KERNEL_MODULES_LOAD += \
	tfa98xx_dlkm.ko \
	focaltech_touch_3683.ko

TARGET_ODM_PROP += $(DEVICE_PATH)/configs/properties/odm.prop
TARGET_VENDOR_PROP += $(DEVICE_PATH)/configs/properties/vendor.prop

# Inherit from the proprietary version
include vendor/xiaomi/myron/BoardConfigVendor.mk