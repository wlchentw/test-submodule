/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein is
 * confidential and proprietary to MediaTek Inc. and/or its licensors. Without
 * the prior written permission of MediaTek inc. and/or its licensors, any
 * reproduction, modification, use or disclosure of MediaTek Software, and
 * information contained herein, in whole or in part, shall be strictly
 * prohibited.
 *
 * MediaTek Inc. (C) 2017. All rights reserved.
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
 *
 * The following software/firmware and/or related documentation ("MediaTek
 * Software") have been modified by MediaTek Inc. All revisions are subject to
 * any receiver's applicable license agreements with MediaTek Inc.
 */

/*=======================================================================*/
/* HEADER FILES                                                          */
/*=======================================================================*/
#include <tz_dapc.h>
#include <print.h>

#define _DEBUG_
#define DBG_DEVAPC

/* Debug message event */
#define DBG_EVT_NONE       (0x00000000U)      /* No event */
#define DBG_EVT_ERR        (0x00000001U)      /* ERR related event */
#define DBG_EVT_DOM        (0x00000002U)      /* DOM related event */

#define DBG_EVT_ALL        (0xffffffffU)

#define DBG_EVT_MASK       (DBG_EVT_DOM)

#ifdef _DEBUG_
#define MSG(evt, fmt, args...) \
    do {    \
        if ((DBG_EVT_##evt) & DBG_EVT_MASK) { \
            tl_printf(fmt, ##args); \
        } \
    } while(0)

#define MSG_FUNC_ENTRY(f)   MSG(FUC, "<FUN_ENT>: %s\n", __FUNCTION__)
#else
#define MSG(evt, fmt, args...) do{} while(0)
#define MSG_FUNC_ENTRY(f)      do{} while(0)
#endif

/*=======================================================================*/
/* STATIC FUNCTIONS                                                      */
/*=======================================================================*/
static void DAPC_dom_init(void)
{
    MSG(DOM, "\nDevice APC domain init setup:\n\n");

#ifdef DBG_DEVAPC
    MSG(DOM, "Domain Setup (0x%x)\n", reg_read32(DEVAPC0_MAS_DOM_0));
    MSG(DOM, "Domain Setup (0x%x)\n", reg_read32(DEVAPC0_MAS_DOM_1));
    MSG(DOM, "Domain Setup (0x%x)\n", reg_read32(DEVAPC0_MAS_DOM_2));
#endif

    /* Set masters to DOMAINX here if needed
     * Default is DOMAIN_0 */

    /* example */
    /*DAPC_SET_MASTER_DOMAIN(
        DEVAPC0_MAS_DOM_0,
        MODULE_DOMAIN(MASTER_SSUSB_XHCI,   DOMAIN_0) |
        MODULE_DOMAIN(MASTER_PWM,          DOMAIN_1) |
        MODULE_DOMAIN(MASTER_MSDC0,        DOMAIN_2) |
        MODULE_DOMAIN(MASTER_MSDC1,        DOMAIN_3)
    );*/

#ifdef DBG_DEVAPC
    MSG(DOM, "Device APC domain after setup:\n");
    MSG(DOM, "Domain Setup (0x%x)\n", reg_read32(DEVAPC0_MAS_DOM_0));
    MSG(DOM, "Domain Setup (0x%x)\n", reg_read32(DEVAPC0_MAS_DOM_1));
    MSG(DOM, "Domain Setup (0x%x)\n", reg_read32(DEVAPC0_MAS_DOM_2));
#endif
}

static void DAPC_trans_init(void)
{
    MSG(DOM, "\nDevice APC master transcation init setup:\n\n");

#ifdef DBG_DEVAPC
    MSG(DOM, "Master Transaction (0x%x)\n", reg_read32(DEVAPC0_MAS_SEC_0));
#endif

    /* Set master transaction here if needed,
     * default is non-secure transaction */

    /* example */
    /*DAPC_SET_MASTER_TRANSACTION(
        DEVAPC0_MAS_SEC_0,
        MODULE_TRANSACTION(MASTER_NFI,          NS_TRANSACTION) |
        MODULE_TRANSACTION(MASTER_SSUSB_XHCI,    S_TRANSACTION) |
        MODULE_TRANSACTION(MASTER_PWM,          NS_TRANSACTION)
    );*/

#ifdef DBG_DEVAPC
    MSG(DOM, "Master Transaction After Init (0x%x)\n", reg_read32(DEVAPC0_MAS_SEC_0));
#endif

}

static void DAPC_slave_perm_init(void)
{
    MSG(DOM, "\nDevice APC slave permission init setup:\n\n");

    /* Set slave permissions here if needed
     * Default is NO_PROTECTION */

    /* Set SEJ and DAPC to secure RW only */
    DAPC_SET_SLAVE_PERMISSION(
        DEVAPC0_D0_APC(0),
        MODULE_PERMISSION(INFRA_AO_SEJ                        , SEC_RW_ONLY) |
        MODULE_PERMISSION(INFRA_AO_DEVICE_APC_AO_INFRA_PERI   , SEC_RW_ONLY)
    );

    DAPC_SET_SLAVE_PERMISSION(
        DEVAPC0_D0_APC(1),
        MODULE_PERMISSION(INFRA_AO_DEVICE_APC_AO_MM   , SEC_RW_ONLY)
    );

    DAPC_SET_SLAVE_PERMISSION(
        DEVAPC0_D0_APC(2),
        MODULE_PERMISSION(INFRASYS_DEVICE_APC   , SEC_RW_ONLY)
    );

}

static void tz_dapc_default_setting(void)
{

    /* Lock DAPC to secure access only  && unmask debug bit && clear VIO status */
    reg_write32(DEVAPC_APC_CON,  0x80000001);
    reg_write32(DEVAPC0_APC_CON, 0x80000001);
    reg_write32(DEVAPC1_APC_CON, 0x80000001);

    /* Set domain of masters */
    //DAPC_dom_init();

    /* Set the transaction type of masters */
    //DAPC_trans_init();

    /* Set the access permission of slaves in domain 0 */
    DAPC_slave_perm_init();
}


/*=======================================================================*/
/* API                                                                   */
/*=======================================================================*/
void tz_dapc_set_master_transaction(unsigned int  master_index , E_TRANSACTION permisssion_control)
{
    reg_set_field(DEVAPC0_MAS_SEC_0 , (0x1 << master_index), permisssion_control);
}


/*=======================================================================*/
/* INIT FUNCTIONS                                                        */
/*=======================================================================*/

void tz_dapc_sec_init(void)
{
    /* do initial settings */
    tz_dapc_default_setting();
}

void tz_dapc_sec_postinit(void)
{
}
