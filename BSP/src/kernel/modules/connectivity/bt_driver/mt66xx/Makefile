export KERNEL_SRC := /lib/modules/$(shell uname -r)/build

$(warning $(CC))

###############################################################################
# stp_cdev_bt
###############################################################################
MODULE_NAME = wmt_cdev_bt

$(MODULE_NAME)-objs := stp_chrdev_bt.o
ifneq ($(CONFIG_MTK_CONNSYS_DEDICATED_LOG_PATH),)
$(MODULE_NAME)-objs += fw_log_bt.o
endif
###############################################################################
# Common
###############################################################################
EXTRA_SYMBOLS = ${TOPDIR}/tmp/work/${PACKAGE_ARCH}${TARGET_VENDOR}-${TARGET_OS}/mt66xx-wmt-drv/1.0-r0/mt66xx-wmt-drv-1.0/Module.symvers

ccflags-y += -fno-pic
ccflags-y += -D MTK_WCN_WMT_STP_EXP_SYMBOL_ABSTRACT
ccflags-y += -D CREATE_NODE_DYNAMIC=1
ccflags-y += -D CFG_MTK_ANDROID_WMT=1
ccflags-y += \
	-I$(CONNECTIVITY_SRC)/wmt_mt66xx/common_main/include \
	-I$(CONNECTIVITY_SRC)/wmt_mt66xx/common_main/linux/include \
	-I$(CONNECTIVITY_SRC)/../../linux/${KERNEL_VER}/drivers/misc/mediatek/include/mt-plat
ifneq ($(CONFIG_MTK_CONNSYS_DEDICATED_LOG_PATH),)
ccflags-y += -I$(CONNECTIVITY_SRC)/wmt_mt66xx/debug_utility
endif

obj-m := $(MODULE_NAME).o

ifeq ($(CONFIG_MTK_CONN_LTE_IDC_SUPPORT),y)
    ccflags-y += -D WMT_IDC_SUPPORT=1
else
    ccflags-y += -D WMT_IDC_SUPPORT=0
endif

all:
	make -C $(KERNEL_SRC) M=$(PWD) modules

bt_drv:
ifneq ($(findstring $(PLATFORM), MT8133_YOCTO MT8512_YOCTO MT2712_YOCTO),)
	make -C $(LINUX_SRC) M=$(PWD) modules
else
	make -C $(KERNEL_SRC) M=$(PWD) $(MODULE_NAME).ko
endif

clean:
	make -C $(KERNEL_SRC) M=$(PWD) clean

