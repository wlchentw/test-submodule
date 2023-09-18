/*
 * Copyright (C) 2015 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#ifndef __CMDQ_ENGINE_H__
#define __CMDQ_ENGINE_H__

typedef enum CMDQ_ENG_ENUM {
#if 1
	CMDQ_ENG_MDP_RDMA0,		/* 0 */
	CMDQ_ENG_MDP_GAMMA,		/* 1 */
	CMDQ_ENG_MDP_DTH,		/* 2 */
	CMDQ_ENG_MDP_RSZ0,		/* 3 */
	CMDQ_ENG_MDP_TDSHP0,		/* 4 */
	CMDQ_ENG_MDP_WROT0,		/* 5 */
	CMDQ_MAX_ENGINE_COUNT		/* ALWAYS keep at the end */
#else
	/* PIPELINE */
	CMDQ_ENG_IMG_RDMA,
	CMDQ_ENG_WB_RDMA,
	CMDQ_ENG_WB_WDMA,
	CMDQ_ENG_PIPELINE,
	CMDQ_ENG_REGAL,
	CMDQ_ENG_PAPER_TOP,

	/* WF_LUT */
	CMDQ_ENG_WF_LUT,
	CMDQ_ENG_DISP_RDMA,
	CMDQ_ENG_TCON,
	CMDQ_ENG_DISP_DPI,

	/* MDP */
	CMDQ_ENG_MDP_RDMA0,
	CMDQ_ENG_MDP_GAMMA,
	CMDQ_ENG_MDP_RSZ0,
	CMDQ_ENG_MDP_DTH,
	CMDQ_ENG_MDP_TDSHP0,
	CMDQ_ENG_MDP_WROT0,

	/* display */
	CMDQ_ENG_DISP_OVL_2L,
	CMDQ_ENG_DISP_WDMA,

	/* imgresz */
	CMDQ_ENG_IMGRESZ,

	/* PNG DEC */
	CMDQ_ENG_PNG_DEC,

	/* JPEG DEC */
	CMDQ_ENG_JPEG_DEC,

	CMDQ_MAX_ENGINE_COUNT	/* ALWAYS keep at the end */

#endif

} CMDQ_ENG_ENUM;

#endif				/* __CMDQ_ENGINE_H__ */
