#
# Copyright (C) 2011 The Android Open-Source Project
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
#

# This file includes all definitions that apply to ALL tuna devices, and
# are also specific to tuna devices
#
# Everything in this directory will become public

ifeq ($(TARGET_PREBUILT_KERNEL),)
LOCAL_KERNEL := device/notionink/adam/kernel
else
LOCAL_KERNEL := $(TARGET_PREBUILT_KERNEL)
endif

DEVICE_PACKAGE_OVERLAYS := device/notionink/adam/overlay

# This device is xhdpi.  However the platform doesn't
# currently contain all of the bitmaps at xhdpi density so
# we do this little trick to fall back to the hdpi version
# if the xhdpi doesn't exist.
#PRODUCT_AAPT_CONFIG := normal hdpi xhdpi xlarge sw600dp sw720dp
#PRODUCT_AAPT_PREF_CONFIG := xlarge
#PRODUCT_AAPT_PREF_CONFIG := 

PRODUCT_COPY_FILES := \
    $(LOCAL_KERNEL):kernel \
    device/notionink/adam/files/init.harmony.rc:root/init.harmony.rc \
    device/notionink/adam/files/ueventd.harmony.rc:root/ueventd.harmony.rc

# APK
#PRODUCT_COPY_FILES += \
#    device/notionink/adam/app/Quadrant.apk:system/app/Quadrant.apk \
#    device/notionink/adam/app/.root_browser:system/etc/.root_browser \
#    device/notionink/adam/app/RootBrowserFree.apk:system/app/RootBrowserFree.apk 

# Modules
PRODUCT_COPY_FILES += \
    device/notionink/adam/modules/scsi_wait_scan.ko:system/lib/hw/wlan/scsi_wait_scan.ko \
    device/notionink/adam/modules/bcm4329.ko:system/lib/hw/wlan/bcm4329.ko \
    device/notionink/adam/files/nvram.txt:system/lib/hw/wlan/nvram.txt

# Bluetooth
PRODUCT_COPY_FILES += \
    device/notionink/adam/files/bcm4329.hcd:system/etc/firmware/bcm4329.hcd
	
# Touchscreen
PRODUCT_COPY_FILES += \
    device/notionink/adam/files/at168_touch.idc:system/usr/idc/at168_touch.idc 

# Graphics
PRODUCT_COPY_FILES += \
    device/notionink/adam/files/media_profiles.xml:system/etc/media_profiles.xml

# Generic
PRODUCT_COPY_FILES += \
   device/notionink/adam/files/vold.fstab:system/etc/vold.fstab \
   device/notionink/adam/files/gps.conf:system/etc/gps.conf

PRODUCT_PROPERTY_OVERRIDES := \
    wifi.interface=wlan0 \
    ro.sf.lcd_density=120 \
    wifi.supplicant_scan_interval=15

# Live Wallpapers
PRODUCT_PACKAGES += \
        LiveWallpapers \
        LiveWallpapersPicker \
        VisualizationWallpapers \
        Basic \
        Galaxy4 \
        HoloSpiral \
        LivePicker \
        MagicSmoke \
        MusicVisualization \
        NoiseField \
        PhaseBeam 
        
# These are the hardware-specific features
PRODUCT_COPY_FILES += \
    frameworks/base/data/etc/tablet_core_hardware.xml:system/etc/permissions/tablet_core_hardware.xml \
    frameworks/base/data/etc/android.hardware.camera.xml:system/etc/permissions/android.hardware.camera.xml \
    frameworks/base/data/etc/android.hardware.camera.front.xml:system/etc/permissions/android.hardware.camera.front.xml \
    frameworks/base/data/etc/android.hardware.location.xml:system/etc/permissions/android.hardware.location.xml \
    frameworks/base/data/etc/android.hardware.location.gps.xml:system/etc/permissions/android.hardware.location.gps.xml \
    frameworks/base/data/etc/android.hardware.wifi.xml:system/etc/permissions/android.hardware.wifi.xml \
    frameworks/base/data/etc/android.hardware.wifi.direct.xml:system/etc/permissions/android.hardware.wifi.direct.xml \
    frameworks/base/data/etc/android.hardware.sensor.proximity.xml:system/etc/permissions/android.hardware.sensor.proximity.xml \
    frameworks/base/data/etc/android.hardware.sensor.light.xml:system/etc/permissions/android.hardware.sensor.light.xml \
    frameworks/base/data/etc/android.hardware.sensor.gyroscope.xml:system/etc/permissions/android.hardware.sensor.gyroscope.xml \
    frameworks/base/data/etc/android.hardware.sensor.accelerometer.xml:system/etc/permissions/android.hardware.sensor.accelerometer.xml \
    frameworks/base/data/etc/android.hardware.touchscreen.multitouch.jazzhand.xml:system/etc/permissions/android.hardware.touchscreen.multitouch.jazzhand.xml \
    frameworks/base/data/etc/android.software.sip.voip.xml:system/etc/permissions/android.software.sip.voip.xml \
    frameworks/base/data/etc/android.hardware.usb.accessory.xml:system/etc/permissions/android.hardware.usb.accessory.xml \
    frameworks/base/data/etc/android.hardware.usb.host.xml:system/etc/permissions/android.hardware.usb.host.xml \
    packages/wallpapers/LivePicker/android.software.live_wallpaper.xml:system/etc/permissions/android.software.live_wallpaper.xml 

PRODUCT_PROPERTY_OVERRIDES += \
	ro.opengles.version=131072

PRODUCT_CHARACTERISTICS := tablet

PRODUCT_TAGS += dalvik.gc.type-precise

PRODUCT_PACKAGES += \
	librs_jni \
	com.android.future.usb.accessory

# Filesystem management tools
PRODUCT_PACKAGES += \
	setup_fs

DEVICE_PACKAGE_OVERLAYS := \
    device/notionink/adam/overlay

# for bugmailer
ifneq ($(TARGET_BUILD_VARIANT),user)
	PRODUCT_PACKAGES += send_bug
	PRODUCT_COPY_FILES += \
		system/extras/bugmailer/bugmailer.sh:system/bin/bugmailer.sh \
		system/extras/bugmailer/send_bug:system/bin/send_bug
endif

$(call inherit-product, frameworks/base/build/tablet-dalvik-heap.mk)

$(call inherit-product-if-exists, vendor/notionink/adam/device-vendor.mk)

BOARD_WLAN_DEVICE_REV := bcm4329
WIFI_BAND             := 802_11_ABG

$(call inherit-product-if-exists, hardware/broadcom/wlan/bcmdhd/firmware/bcm4329/device-bcm.mk)
