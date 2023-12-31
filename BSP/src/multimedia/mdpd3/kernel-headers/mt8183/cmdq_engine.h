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
	/* ISP */
	CMDQ_ENG_WPEI = 0,
	CMDQ_ENG_WPEO,			/* 1 */
	CMDQ_ENG_WPEI2,			/* 2 */
	CMDQ_ENG_WPEO2,			/* 3 */
	CMDQ_ENG_ISP_IMGI,		/* 4 */
	CMDQ_ENG_ISP_VIPI,		/* 5 */
	CMDQ_ENG_ISP_LCEI,		/* 6 */
	CMDQ_ENG_ISP_IMGO,		/* 7 */
	CMDQ_ENG_ISP_IMG2O,		/* 8 */
	CMDQ_ENG_ISP_IMG3O,		/* 9 */
	CMDQ_ENG_ISP_SMXIO,		/* 10 */

	/* IPU */
	CMDQ_ENG_IPUI,			/* 11 */
	CMDQ_ENG_IPUO,			/* 12 */

	/* MDP */
	CMDQ_ENG_MDP_CAMIN,		/* 13 */
	CMDQ_ENG_MDP_CAMIN2,		/* 14 */
	CMDQ_ENG_MDP_RDMA0,		/* 15 */
	CMDQ_ENG_MDP_AAL0,		/* 16 */
	CMDQ_ENG_MDP_CCORR0,		/* 17 */
	CMDQ_ENG_MDP_RSZ0,		/* 18 */
	CMDQ_ENG_MDP_RSZ1,		/* 19 */
	CMDQ_ENG_MDP_TDSHP0,		/* 20 */
	CMDQ_ENG_MDP_COLOR0,		/* 21 */
	CMDQ_ENG_MDP_PATH0_SOUT,	/* 22 */
	CMDQ_ENG_MDP_PATH1_SOUT,	/* 23 */
	CMDQ_ENG_MDP_WROT0,		/* 24 */
	CMDQ_ENG_MDP_WDMA,		/* 25 */

	/* JPEG & VENC */
	CMDQ_ENG_JPEG_ENC,		/* 26 */
	CMDQ_ENG_VIDEO_ENC,		/* 27 */
	CMDQ_ENG_JPEG_DEC,		/* 28 */
	CMDQ_ENG_JPEG_REMDC,		/* 29 */

	/* DISP */
	CMDQ_ENG_DISP_UFOE,		/* 30 */
	CMDQ_ENG_DISP_AAL,		/* 31 */
	CMDQ_ENG_DISP_COLOR0,		/* 32 */
	CMDQ_ENG_DISP_RDMA0,		/* 33 */
	CMDQ_ENG_DISP_RDMA1,		/* 34 */
	CMDQ_ENG_DISP_RDMA2,		/* 35 */
	CMDQ_ENG_DISP_WDMA0,		/* 36 */
	CMDQ_ENG_DISP_WDMA1,		/* 37 */
	CMDQ_ENG_DISP_OVL0,		/* 38 */
	CMDQ_ENG_DISP_OVL1,		/* 39 */
	CMDQ_ENG_DISP_OVL2,		/* 40 */
	CMDQ_ENG_DISP_GAMMA,		/* 41 */
	CMDQ_ENG_DISP_DSI0_VDO,		/* 42 */
	CMDQ_ENG_DISP_DSI0_CMD,		/* 43 */
	CMDQ_ENG_DISP_DSI0,		/* 44 */
	CMDQ_ENG_DISP_2L_OVL0,		/* 45 */
	CMDQ_ENG_DISP_2L_OVL1,		/* 46 */
	CMDQ_ENG_DISP_2L_OVL2,		/* 47 */

	/* ISP */
	CMDQ_ENG_DPE,			/* 48 */
	CMDQ_ENG_RSC,			/* 49 */
	CMDQ_ENG_GEPF,			/* 50 */
	CMDQ_ENG_EAF,			/* 51 */
	CMDQ_ENG_OWE,			/* 52 */
	CMDQ_ENG_MFB,			/* 53 */

	/* temp: CMDQ internal usage */
	CMDQ_ENG_CMDQ,			/* 54 */
	CMDQ_ENG_DISP_MUTEX,		/* 55 */
	CMDQ_ENG_MMSYS_CONFIG,		/* 56 */

	/* Dummy Engine */
	CMDQ_ENG_MDP_RDMA1,		/* 57 */
	CMDQ_ENG_MDP_RSZ2,		/* 58 */
	CMDQ_ENG_MDP_TDSHP1,		/* 59 */
	CMDQ_ENG_MDP_WROT1,		/* 60 */

	CMDQ_MAX_ENGINE_COUNT		/* ALWAYS keep at the end */
} CMDQ_ENG_ENUM;

#endif				/* __CMDQ_ENGINE_H__ */
