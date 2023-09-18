LOCAL_H_DIR := $(GET_LOCAL_DIR)
LOCAL_DIR := v4l2_mdpd/posix

OBJS += \
	$(LOCAL_DIR)/kernel/event.c \
	$(LOCAL_DIR)/kernel/ipi.c \
	$(LOCAL_DIR)/kernel/mutex.c \
	$(LOCAL_DIR)/kernel/thread.c \

INCLUDES += \
	$(LOCAL_H_DIR)/include
