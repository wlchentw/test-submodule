#include <DpChannel.h>
#include <DpStream.h>
#include <DpDataType.h>

#include "DpBlitStream.h"

#include "mdp_lib_if.h"

#pragma align 64
__section(".dtcm") DpBlitStream g_blit[MTK_MDP_MAX_CTX];

/**
 * called by handle_mdp_init_msg
 */
DP_STATUS_ENUM mdp_if_init(struct mdp_param *param)
{
	return DP_STATUS_RETURN_SUCCESS;
}

/**
 * called by handle_mdp_deinit_msg
 */
DP_STATUS_ENUM mdp_if_deinit(struct mdp_param *param)
{
	return DP_STATUS_RETURN_SUCCESS;
}

static DP_STATUS_ENUM handle_mdp_src_config(DpBlitStream *pblt, void* msg)
{
	DP_STATUS_ENUM status = DP_STATUS_RETURN_SUCCESS;
	struct mdp_config *in = (struct mdp_config *)msg;
	DpRect roi; /* region of interisting */
	//DpColorFormat informat;

	MDP_APIEntryEx();

	roi.x = in->crop_x;
	roi.y = in->crop_y;
	roi.sub_x = 0;
	roi.sub_y = 0;
	roi.w = in->crop_w;
	roi.h = in->crop_h;

	#if 0
	/* use local variable not to reveal format value to kernel */
	informat = (DpColorFormat)in->format;
	if (0 != DP_COLOR_GET_BLOCK_MODE(informat)) {
		informat = DP_COLOR_420_BLKP; /* Block mode */
	}
	#endif

	if (DP_COLOR_GET_BLOCK_MODE(in->format) && in->pitch[0] > 0) {
		in->pitch[0] = 0;
		in->pitch[1] = 0;
		in->pitch[2] = 0;

		MDP_PrintfEx(INFO, "block mode pitch, force use default.");
	} else if (DP_COLOR_GET_PLANE_COUNT(in->format) > 1 &&
		in->pitch[1] < DP_COLOR_GET_MIN_UV_PITCH(in->format, in->w)) {
		in->pitch[1] <<= 1; /* x2 */
		in->pitch[2] <<= 1; /* x2 */
		MDP_PrintfEx(INFO, "UV pitch, x2.");
	}

	MDP_PrintfEx(INFO, "format=%08x, wxh=%ux%u, crop=%u,%u,%u,%u, pitch:%u,%u,%u, minimal pitch:%u,%u",
		in->format, in->w, in->h, in->crop_x, in->crop_y, in->crop_w, in->crop_h,
		in->pitch[0], in->pitch[1], in->pitch[2],
		DP_COLOR_GET_MIN_Y_PITCH(in->format, in->w),
		DP_COLOR_GET_MIN_UV_PITCH(in->format, in->w));

	if (in->pitch[0] > 0) {
		pblt->setSrcConfig(in->w, in->h, in->pitch[0], in->pitch[1],
							(DpColorFormat)in->format,
							DP_PROFILE_BT601,
							eInterlace_None, &roi,
							DP_SECURE_NONE, true);
	} else {
		pblt->setSrcConfig(in->w, in->h,
							(DpColorFormat)in->format,
							eInterlace_None, &roi);
	}

	MDP_APILeaveEx();
	return status;
}

static DP_STATUS_ENUM handle_mdp_src_buffer(DpBlitStream *pblt, void* msg)
{
	DP_STATUS_ENUM status = DP_STATUS_RETURN_SUCCESS;
	struct mdp_buffer *in = (struct mdp_buffer *)msg;
	void *addr[3];
	uint32_t mva[3];
	uint32_t size[3];

	MDP_APIEntryEx();

	mva[0] = (uint32_t)(unsigned long)in->addr_mva[0];
	mva[1] = (uint32_t)(unsigned long)in->addr_mva[1];
	mva[2] = (uint32_t)(unsigned long)in->addr_mva[2];

	addr[0] = (void *)(unsigned long)in->addr_mva[0];
	addr[1] = (void *)(unsigned long)in->addr_mva[1];
	addr[2] = (void *)(unsigned long)in->addr_mva[2];
	size[0] = in->plane_size[0];
	size[1] = in->plane_size[1];
	size[2] = in->plane_size[2];

	pblt->setSrcBuffer(addr, (void **)mva, size, in->plane_num);

	MDP_APILeaveEx();

	return status;
}

static DP_STATUS_ENUM handle_mdp_dst_config(DpBlitStream *pblt, void* msg)
{
	DP_STATUS_ENUM status = DP_STATUS_RETURN_SUCCESS;
	struct mdp_config *in = (struct mdp_config *)msg;
	DpRect roi; /* region of interisting */

	MDP_APIEntryEx();

	roi.x = in->crop_x;
	roi.y = in->crop_y;
	roi.sub_x = 0;
	roi.sub_y = 0;
	roi.w = in->crop_w;
	roi.h = in->crop_h;

	if (DP_COLOR_GET_BLOCK_MODE(in->format) && in->pitch[0] > 0) {
		in->pitch[0] = 0;
		in->pitch[1] = 0;
		in->pitch[2] = 0;

		MDP_PrintfEx(INFO, "block mode pitch, force use default.");
	} else if (DP_COLOR_GET_PLANE_COUNT(in->format) > 1 &&
		in->pitch[1] < DP_COLOR_GET_MIN_UV_PITCH(in->format, in->w)) {
		in->pitch[1] <<= 1; /* x2 */
		in->pitch[2] <<= 1; /* x2 */
		MDP_PrintfEx(INFO, "UV pitch, x2.");
	}

	MDP_PrintfEx(INFO, "format=%08x, wxh=%ux%u, crop=%u,%u,%u,%u, pitch:%u,%u,%u, minimal pitch:%u,%u",
		in->format, in->w, in->h, in->crop_x, in->crop_y, in->crop_w, in->crop_h,
		in->pitch[0], in->pitch[1], in->pitch[2],
		DP_COLOR_GET_MIN_Y_PITCH(in->format, in->w),
		DP_COLOR_GET_MIN_UV_PITCH(in->format, in->w));

	if (in->pitch[0] > 0) {
		pblt->setDstConfig(in->w, in->h, in->pitch[0], in->pitch[1],
							(DpColorFormat)in->format,
							DP_PROFILE_BT601,
							eInterlace_None, &roi,
							DP_SECURE_NONE, true);
	} else {
		pblt->setDstConfig(in->w, in->h,
							(DpColorFormat)in->format,
							eInterlace_None, &roi);
	}

	MDP_APILeaveEx();

	return status;
}

static DP_STATUS_ENUM handle_mdp_dst_buffer(DpBlitStream *pblt, void* msg)
{
	DP_STATUS_ENUM status = DP_STATUS_RETURN_SUCCESS;
	struct mdp_buffer *in = (struct mdp_buffer *)msg;
	void  *addr[3];
	uint32_t mva[3];
	uint32_t size[3];

	MDP_APIEntryEx();

	MDP_Printf("dst buf %x %x", in->plane_num, in->plane_size[0]);

	mva[0] = (uint32_t)(unsigned long)in->addr_mva[0];
	mva[1] = (uint32_t)(unsigned long)in->addr_mva[1];
	mva[2] = (uint32_t)(unsigned long)in->addr_mva[2];

	addr[0] = (void *)(unsigned long)in->addr_mva[0];
	addr[1] = (void *)(unsigned long)in->addr_mva[1];
	addr[2] = (void *)(unsigned long)in->addr_mva[2];
	size[0] = in->plane_size[0];
	size[1] = in->plane_size[1];
	size[2] = in->plane_size[2];

	pblt->setDstBuffer(addr, (void **)mva, size, in->plane_num);

	MDP_APILeaveEx();

	return status;
}


static DP_STATUS_ENUM handle_mdp_config_misc(DpBlitStream *pblt, void* msg)
{
	int32_t rot, flip;
	DP_STATUS_ENUM status = DP_STATUS_RETURN_SUCCESS;
	struct mdp_config_misc *in = (struct mdp_config_misc *)msg;

	MDP_APIEntryEx();

	in->orientation %= 360;

	flip = 0;
	rot = in->orientation;

	// operate on FLIP_H, FLIP_V and ROT_90 respectively
	// to achieve the final orientation
	if (in->hflip) {
		flip ^= 1;
	}

	// FLIP_V is equivalent to a 180-degree rotation with a horizontal flip
	if (in->vflip) {
	        rot += 180;
	        flip ^= 1;
	}

	//if (0 == in->hflip && 0 == in->vflip) {
	//	rot = in->orientation;
	//}

	rot %= 360;

	pblt->setRotate(rot);
	pblt->setFlip(flip);

	MDP_APILeaveEx();

	return status;
}

DP_STATUS_ENUM mdp_if_mdp_process(struct mdp_param *param)
{
	DP_STATUS_ENUM dp_status;
	struct mdp_process_vsi *vsi = &param->vsi;
	DpBlitStream *pblt = &g_blit[param->h_drv];
	DpPqParam pqParam;

	pqParam.sharpness_enable = vsi->sharpness_enable;
	pqParam.sharpness_level = vsi->sharpness_level;
	pqParam.gamma_enable = vsi->gamma_enable;
	pqParam.gamma_type = vsi->gamma_type;
	memcpy(pqParam.gamma_table, vsi->gamma_table, 256);
	pqParam.invert = vsi->invert;
	pqParam.dth_enable = vsi->dth_enable;
	pqParam.dth_algo = vsi->dth_algo;
	pqParam.rsz_algo = vsi->rsz_algo;

	MDP_APIEntryEx();

	MDP_Printf("drv=%d, blit=%p, param=%p", param->h_drv, &g_blit[param->h_drv], param);

	dp_status = handle_mdp_src_config(pblt, &vsi->src_config);
	if (DP_STATUS_RETURN_SUCCESS != dp_status)
		goto error_return;

	dp_status = handle_mdp_src_buffer(pblt, &vsi->src_buffer);
	if (DP_STATUS_RETURN_SUCCESS != dp_status)
		goto error_return;

	dp_status = handle_mdp_dst_config(pblt, &vsi->dst_config);
	if (DP_STATUS_RETURN_SUCCESS != dp_status)
		goto error_return;

	dp_status = handle_mdp_dst_buffer(pblt, &vsi->dst_buffer);
	if (DP_STATUS_RETURN_SUCCESS != dp_status)
		goto error_return;

	dp_status = handle_mdp_config_misc(pblt, &vsi->misc);
	if (DP_STATUS_RETURN_SUCCESS != dp_status)
		goto error_return;

	pblt->setPQParameter(&pqParam);

	dp_status = pblt->invalidate();

error_return:
	MDP_APILeaveEx();

	return dp_status;
}

//extern uint32_t s_test_path;

void mdp_if_dbg_para(void)
{
//	fprintf(stderr, "s_test_path=%d\n", s_test_path);
}

void mdp_if_dbg_proc(const char *buf, size_t count)
{
	char *t;
	char *k, *v;

	t = (char *)malloc(count);
	memcpy(t, buf, count);

	k = strtok(t, " \t=");
	v = strtok(NULL, " \t");
	fprintf(stderr, "k=%s, v=%s\n", k, v);
	if (strcmp(k, "path") == 0) {
		if (v) {
//			s_test_path = atoi(v);
//			fprintf(stderr, "set s_test_path=%d\n", s_test_path);
		}
	}

	free(t);
}

struct mtk_mdp_lib_if mtk_mdp_if = {
	.init               = mdp_if_init,
	.deinit             = mdp_if_deinit,
	.process            = mdp_if_mdp_process,
	.process_thread     = NULL,
	.process_continue   = NULL,
	.prepare_cmdq_param = NULL,
	.wait_cmdq_done     = NULL,
	.cmdq_flushing_status = NULL,
	.dbg_para           = mdp_if_dbg_para,
	.dbg_proc           = mdp_if_dbg_proc,
};

