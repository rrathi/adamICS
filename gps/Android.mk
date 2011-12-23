# hardware/libadam_gps/Android.mk

  LOCAL_PATH := $(call my-dir)/nmea

  include $(CLEAR_VARS)
  
  LOCAL_SRC_FILES := \
    generate.c 	\
    generator.c \
    parse.c	\
    parser.c	\
    tok.c	\
    context.c	\
    time.c	\
    info.c	\
    gmath.c	\
    sentence.c

  LOCAL_MODULE := nmea

  LOCAL_MODULE_TAGS := optional

  LOCAL_SHARED_LIBRARIES := \
    libutils \
    libcutils \
    libdl \
    libc
  
  LOCAL_PRELINK_MODULE := false
  include $(BUILD_STATIC_LIBRARY)

  #LOCAL_PATH := $(call my-dir)

  include $(CLEAR_VARS)
  
  LOCAL_SRC_FILES := \
	../gpslib.c

  LOCAL_MODULE := gps.harmony

  LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/hw

  LOCAL_MODULE_TAGS := optional

  LOCAL_STATIC_LIBRARIES := \
	nmea

  LOCAL_SHARED_LIBRARIES := \
    libutils \
    libcutils \
    libdl \
    libc
  
  LOCAL_PRELINK_MODULE := false
  include $(BUILD_SHARED_LIBRARY)
