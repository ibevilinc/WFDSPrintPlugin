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

LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_ARM_MODE := arm

LOCAL_SRC_FILES := \
	jcapimin.c jcapistd.c jccoefct.c jccolor.c jcdctmgr.c jchuff.c \
	jcinit.c jcmainct.c jcmarker.c jcmaster.c jcomapi.c jcparam.c \
	jcphuff.c jcprepct.c jcsample.c jctrans.c jdapimin.c jdapistd.c \
	jdatadst.c jdatasrc.c jdcoefct.c jdcolor.c jddctmgr.c jdhuff.c \
	jdinput.c jdmainct.c jdmarker.c jdmaster.c jdmerge.c jdphuff.c \
	jdpostct.c jdsample.c jdtrans.c jerror.c jfdctflt.c jfdctfst.c \
	jfdctint.c jidctflt.c jidctfst.c jidctint.c jidctred.c jquant1.c \
	jquant2.c jutils.c jmemmgr.c armv6_idct.S

# use ashmem as libjpeg decoder's backing store
#LOCAL_CFLAGS += -DUSE_ANDROID_ASHMEM
#LOCAL_SRC_FILES += \
#	jmem-ashmem.c

# the original android memory manager.
# use sdcard as libjpeg decoder's backing store
LOCAL_SRC_FILES += \
	jmem-android.c

LOCAL_CFLAGS += -DAVOID_TABLES 
LOCAL_CFLAGS += -O3 -fstrict-aliasing -fprefetch-loop-arrays
#LOCAL_CFLAGS += -march=armv6j

# enable tile based decode
LOCAL_CFLAGS += -DANDROID_TILE_BASED_DECODE

# enable armv6 idct assembly
ifeq ($(TARGET_ARCH_VARIANT),x86-atom)
LOCAL_CFLAGS += -DANDROID_INTELSSE2_IDCT
LOCAL_SRC_FILES += jidctintelsse.c
else
# enable armv6 idct assembly
LOCAL_CFLAGS += -DANDROID_ARMV6_IDCT
endif

LOCAL_MODULE:= lib$(PRIV_LIB_NAME)jpeg

LOCAL_SHARED_LIBRARIES := \
	libcutils

include $(BUILD_SHARED_LIBRARY)
