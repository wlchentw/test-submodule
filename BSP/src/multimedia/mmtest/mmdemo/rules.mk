LOCAL_DIR := $(GET_LOCAL_DIR)

OBJS += \
	$(LOCAL_DIR)/mdp_api.o \
	$(LOCAL_DIR)/main.o \
	$(LOCAL_DIR)/pattern.o \
	$(LOCAL_DIR)/pattern_graphic.o \
	$(LOCAL_DIR)/pattern_picture.o \
	$(LOCAL_DIR)/pattern_text.o \
	$(LOCAL_DIR)/pattern_ui.o \
	$(LOCAL_DIR)/hwtcon_display.o \
	$(LOCAL_DIR)/img_hal.o \
	$(LOCAL_DIR)/md5.o \
	$(LOCAL_DIR)/ut_v4l2.o \

INCLUDES += \
        -I$(LOCAL_DIR)/../include \
        -I$(LOCAL_DIR)/../mdp \
        -I$(LOCAL_DIR)/../hwtcon_test \
        -I$(LOCAL_DIR)/../img_hal \
