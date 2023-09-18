# main project for aud8110m1-emmc
ARCH := arm64
ARM_CPU := cortex-a53
TARGET := aud8110m1-emmc
WITH_KERNEL_VM := 0
WITH_LINKER_GC := 1
WITH_PMIC_MT6398 := 1
WITH_VCORE_PWM_BUCK := 1
WITH_CLKSQ_MONCK_OFF := 1
EMMC_PROJECT := 1

# arch for lz4 compression lib
GLOBAL_DEFINES += \
    LZ4_ARCH64=1

# add for dram config
GLOBAL_DEFINES += \
    SUPPORT_TYPE_LPDDR4=1 \
    LP4_MAX_2400=1 \
    BOOT_FREQ_1000=1

GLOBAL_DEFINES += EMMC_PROJECT=1

MODULES += \
        app/fitboot \

LK_HEAP_IMPLEMENTATION=miniheap
