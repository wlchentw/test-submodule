/*
 * Copyright (c) 2015 MediaTek Inc.
 * Author: PC Chen <pc.chen@mediatek.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

extern "C"
{
#include <app.h>
#include <debug.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <platform.h>
#include <kernel/thread.h>
#include <kernel/ipi.h>
#include <kernel/event.h>
}

#include "mdp_debug.h"
#include "mdp_drv.h"
#include "mdp_ipi.h"
#include "mdp_chip.h"
#include "mdp_lib_if.h"

struct mtk_mdp_lib_if *g_mdp_if = NULL;
void *g_mdp_lib_handle = NULL;

__section(".dtcm") uint32_t g_mdp_drv_used = 0;

#pragma align 64
#ifdef __ANDROID__
__section(".dmem_ext") static struct mdp_param g_shmem_mdp_param[MTK_MDP_MAX_CTX] = {[0]={.h_drv = 1}};
#else
__section(".dmem_ext") static struct mdp_param g_shmem_mdp_param[MTK_MDP_MAX_CTX];
#endif

#if CONFIG_FOR_TUNING
__section(".dmem_ext") static struct mdp_tuning g_shmem_mdp_tuning = {{1}};
#endif

struct mdp_path_param g_path[MDP_PATH_CNT];


static int32_t mdp_cmdq_flush_callback(void *priv, void *cmdq_param);

static const char * mdp_chip_name(void)
{
	switch(MDP_CHIP())
	{
	case MDP_2701:
		return "mt2701";
	case MDP_8173:
		return "mt8173";
	case MDP_2712:
		return "mt2712";
	case MDP_6799:
		return "mt6799";
	case MDP_8167:
		return "mt8167";
	case MDP_8183:
		return "mt8183";
	case MDP_8512:
		return "mt8512";
	default:
		return "unknown";
	}
}

void printf_mdp_if(struct mtk_mdp_lib_if *mdp)
{
	MDP_ERR("mdp interface for %x", MDP_CHIP());
	MDP_ERR("mdp->init = %p", mdp->init);
	MDP_ERR("mdp->deinit = %p", mdp->deinit);
	MDP_ERR("mdp->process = %p", mdp->process);
	MDP_ERR("mdp->process_thread = %p", mdp->process_thread);
	MDP_ERR("mdp->process_continue = %p", mdp->process_continue);
	MDP_ERR("mdp->prepare_cmdq_param = %p", mdp->prepare_cmdq_param);
	MDP_ERR("mdp->wait_cmdq_done = %p", mdp->wait_cmdq_done);
	MDP_ERR("mdp->cmdq_flushing_status = %p", mdp->cmdq_flushing_status);
	MDP_ERR("mdp->dbg_para = %p", mdp->dbg_para);
	MDP_ERR("mdp->dbg_proc = %p", mdp->dbg_proc);
}

static bool validate_mdp_if(enum mdp_chip c, struct mtk_mdp_lib_if *mdp)
{
	switch(c)
	{
	case MDP_2701:
	case MDP_8183:
	case MDP_8512:
		/* not use upstream cmdq driver, libmdp call cmdq by ioctl */
		return (mdp->init &&
			mdp->deinit &&
			mdp->process);
	case MDP_8173:
	case MDP_2712:
	case MDP_6799:
	case MDP_8167:
		/* use upstream cmdq driver, libmdp set cmdq param to mdp driver then call cmdq */
		return (mdp->init &&
			mdp->deinit &&
			mdp->process_thread &&
			mdp->process_continue &&
			mdp->prepare_cmdq_param &&
			mdp->wait_cmdq_done &&
			mdp->cmdq_flushing_status);
	default:
		return false;
	}
}

static int mdp_lib_init(void)
{
	char lib_mdp_name[32];
	char *err;

	sprintf(lib_mdp_name, "libmdp.%s.so", mdp_chip_name());
	g_mdp_lib_handle = dlopen(lib_mdp_name, RTLD_LAZY);
	if (NULL == g_mdp_lib_handle) {
		MDP_ERR("dlopen %s, err=%s", lib_mdp_name, dlerror());
		return -1;
	}

	dlerror(); // clear the previous error if any.
	g_mdp_if = (struct mtk_mdp_lib_if *)dlsym(g_mdp_lib_handle, "mtk_mdp_if");
	if (NULL == g_mdp_if) {
		err = dlerror();
		MDP_ERR("dlsym %s, err=%s", "mtk_mdp_if", (err ? err : "NA"));
		dlclose(g_mdp_lib_handle);
		return -1;
	}

	if (!validate_mdp_if(MDP_CHIP(), g_mdp_if)) {
		MDP_ERR("mdp_if is invalid");
		printf_mdp_if(g_mdp_if);
		dlclose(g_mdp_lib_handle);
		return -1;
	}

	MDP_Printf("load %s success, %p, %p", lib_mdp_name, g_mdp_lib_handle, g_mdp_if);
	return 0;
}

static int mdp_lib_uninit()
{
	int ret = 0;

	if (NULL != g_mdp_lib_handle) {
		ret = dlclose(g_mdp_lib_handle);
		if (0 != ret) {
			MDP_ERR("fail to close libmdp.mt%x.so", MDP_CHIP());
		}
	}

	g_mdp_lib_handle = NULL;
	g_mdp_if = NULL;

	return 0;
}

void mdp_instance_service_init(struct mdp_path_param *path)
{
	MDP_Printf("hw=%d, inst_cnt=%u, used=%u, mdp_param=%p, size=%u",
		path->id,
		MTK_MDP_MAX_CTX, g_mdp_drv_used,
		g_shmem_mdp_param, (uint32_t)sizeof(g_shmem_mdp_param));

	if (g_mdp_lib_handle == NULL) {
		mdp_lib_init();

		#if CONFIG_FOR_TUNING
		memset(&g_shmem_mdp_tuning, 0, sizeof(struct mdp_tuning));
		#endif
	}
}

static ipi_status handle_mdp_init_msg(void *msg)
{
	int i;
	ipi_status status;
	struct mdp_ipi_init *in = (struct mdp_ipi_init *)msg;
	struct mdp_ipi_comm_ack out;
	struct mdp_param *param;

	MDP_APIEntryEx("msg_id=%x", in->msg_id);

	// g_mdp_drv_used should be protected exclusively for multi-instance
	enter_critical_section();

	/* find unused drv instance id */
	for (i = 0; i < MTK_MDP_MAX_CTX; i++) {
		if (0 == ((g_mdp_drv_used >> i) & 0x1)) {
			break;
		}
	}

	MDP_Printf("instance id=%d/%d, used=%u", i, MTK_MDP_MAX_CTX, g_mdp_drv_used);

	if (i < MTK_MDP_MAX_CTX) {
		param = &g_shmem_mdp_param[i];

		MDP_Printf("param=%p, size=%u", param, (uint32_t)sizeof(*param));
		memset(param, 0, sizeof(*param));
		if (IS_MDP_2712() || IS_MDP_8173() || IS_MDP_6799() || IS_MDP_8167()) {
			param->vsi.cmdq.vpu_buf_addr = (uint64_t)(unsigned long)param->cmdq_buffer;
			param->vsi.cmdq.buf_size = sizeof(param->cmdq_buffer);

			MDP_Printf("param->vsi=%p, &param->vsi.cmdq=%p, cmdq buf_addr=%p, size=%u",
				&param->vsi, &param->vsi.cmdq,
				(void *)(unsigned long)param->vsi.cmdq.vpu_buf_addr,
				param->vsi.cmdq.buf_size);
		}
		param->h_drv = i;
		#if CONFIG_FOR_TUNING
		param->shmem_mdp_tuning = &g_shmem_mdp_tuning;
		#endif

		g_mdp_drv_used |= 1 << i;

		g_mdp_if->init(param);

		out.status = MDP_IPI_MSG_STATUS_OK;
		out.vpu_inst_addr = (uint64_t)(unsigned long)param;

		MDP_Printf("drv=%d, param=%p", i, param);
	}
	else {
		out.status = MDP_IPI_MSG_STATUS_FAIL;
		out.vpu_inst_addr = 0;
	}

	exit_critical_section();

   	out.msg_id = VPU_MDP_INIT_ACK;
	out.ipi_id = in->ipi_id;
	out.ap_inst = in->ap_inst;

	status = vpu_ipi_send((enum ipi_id)out.ipi_id, &out, sizeof(out), 1);

	MDP_APILeaveEx("status=%d", status);

	return status;
}

static ipi_status handle_mdp_deinit_msg(void *msg)
{
	ipi_status status;
	struct mdp_ipi_comm *in = (struct mdp_ipi_comm *)msg;
	struct mdp_ipi_comm_ack out;
	struct mdp_param *param;

	// g_mdp_drv_used should be protected exclusively for multi-instance

	MDP_APIEntryEx("msg_id=%x", in->msg_id);

	param = (struct mdp_param *)in->vpu_inst_addr;
	enter_critical_section();
	if (param && (param->h_drv < MTK_MDP_MAX_CTX) &&
	    ((g_mdp_drv_used >> param->h_drv) & 0x1)) {

		g_mdp_if->deinit(param);
		g_mdp_drv_used &= ~(1 << param->h_drv);
		param->h_drv = (uint32_t)(-1);

		out.status = MDP_IPI_MSG_STATUS_OK;
	}
	else {
		out.status = MDP_IPI_MSG_STATUS_FAIL;
	}

	exit_critical_section();
	out.msg_id = VPU_MDP_DEINIT_ACK;
	out.ipi_id = in->ipi_id;
	out.ap_inst = in->ap_inst;
	out.vpu_inst_addr = in->vpu_inst_addr;

	status = vpu_ipi_send((enum ipi_id)out.ipi_id, &out, sizeof(out), 1);

	MDP_APILeaveEx("status=%d", status);

	return status;
}

static ipi_status handle_mdp_process_msg(void *msg)
{
	ipi_status status = ERROR;
	DP_STATUS_ENUM dp_status;
	struct mdp_ipi_comm *in = (struct mdp_ipi_comm *)msg;
	struct mdp_ipi_comm_ack out;
	struct mdp_param *param;

	MDP_APIEntryEx();

	param = (struct mdp_param *)in->vpu_inst_addr;

	if (param && (param->h_drv < MTK_MDP_MAX_CTX) &&
	   ((g_mdp_drv_used >> param->h_drv) & 0x1)) {

		dp_status = g_mdp_if->process(param);

	}
	else {
		MDP_ERR("invalid param");
		dp_status = DP_STATUS_INVALID_PARAX;
	}

	out.msg_id = VPU_MDP_PROCESS_ACK;
	out.ipi_id = in->ipi_id;

	if (DP_STATUS_RETURN_SUCCESS != dp_status)
		out.status = (int32_t)dp_status;
	else
		out.status = MDP_IPI_MSG_STATUS_OK;

	out.ap_inst = in->ap_inst;
	out.vpu_inst_addr = in->vpu_inst_addr;

	status = vpu_ipi_send((enum ipi_id)out.ipi_id, &out, sizeof(out), 1);

	MDP_APILeaveEx();

	return status;
}


ipi_status mdp_send_common_ack_msg(void *msg, uint32_t msg_id, DP_STATUS_ENUM dp_status)
{
	ipi_status status = ERROR;
	struct mdp_ipi_comm *in = (struct mdp_ipi_comm *)msg;
	struct mdp_ipi_comm_ack out;

	MDP_APIEntryEx("msg_id=%u", msg_id);

	out.msg_id = msg_id;
	out.ipi_id = in->ipi_id;

	if (DP_STATUS_RETURN_SUCCESS != dp_status)
		out.status = (int32_t)dp_status;
	else
		out.status = MDP_IPI_MSG_STATUS_OK;

	out.ap_inst = in->ap_inst;
	out.vpu_inst_addr = in->vpu_inst_addr;

	status = vpu_ipi_send((enum ipi_id)out.ipi_id, &out, sizeof(out), 1);

	MDP_APILeaveEx();

	return status;
}

static ipi_status handle_mdp_process_msg_thread(void *msg)
{
    ipi_status status = ERROR;
    DP_STATUS_ENUM dp_status;
    struct mdp_ipi_comm *in = (struct mdp_ipi_comm *)msg;
    struct mdp_param *param;
    struct mdp_process_vsi *vsi = NULL;
    struct MDP_CMDQ_FLUSH_CB_PRIV_DATA mdpd_priv_data;

    MDP_APIEntryEx();

    param = (struct mdp_param *)in->vpu_inst_addr;

    if (param && (param->h_drv < MTK_MDP_MAX_CTX) &&
        ((g_mdp_drv_used >> param->h_drv) & 0x1)) {
        vsi = &param->vsi;

        mdpd_priv_data.priv_data = msg;
        mdpd_priv_data.cmdq_buf_pa = (unsigned int)vsi->cmdq.ap_buf_pa; /* Currently, 32bit in use. */

        MDP_Printf("get pq[0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x]",
            vsi->pq.sharpness_enable, vsi->pq.sharpness_level,
            vsi->pq.dynamic_contrast_enable,
            vsi->pq.brightness_enable, vsi->pq.brightness_level,
            vsi->pq.contrast_enable, vsi->pq.contrast_level);

        MDP_Printf("get blur[h%d, v%d, hl%d, vl%d]",
            vsi->blur.hor_blur_enable, vsi->blur.ver_blur_enable, vsi->blur.hor_blur_level, vsi->blur.ver_blur_level);

        MDP_Printf("[%p], addr=%x,%x,%x, ipi_id=%d",
            (void *)(unsigned long)in->ap_inst,
            (uint32_t)vsi->src_buffer.addr_mva[0],
            (uint32_t)vsi->src_buffer.addr_mva[1],
            (uint32_t)vsi->src_buffer.addr_mva[2],
                in->ipi_id);

        MDP_Printf("src config=%x,%x,%x",
            (uint32_t)vsi->src_config.w,
            (uint32_t)vsi->src_config.h,
            (uint32_t)vsi->src_config.format);

        MDP_Printf("dst addr=%x,%x,%x",
            (uint32_t)vsi->dst_buffer.addr_mva[0],
            (uint32_t)vsi->dst_buffer.addr_mva[1],
            (uint32_t)vsi->dst_buffer.addr_mva[2]);

        dp_status = g_mdp_if->process_thread(param, mdp_cmdq_flush_callback, &mdpd_priv_data);
    }else {
        MDP_ERR("invalid param");
        dp_status = DP_STATUS_INVALID_PARAX;
    }

    /*
    if (return before execute_command)
        send process_ack;
    else if (return after execute_command done)
        send cmdq_done_ack
    */
    if (!vsi ||
         g_mdp_if->cmdq_flushing_status(param) != DP_STATUS_RETURN_SUCCESS) {
            mdp_send_common_ack_msg(msg, VPU_MDP_PROCESS_ACK, dp_status);
    }else {
        mdp_send_common_ack_msg(msg, VPU_MDP_CMDQ_DONE_ACK, dp_status);
    }

    MDP_APILeaveEx();

    return status;
}

static ipi_status handle_mdp_cmdq_done_msg(void *msg)
{
	ipi_status status = DONE;
	DP_STATUS_ENUM dp_status;
	struct mdp_ipi_comm *in = (struct mdp_ipi_comm *)msg;
	struct mdp_param *param;

	MDP_APIEntryEx();

	param = (struct mdp_param *)in->vpu_inst_addr;
	if (param && (param->h_drv < MTK_MDP_MAX_CTX) &&
	   ((g_mdp_drv_used >> param->h_drv) & 0x1)) {

		MDP_Printf("got cmdq execute done msg from AP!");
		g_mdp_if->process_continue(param);
	}
	else {
		MDP_ERR("invalid param");
		status = mdp_send_common_ack_msg(msg, VPU_MDP_CMDQ_DONE_ACK, DP_STATUS_INVALID_PARAX);
	}

	MDP_APILeaveEx();

	return status;
}

#if CONFIG_FOR_TUNING
static ipi_status handle_mdp_tuning_msg(void *msg)
{
	ipi_status status;
	struct mdp_ipi_init *in = (struct mdp_ipi_init *)msg;
	struct mdp_ipi_comm_ack out;

	MDP_APIEntryEx("msg_id=%x", in->msg_id);

	out.status = MDP_IPI_MSG_STATUS_OK;
	out.vpu_inst_addr = (uint64_t)(&g_shmem_mdp_tuning);
	out.msg_id = VPU_MDP_TUNING_ACK;
	out.ipi_id = in->ipi_id;
	out.ap_inst = in->ap_inst;

	MDP_Printf("[tuning] %s[%d], vpu_ipi_send start, st[%d] addr[0x%lx] msg_id[%d] ipi_id[%d] ap[0x%lx]",
		__func__, __LINE__, out.status, out.vpu_inst_addr, out.msg_id, out.ipi_id, out.ap_inst);

	status = vpu_ipi_send((enum ipi_id)out.ipi_id, &out, sizeof(out), 1);

	MDP_Printf("[tuning] %s[%d], vpu_ipi_send end, st[%d]",
		__func__, __LINE__, status);

	MDP_APILeaveEx("status=%d", status);

	return status;
}
#endif

void mdp_drv_msg_handler(void *pmsg)
{
	ipi_status status = ERROR;
	struct mdp_path_param *path = (struct mdp_path_param *)pmsg;
	void *msg = path->msg;
	unsigned int msg_id = *(unsigned int *)msg;

	MDP_APIEntryEx("msg_id=%x", msg_id);

	switch (msg_id) {
	case AP_MDP_INIT:
		status = handle_mdp_init_msg(msg);
		break;

	case AP_MDP_DEINIT:
		status = handle_mdp_deinit_msg(msg);
		break;

	case AP_MDP_PROCESS:
		if (IS_MDP_2712() || IS_MDP_8173() || IS_MDP_6799() || IS_MDP_8167()) {
			mdp_process_signal(path->id);
			status = DONE;
		} else {
			status = handle_mdp_process_msg(msg);
		}
		break;
	case AP_MDP_CMDQ_DONE:
		// goes here if using upstream cmdq driver,
		status = handle_mdp_cmdq_done_msg(msg);
		break;
	#if CONFIG_FOR_TUNING
	case AP_MDP_TUNING:
		status = handle_mdp_tuning_msg(msg);
		break;
	#endif
	default:
		MDP_ERR("handle unknown ipi msg %x!", msg_id);
		break;
	}

	MDP_APILeaveEx("status=%d", status);
	if (status != DONE)
	{
		MDP_ERR("msg_id=%x, ret=%d", msg_id, status);
	}
}

int32_t mdp_cmdq_flush_callback(void *priv, void *cmdq_param)
{
	DP_STATUS_ENUM dp_status = DP_STATUS_RETURN_SUCCESS;
	struct mdp_ipi_comm *in = (struct mdp_ipi_comm *)(((struct MDP_CMDQ_FLUSH_CB_PRIV_DATA *)priv)->priv_data);
	struct mdp_param *param;

	MDP_APIEntryEx("priv=%p, cmdq_param=%p", in, cmdq_param);

	/* to do: check ptr 'in' validation */
	param = (struct mdp_param *)in->vpu_inst_addr;

	if (param && (param->h_drv < MTK_MDP_MAX_CTX) &&
	   ((g_mdp_drv_used >> param->h_drv) & 0x1)) {

		/* prepare cmdq param for AP (kernel driver) */
		dp_status = g_mdp_if->prepare_cmdq_param(param, cmdq_param);

		if (dp_status == DP_STATUS_RETURN_SUCCESS) {
			/* send process ack */
			if (mdp_send_common_ack_msg((void *)in, VPU_MDP_PROCESS_ACK, dp_status) == DONE) {
				/* wait cmdq execute done in AP (kernel driver) */
				dp_status = g_mdp_if->wait_cmdq_done(param, cmdq_param);
			}
			else {
				MDP_ERR("send msg fail!");
				dp_status = DP_STATUS_UNKNOWN_ERROR;
			}
		}
		else {
			MDP_ERR("prepare_cmdq_param return error %d!", dp_status);
		}
	}
	else {
		MDP_ERR("invalid param!");
		dp_status = DP_STATUS_INVALID_PARAX;
	}

	MDP_APILeaveEx("status=%d", dp_status);

	return (int32_t)dp_status;
}

void mdp_process_init(enum mdp_path_id id)
{
	event_init(&g_path[id].event, false, EVENT_FLAG_AUTOUNSIGNAL);
}

void mdp_process_signal(enum mdp_path_id id)
{
	event_signal(&g_path[id].event, false);
}

void mdp_process(enum mdp_path_id id)
{
	while (1) {
		event_wait(&g_path[id].event);
		handle_mdp_process_msg_thread(g_path[id].msg);
	}
}

extern "C"
void mdp_dbg_para(void)
{
	if (g_mdp_if->dbg_para)
		g_mdp_if->dbg_para();
}

extern "C"
void mdp_dbg_proc(const char *buf, size_t count)
{
	if (g_mdp_if->dbg_proc)
		g_mdp_if->dbg_proc(buf, count);
}
