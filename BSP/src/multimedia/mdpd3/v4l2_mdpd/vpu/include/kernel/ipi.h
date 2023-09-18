#ifndef __IPI_H
#define __IPI_H

/**
 * Please note that the values of IPI IDs MUST be same as mtk-vcu.h in kernel.
 * Each daemon can only keep the IPI IDs used by itself.
 */
enum ipi_id {
	IPI_VPU_INIT = 0,
	IPI_MDP = 13,
	IPI_MDP_1 = 14,
	IPI_MDP_2 = 15,
	IPI_MDP_3 = 16,
	NR_IPI = 50,
};

enum ipi_status
{
    ERROR =-1,
    DONE,
    BUSY,
};

typedef void(*ipi_handler_t)(int id, void * data, uint len);

struct ipi_ctx {
    int fd;
    int cmd;
};

struct ipi_desc{
    ipi_handler_t handler;
    const char  *name;
};

#define SHARE_BUF_SIZE 64
struct share_obj {
    enum ipi_id id;
    unsigned int len;
    unsigned char share_buf[SHARE_BUF_SIZE - 16];
};

#define IPI_INST_CNT 1 /* Currently only define 2 in vpud */
#define IPI_SYNC_ONLY

void vpu_ipi_init(void);
enum ipi_status vpu_ipi_registration(enum ipi_id id, ipi_handler_t handler, const char *name);

enum ipi_status vpu_ipi_send(enum ipi_id id, void* buf, uint len, unsigned int wait);
enum ipi_status vpu_ipi_status(enum ipi_id id);

#endif
