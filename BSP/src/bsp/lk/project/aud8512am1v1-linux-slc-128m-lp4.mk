# main project for aud8512am1v1-linux-slc-128m-lp4
ARCH := arm
ARM_CPU := cortex-a7
TARGET := aud8512am1v1-linux-slc-128m-lp4
WITH_KERNEL_VM := 0
WITH_LIB_NFTL := 1
WITH_LINKER_GC := 1
WITH_PMIC_MT6398 := 1
WITH_VCORE_PWM_BUCK := 1
WITH_EXT_32K := 1

# arch for lz4 compression lib
GLOBAL_DEFINES += \
    LZ4_ARCH64=1

#add for PEMI Init by haohao sun
GLOBAL_DEFINES += \
    SUPPORT_TYPE_LPDDR4=1 \
    LP4_MAX_2667=1 \

# add SLC nand GPT backup config
GLOBAL_DEFINES += \
    SUPPORT_SLC_GPT_BACKUP=1

MODULES += \
        app/fitboot \

LK_HEAP_IMPLEMENTATION=miniheap
