LOCAL_DIR := $(GET_LOCAL_DIR)

OBJS += \
	$(LOCAL_DIR)/app/app.o \
	$(LOCAL_DIR)/kernel/event.o \
	$(LOCAL_DIR)/kernel/ipi.o \
	$(LOCAL_DIR)/kernel/mutex.o \
	$(LOCAL_DIR)/kernel/thread.o \

INCLUDES += \
	-I$(LOCAL_DIR)/include \
