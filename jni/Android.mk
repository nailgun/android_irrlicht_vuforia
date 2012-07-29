LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := irrlicht_vuforia

LS_CPP=$(subst $(1)/,,$(wildcard $(1)/*.cpp))
LOCAL_SRC_FILES := $(call LS_CPP,$(LOCAL_PATH))

LOCAL_LDLIBS := -llog
LOCAL_SHARED_LIBRARIES := QCAR
LOCAL_STATIC_LIBRARIES := irrlicht

include $(BUILD_SHARED_LIBRARY)

IRRLICHT_COMPILE_CONFIG_DIR := $(LOCAL_PATH)/libs/irrlicht
include $(LOCAL_PATH)/libs/QCAR/Android.mk
$(call import-module,irrlicht)
