# (c) Copyright 2013 Hewlett-Packard Development Company, L.P.
# 
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
# 
# http://www.apache.org/licenses/LICENSE-2.0
# 
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

LOCAL_PATH := $(call my-dir)

my_LOCAL_PATH := $(call my-dir)

PNG_LIB_DIR        := externals/libpng
JPEG_LIB_DIR       := externals/jpeg
OPENSSL_LIB_DIR    := externals/openssl
WPRINT_LIB_DIR     := wprint/lib
WPRINT_PLUGINS_DIR := wprint/plugins
IPP_HELPER_DIR     := wprint/ipphelper
CUPS_BASE_DIR      := externals/cups
CUPS_LIB_DIR       := externals/cups/cups


include $(CLEAR_VARS)

PCLM_SRC_FILES := \
	$(WPRINT_PLUGINS_DIR)/lib_pclm.c \
	$(WPRINT_PLUGINS_DIR)/lib_pwg.c \
	$(WPRINT_PLUGINS_DIR)/genPCLm/src/genPCLm.cpp  \
	$(WPRINT_PLUGINS_DIR)/genPCLm/src/genJPEGStrips.cpp  \
	$(WPRINT_PLUGINS_DIR)/plugin_pcl.c  \
	$(WPRINT_PLUGINS_DIR)/pclm_wrapper_api.cpp  \
	$(WPRINT_PLUGINS_DIR)/wprint_hpimage.c  \
	$(WPRINT_PLUGINS_DIR)/wprint_image.c  \
	$(WPRINT_PLUGINS_DIR)/wprint_image_platform.c  \
	$(WPRINT_PLUGINS_DIR)/wprint_jpeg.c  \
	$(WPRINT_PLUGINS_DIR)/wprint_osimg.c  \
	$(WPRINT_PLUGINS_DIR)/wprint_png.c  \
	$(WPRINT_PLUGINS_DIR)/wprint_ppm.c  \
	$(WPRINT_PLUGINS_DIR)/wprint_scaler.c 

WPRINT_SRC_FILES:=\
	$(WPRINT_LIB_DIR)/lib_wprint.c  \
	$(WPRINT_LIB_DIR)/plugin_db.c \
	$(WPRINT_LIB_DIR)/printer.c \
	$(WPRINT_LIB_DIR)/wprint_debug.c \
	$(WPRINT_LIB_DIR)/wprint_msgq.c \
	$(WPRINT_LIB_DIR)/printable_area.c \
	$(WPRINT_LIB_DIR)/wprintJNI.c

IPP_HELPER_SRC_FILES := \
	$(IPP_HELPER_DIR)/ipp_print.c \
	$(IPP_HELPER_DIR)/ipphelper.c \
	$(IPP_HELPER_DIR)/ippstatus_capabilities.c \
	$(IPP_HELPER_DIR)/ippstatus_monitor.c

LOCAL_SRC_FILES:= \
	$(WPRINT_SRC_FILES) \
	$(PCLM_SRC_FILES) \
	$(IPP_HELPER_SRC_FILES)

LOCAL_C_INCLUDES += \
	$(my_LOCAL_PATH)/include  \
	$(my_LOCAL_PATH)/$(OPENSSL_LIB_DIR)/include \
	$(my_LOCAL_PATH)/$(JPEG_LIB_DIR)  \
	$(my_LOCAL_PATH)/$(PNG_LIB_DIR) \
	$(my_LOCAL_PATH)/$(WPRINT_PLUGINS_DIR)/genPCLm/inc \
	$(my_LOCAL_PATH)/$(IPP_HELPER_DIR) \
	$(my_LOCAL_PATH)/$(CUPS_BASE_DIR) \
	$(my_LOCAL_PATH)/$(CUPS_LIB_DIR) \

LOCAL_SHARED_LIBRARIES := \
	lib$(PRIV_LIB_NAME)jpeg \
	lib$(PRIV_LIB_NAME)png \
	lib$(PRIV_LIB_NAME)cups \

LOCAL_LDLIBS += -llog -lz -ljnigraphics
LOCAL_MODULE := lib$(PRIV_LIB_NAME)
LOCAL_MODULE_TAGS := optional
include $(BUILD_SHARED_LIBRARY)

# Build the openssl library
include $(CLEAR_VARS)
include $(my_LOCAL_PATH)/$(OPENSSL_LIB_DIR)/Android.mk

# Build PNG library
include $(CLEAR_VARS)
include $(my_LOCAL_PATH)/$(PNG_LIB_DIR)/Android.mk

# Build JPEG library
include $(CLEAR_VARS)
include $(my_LOCAL_PATH)/$(JPEG_LIB_DIR)/Android.mk

# Build CUPS library
include $(CLEAR_VARS)
include $(my_LOCAL_PATH)/$(CUPS_BASE_DIR)/Android.mk
