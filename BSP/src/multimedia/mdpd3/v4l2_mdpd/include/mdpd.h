#include <stdint.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/ioctl.h>
#include "kernel/ipi.h"

#define PATH_DEV_VCU	"/dev/vcu1"
#define PATH_DEV_VPU	"/dev/vpu1"
#define DEV_NAME	"mdpd"

#define MAP_SHMEM_ALLOCATE	0x80000000
#define MAP_SHMEM_COMMIT	0x88000000
#define MAP_SHMEM_MM		0x90000000

/**
 * struct mem_obj - memory buffer allocated in kernel
 *
 * @iova:	iova of buffer
 * @len:	buffer length
 * @va: kernel virtual address
 */
struct mem_obj {
	uint32_t iova;
	uint32_t len;
	void *va;
};

enum {
	VPU_SET_OBJECT	= _IOW('v', 0, struct share_obj),
	VPUD_MVA_ALLOCATION	= _IOWR('v', 1, struct mem_obj),
	VPUD_MVA_FREE	= _IOWR('v', 2, struct mem_obj),
	VPUD_CACHE_FLUSH_ALL =	_IOR('v', 3, struct mem_obj),
	VPUD_GET_OBJECT	= _IOWR('v', 4, struct share_obj),
};
