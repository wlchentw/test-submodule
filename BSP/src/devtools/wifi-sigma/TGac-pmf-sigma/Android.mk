
LOCAL_COMPILE=yes

ifeq ($(LOCAL_COMPILE),yes)
BIN_PATH := $(call my-dir)/Release/scripts/Update_Software/sw
SIGMA_TARGET = tgac_pmf
SIGMA_LOCAL_DIR := $(call my-dir)

MTK_P2P_CFLAGS := -DMTK_P2P_SUPPLICANT -DMTK_P2P_SIGMA
MTK_HS20_CFLAGS := -DMTK_HS20_SIGMA
MTK_AC_CFLAGS := -DMTK_AC_SIGMA
MTK_PMF_CFLAGS := -DMTK_PMF_SIGMA

### Modify here for different project
MTK_CFLAGS := $(MTK_AC_CFLAGS) $(MTK_HS20_CFLAGS) $(MTK_P2P_CFLAGS) $(MTK_PMF_CFLAGS) -DMTK_PLATFORM

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
