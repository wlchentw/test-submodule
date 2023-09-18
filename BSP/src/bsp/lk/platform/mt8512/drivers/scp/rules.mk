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
    $(LOCAL_DIR)/mt_scp.c

include make/module.mk
