#include <arch_helpers.h>
#include <arch.h>
#include <l2c.h>
#include <mcucfg.h>
#include <mmio.h>
#include <plat_private.h>
#include <platform_def.h>
#include <spinlock.h>

void config_L2_size(void)
{
	mmio_write_32(L2C_CFG_MP0, L2C_SIZE_CFG_OFF);
}
