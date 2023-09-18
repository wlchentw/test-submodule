LOCAL_H_DIR := $(GET_LOCAL_DIR)
LOCAL_DIR := v4l2_mdpd/vpu/lib/libc

OBJS += \
	$(LOCAL_DIR)/string/memcpy.c \
	$(LOCAL_DIR)/string/memset.c

INCLUDES += \
	$(LOCAL_H_DIR)

