LOCAL_DIR := $(GET_LOCAL_DIR)

$(warning DESTDIR = $(DESTDIR))

INCLUDES += \
    -I$(LOCAL_DIR)/ \
    -I$(LOCAL_DIR)/../../include \
    -I$(LOCAL_DIR)/../../../../libmdp/if \
    -I$(LOCAL_DIR)/../../../../libmdp/dpframework/include

MDPOBJS  += \
    $(LOCAL_DIR)/mdp_chip.o \
    $(LOCAL_DIR)/mdp_srv.o \
    $(LOCAL_DIR)/mdp_process_srv.o \
    $(LOCAL_DIR)/mdp_drv.o
