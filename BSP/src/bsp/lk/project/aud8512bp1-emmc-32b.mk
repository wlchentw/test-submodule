# main project for aud8512bp1-emmc-32b
ARCH := arm64
ARM_CPU := cortex-a53
TARGET := aud8512bp1-emmc-32b
WITH_KERNEL_VM := 1
WITH_LINKER_GC := 1
WITH_PMIC_MT6398 := 1

# arch for lz4 compression lib
GLOBAL_DEFINES += \
    LZ4_ARCH64=1

# add for dram config
GLOBAL_DEFINES += \
    SUPPORT_TYPE_LPDDR4=1

MODULES += \
        app/fitboot \

LK_HEAP_IMPLEMENTATION=miniheap
