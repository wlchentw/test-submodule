# main project for aud8110p1
ARCH := arm64
ARM_CPU := cortex-a53
TARGET := aud8110p1
WITH_KERNEL_VM := 0
WITH_LINKER_GC := 1
WITH_PMIC_BD71828 := 1
WITH_VCORE_PWM_BUCK := 1
WITH_EXT_32K := 1
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

# add rohm workaround for power-on issue
GLOBAL_DEFINES += \
    ROHM_DISABLE_PG

MODULES += \
        app/fitboot \

LK_HEAP_IMPLEMENTATION=miniheap
