#
# Copyright (C) 2020 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

# Reference to mdif_github/src/hdlc
HDLC_PATH=$(LOCAL_PATH)/../../../../../hdlc

# Include c-files for HDLC Java port
HDLC_SRC=$(HDLC_PATH)/dlc/dlc.c \
            $(HDLC_PATH)/yahdlc/yahdlc.c \
            $(HDLC_PATH)/yahdlc/fcs.c \
            $(HDLC_PATH)/ports/java/java_port.c \
            $(HDLC_PATH)/ports/java/hdlc_jni.c

# Name of local module referenced in Android
LOCAL_MODULE     := hdlc-lib
# Source files for this library including JNI
LOCAL_SRC_FILES  := $(HDLC_SRC) hdlc_jni_wrapper.c
LOCAL_LDLIBS     := -llog -landroid

# Path include for library
LOCAL_C_INCLUDES += $(HDLC_PATH)/ports/java
# Include "mdif_github/src" as this is used as base reference in some files
LOCAL_C_INCLUDES += $(HDLC_PATH)/..

include $(BUILD_SHARED_LIBRARY)
