#include <arch_helpers.h>
#include <bl_common.h>
#include <mmio.h>
#include <mtk_sip_svc.h>
#include <platform_def.h>

#include "mpu_v1.h"

static unsigned char region_lock_state[EMI_MPU_REGION_NUM];
uint64_t sip_emi_mpu_set_region_protection(unsigned int start, unsigned int end,
							unsigned int region, unsigned int access_permission)
{
	unsigned int ax_pm;
	int ret = 0;

	ax_pm = (access_permission << 16) >> 16;

	if (region > EMI_MPU_REGION_NUM)
		return MTK_SIP_E_INVALID_RANGE;

	if ((start >= DRAM_OFFSET) && (end >= start)) {
		start -= DRAM_OFFSET;
		end -= DRAM_OFFSET;
	} else
		return MTK_SIP_E_INVALID_RANGE;
	start = start>>16;
	end = end>>16;
	mmio_write_32(EMI_MPU_APC(region, 0), 0);
	mmio_write_32(EMI_MPU_SA(region), start);
	mmio_write_32(EMI_MPU_EA(region),  end);
	mmio_write_32(EMI_MPU_APC(region, 0), ax_pm);
	return ret;
}

uint64_t sip_emi_mpu_set_region_protection_domain8(unsigned int start, unsigned int end,
							unsigned int region, unsigned int access_permission)
{

	int ret = 0;

	if (region >= EMI_MPU_REGION_NUM)
		return MTK_SIP_E_INVALID_RANGE;

#if ENABLE_EMI_MPU_SW_LOCK
	if (region_lock_state[region] == 1)
		return MTK_SIP_E_PERMISSION_DENY;

	if ((access_permission >> 31) & 0x1)
		region_lock_state[region] = 1;

	access_permission &= 0x00FFFFFF;
#else
	access_permission &= 0x80FFFFFF;
#endif

	if ((start >= DRAM_START_ADDRESS) && (end >= start)) {
		start -= DRAM_START_ADDRESS;
		end -= DRAM_START_ADDRESS;
	} else
		return MTK_SIP_E_INVALID_RANGE;

	start = start>>EMI_MPU_ALIGN_BITS;
	end = end>>EMI_MPU_ALIGN_BITS;
	mmio_write_32(EMI_MPU_APC(region, 0), 0);
	mmio_write_32(EMI_MPU_SA(region), start);
	mmio_write_32(EMI_MPU_EA(region),  end);
	mmio_write_32(EMI_MPU_APC(region, 0), access_permission);
	return ret;
}
uint64_t sip_emi_mpu_set_protection(unsigned int start, unsigned int end, unsigned int apc)
{
	unsigned int dgroup;
	unsigned int region;

	region = (start >> 24) & 0xFF;
	start &= 0x00FFFFFF;
	dgroup = (end >> 24) & 0xFF;
	end &= 0x00FFFFFF;

	if	((region >= EMI_MPU_REGION_NUM) || (dgroup > EMI_MPU_DGROUP_NUM))
		return MTK_SIP_E_INVALID_RANGE;

#if ENABLE_EMI_MPU_SW_LOCK
	if (region_lock_state[region] == 1)
		return MTK_SIP_E_PERMISSION_DENY;

	if ((dgroup == 0) && ((apc >> 31) & 0x1))
		region_lock_state[region] = 1;

	apc &= 0x00FFFFFF;
#else
	apc &= 0x80FFFFFF;
#endif

	if ((start >= DRAM_OFFSET) && (end >= start)) {
		start -= DRAM_OFFSET;
		end -= DRAM_OFFSET;
	} else
		return MTK_SIP_E_INVALID_RANGE;

	mmio_write_32(EMI_MPU_SA(region), start);
	mmio_write_32(EMI_MPU_EA(region), end);
	mmio_write_32(EMI_MPU_APC(region, dgroup), apc);

	return MTK_SIP_E_SUCCESS;
}

uint64_t sip_emi_mpu_clear_protection(unsigned int region)
{
	unsigned int dgroup;

	if	(region >= EMI_MPU_REGION_NUM)
		return MTK_SIP_E_INVALID_RANGE;

#if ENABLE_EMI_MPU_SW_LOCK
	if (region_lock_state[region] == 1)
		return MTK_SIP_E_PERMISSION_DENY;
#endif
	if (mmio_read_32(EMI_MPU_APC(region, 0)) & 0x80000000)
		return MTK_SIP_E_PERMISSION_DENY;

	for (dgroup = 0; dgroup < EMI_MPU_DGROUP_NUM; dgroup++)
		mmio_write_32(EMI_MPU_APC(region, dgroup), 0x0);

	mmio_write_32(EMI_MPU_SA(region), 0x0);
	mmio_write_32(EMI_MPU_EA(region), 0x0);

	return MTK_SIP_E_SUCCESS;
}

static int emi_mpu_access_forbidden(unsigned int offset)
{
	if ((offset >= EMI_MPU_START) && (offset <= EMI_MPU_END))
		return 0;
	else
		return 1;
}


uint64_t sip_emi_mpu_write(unsigned int offset, unsigned int reg_value)
{
	return MTK_SIP_E_PERMISSION_DENY;
}

uint64_t sip_emi_mpu_read(unsigned int offset)
{
	if (emi_mpu_access_forbidden(offset))
		return MTK_SIP_E_INVALID_RANGE;

	return (uint64_t) mmio_read_32(EMI_MPU_BASE + offset);
}

uint64_t emi_mpu_set_protection(struct emi_region_info_t *region_info)
{
	unsigned int start, end;
	int i;

	if (region_info->region >= EMI_MPU_REGION_NUM)
		return -1;

	start = (unsigned int)(region_info->start >> EMI_MPU_ALIGN_BITS) |
		(region_info->region << 24);

	for (i = EMI_MPU_DGROUP_NUM - 1; i >= 0; i--) {
		end = (unsigned int)(region_info->end >> EMI_MPU_ALIGN_BITS) |
			(i << 24);
		sip_emi_mpu_set_protection(start, end, region_info->apc[i]);
	}

	return 0;
}

