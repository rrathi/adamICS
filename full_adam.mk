# Copyright (C) 2011 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# Camera
PRODUCT_PACKAGES := \
    Camera \
    SpareParts \
    PQiToggle \
    Development \
	Stk \
	Mms

$(call inherit-product, $(SRC_TARGET_DIR)/product/full_base.mk)

# Inherit from ADAM device
$(call inherit-product, device/notionink/adam/device.mk)

# The gps config appropriate for this device
$(call inherit-product, device/common/gps/gps_us_supl.mk)

$(call inherit-product-if-exists, vendor/notionink/adam/device-vendor.mk)

PRODUCT_NAME := full_adam
PRODUCT_DEVICE := adam
PRODUCT_BRAND := NotionInk
PRODUCT_MODEL := Notion Ink ADAM
