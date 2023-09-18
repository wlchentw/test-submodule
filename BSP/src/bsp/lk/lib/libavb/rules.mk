LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)
MODULE_BUILDDIR := $(call TOBUILDDIR,$(MODULE))
MODULE_INCLUDES += $(MODULE_BUILDDIR)/../../../include

MODULE_SRCS += \
$(LOCAL_DIR)/avb_hashtree_descriptor.c \
$(LOCAL_DIR)/avb_kernel_cmdline_descriptor.c \
$(LOCAL_DIR)/avb_vbmeta_image.c \
$(LOCAL_DIR)/avb_property_descriptor.c \
$(LOCAL_DIR)/avb_hash_descriptor.c \
$(LOCAL_DIR)/avb_slot_verify.c \
$(LOCAL_DIR)/avb_chain_partition_descriptor.c \
$(LOCAL_DIR)/avb_rsa.c \
$(LOCAL_DIR)/avb_sysdeps_posix.c \
$(LOCAL_DIR)/avb_sha256.c \
$(LOCAL_DIR)/avb_version.c \
$(LOCAL_DIR)/avb_util.c \
$(LOCAL_DIR)/avb_footer.c \
$(LOCAL_DIR)/avb_crc32.c \
$(LOCAL_DIR)/avb_crypto.c \
$(LOCAL_DIR)/avb_descriptor.c \
$(LOCAL_DIR)/avb_sha512.c \

MODULE_DEFINES += AVB_COMPILATION AVB_ENABLE_DEBUG
# log for debug
#CFLAGS += -DAVB_ENABLE_DEBUG

include make/module.mk
