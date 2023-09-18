LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

# Basic compile options
L_CFLAGS += -g -O2 -D_REENTRANT  -DWFA_WMM_PS_EXT -DWFA_WMM_AC -DWFA_VOICE_EXT -DWFA_STA_TB -Wall -I../inc -static 

# Enable CFG80211 Support
L_CFLAGS += -DMTK_CFG80211_SIGMA 

# Enable Android TV Environment Support
L_CFLAGS += -DMTK_ANDROID_SIGMA

# Android AOSP
L_CFLAGS += -DANDROID_AOSP -DCTRL_IFACE_PATH=\"/data/misc/wifi/sockets\"

# Enable WiFi Direct Support
L_CFLAGS += -DMTK_P2P_SIGMA 


include $(LOCAL_PATH)/lib/Android.mk \
        $(LOCAL_PATH)/dut/Android.mk \
        $(LOCAL_PATH)/ca/Android.mk
