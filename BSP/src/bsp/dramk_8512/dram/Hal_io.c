/** @file hal_io.cpp
 *  hal_io.cpp provides functions of register access
 */

#include <platform/dramc_common.h>
#include <platform/x_hal_io.h>
#include <platform/dramc_api.h>

#if __ETT__
#include <barriers.h>
#endif

#if 1//REG_ACCESS_PORTING_DGB
extern U8 RegLogEnable;
#endif

extern DRAMC_CTX_T *psCurrDramCtx;

#ifdef DUMP_INIT_RG_LOG_TO_DE
    U8  gDUMP_INIT_RG_LOG_TO_DE_RG_log_flag = 0;
#endif


U64 u4RegBaseAddrTraslate(DRAM_RANK_T eRank, U64 u4reg_addr)
{
    U32 u4Offset = u4reg_addr & 0x1fff;
    U64 u4RegType = ((u4reg_addr-Channel_A_DRAMC_NAO_BASE_VIRTUAL) >> POS_BANK_NUM) & 0xf;
    U64 u4BaseAddr = 0;

	//mcSHOW_DBG_MSG("IOPHYS:0x%llx, %s addr: passin: 0x%llx, type:%d\n", IO_PHYS, __func__, u4reg_addr, u4RegType);
    if (u4reg_addr < Channel_A_DRAMC_NAO_BASE_VIRTUAL || u4reg_addr >= MAX_BASE_VIRTUAL)
		return u4reg_addr;

#if (fcFOR_CHIP_ID != fcSchubert)
    if (eRank == RANK_1)
    {
        if (u4RegType <=1)
        {
            // DramC NAO Register
            if (u4Offset >= (DRAMC_REG_RK0_DQSOSC_STATUS-DRAMC_NAO_BASE_ADDRESS) && u4Offset < (DRAMC_REG_RK1_DQSOSC_STATUS-DRAMC_NAO_BASE_ADDRESS))
            {
                u4Offset += 0x100;
            }
        }
        else if (u4RegType >=2 && u4RegType <=3)
        {
            // DramC AO Register
            if (u4Offset >= (DRAMC_REG_RK0_DQSOSC-DRAMC_AO_BASE_ADDRESS) && u4Offset < (DRAMC_REG_RK1_DQSOSC-DRAMC_AO_BASE_ADDRESS))
            {
                u4Offset += 0x100;
            }
            else if (u4Offset >= (DRAMC_REG_SHURK0_DQSCTL-DRAMC_AO_BASE_ADDRESS) && u4Offset < (DRAMC_REG_SHURK1_DQSCTL-DRAMC_AO_BASE_ADDRESS))
            {
                u4Offset += 0x100;
            }
            else if (u4Offset >= (DRAMC_REG_SHU2RK0_DQSCTL-DRAMC_AO_BASE_ADDRESS) && u4Offset < (DRAMC_REG_SHU2RK1_DQSCTL-DRAMC_AO_BASE_ADDRESS))
            {
                u4Offset += 0x100;
            }
            else if (u4Offset >= (DRAMC_REG_SHU3RK0_DQSCTL-DRAMC_AO_BASE_ADDRESS) && u4Offset < (DRAMC_REG_SHU3RK1_DQSCTL-DRAMC_AO_BASE_ADDRESS))
            {
                u4Offset += 0x100;
            }
        }
        else if (u4RegType >=4 && u4RegType <=5)
        {
            //PHY NAO Register
        }
        else
        {
            // PHY AO Register
            if (u4Offset >= (DDRPHY_R0_B0_RXDVS0-DDRPHY_AO_BASE_ADDR) && u4Offset < (DDRPHY_R1_B0_RXDVS0-DDRPHY_AO_BASE_ADDR))
            {
                if ((u4Offset >= (DDRPHY_RFU_0X620-DDRPHY_AO_BASE_ADDR) && u4Offset < (DDRPHY_R0_B1_RXDVS0-DDRPHY_AO_BASE_ADDR))
                    || (u4Offset >= (DDRPHY_RFU_0X6A0-DDRPHY_AO_BASE_ADDR) && u4Offset < (DDRPHY_R1_B0_RXDVS0-DDRPHY_AO_BASE_ADDR))
                   )
                {
                    //mcSHOW_DBG_MSG("\n[u4RegBaseAddrTraslate] Not Rank address accessed in Rank address!\n");
                }
                else
                {
                    u4Offset += 0x200;
                }
            }
            else if (u4Offset >= (DDRPHY_SHU1_R0_B0_DQ0-DDRPHY_AO_BASE_ADDR) && u4Offset < (DDRPHY_SHU1_R1_B0_DQ0-DDRPHY_AO_BASE_ADDR))
            {
                u4Offset += 0x100;
            }
            else if (u4Offset >= (DDRPHY_MISC_STBERR_RK0_R-DDRPHY_AO_BASE_ADDR) && u4Offset < (DDRPHY_MISC_STBERR_RK1_R-DDRPHY_AO_BASE_ADDR))
            {
                u4Offset += 0x8;
            }
        }
    }
#endif

#if (fcFOR_CHIP_ID == fcSchubert)
    /*const U32 arRegBaseAddrList[8] =
    {
        Channel_A_DRAMC_NAO_BASE_ADDRESS,
        Channel_B_DRAMC_NAO_BASE_ADDRESS,
        Channel_A_DRAMC_AO_BASE_ADDRESS,
        Channel_B_DRAMC_AO_BASE_ADDRESS,
        Channel_A_PHY_NAO_BASE_ADDRESS,
        Channel_B_PHY_NAO_BASE_ADDRESS,
        Channel_A_PHY_AO_BASE_ADDRESS,
        Channel_B_PHY_AO_BASE_ADDRESS,
    };*/
    switch(u4RegType)
    {
        case 0:
             u4BaseAddr = Channel_A_DRAMC_NAO_BASE_ADDRESS;
             break;
         case 1:
             u4BaseAddr = Channel_B_DRAMC_NAO_BASE_ADDRESS;
             break;
         case 2:
             u4BaseAddr = Channel_A_DRAMC_AO_BASE_ADDRESS;
             break;
         case 3:
             u4BaseAddr = Channel_B_DRAMC_AO_BASE_ADDRESS;
             break;
         case 4:
               u4BaseAddr = Channel_A_PHY_NAO_BASE_ADDRESS;
               break;
         case 5:
               u4BaseAddr = Channel_B_PHY_NAO_BASE_ADDRESS;
               break;
         case 6:
               u4BaseAddr = Channel_A_PHY_AO_BASE_ADDRESS;
               break;
         case 7:
               u4BaseAddr = Channel_B_PHY_AO_BASE_ADDRESS;
               break;
	#if PSRAM_SPEC
		 case 8:
			   u4BaseAddr = Channel_A_PSRAM_AO_BASE_ADDRESS;
			   break;
		 case 10:
			   u4BaseAddr = Channel_A_PSRAM_NAO_BASE_ADDRESS;
               break;

	#endif
    }

    return (u4BaseAddr + u4Offset);
#else
    #error No defined arRegBaseAddrList for your chip !!!
#endif
    //mcSHOW_DBG_MSG("\n[u4RegBaseAddrTraslate]  0x%x => 0x%x(0x%x, 0x%x)", u4reg_addr , (arRegBaseAddrList[u4RegType] +u4Offset), arRegBaseAddrList[u4RegType], u4Offset);
    //return (arRegBaseAddrList[u4RegType] +u4Offset);
}

//-------------------------------------------------------------------------
/** ucDram_Register_Read
 *  DRAM register read (32-bit).
 *  @param  u4reg_addr    register address in 32-bit.
 *  @param  pu4reg_value  Pointer of register read value.
 *  @retval 0: OK, 1: FAIL
 */
//-------------------------------------------------------------------------
// This function need to be porting by BU requirement
U32 u4Dram_Register_Read(U64 u4reg_addr)
{
    U32 u4reg_value;
	U64 u4RegType;


	if(psCurrDramCtx->dram_type == TYPE_PSRAM)
	{
	    u4RegType = ((u4reg_addr-Channel_A_DRAMC_NAO_BASE_VIRTUAL) >> POS_BANK_NUM) & 0xf;
	    if(u4RegType%2 == 1)
	    {
	        mcSHOW_DBG_MSG("\n[ucDram_Register_Read] skip CH-B address 0x%x \n",u4reg_addr);
	        return 0;
	    }
	}

	//mcSHOW_DBG_MSG("%s addr: passin: 0x%llx\n", __func__, u4reg_addr);
    u4reg_addr = u4RegBaseAddrTraslate(psCurrDramCtx->rank, u4reg_addr);

	//mcSHOW_DBG_MSG("%s addr:0x%llx\n", __func__, u4reg_addr);

    #if (FOR_DV_SIMULATION_USED==1)   //DV
    u4reg_value = register_read_c(u4reg_addr);
    #elif (SW_CHANGE_FOR_SIMULATION==1) // whole chip sim
    u4reg_value = *((UINT32P)(u4reg_addr));
    #else // real chip
    u4reg_value = (*(volatile unsigned int *)(u4reg_addr));
    #endif

	//mcSHOW_DBG_MSG("%s value:0x%llx\n", __func__, u4reg_value);
    return u4reg_value;
}


//-------------------------------------------------------------------------
/** ucDram_Register_Write
 *  DRAM register write (32-bit).
 *  @param  u4reg_addr    register address in 32-bit.
 *  @param  u4reg_value   register write value.
 *  @retval 0: OK, 1: FAIL
 */
//-------------------------------------------------------------------------
#if REG_SHUFFLE_REG_CHECK
extern U8 ShuffleRegCheck;
extern void ShuffleRegCheckProgram(U32 u4Addr);
#endif

#if REG_ACCESS_NAO_DGB
#if (fcFOR_CHIP_ID == fcSchubert)
U8 Check_RG_Not_AO(U32 u4reg_addr)
{
    U8 RegNotAO = 0;
    if((u4reg_addr >= DRAMC_AO_BASE_ADDRESS) && (u4reg_addr <= DRAMC_REG_SHU4_DQSG_RETRY))
    {
    }
    else if ((u4reg_addr >= DRAMC_AO_BASE_ADDRESS + SHIFT_TO_CHB_ADDR) && (u4reg_addr <= DRAMC_REG_SHU4_DQSG_RETRY + SHIFT_TO_CHB_ADDR))
    {
    }
    else if ((u4reg_addr >= DDRPHY_AO_BASE_ADDR) && (u4reg_addr <= DDRPHY_RFU_0X1FCC))
    {
    }
    else if ((u4reg_addr >= DDRPHY_AO_BASE_ADDR + SHIFT_TO_CHB_ADDR) && (u4reg_addr <= DDRPHY_RFU_0X1FCC + SHIFT_TO_CHB_ADDR))
    {
    }
    else
    {
        RegNotAO = 1;
    }
    return RegNotAO;
}
#endif
#endif

//This function need to be porting by BU requirement
U8 ucDram_Register_Write(U64 u4reg_addr, U32 u4reg_value)
{
    U8 ucstatus;
    ucstatus = 0;  //for SW_CHANGE_FOR_SIMULATION
#if REG_ACCESS_NAO_DGB
    U8 RegNotAO = 0;
#endif
    U32 u4RegType;

	//mcSHOW_DBG_MSG("%s addr:0x%llx\n", __func__, u4reg_addr);
	if(psCurrDramCtx->dram_type == TYPE_PSRAM)
	{
	    u4RegType = ((u4reg_addr-Channel_A_DRAMC_NAO_BASE_VIRTUAL) >> POS_BANK_NUM) & 0xf;
	    if(u4RegType%2 == 1)
	    {
	        mcSHOW_DBG_MSG("\n[ucDram_Register_Write] skip CH-B address 0x%x \n",u4reg_addr);
	        return 0;
	    }
	}
#if __ETT__
    if(u1IsLP4Family(psCurrDramCtx->dram_type))
    {
        CheckDramcWBR(u4reg_addr);
    }
#if REG_ACCESS_NAO_DGB
    RegNotAO = Check_RG_Not_AO(u4reg_addr);
#endif
#endif

    u4reg_addr = u4RegBaseAddrTraslate(psCurrDramCtx->rank, u4reg_addr);
	//mcSHOW_DBG_MSG("%s addr:0x%llx\n", __func__, u4reg_addr);

#ifdef DUMP_INIT_RG_LOG_TO_DE
    if (gDUMP_INIT_RG_LOG_TO_DE_RG_log_flag == 1)
    {
        mcSHOW_DUMP_INIT_RG_MSG(("*((UINT32P)(0x%x)) = 0x%x;\n",u4reg_addr,u4reg_value));
        mcDELAY_MS(1);
    }
#endif


#if (FOR_DV_SIMULATION_USED==1) //DV
    register_write_c(u4reg_addr,u4reg_value);
#elif (SW_CHANGE_FOR_SIMULATION==1) //whole chip sim
    (*(volatile unsigned int *)u4reg_addr) = u4reg_value;
#else
    (*(volatile unsigned int *)u4reg_addr) = u4reg_value;//real chip
    dsb();
#endif

#if REG_ACCESS_PORTING_DGB
    if(RegLogEnable)
    {
        mcSHOW_DBG_MSG("\n[REG_ACCESS_PORTING_DBG]   ucDramC_Register_Write Reg(0x%X) = 0x%X\n", u4reg_addr, u4reg_value);
        mcFPRINTF(fp_A60501, "\n[REG_ACCESS_PORTING_DBG]   ucDramC_Register_Write Reg(0x%X) = 0x%X\n", u4reg_addr, u4reg_value);
        //mcFPRINTF(fp_A60501, "\nRISCWrite ('h%05X , 32'h%X);\n",u4reg_addr&0xFFFFF,  u4reg_value);
    }
#endif
#if REG_ACCESS_NAO_DGB
    if(RegNotAO)
    {
        mcSHOW_DBG_MSG("\n[REG_ACCESS_NAO_DGB] ucDramC_Register_Write Reg(0x%X) = 0x%X\n", u4reg_addr, u4reg_value);
    }
#endif
#if REG_SHUFFLE_REG_CHECK
    if(ShuffleRegCheck)
    {
        ShuffleRegCheckProgram(u4reg_addr);
    }
#endif
    return ucstatus;
}

#if SW_CHANGE_FOR_SIMULATION
U32 u4IO32ReadFldAlign2(U32 reg32, U32 fld)
{
    if(Fld_ac(fld)==AC_FULLDW)
    {
        return u4IO32Read4B(reg32);
    }
    else
    {
        return ((u4IO32Read4B(reg32)&Fld2Msk32(fld))>>Fld_shft(fld));
    }
}

void vIO32WriteFldAlign2(U32 reg32, U32 val, U32 fld)
{
    if(Fld_ac(fld)==AC_FULLDW)
    {
        vIO32Write4B((reg32),(val));
    }
    else
    {
        vIO32Write4BMsk((reg32),((U32)(val)<<Fld_shft(fld)),Fld2Msk32(fld));
    }
}

void vIO32WriteFldAlign_All2(U32 reg32, U32 val, U32 fld)
{
    if(Fld_ac(fld)==AC_FULLDW)
    {
        vIO32Write4B_All((reg32),(val));
    }
    else
    {
        vIO32Write4BMsk_All((reg32),((U32)(val)<<Fld_shft(fld)),Fld2Msk32(fld));
    }
}
#endif

void vIO32Write4BMsk2(U64 reg32, U32 val32, U32 msk32)
{
    U32 u4Val;

    val32 &=msk32;

    u4Val = u4Dram_Register_Read(reg32);
    u4Val = ((u4Val &~msk32)|val32);
    ucDram_Register_Write(reg32, u4Val);
}


void vIO32Write4B_All2(U64 reg32, U32 val32)
{
    U8 ii, u1AllCount;
    U64 u4RegType = (reg32 & (0xf <<POS_BANK_NUM));

#if __ETT__
    if (GetDramcBroadcast()==DRAMC_BROADCAST_ON)
    {
        mcSHOW_ERR_MSG("Error! virtual address 0x%x don't have to use write_all when Dramc WBR is on\n", reg32);
        while(1);
    }
#endif

    reg32 &= 0xffff;     // remove channel information

#if CHANNEL_NUM==4
    u1AllCount = 4;
#else
    u1AllCount = 2;
#endif

    if((u4RegType >=Channel_A_PHY_AO_BASE_VIRTUAL)&&(u4RegType < Channel_A_PSRAM_AO_BASE_VIRTUAL))//PHY
    {
        #if (fcFOR_CHIP_ID == fcSchubert)
        reg32 += Channel_A_PHY_AO_BASE_VIRTUAL;
        if(u1IsLP4Family(psCurrDramCtx->dram_type) || (psCurrDramCtx->dram_type == TYPE_PSRAM))
        {
            u1AllCount = 1;
        }
        else //Lp3 use CHB B0
        {
            if((reg32 >= DDRPHY_B1_DLL_ARPI0 && reg32 <= DDRPHY_CA_CMD10)
                || (reg32 >= DDRPHY_B1_RXDVS0 && reg32 <= DDRPHY_R1_CA_RXDVS9)
                || (reg32 >= DDRPHY_SHU1_B1_DQ0 && reg32 <= DDRPHY_SHU1_CA_DLL1)
                || (reg32 >= DDRPHY_SHU1_R0_B1_DQ0 && reg32 <= DDRPHY_SHU1_R1_CA_CMD9)
                || (reg32 >= DDRPHY_SHU2_B1_DQ0 && reg32 <= DDRPHY_SHU2_CA_DLL1)
                || (reg32 >= DDRPHY_SHU2_R0_B1_DQ0 && reg32 <= DDRPHY_SHU2_R1_CA_CMD9)
                || (reg32 >= DDRPHY_SHU3_B1_DQ0 && reg32 <= DDRPHY_SHU3_CA_DLL1)
                || (reg32 >= DDRPHY_SHU3_R0_B1_DQ0 && reg32 <= DDRPHY_SHU3_R1_CA_CMD9)
                || (reg32 >= DDRPHY_SHU4_B1_DQ0 && reg32 <= DDRPHY_SHU4_CA_DLL1)
                || (reg32 >= DDRPHY_SHU4_R0_B1_DQ0 && reg32 <= DDRPHY_SHU4_R1_CA_CMD9)
                )
            {
                u1AllCount = 1;
            }
        }
        #endif
    }
    else if((u4RegType >= Channel_A_DRAMC_AO_BASE_VIRTUAL) && (u4RegType < Channel_A_PHY_NAO_BASE_VIRTUAL))// DramC AO
    {
        reg32 += Channel_A_DRAMC_AO_BASE_VIRTUAL;
        #if (fcFOR_CHIP_ID == fcSchubert)
        u1AllCount = 1;
        #endif
    }
    else if (u4RegType == Channel_A_PSRAM_AO_BASE_VIRTUAL)// PSRAMC AO
    {
        reg32 += Channel_A_PSRAM_AO_BASE_VIRTUAL;
        #if (fcFOR_CHIP_ID == fcSchubert)
        u1AllCount = 1;
        #endif
    }
    else
    {
        mcSHOW_ERR_MSG("Error! NAO address was not written\n");
    }
    for(ii=0; ii< u1AllCount; ii++)
    {
        vIO32Write4B(reg32+((U32)ii<<POS_BANK_NUM), val32);
    }

#if 0
    // debug
    //mcSHOW_DBG_MSG("RISCWrite : address %05x data %08x\n", reg32&0x000fffff, val32);
    mcFPRINTF(fp_A60501, "RISCWrite : address %05x data %08x wait  0\n", reg32&0x000fffff, val32);
    //mcSHOW_DBG_MSG("RISCWrite : address %05x data %08x\n", (reg32+((U32)1<<POS_BANK_NUM))&0x000fffff, val32);
    mcFPRINTF(fp_A60501, "RISCWrite : address %05x data %08x wait  0\n", (reg32+((U32)1<<POS_BANK_NUM))&0x000fffff, val32);
    //mcSHOW_DBG_MSG("RISCWrite : address %05x data %08x\n", (reg32+((U32)2<<POS_BANK_NUM))&0x000fffff, val32);
    mcFPRINTF(fp_A60501, "RISCWrite : address %05x data %08x wait  0\n", (reg32+((U32)2<<POS_BANK_NUM))&0x000fffff, val32);
#endif
}

void vIO32Write4BMsk_All2(U64 reg32, U32 val32, U32 msk32)
{
    U32 u4Val;
    U8 ii, u1AllCount;
    U64 u4RegType = (reg32 & (0xf <<POS_BANK_NUM));

#if __ETT__
    if (GetDramcBroadcast()==DRAMC_BROADCAST_ON)
    {
        mcSHOW_ERR_MSG("Error! virtual address 0x%x don't have to use write_all when Dramc WBR is on\n", reg32);
        while(1);
    }
#endif

    reg32 &= 0xffff;     // remove channel information

#if CHANNEL_NUM==4
        u1AllCount = 4;
#else
        u1AllCount = 2;
#endif

    if((u4RegType >= Channel_A_PHY_AO_BASE_VIRTUAL)&&(u4RegType < Channel_A_PSRAM_AO_BASE_VIRTUAL))//PHY
    {
        #if (fcFOR_CHIP_ID == fcSchubert)
        reg32 += Channel_A_PHY_AO_BASE_VIRTUAL;
        if(u1IsLP4Family(psCurrDramCtx->dram_type) || (psCurrDramCtx->dram_type == TYPE_PSRAM))
        {
            u1AllCount = 1;
        }
        else
        {
            if((reg32 >= DDRPHY_B1_DLL_ARPI0 && reg32 <= DDRPHY_CA_CMD10)
                || (reg32 >= DDRPHY_B1_RXDVS0 && reg32 <= DDRPHY_R1_CA_RXDVS9)
                || (reg32 >= DDRPHY_SHU1_B1_DQ0 && reg32 <= DDRPHY_SHU1_CA_DLL1)
                || (reg32 >= DDRPHY_SHU1_R0_B1_DQ0 && reg32 <= DDRPHY_SHU1_R1_CA_CMD9)
                || (reg32 >= DDRPHY_SHU2_B1_DQ0 && reg32 <= DDRPHY_SHU2_CA_DLL1)
                || (reg32 >= DDRPHY_SHU2_R0_B1_DQ0 && reg32 <= DDRPHY_SHU2_R1_CA_CMD9)
                || (reg32 >= DDRPHY_SHU3_B1_DQ0 && reg32 <= DDRPHY_SHU3_CA_DLL1)
                || (reg32 >= DDRPHY_SHU3_R0_B1_DQ0 && reg32 <= DDRPHY_SHU3_R1_CA_CMD9)
                || (reg32 >= DDRPHY_SHU4_B1_DQ0 && reg32 <= DDRPHY_SHU4_CA_DLL1)
                || (reg32 >= DDRPHY_SHU4_R0_B1_DQ0 && reg32 <= DDRPHY_SHU4_R1_CA_CMD9)
                )
            {
                u1AllCount = 1;
            }
        }
        #endif
    }
    else if((u4RegType >= Channel_A_DRAMC_AO_BASE_VIRTUAL) && (u4RegType < Channel_A_PHY_NAO_BASE_VIRTUAL))// DramC AO
    {
        reg32 += Channel_A_DRAMC_AO_BASE_VIRTUAL;
        #if (fcFOR_CHIP_ID == fcSchubert)
        u1AllCount = 1;
        #endif
    }
    else if (u4RegType == Channel_A_PSRAM_AO_BASE_VIRTUAL)// PSRAM AO
    {
        reg32 += Channel_A_PSRAM_AO_BASE_VIRTUAL;
        #if (fcFOR_CHIP_ID == fcSchubert)
        u1AllCount = 1;
        #endif
    }
    else
    {
        mcSHOW_ERR_MSG("Error! NAO address was not written\n");
    }

    for(ii=0; ii< u1AllCount; ii++)
    {
        u4Val = u4Dram_Register_Read(reg32+((U32)ii<<POS_BANK_NUM));
        u4Val = ((u4Val &~msk32)|val32);
        ucDram_Register_Write(reg32+((U32)ii<<POS_BANK_NUM), u4Val);
    }
}
