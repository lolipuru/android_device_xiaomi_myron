#
# Copyright (C) 2026 The Android Open Source Project
#
# SPDX-License-Identifier: Apache-2.0
#

DEVICE_PATH := device/xiaomi/myron
KERNEL_PATH := $(DEVICE_PATH)-kernel

# Inherit from sm8650-common
include device/xiaomi/sm8850-common/BoardConfigCommon.mk

# Display
TARGET_SCREEN_DENSITY := 520

# Dtb/o
BOARD_PREBUILT_DTBOIMAGE := $(KERNEL_PATH)/dtbo.img
BOARD_PREBUILT_DTBIMAGE_DIR := $(KERNEL_PATH)/dtb

TARGET_NO_KERNEL_OVERRIDE := true
TARGET_KERNEL_SOURCE := $(KERNEL_PATH)/kernel-headers
PRODUCT_COPY_FILES += \
	$(KERNEL_PATH)/kernel:kernel

# Kernel modules
BOARD_VENDOR_RAMDISK_KERNEL_MODULES_LOAD := $(strip $(shell cat $(KERNEL_PATH)/vendor_ramdisk/modules.load))
BOARD_VENDOR_RAMDISK_RECOVERY_KERNEL_MODULES_LOAD := $(strip $(shell cat $(KERNEL_PATH)/vendor_ramdisk/modules.load.recovery))
BOARD_VENDOR_KERNEL_MODULES_LOAD := $(strip $(shell cat $(KERNEL_PATH)/vendor_dlkm/modules.load))

PRODUCT_COPY_FILES += \
    $(call find-copy-subdir-files,*,$(KERNEL_PATH)/vendor_dlkm/,$(TARGET_COPY_OUT_VENDOR_DLKM)/lib/modules) \
    $(call find-copy-subdir-files,*,$(KERNEL_PATH)/vendor_ramdisk/,$(TARGET_COPY_OUT_VENDOR_RAMDISK)/lib/modules) \
    $(call find-copy-subdir-files,*,$(KERNEL_PATH)/system_dlkm_flatten/,$(TARGET_COPY_OUT_SYSTEM_DLKM)/flatten/lib/modules) \
    $(call find-copy-subdir-files,*,$(KERNEL_PATH)/system_dlkm/,$(TARGET_COPY_OUT_SYSTEM_DLKM)/lib/modules/6.12.23-android16-5-g316453da9a9e-abogki441133159-4k)

TARGET_ODM_PROP += $(DEVICE_PATH)/configs/properties/odm.prop
TARGET_VENDOR_PROP += $(DEVICE_PATH)/configs/properties/vendor.prop

# Inherit from the proprietary version
include vendor/xiaomi/myron/BoardConfigVendor.mk