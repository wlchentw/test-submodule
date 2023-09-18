LOCAL_DIR := $(GET_LOCAL_DIR)
MODULE := $(LOCAL_DIR)

CFLAGS := $(filter-out -Werror, $(CFLAGS))
GLOBAL_CFLAGS := $(filter-out -Werror, $(GLOBAL_CFLAGS))

CFLAGS := $(filter-out -Werror=return-type, $(CFLAGS))
GLOBAL_CFLAGS := $(filter-out -Werror=return-type, $(GLOBAL_CFLAGS))

CFLAGS := $(filter-out -Werror=implicit-function-declaration, $(CFLAGS))
GLOBAL_CFLAGS := $(filter-out -Werror=implicit-function-declaration, $(GLOBAL_CFLAGS))

MODULE_DEPS += \
    lib/bio \
    lib/partition \

MODULE_SRCS += \
    $(LOCAL_DIR)/slc_bdev.c \
    $(LOCAL_DIR)/slc/slc.c \
    $(LOCAL_DIR)/slc/slc_ids.c \
    $(LOCAL_DIR)/slc/ecc/ecc.c \
    $(LOCAL_DIR)/slc/bbt/bbt.c \
    $(LOCAL_DIR)/slc/nfi/nfi.c \
    $(LOCAL_DIR)/slc/test/slc_test.c \

include make/module.mk
