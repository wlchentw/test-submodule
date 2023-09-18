# main project for aud8512fpga-ldvt-slc
ARCH := arm64
ARM_CPU := cortex-a53
TARGET := aud8512fpga-ldvt-slc
WITH_KERNEL_VM := 1
WITH_LINKER_GC := 1
WITH_PMIC_MT6395 := 1
WITH_VCORE_PWM_BUCK := 1
WITH_32K_OSC := 1
WITH_USB_MD1122 := 1

# arch for lz4 compression lib
GLOBAL_DEFINES += \
    LZ4_ARCH64=1

# add for dram config
GLOBAL_DEFINES += \
    SUPPORT_TYPE_LPDDR3=1

# add SLC nand GPT backup config
GLOBAL_DEFINES += \
    SUPPORT_SLC_GPT_BACKUP=1

MODULES += \
        app/fitboot \

LK_HEAP_IMPLEMENTATION=miniheap
