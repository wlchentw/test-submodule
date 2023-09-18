/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to MediaTek Inc. and/or its licensors.
 * Without the prior written permission of MediaTek inc. and/or its licensors,
 * any reproduction, modification, use or disclosure of MediaTek Software,
 * and information contained herein, in whole or in part, shall be strictly prohibited.
 */
/* MediaTek Inc. (C) 2015. All rights reserved.
 *
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
 * RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER ON
 * AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
 * NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
 * SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
 * SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES TO LOOK ONLY TO SUCH
 * THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY ACKNOWLEDGES
 * THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES
 * CONTAINED IN MEDIATEK SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK
 * SOFTWARE RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND
 * CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
 * AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE,
 * OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER TO
 * MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 */

#include <platform/mt_i2c.h>
#include <platform/rt5748.h>

/**********************************************************
  *
  *   [I2C Function For Read/Write rt5748]
  *
  *********************************************************/
u32 rt5748_write_byte(u8 addr, u8 value, u8 i2c_bus)
{
    int ret_code = 0;
    u8 write_data[2];
    u8 len;

    write_data[0]= addr;
    write_data[1] = value;

    len = 2;

    ret_code = mtk_i2c_write(i2c_bus, rt5748_SLAVE_ADDR, 100, write_data, len);

    if(ret_code == 0)
        return 0; // ok
    else
        return -1; // fail
}

u32 rt5748_read_byte (u8 addr, u8 *dataBuffer, u8 i2c_bus)
{
    int ret_code = 0;
    u8 len;
    *dataBuffer = addr;

    len = 1;

    ret_code = mtk_i2c_write_read(i2c_bus, rt5748_SLAVE_ADDR, 100,
               dataBuffer, dataBuffer, len, len);

    if(ret_code == 0)
        return 0; // ok
    else
        return -1; // fail
}

/**********************************************************
  *
  *   [Read / Write Function]
  *
  *********************************************************/
u32 rt5748_read_interface (u8 RegNum, u8 *val, u8 MASK, u8 SHIFT, u8 i2c_bus)
{
    u8 rt5748_reg = 0;
    u32 ret = 0;

    ret = rt5748_read_byte(RegNum, &rt5748_reg, i2c_bus);

    rt5748_reg &= (MASK << SHIFT);
    *val = (rt5748_reg >> SHIFT);

    return ret;
}

u32 rt5748_config_interface (u8 RegNum, u8 val, u8 MASK, u8 SHIFT, u8 i2c_bus)
{
    u8 rt5748_reg = 0;
    u32 ret = 0;

    ret = rt5748_read_byte(RegNum, &rt5748_reg, i2c_bus);

    rt5748_reg &= ~(MASK << SHIFT);
    rt5748_reg |= (val << SHIFT);

    ret = rt5748_write_byte(RegNum, rt5748_reg, i2c_bus);

    return ret;
}


void RT5748_INIT_SETTING_V1(void)
{
	rt5748_config_interface(0x02, 0, 0x1, 1, rt5748_I2C_ID_P1);
	rt5748_config_interface(0x02, 0, 0x1, 1, rt5748_I2C_ID_M2);
}

//==============================================================================
// BUCK RT5748 Init Code
//==============================================================================
void rt5748_init (void)
{
    printf("[buck5748_init] Preloader Start..................\n");

    /* buck initial setting */
    RT5748_INIT_SETTING_V1();

    printf("[buck5748_init] Done...................\n");
}

