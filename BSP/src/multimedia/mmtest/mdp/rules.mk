LOCAL_DIR := $(GET_LOCAL_DIR)

OBJS += \
	$(LOCAL_DIR)/mdp_api.o \
	$(LOCAL_DIR)/mdp_ut.o \

INCLUDES += \
         -I$(LOCAL_DIR)/../include/
#        -I$(LOCAL_DIR)/../../../kernel/linux/v4.9/include/uapi/linux


