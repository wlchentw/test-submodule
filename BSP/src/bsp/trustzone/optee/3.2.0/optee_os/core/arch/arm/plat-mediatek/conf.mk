PLATFORM_FLAVOR ?= mt8173

include core/arch/arm/cpu/cortex-armv8-0.mk

CFG_TZDRAM_START ?= 0xbe000000
CFG_TZDRAM_SIZE ?= 0x01e00000
CFG_SHMEM_START ?= 0xbfe00000
CFG_SHMEM_SIZE ?= 0x00200000

$(call force,CFG_8250_UART,y)
$(call force,CFG_GENERIC_BOOT,y)
$(call force,CFG_PM_STUBS,y)
$(call force,CFG_SECURE_TIME_SOURCE_CNTPCT,y)
$(call force,CFG_WITH_ARM_TRUSTED_FW,y)
$(call force,CFG_GIC,y)

ta-targets = ta_arm32

ifeq ($(CFG_ARM64_core),y)
$(call force,CFG_WITH_LPAE,y)
ta-targets += ta_arm64
else
$(call force,CFG_ARM32_core,y)
endif

CFG_WITH_STACK_CANARIES ?= y

ifeq ($(findstring fpga,$(MTK_PROJECT)),fpga)
CFG_FPGA ?= y
endif

ifeq ($(PLATFORM_FLAVOR),mt8173)
$(call force,CFG_TEE_CORE_NB_CORE,4)
# 2**1 = 2 cores per cluster
$(call force,CFG_CORE_CLUSTER_SHIFT,1)
endif

ifeq ($(PLATFORM_FLAVOR),mt8512)
$(call force,CFG_TEE_CORE_NB_CORE,2)
$(call force,CFG_ARM_GICV3,y)
endif

ifeq ($(PLATFORM_FLAVOR),mt8532)
$(call force,CFG_TEE_CORE_NB_CORE,4)
# 2**0 = 1 core per cluster
$(call force,CFG_CORE_CLUSTER_SHIFT,0)
$(call force,CFG_ARM_GICV3,y)
$(call force,CFG_CORE_HEAP_SIZE,131072)
# allow unaligned load/store
$(call force,CFG_SCTLR_ALIGNMENT_CHECK,n)
endif
