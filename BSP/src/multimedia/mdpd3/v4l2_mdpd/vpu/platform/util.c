#include <sys/types.h>
#include <kernel/ipi.h>
#include <debug.h>
#include <kernel/thread.h>
#include <platform/util.h>
#include <platform/mt_typedefs.h>
#include <string.h>

#define VPU_FW_VER_LEN		16
#define VCODEC_CAPABILITY_4K_DISABLED 0x10
#define VCODEC_DEC_CAPABILITY	VCODEC_CAPABILITY_4K_DISABLED
#define VCODEC_ENC_CAPABILITY 	0x0

struct vpu_run_s {
	u32 signaled;
	char fw_ver[VPU_FW_VER_LEN];
	u32 vdec_capability;
	u32 venc_capability;
};

const static char vpu_fw_ver[] = {"0.2.13"};

/**
 * @brief  calculate idle task usage
 */
#ifdef THREAD_STATS
void send_thread_info(void)
{
	struct thread_stats thread_info;

	thread_info.uptime = (unsigned long)&thread_stats.uptime;
	thread_info.idle_time = (unsigned long)&thread_stats.idle_time;
	thread_info.last_idle_timestamp = (unsigned long)&
					  (thread_stats.last_idle_timestamp);

	while (vpu_ipi_send(IPI_THREAD_INFO, (void *)&thread_info,
			     sizeof(struct thread_stats), 1) == BUSY)
			;
}
#endif

/**
 * @brief  if VPU is ready, it will send ipi message to notify APMCU
 */
void send_init_fin(void)
{
	struct vpu_run_s vpu_run;

	vpu_run.signaled = true;
	strncpy(vpu_run.fw_ver, vpu_fw_ver, sizeof(vpu_fw_ver));
	//vpu_run.vdec_capability = VCODEC_DEC_CAPABILITY;
	//vpu_run.venc_capability = VCODEC_ENC_CAPABILITY;

	vpu_ipi_send(IPI_VPU_INIT, (void *)&vpu_run, sizeof(vpu_run), 1);
}

void send_uninit_fin(void)
{
	struct vpu_run_s vpu_run;

	vpu_run.signaled = false;
	strncpy(vpu_run.fw_ver, vpu_fw_ver, sizeof(vpu_fw_ver));

	vpu_ipi_send(IPI_VPU_INIT, (void *)&vpu_run, sizeof(vpu_run), 1);
}

void vpu_fw_version(void)
{
	dprintf(ALWAYS, "The firmware version is %s\n", vpu_fw_ver);
}
