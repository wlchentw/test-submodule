SIGMA_LOCAL_DIR := $(call my-dir)
BIN_OUT_PATH := $(SIGMA_LOCAL_DIR)/Release/programs

include $(SIGMA_LOCAL_DIR)/lib/Android.mk
include $(SIGMA_LOCAL_DIR)/dut/Android.mk
include $(SIGMA_LOCAL_DIR)/console_src/Android.mk