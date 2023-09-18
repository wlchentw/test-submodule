
#export DISABLE_AUTO_INSTALLCLEAN=true
#rm -rf   out/target/product/mt6620_testbed/obj/STATIC_LIBRARIES/libwfa_*

LOCAL_COMPILE=yes

ifeq ($(LOCAL_COMPILE),yes)

SIGMA_LOCAL_DIR := $(call my-dir)
include $(SIGMA_LOCAL_DIR)/lib/Android.mk
include $(SIGMA_LOCAL_DIR)/dut/Android.mk
#include $(SIGMA_LOCAL_DIR)/ca/Android.mk
#include $(SIGMA_LOCAL_DIR)/cli/Android.mk
#include $(SIGMA_LOCAL_DIR)/corelib/Android.mk
#include $(SIGMA_LOCAL_DIR)/ofconfig/Android.mk



# ############ wfa_dut package
# LOCAL_PATH:= $(call my-dir)
# include $(CLEAR_VARS)

# LOCAL_SRC_FILES:= \
# 	$(LOCAL_MODULE_PATH)/wfa_dut \
# 	$(LOCAL_PATH)/script


# LOCAL_PREBUILT_LIBRARIES := \


# LOCAL_PACKAGE_NAME:= wfa_dut_apk
# LOCAL_MODULE_TAGS := eng debug tests 

# include $(BUILD_STATIC_LIBRARY)

endif
