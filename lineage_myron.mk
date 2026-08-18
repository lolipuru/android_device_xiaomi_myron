#
# Copyright (C) 2024 The Android Open Source Project
#
# SPDX-License-Identifier: Apache-2.0
#

# Inherit from products. Most specific first.
$(call inherit-product, $(SRC_TARGET_DIR)/product/core_64_bit_only.mk)
$(call inherit-product, $(SRC_TARGET_DIR)/product/full_base_telephony.mk)

# Inherit some common Lineage stuff.
$(call inherit-product, vendor/lineage/config/common_full_phone.mk)

# Inherit from myron device.
$(call inherit-product, device/xiaomi/myron/device.mk)

## Device identifier
PRODUCT_DEVICE := myron
PRODUCT_NAME := lineage_myron
PRODUCT_BRAND := Redmi
PRODUCT_MODEL := 25102RKBEC
PRODUCT_MANUFACTURER := xiaomi

BUILD_FINGERPRINT := Xiaomi/myron/myron:16/BQ2A.250705.001-BP2A.250605.031.A3/OS3.0.303.0.WPMCNXM:user/release-keys
