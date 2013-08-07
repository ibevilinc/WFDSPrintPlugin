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

###############################################################################
#
# Generic
#
###############################################################################
MKDIR := mkdir -p
RMDIR := rm -r -f

OBJ_DIR_BASE := obj
BIN_DIR_BASE := bin
LIB_DIR_BASE := lib

ifeq ($(CROSS_COMPILE),)
CROSS_COMPILE="/usr/bin/"
endif


OBJ_DIR := $(OBJ_DIR_BASE)
BIN_DIR := $(BIN_DIR_BASE)
LIB_DIR := $(LIB_DIR_BASE)

M32 :=
#M32 := -m32
OS_VERSION := $(shell uname -s)

override CFLAGS :=

CP=/bin/cp
CC := $(CROSS_COMPILE)g++

COMMON_FLAGS := $(M32) -g -Wall -I. -Iinclude -Iwprint/plugins/ -Iwprint/plugins/genPCLm/inc/
COMMON_CFLAGS := $(M32) -x c $(COMMON_FLAGS)
COMMON_CPPFLAGS := $(M32) -x c++ $(COMMON_FLAGS)

###############################################################################
#
#  Plugin generic
#
###############################################################################
WPRINT_LIB_DIR := wprint/lib
WPRINT_TOOLS_DIR := wprint/tools
PLUGIN_DIR := wprint/plugins
GENPCLM_DIR:=$(PLUGIN_DIR)/genPCLm/src
OBJ_DIRS += $(OBJ_DIR)/$(PLUGIN_DIR)


##########################################################
#
# wprint tools
#
###############################################################################
WPRINT_TOOLS_OS_CFLAGS_Darwin := -DEXCLUDE_JPEG=1 -DEXCLUDE_PNG=1 -DEXCLUDE_PCLM=1
WPRINT_TOOLS_OBJS_OS_Darwin :=

WPRINT_TOOLS_OBJS_OS_Linux := \
	$(OBJ_DIR)/$(PLUGIN_DIR)/pclm_wrapper_api.o \
	$(OBJ_DIR)/$(GENPCLM_DIR)/genJPEGStrips.o \
	$(OBJ_DIR)/$(GENPCLM_DIR)/genPCLm.o \

WPRINT_TOOLS_OBJS := \
	$(OBJ_DIR)/$(PLUGIN_DIR)/wprint_image.o \
	$(OBJ_DIR)/$(PLUGIN_DIR)/wprint_image_platform.o \
	$(OBJ_DIR)/$(PLUGIN_DIR)/wprint_scaler.o \
	$(OBJ_DIR)/$(PLUGIN_DIR)/wprint_jpeg.o \
	$(OBJ_DIR)/$(PLUGIN_DIR)/wprint_png.o \
	$(OBJ_DIR)/$(PLUGIN_DIR)/wprint_ppm.o \
	$(OBJ_DIR)/$(PLUGIN_DIR)/wprint_hpimage.o \
	$(OBJ_DIR)/$(PLUGIN_DIR)/wprint_osimg.o \
	$(OBJ_DIR)/$(WPRINT_TOOLS_DIR)/scalerTest.o \
	$(OBJ_DIR)/$(WPRINT_LIB_DIR)/wprint_debug.o \
	$(OBJ_DIR)/$(WPRINT_LIB_DIR)/wprint_msgq.o \
	$(OBJ_DIR)/$(WPRINT_LIB_DIR)/printable_area.o \
	$(WPRINT_TOOLS_OBJS_OS_$(OS_VERSION))

WPRINT_TOOLS_WRAPPER := $(BIN_DIR)/scaler_test
WPRINT_TOOLS_WRAPPER_SCRIPT := $(WPRINT_TOOLS_DIR)/scaler_test
WPRINT_TOOLS := $(BIN_DIR)/scalerTest
WPRINT_TOOLS_CFLAGS := $(CFLAGS) $(COMMON_CFLAGS) -I$(WPRINT_LIB_DIR) $(WPRINT_TOOLS_OS_CFLAGS_$(OS_VERSION))
WPRINT_TOOLS_CPPFLAGS := $(CFLAGS) $(COMMON_CPPFLAGS) -I$(WPRINT_LIB_DIR) $(WPRINT_TOOLS_OS_CFLAGS_$(OS_VERSION))

$(OBJ_DIR)/$(GENPCLM_DIR)/%.d: $(GENPCLM_DIR)/%.cpp

$(OBJ_DIR)/$(GENPCLM_DIR)/%.o: $(GENPCLM_DIR)/%.cpp $(MAKEFILE_LIST) | $(OBJ_DIR)/$(GENPCLM_DIR)
	$(CC) -MD -c $(WPRINT_TOOLS_CPPFLAGS) $< -o $@	
	

$(OBJ_DIR)/$(WPRINT_LIB_DIR)/%.d: $(WPRINT_LIB_DIR)/%.c

$(OBJ_DIR)/$(WPRINT_LIB_DIR)/%.o: $(WPRINT_LIB_DIR)/%.c $(MAKEFILE_LIST) | $(OBJ_DIR)/$(WPRINT_LIB_DIR)
	$(CC) -MD -c $(WPRINT_TOOLS_CFLAGS) $< -o $@

$(OBJ_DIR)/$(WPRINT_TOOLS_DIR)/%.d: $(WPRINT_TOOLS_DIR)/%.c

$(OBJ_DIR)/$(WPRINT_TOOLS_DIR)/%.o: $(WPRINT_TOOLS_DIR)/%.c $(MAKEFILE_LIST) | $(OBJ_DIR)/$(WPRINT_TOOLS_DIR)
	$(CC) -MD -c $(WPRINT_TOOLS_CFLAGS) $< -o $@

$(OBJ_DIR)/$(PLUGIN_DIR)/%.d: $(PLUGIN_DIR)/%.c

$(OBJ_DIR)/$(PLUGIN_DIR)/%.o: $(PLUGIN_DIR)/%.c $(MAKEFILE_LIST) | $(OBJ_DIR)/$(PLUGIN_DIR)
	$(CC) -MD -c $(WPRINT_TOOLS_CFLAGS) $< -o $@
	
$(OBJ_DIR)/$(PLUGIN_DIR)/%.d: $(PLUGIN_DIR)/%.cpp

$(OBJ_DIR)/$(PLUGIN_DIR)/%.o: $(PLUGIN_DIR)/%.cpp $(MAKEFILE_LIST) | $(OBJ_DIR)/$(PLUGIN_DIR)
	$(CC) -MD -c $(WPRINT_TOOLS_CPPFLAGS) $< -o $@	

OBJ_DIRS += $(OBJ_DIR)/$(WPRINT_LIB_DIR)
OBJ_DIRS += $(OBJ_DIR)/$(WPRINT_TOOLS_DIR)
OBJ_DIRS += $(OBJ_DIR)/$(GENPCLM_DIR)
OBJS += $(WPRINT_TOOLS_OBJS)

LD_LIBS_OS_Linux  := -ljpeg -lpng -lz -lrt -lcupsimage
LD_LIBS_OS_Darwin := -lcupsimage

###############################################################################
#
# Build targets
#
###############################################################################
.PHONY: all clean tools $(BIN_DIR) $(LIB_DIR) $(OBJ_DIR) $(OBJ_DIRS)
.DEFAULT: all

all: tools

clean:
	@$(RMDIR) $(BIN_DIR) $(OBJ_DIR) $(LIB_DIR)

$(BIN_DIR) $(LIB_DIR) $(OBJ_DIRS) $(LIB_WPRINT_DIR):
	@$(MKDIR) $@

tools: $(WPRINT_TOOLS) $(WPRINT_TOOLS_WRAPPER)

$(WPRINT_TOOLS): $(WPRINT_TOOLS_OBJS) | $(BIN_DIR)
	$(RUNPATH) $(CC) $(M32) -o $@ $(WPRINT_TOOLS_OBJS) $(LD_LIBS_OS_$(OS_VERSION)) -lpthread

$(WPRINT_TOOLS_WRAPPER): $(WPRINT_TOOLS_WRAPPER_SCRIPT) | $(BIN_DIR)
	@$(CP) $(WPRINT_TOOLS_WRAPPER_SCRIPT) $@

-include $(OBJS:.o=.d)
