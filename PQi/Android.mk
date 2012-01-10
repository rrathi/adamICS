LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
   
# Module name should match apk name to be installed.
LOCAL_MODULE := PQiToggle
LOCAL_SRC_FILES := $(LOCAL_MODULE).apk
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := APPS
LOCAL_CERTIFICATE := platform
LOCAL_MODULE_SUFFIX := $(COMMON_ANDROID_PACKAGE_SUFFIX)
include $(BUILD_PREBUILT)
