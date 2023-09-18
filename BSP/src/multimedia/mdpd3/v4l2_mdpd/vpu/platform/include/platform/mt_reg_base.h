#pragma once

#include <stdint.h>

#define MAP_VDEC_BASE		0x70000000
#define MAP_VDEC_RANGE		0x00030000
#define MAP_VENC_BASE		0x71000000
#define MAP_VENC_RANGE		0x00001000
#define MAP_VENC_LT_BASE	0x72000000
#define MAP_VENC_LT_RANGE	0x00001000

#define MAP_SHMEM_ALLOCATE	0x80000000
#define MAP_SHMEM_COMMIT	0x88000000

extern uintptr_t base_vdec;
extern uintptr_t base_venc;
extern uintptr_t base_venc_lt;

#define MT_VDEC_HW_BASE			base_vdec
#define MT8173_VENC_HW_BASE		base_venc
#define MT8173_VENC_LT_HW_BASE		base_venc_lt
