#include <stdio.h>
#include <sys/ioctl.h>
#include <assert.h>
#include <semaphore.h>
#include <pthread.h>
#include <stdbool.h>
#include <string.h>

#include "sys/types.h"
#include "kernel/ipi.h"

struct ipi_desc ipi_desc[NR_IPI];

#ifndef IPI_SYNC_ONLY
static sem_t work_sem[IPI_INST_CNT];
static sem_t done_sem[IPI_INST_CNT];
#endif
static pthread_mutex_t mutex[IPI_INST_CNT] = {PTHREAD_MUTEX_INITIALIZER
					#if IPI_INST_CNT == 2
					, PTHREAD_MUTEX_INITIALIZER
					#endif
};
static struct share_obj obj[IPI_INST_CNT];
static bool busy[IPI_INST_CNT];
static bool sync[IPI_INST_CNT];
struct ipi_ctx g_ctx_s; /* initialized in daemon (e.g. mdpd.c) */

#define ipi_log(f, a...) fprintf(stderr, f, ##a)

int ipi_id_from_inst_id(int id)
{
#if IPI_INST_CNT > 1
	/* Only vpud goes here */
	/* Assume instance 1 is for VENC and 0 for others. */
	if (IPI_INST_CNT == 2 && id == 1)
		return IPI_VENC_H264; /* set a any of IPI_VENC_ id.*/
	else
		return IPI_VPU_INIT;  /* set a any of non VENC IPI id. */
#else
	return IPI_VPU_INIT;  /* set a any of non VENC IPI id. */
#endif
}

inline int ipi_id_to_inst_id(int id)
{
#if IPI_INST_CNT > 1
	/* Only vpud goes here */
	/* Assume VENC uses instance 1 and others use 0. */
	if (id == IPI_VENC_H264 ||
	    id ==  IPI_VENC_VP8 ||
	    id == IPI_VENC_HYBRID_H264)
		return IPI_INST_CNT-1;
#endif
	return 0;
}

void ipi_handler(struct share_obj *obj)
{
	assert(ipi_desc[obj->id].handler != NULL);
	ipi_desc[obj->id].handler(obj->id, obj->share_buf, obj->len);
}

void vpu_ipi_init(void)
{
	int i;

	for (i = 0; i < IPI_INST_CNT; i++) {
		#ifndef IPI_SYNC_ONLY
		sem_init(&work_sem[i], 0, 0);
		sem_init(&done_sem[i], 0, 0);
		#endif
	}
}

enum ipi_status vpu_ipi_registration(enum ipi_id id, ipi_handler_t handler, const char *name)
{
	assert(id >= 0 && id < NR_IPI);
	assert(handler != NULL);

	ipi_desc[id].handler = handler;
	ipi_desc[id].name = name;

	return DONE;
}

enum ipi_status vpu_ipi_send(enum ipi_id id, void *buf, uint len, unsigned int wait)
{
	int i = 0;
	assert(id >= 0 && id < NR_IPI);
	assert(buf != NULL);
	assert(len <= sizeof(obj[0].share_buf));

	i = ipi_id_to_inst_id(id);

	while (pthread_mutex_lock(&mutex[i]) != 0);
	if (busy[i]) {
		ipi_log("send id = %d return\n", id);
		while (pthread_mutex_unlock(&mutex[i]) != 0);
		return BUSY;
	} else
		busy[i] = true;

	obj[i].id = id;
	obj[i].len = len;
	memcpy(obj[i].share_buf, buf, len);
	sync[i] = wait ? true : false;

	if (sync[i]) {
		/*
		 * For synchronous call of vpu_ipi_send, skip sender thread in
		 * daemon and skip 2 semaphores synchronization, which decreases
		 * the latency.
		 */
		int ret;
		ret = ioctl(g_ctx_s.fd, g_ctx_s.cmd, &obj[i]);
		assert(ret != -1);
		busy[i] = false;
	} else {
		#ifndef IPI_SYNC_ONLY
		sem_post(&work_sem[i]);

		if (sync[i]) {
			sem_wait(&done_sem[i]);
			busy[i] = false;
		}
		#else
		assert(0);
		#endif
	}

	while (pthread_mutex_unlock(&mutex[i]) != 0);

	return DONE;
}

#ifndef IPI_SYNC_ONLY
void send_obj(int fd, int cmd, int i)
{
	int ret;

	sem_wait(&work_sem[i]);

	ret = ioctl(fd, cmd, &obj[i]);
	assert(ret != -1);

	if (sync[i])
		sem_post(&done_sem[i]);
	else {
		while (pthread_mutex_lock(&mutex[i]) != 0);
		busy[i] = false;
		while (pthread_mutex_unlock(&mutex[i]) != 0);
	}
}
#endif
