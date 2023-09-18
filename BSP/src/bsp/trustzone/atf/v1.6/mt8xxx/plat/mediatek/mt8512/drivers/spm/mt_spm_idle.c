#include <mmio.h>
#include <platform_def.h>
#include <mt_spm_reg.h>

/* infra */
#define MODULE_SW_CG_0_STA		(INFRACFG_AO_BASE + 0x90)
#define MODULE_SW_CG_1_STA		(INFRACFG_AO_BASE + 0x94)
#define MODULE_SW_CG_2_STA		(INFRACFG_AO_BASE + 0xac)
#define MODULE_SW_CG_3_STA		(INFRACFG_AO_BASE + 0xc8)
#define MODULE_SW_CG_4_STA		(INFRACFG_AO_BASE + 0xd8)
#define MODULE_SW_CG_1_MASK		(0x08000000)

/* mmsys */
#define MMSYS_CG_CON0			(MMSYS_BASE + 0x100)
#define MMSYS_CG_OTH			(0x10006374)
#define IMAGE_CG_CON0			(0x15000100)
#define IMAGE_CG_CON1			(0x15000110)
#define IMAGE_CG_OTH			(0x1000638C)

uint64_t spm_idle_check_secure_cg(uint64_t x1, uint64_t x2, uint64_t x3)
{
	uint32_t val = 0;

	/* Check DXCC secure core CG: 0x10001094[27] */
	val = ~mmio_read_32(MODULE_SW_CG_1_STA);
	val &= MODULE_SW_CG_1_MASK;

	return (uint64_t)val;
}

uint32_t spm_get_infra1_sta(void)
{
	return mmio_read_32(MODULE_SW_CG_1_STA);
}

uint32_t spm_get_infra0_sta(void)
{
	return mmio_read_32(MODULE_SW_CG_0_STA);
}

uint32_t spm_get_infra2_sta(void)
{
	return mmio_read_32(MODULE_SW_CG_2_STA);
}

uint32_t spm_get_infra3_sta(void)
{
	return mmio_read_32(MODULE_SW_CG_3_STA);
}

uint32_t spm_get_infra4_sta(void)
{
	return mmio_read_32(MODULE_SW_CG_4_STA);
}

uint32_t spm_get_mmsys_con0(void)
{
	return mmio_read_32(MMSYS_CG_CON0);
}

uint32_t spm_get_mmsys_oth(void)
{
	return mmio_read_32(MMSYS_CG_OTH);
}

uint32_t spm_get_image_con0(void)
{
	return mmio_read_32(IMAGE_CG_CON0);
}

uint32_t spm_get_image_con1(void)
{
	return mmio_read_32(IMAGE_CG_CON1);
}

uint32_t spm_get_image_oth(void)
{
	return mmio_read_32(IMAGE_CG_OTH);
}
