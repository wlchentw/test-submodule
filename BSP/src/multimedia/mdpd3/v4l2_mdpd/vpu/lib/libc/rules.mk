LOCAL_DIR := $(GET_LOCAL_DIR)

OBJS += \
	$(LOCAL_DIR)/string/memcpy.o \
	$(LOCAL_DIR)/string/memset.o

INCLUDES += \
	-I$(LOCAL_DIR)

