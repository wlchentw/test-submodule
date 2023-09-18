LOCAL_DIR := $(GET_LOCAL_DIR)

OBJS += \
	$(LOCAL_DIR)/img_hal_test.o \
	$(LOCAL_DIR)/img_hal.o \
	$(LOCAL_DIR)/md5.o \
	$(LOCAL_DIR)/ut_v4l2.o \

INCLUDES += \
         -I$(LOCAL_DIR)/../include/