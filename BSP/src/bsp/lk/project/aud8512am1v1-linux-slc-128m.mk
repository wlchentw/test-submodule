# main project for aud8512am1v1-linux-slc-128m
ARCH := arm64
ARM_CPU := cortex-a53
TARGET := aud8512am1v1-linux-slc-128m
WITH_KERNEL_VM := 1
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
    SUPPORT_TYPE_LPDDR3=1 \
    LP3_128MB_SUPPORT=1 \

# add SLC nand GPT backup config
GLOBAL_DEFINES += \
    SUPPORT_SLC_GPT_BACKUP=1

MODULES += \
        app/fitboot \

LK_HEAP_IMPLEMENTATION=miniheap