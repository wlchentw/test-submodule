# main project for fpga8512-snand-32b
ARCH := arm64
ARM_CPU := cortex-a53
TARGET := fpga8512-snand-32b
WITH_KERNEL_VM := 1
WITH_LINKER_GC := 1
WITH_PMIC_MT6395 := 0
WITH_VCORE_I2C_BUCK := 0
WITH_USB_MD1122 := 1

# arch for lz4 compression lib
GLOBAL_DEFINES += \
    LZ4_ARCH64=1

# add for fpga config
GLOBAL_DEFINES += \
    FPGA_PLATFORM=1

# add for dram config
GLOBAL_DEFINES += \
    SUPPORT_TYPE_LPDDR4=1

# add SLC nand GPT backup config
GLOBAL_DEFINES += \
    SUPPORT_SLC_GPT_BACKUP=1

MODULES += \
        app/fitboot \

LK_HEAP_IMPLEMENTATION=miniheap
