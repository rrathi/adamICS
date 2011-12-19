# Copyright (C) ST-Ericsson AB 2008-2009
# Copyright 2006 The Android Open Source Project
#
# Based on reference-ril
# Modified for ST-Ericsson U300 modems.
# Author: Christian Bejram <christian.bejram@stericsson.com>
#
# XXX using libutils for simulator build only...
#
LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
    u300-ril.c \
    u300-ril-config.h \
    u300-ril-messaging.c \
    u300-ril-messaging.h \
    u300-ril-network.c \
    u300-ril-network.h \
    u300-ril-pdp.c \
    u300-ril-pdp.h \
    u300-ril-requestdatahandler.c \
    u300-ril-requestdatahandler.h \
    u300-ril-device.c \
    u300-ril-device.h \
    u300-ril-sim.c \
    u300-ril-sim.h \
    u300-ril-oem.c \
    u300-ril-oem.h \
    u300-ril-error.c \
    u300-ril-error.h \
    u300-ril-stk.c \
    u300-ril-stk.h \
    atchannel.c \
    atchannel.h \
    misc.c \
    misc.h \
    fcp_parser.c \
    fcp_parser.h \
    at_tok.c \
    at_tok.h \
    net-utils.c \
    net-utils.h

LOCAL_SHARED_LIBRARIES := \
    libcutils libutils libril
# libnetutils

# For asprinf
LOCAL_CFLAGS := -D_GNU_SOURCE

LOCAL_C_INCLUDES := $(KERNEL_HEADERS) $(TOP)/hardware/ril/libril/

# Disable prelink, or add to build/core/prelink-linux-arm.map
LOCAL_PRELINK_MODULE := false

LOCAL_MODULE_TAGS := optional

# Build shared library
LOCAL_SHARED_LIBRARIES += \
    libcutils libutils
LOCAL_LDLIBS += -lpthread
LOCAL_LDLIBS += -lrt
LOCAL_CFLAGS += -DRIL_SHLIB
LOCAL_CFLAGS += -Wall
LOCAL_MODULE:= libmbm-ril
include $(BUILD_SHARED_LIBRARY)
