ccflags-y += -I$(srctree)/include/soc
ccflags-y += -I$(srctree)/include/linux/
ccflags-y += -I$(srctree)/lib/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/hwtcon/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/hwtcon/hal
ccflags-y += -I$(srctree)/drivers/misc/mediatek/cmdq/v3
ccflags-y += -I$(srctree)/drivers/misc/mediatek/cmdq/v3/mt8512

MODULE_NAME = cfa

$(MODULE_NAME)-src := cfa_driver.c epdfb_dc.c cfa_ntx.c
$(MODULE_NAME)-objs := cfa_driver.o epdfb_dc.o cfa_ntx.o
$(MODULE_NAME)-objs += EInk_Kaleido_render.o
obj-$(CONFIG_MTK_CFA) += $(MODULE_NAME).o
