ccflags-y += -I$(srctree)/drivers/misc/mediatek/hwtcon/hal
ccflags-y += -I$(srctree)/drivers/misc/mediatek/pmic/fiti
ccflags-y += -I$(srctree)/include/soc
ccflags-y += -I$(srctree)/include/linux/
ccflags-y += -I$(srctree)/lib/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/cmdq/v3
ccflags-y += -I$(srctree)/drivers/misc/mediatek/cmdq/v3/mt8512
ccflags-y += -I$(srctree)/drivers/media/platform/mtk-mdp
ccflags-y += -I$(srctree)/drivers/media/platform/mtk-vcu
ccflags-y += -I$(srctree)/drivers/misc/mediatek/
ccflags-y += -I$(srctree)/drivers/media/platform/mtk-imgrz

ccflags-y += -DHWTCON_M4U_SUPPORT
# ccflags-y += -DFPGA_EARLY_PORTING
# ccflags-y += -DMARKER_V2_ENABLE
ccflags-y += -DCMDQ_RELEASE_LUT

MODULE_NAME = hwtcon

$(MODULE_NAME)-objs := hwtcon_epd.o
$(MODULE_NAME)-objs += hwtcon_driver.o
$(MODULE_NAME)-objs += hwtcon_fb.o
$(MODULE_NAME)-objs += hwtcon_rect.o
$(MODULE_NAME)-objs += hwtcon_core.o
$(MODULE_NAME)-objs += hwtcon_debug.o
$(MODULE_NAME)-objs += hwtcon_mdp.o
$(MODULE_NAME)-objs += hwtcon_file.o
$(MODULE_NAME)-objs += decompress_inflate.o

#HAL code
$(MODULE_NAME)-objs += hal/hwtcon_hal.o
$(MODULE_NAME)-objs += hal/hwtcon_dpi_config.o
$(MODULE_NAME)-objs += hal/hwtcon_tcon_config.o
$(MODULE_NAME)-objs += hal/hwtcon_pipeline_config.o
$(MODULE_NAME)-objs += hal/hwtcon_regal_config.o
$(MODULE_NAME)-objs += hal/hwtcon_wf_lut_config.o
$(MODULE_NAME)-objs += hal/hwtcon_wf_lut_rdma_config.o

obj-$(CONFIG_MTK_HWTCON) += $(MODULE_NAME).o
