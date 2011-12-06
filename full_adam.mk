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
	$(LOCAL_PATH)/prebuilt/init.harmony.rc:root/init.harmony.rc \
	$(LOCAL_PATH)/prebuilt/init.rc:root/init.rc \
	$(LOCAL_PATH)/prebuilt/ueventd.harmony.rc:root/ueventd.harmony.rc \
	$(LOCAL_PATH)/prebuilt/ueventd.rc:root/ueventd.rc \
	$(LOCAL_PATH)/prebuilt/nvram.txt:system/etc/nvram.txt \
	$(LOCAL_PATH)/prebuilt/at168_touch.idc:system/usr/idc/at168_touch.idc \
	$(LOCAL_PATH)/prebuilt/vold.fstab:system/etc/vold.fstab 

PRODUCT_PACKAGES += \
    libreference-ril

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
