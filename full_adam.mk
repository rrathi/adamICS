$(call inherit-product-if-exists, vendor/notionink/adam/device-vendor.mk)
$(call inherit-product, frameworks/base/build/tablet-dalvik-heap.mk)

# Kernel
ifeq ($(TARGET_PREBUILT_KERNEL),)
        LOCAL_KERNEL := device/notionink/adam/kernel
else
        LOCAL_KERNEL := $(TARGET_PREBUILT_KERNEL)
endif

PRODUCT_DEFAULT_PROPERTY_OVERRIDES += \
	persist.service.adb.enable=1 

PRODUCT_COPY_FILES += \
	$(LOCAL_KERNEL):kernel \
	$(LOCAL_PATH)/wifi/bcm4329.ko:system/lib/hw/wlan/bcm4329.ko \
	$(LOCAL_PATH)/wifi/fw_bcm4329.bin:system/lib/hw/wlan/fw_bcm4329.bin \
	$(LOCAL_PATH)/wifi/fw_bcm4329_apsta.bin:system/lib/hw/wlan/fw_bcm4329_apsta.bin \
	$(LOCAL_PATH)/wifi/scsi_wait_scan.ko:system/lib/hw/wlan/scsi_wait_scan.ko \
	$(LOCAL_PATH)/prebuilt/init.harmony.rc:root/init.harmony.rc \
	$(LOCAL_PATH)/prebuilt/init.rc:root/init.rc \
	$(LOCAL_PATH)/prebuilt/ueventd.harmony.rc:root/ueventd.harmony.rc \
	$(LOCAL_PATH)/prebuilt/ueventd.rc:root/ueventd.rc \
	$(LOCAL_PATH)/prebuilt/nvram.txt:system/lib/hw/wlan/nvram.txt \
	$(LOCAL_PATH)/prebuilt/at168_touch.idc:system/usr/idc/at168_touch.idc \
	$(LOCAL_PATH)/prebuilt/vold.fstab:system/etc/vold.fstab 

# Place permission files
PRODUCT_COPY_FILES += \
    frameworks/base/data/etc/tablet_core_hardware.xml:system/etc/permissions/tablet_core_hardware.xml \
    frameworks/base/data/etc/android.hardware.camera.front.xml:system/etc/permissions/android.hardware.camera.front.xml \
    frameworks/base/data/etc/android.hardware.camera.autofocus.xml:system/etc/permissions/android.hardware.camera.autofocus.xml \
    frameworks/base/data/etc/android.hardware.telephony.gsm.xml:system/etc/permissions/android.hardware.telephony.gsm.xml \
    frameworks/base/data/etc/android.hardware.location.gps.xml:system/etc/permissions/android.hardware.location.gps.xml \
    frameworks/base/data/etc/android.hardware.wifi.xml:system/etc/permissions/android.hardware.wifi.xml \
    frameworks/base/data/etc/android.hardware.sensor.proximity.xml:system/etc/permissions/android.hardware.sensor.proximity.xml \
    frameworks/base/data/etc/android.hardware.sensor.light.xml:system/etc/permissions/android.hardware.sensor.light.xml \
    frameworks/base/data/etc/android.hardware.sensor.accelerometer.xml:system/etc/permissions/android.hardware.sensor.accelerometer.xml \
    frameworks/base/data/etc/android.hardware.touchscreen.multitouch.distinct.xml:system/etc/permissions/android.hardware.touchscreen.multitouch.distinct.xml \
    frameworks/base/data/etc/android.software.sip.voip.xml:system/etc/permissions/android.software.sip.voip.xml

PRODUCT_PACKAGES += \
    librs_jni \
    libreference-ril

BOARD_WLAN_DEVICE_REV := bcm4329
WIFI_BAND             := 802_11_ABG

# Inherit from those products. Most specific first.
$(call inherit-product, $(SRC_TARGET_DIR)/product/full_base.mk)
# This is where we'd set a backup provider if we had one
#$(call inherit-product, device/sample/products/backup_overlay.mk)
# Inherit from tuna device
#$(call inherit-product, $(LOCAL_PATH)/device.mk
# Set those variables here to overwrite the inherited values.
PRODUCT_NAME := full_adam
PRODUCT_DEVICE := adam
PRODUCT_BRAND := NotionInk
PRODUCT_MODEL := PixelQi
