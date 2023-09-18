/*
 * Copyright Statement:

 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein is
 * confidential and proprietary to MediaTek Inc. and/or its licensors. Without
 * the prior written permission of MediaTek inc. and/or its licensors, any
 * reproduction, modification, use or disclosure of MediaTek Software, and
 * information contained herein, in whole or in part, shall be strictly
 * prohibited.
 *
 * MediaTek Inc. (C) 2015. All rights reserved.
 *
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
 * RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER
 * ON AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL
 * WARRANTIES, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR
 * NONINFRINGEMENT. NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH
 * RESPECT TO THE SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY,
 * INCORPORATED IN, OR SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES
 * TO LOOK ONLY TO SUCH THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO.
 * RECEIVER EXPRESSLY ACKNOWLEDGES THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO
 * OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES CONTAINED IN MEDIATEK
 * SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK SOFTWARE
 * RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S
 * ENTIRE AND CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE
 * RELEASED HEREUNDER WILL BE, AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE
 * MEDIATEK SOFTWARE AT ISSUE, OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE
 * CHARGE PAID BY RECEIVER TO MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 */

#ifndef __MDP_DEBUG_H__
#define __MDP_DEBUG_H__

#include "debug.h"

#ifdef DEBUG
#define MDP_DEBUG_ENABLE	(DEBUG) /* Enable to debug, logs, debug codes. */
#else
#define MDP_DEBUG_ENABLE	0 /* Enable to debug, logs, debug codes. */
#endif
#define VPU_MARK_UNUSED		1 /* 1: unused or debug code will not be compiled. */


#define MDP_LOG_ALL		0
#define MDP_LOG_INFO	1
#define MDP_LOG_WARN	2
#define MDP_LOG_ERR		3

#if MDP_DEBUG_ENABLE
#define MDP_LOG_LEVEL MDP_LOG_ALL
#else
#define MDP_LOG_LEVEL MDP_LOG_ERR
#endif

//extern int mdp_log_on;
#define mdp_log_on 1
#define MDP_LOG_TAG "[MDP]"

#if MDP_LOG_LEVEL < MDP_LOG_ERR
#define MDP_Printf(format, arg...) \
	    do { \
            if (mdp_log_on) \
                dprintf(CRITICAL, "%s" format"\n", MDP_LOG_TAG, ##arg);  \
        } while (0)

#define MDP_PrintfEx(level, format, arg...) \
		do { \
			if (mdp_log_on) \
				dprintf(level, "%s" format" %s,%d\n", MDP_LOG_TAG, ##arg, __FUNCTION__, __LINE__);  \
		} while (0)

#define MDP_Printf_DP(format, arg...) \
		do { \
			if (mdp_log_on) \
				dprintf(CRITICAL, "%s"" %s,%d " format, MDP_LOG_TAG, __FUNCTION__, __LINE__, ##arg);  \
		} while (0)
#else
#define MDP_Printf(format, arg...)
#define MDP_PrintfEx(level, format, arg...)
#define MDP_Printf_DP(format, arg...)
#endif

#define MDP_APIEntry() \
            MDP_Printf("-->%s, %d\n", __FUNCTION__, __LINE__)

#define MDP_APILeave() \
            MDP_Printf("<--%s, %d\n", __FUNCTION__, __LINE__)

#define MDP_APIEntryEx(format, arg...) \
            MDP_Printf("-->%s(" format"), %d\n", __FUNCTION__, ##arg, __LINE__)

#define MDP_APILeaveEx(format, arg...) \
            MDP_Printf("<--%s(" format"), %d\n", __FUNCTION__, ##arg, __LINE__)

#define TILE_PrintfEx(format, arg...) \
		do { \
			if (mdp_log_on) \
	            dprintf(CRITICAL, "%s""%s,%d" format, MDP_LOG_TAG, __FUNCTION__, __LINE__, ##arg);  \
        } while (0)

#define MDP_ERR(format, arg...) \
            dprintf(CRITICAL, "%s" format" %s,%d\n", MDP_LOG_TAG, ##arg, __FUNCTION__, __LINE__)

#define MDP_ASSERT(x) ASSERT(x)


#define DPLOGI MDP_Printf_DP
#define DPLOGW MDP_Printf_DP
#define DPLOGD MDP_Printf_DP
#define DPLOGE MDP_ERR
#if !__ANDROID__
#define ALOGD  MDP_Printf
#define ALOGE  MDP_Printf
#define ALOGI  MDP_Printf
#define PQ_LOGD MDP_Printf
#define PQ_LOGE MDP_Printf
#define PQ_LOGI MDP_Printf
#else
#include <cutils/log.h>
#endif

#define printf MDP_Printf_DP

#endif /* #ifndef __MDP_DEBUG_H__ */
