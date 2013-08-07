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

LOCAL_SRC_FILES:= \
	cups/array.c \
	cups/attr.c \
	cups/auth.c \
	cups/backchannel.c \
	cups/backend.c \
	cups/conflicts.c \
	cups/custom.c \
	cups/debug.c \
	cups/dest.c \
	cups/dest-job.c \
	cups/dest-localization.c \
	cups/dest-options.c \
	cups/dir.c \
	cups/emit.c \
	cups/encode.c \
	cups/file.c \
	cups/getdevices.c \
	cups/getifaddrs.c \
	cups/getputfile.c \
	cups/globals.c \
	cups/http.c \
	cups/http-addr.c \
	cups/http-addrlist.c \
	cups/http-support.c \
	cups/ipp.c \
	cups/ipp-support.c \
	cups/langprintf.c \
	cups/language.c \
	cups/localize.c \
	cups/mark.c \
	cups/md5.c \
	cups/md5passwd.c \
	cups/notify.c \
	cups/options.c \
	cups/page.c \
	cups/ppd.c \
	cups/ppd-cache.c \
	cups/pwg-media.c \
	cups/request.c \
	cups/sidechannel.c \
	cups/snmp.c \
	cups/snprintf.c \
	cups/string.c \
	cups/tempfile.c \
	cups/thread.c \
	cups/transcode.c \
	cups/usersys.c \
	cups/util.c \
	filter/error.c \
	filter/raster.c \

disabled_src_files:= \


LOCAL_C_INCLUDES := cups
LOCAL_CFLAGS := -D_PPD_DEPRECATED=
LOCAL_MODULE := lib$(PRIV_LIB_NAME)cups
LOCAL_MODULE_TAGS := optional
LOCAL_LDLIBS += -lz


include $(BUILD_SHARED_LIBRARY)
