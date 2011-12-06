# inherit from the proprietary version
-include vendor/notionink/adam/BoardConfigVendor.mk

TARGET_BOARD_PLATFORM := tegra
TARGET_CPU_ABI := armeabi-v7a
TARGET_CPU_ABI2 := armeabi
TARGET_CPU_SMP := true
TARGET_ARCH_VARIANT := armv7-a
TARGET_ARCH_VARIANT_CPU := cortex-a9
TARGET_ARCH_VARIANT_FPU := vfpv3-d16
TARGET_CPU_SMP := true
ARCH_ARM_HAVE_TLS_REGISTER := true

BOARD_USE_USB_MASS_STORAGE_SWITCH := true

TARGET_NO_BOOTLOADER := true
TARGET_BOOTLOADER_BOARD_NAME := harmony

# Modem
TARGET_NO_RADIOIMAGE := true

# Wifi related defines
BOARD_WPA_SUPPLICANT_DRIVER := WEXT
WPA_SUPPLICANT_VERSION      := VER_0_8_X
BOARD_WPA_SUPPLICANT_PRIVATE_LIB := lib_driver_cmd_wext
BOARD_HOSTAPD_DRIVER        := WEXT
BOARD_HOSTAPD_PRIVATE_LIB   := lib_driver_cmd_wext
BOARD_WLAN_DEVICE           := bcm4329
WIFI_DRIVER_FW_PATH_PARAM   := "/sys/module/bcm4329/parameters/firmware_path"
WIFI_DRIVER_MODULE_PATH     := "/system/lib/hw/wlan/bcm4329.ko"
WIFI_DRIVER_FW_PATH_STA     := "/lib/hw/wlan/fw_bcm4329.bin"
WIFI_DRIVER_FW_PATH_AP      := "/lib/hw/wlan/fw_bcm4329_apsta.bin"
WIFI_DRIVER_MODULE_NAME     := "bcm4329"
WIFI_DRIVER_MODULE_ARG      := "firmware_path=/system/lib/hw/wlan/fw_bcm4329.bin nvram_path=/system/lib/hw/wlan/nvram.txt iface_name=wlan0"

BOARD_HAVE_BLUETOOTH := true
BOARD_HAVE_BLUETOOTH_CSR := true

BOARD_KERNEL_CMDLINE := tegra_fbmem=8192000@0x1e018000 video=tegrafb console=ttyGS0,115200n8 androidboot.console=ttyGS0 mem=1024M@0M lp0_vec=8192@0x1e7f1020 lcd_manfid=AUO usbcore.old_scheme_first=1 tegraboot=nand mtdparts=tegra_nand:16384K@9984K(misc),16384K@26880K(recovery),16384K@43904K(boot),204800K@60928K(system),781824K@266240K(cache)
BOARD_KERNEL_BASE := 0x10000000
BOARD_PAGE_SIZE := 0x00000800

BOARD_USES_GENERIC_AUDIO := false
BOARD_PREBUILT_LIBAUDIO := true

# Use dirty hack to allow froyo libaudio
BOARD_USE_KINETO_COMPATIBILITY := true

# EGL config 
TARGET_LIBAGL_USE_GRALLOC_COPYBITS := true
BOARD_NO_RGBX_8888 := true
BOARD_EGL_CFG := device/notionink/adam/egl.cfg

# Use screencap to capture frame buffer for ddms
BOARD_USE_SCREENCAP := true

# fix this up by examining /proc/mtd on a running device
# dev:    size   erasesize  name
mtd0: 01000000 00020000 "misc"
mtd1: 01000000 00020000 "recovery"
mtd2: 01000000 00020000 "boot"
mtd3: 0c800000 00020000 "system"
mtd4: 2fbc0000 00020000 "cache"

BOARD_BOOTIMAGE_PARTITION_SIZE := 0x01000000
BOARD_RECOVERYIMAGE_PARTITION_SIZE := 0x01000000
BOARD_SYSTEMIMAGE_PARTITION_SIZE := 0x0c800000
BOARD_USERDATAIMAGE_PARTITION_SIZE := 0x105c0000
BOARD_FLASH_BLOCK_SIZE := 131072

# Indicate that the board has an Internal SD Card
BOARD_HAS_SDCARD_INTERNAL := true

# Below is a sample of how you can tweak the mount points using the board config.
# This is for the Samsung Galaxy S.
# Feel free to tweak or remove this code.
# If you want to add/tweak a mount point, the BOARD_X_FILESYSTEM_OPTIONS are optional.
BOARD_DATA_DEVICE := /dev/block/mmcblk3p2
BOARD_DATA_FILESYSTEM := ext3
BOARD_CACHE_DEVICE := /dev/block/mtdblock4
BOARD_CACHE_FILESYSTEM := yaffs2

BOARD_SDCARD_DEVICE_PRIMARY := /dev/block/mmcblk2p1
BOARD_SDCARD_DEVICE_SECONDARY := /dev/block/mmcblk3p1
BOARD_SDCARD_DEVICE_INTERNAL := /dev/block/mmcblk3p1
BOARD_SDEXT_DEVICE := /dev/block/mmcblk2p2

TARGET_PREBUILT_KERNEL := device/notionink/adam/kernel

# Override cyanogen squisher to customize our update zip package
TARGET_CUSTOM_RELEASETOOL := ./device/notionink/adam/releasetools/squisher

# custom recovery ui
BOARD_CUSTOM_RECOVERY_KEYMAPPING := ../../device/notionink/adam/recovery/recovery_ui.c
