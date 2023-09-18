LOCAL_H_DIR := $(GET_LOCAL_DIR)
LOCAL_DIR := v4l2_mdpd/vpu/app/mdp

$(warning DESTDIR = $(DESTDIR))

INCLUDES += \
    $(LOCAL_H_DIR)/ \
    $(LOCAL_H_DIR)/../../include \
    $(LOCAL_H_DIR)/../../../include \
    $(LOCAL_H_DIR)/../../../../libmdp/if

MDPOBJS_BUILD_ERR  += \
    $(LOCAL_DIR)/../app.c \
    $(LOCAL_DIR)/mdp_srv.cpp \
    $(LOCAL_DIR)/mdp_process_srv.cpp \
    $(LOCAL_DIR)/mdp_drv.cpp

MDPOBJS  += \
    v4l2_mdpd/vpu/app/app.c \
    v4l2_mdpd/vpu/app/mdp/mdp_chip.cpp \
    v4l2_mdpd/vpu/app/mdp/mdp_srv.cpp \
    v4l2_mdpd/vpu/app/mdp/mdp_process_srv.cpp \
    v4l2_mdpd/vpu/app/mdp/mdp_drv.cpp

