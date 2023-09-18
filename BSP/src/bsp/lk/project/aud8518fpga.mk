# main project for aud8518fpga
ARCH := arm64
ARM_CPU := cortex-a53
TARGET := aud8518fpga
WITH_KERNEL_VM := 1
WITH_LINKER_GC := 1

# arch for lz4 compression lib
GLOBAL_DEFINES += \
    LZ4_ARCH64=1

# add for fpga config
GLOBAL_DEFINES += \
    FPGA_PLATFORM=1

# add for dram config
GLOBAL_DEFINES += \
    SUPPORT_TYPE_LPDDR3=1

MODULES += \
        app/fitboot \

LK_HEAP_IMPLEMENTATION=miniheap
