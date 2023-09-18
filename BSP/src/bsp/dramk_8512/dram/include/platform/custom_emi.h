#ifndef __CUSTOM_EMI__
#define __CUSTOM_EMI__

#include <platform/emi.h>

#if SUPPORT_TYPE_LPDDR4

#warning "defined SUPPORT_TYPE_LPDDR4"
#ifdef LP4_8Gb_SUPPORT
EMI_SETTINGS default_emi_setting =
{
	.sub_version = 0x1,		/* sub_version */
	.type = 0x5,			/* TYPE : 0x3:LP3, 0x5:LP4 0x8:PSRAM*/
	.id_length = 0,			/* EMMC ID/FW ID checking length */
	.fw_id_length = 0,		/* FW length */
	.ID = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0},	/* NAND_EMMC_ID */
	.fw_id = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0},	/* FW_ID */
	.EMI_CONA_VAL = 0xf050f054,/*256Mbx16:0x1000a050,128Mbx16:0x10005050*/
	.EMI_CONH_VAL = 0x00000000,
	.DRAMC_ACTIME_UNION = {
		0x00000000,		/* U 00 */
		0x00000000,		/* U 01 */
		0x00000000,		/* U 02 */
		0x00000000,		/* U 03 */
		0x00000000,		/* U 04 */
		0x00000000,		/* U 05 */
		0x00000000,		/* U 06 */
		0x00000000,		/* U 07 */
	},
	.DRAM_RANK_SIZE = {0x40000000,0,0,0},   /* rank size */
	.EMI_CONF_VAL = 0x0, /* EMI_CONF_VAL */
	.CHN0_EMI_CONA_VAL = 0x0444f050,
	.CHN1_EMI_CONA_VAL = 0x0,
	.dram_cbt_mode_extern = CBT_R0_R1_NORMAL,    /* dram_cbt_mode_extern */
	.reserved = {0,0,0,0,0,0},          /* reserved 6 */
	.iLPDDRX_MODE_REG_5 = 0x00000000,   /* LPDDRX_MODE_REG5 */
	.PIN_MUX_TYPE = 0x0,                /* PIN_MUX_TYPE for tablet */
};
#else	/* default: 4Gb */
EMI_SETTINGS default_emi_setting =
{
	.sub_version = 0x1,		/* sub_version */
	.type = 0x5,			/* TYPE : 0x3:LP3, 0x5:LP4 0x8:PSRAM*/
	.id_length = 0,			/* EMMC ID/FW ID checking length */
	.fw_id_length = 0,		/* FW length */
	.ID = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0},	/* NAND_EMMC_ID */
	.fw_id = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0},	/* FW_ID */
	.EMI_CONA_VAL = 0xf050e054,/*256Mbx16:0x1000a050,128Mbx16:0x10005050*/
	.EMI_CONH_VAL = 0x00000000,
	.DRAMC_ACTIME_UNION = {
		0x00000000,		/* U 00 */
		0x00000000,		/* U 01 */
		0x00000000,		/* U 02 */
		0x00000000,		/* U 03 */
		0x00000000,		/* U 04 */
		0x00000000,		/* U 05 */
		0x00000000,		/* U 06 */
		0x00000000,		/* U 07 */
	},
	.DRAM_RANK_SIZE = {0x20000000,0,0,0},   /* rank size */
	.EMI_CONF_VAL = 0x0, /* EMI_CONF_VAL */
	.CHN0_EMI_CONA_VAL = 0x0442e050,
	.CHN1_EMI_CONA_VAL = 0x0,
	.dram_cbt_mode_extern = CBT_R0_R1_NORMAL,    /* dram_cbt_mode_extern */
	.reserved = {0,0,0,0,0,0},          /* reserved 6 */
	.iLPDDRX_MODE_REG_5 = 0x00000000,   /* LPDDRX_MODE_REG5 */
	.PIN_MUX_TYPE = 0x0,                /* PIN_MUX_TYPE for tablet */
};
#endif

#elif SUPPORT_TYPE_LPDDR3

#ifdef LP3_128MB_SUPPORT	/* LP3_1Gb_SUPPORT */
EMI_SETTINGS default_emi_setting =
{
	.sub_version = 0x1,		/* sub_version */
	.type = 0x3,			/* TYPE : 0x3:LP3, 0x5:LP4 0x8:PSRAM*/
	.id_length = 0,			/* EMMC ID/FW ID checking length */
	.fw_id_length = 0,		/* FW length */
	.ID = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0},	/* NAND_EMMC_ID */
	.fw_id = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0},	/* FW_ID */
	.EMI_CONA_VAL = 0xf050c054,
	.EMI_CONH_VAL = 0x00000000,
	.DRAMC_ACTIME_UNION = {
		0x00000000,		/* U 00 */
		0x00000000,		/* U 01 */
		0x00000000,		/* U 02 */
		0x00000000,		/* U 03 */
		0x00000000,		/* U 04 */
		0x00000000,		/* U 05 */
		0x00000000,		/* U 06 */
		0x00000000,		/* U 07 */
	},
	.DRAM_RANK_SIZE = {0x08000000,0,0,0},	/* rank size */
	.EMI_CONF_VAL = 0x0, /* EMI_CONF_VAL */
	.CHN0_EMI_CONA_VAL = 0x0040c050,
	.CHN1_EMI_CONA_VAL = 0x0,
	.dram_cbt_mode_extern = CBT_R0_R1_NORMAL,    /* dram_cbt_mode_extern */
	.reserved = {0,0,0,0,0,0},          /* reserved 6 */
	.iLPDDRX_MODE_REG_5 = 0x00000000,   /* LPDDRX_MODE_REG5 */
	.PIN_MUX_TYPE = 0x0,                /* PIN_MUX_TYPE for tablet */
};
#else	/* default: 512Mb: 64MB */
EMI_SETTINGS default_emi_setting =
{
	.sub_version = 0x1,		/* sub_version */
	.type = 0x3,			/* TYPE : 0x3:LP3, 0x5:LP4 0x8:PSRAM*/
	.id_length = 0,			/* EMMC ID/FW ID checking length */
	.fw_id_length = 0,		/* FW length */
	.ID = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0},	/* NAND_EMMC_ID */
	.fw_id = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0},	/* FW_ID */
	.EMI_CONA_VAL = 0xf050c054,
	.EMI_CONH_VAL = 0x00000000,
	.DRAMC_ACTIME_UNION = {
		0x00000000,		/* U 00 */
		0x00000000,		/* U 01 */
		0x00000000,		/* U 02 */
		0x00000000,		/* U 03 */
		0x00000000,		/* U 04 */
		0x00000000,		/* U 05 */
		0x00000000,		/* U 06 */
		0x00000000,		/* U 07 */
	},
	.DRAM_RANK_SIZE = {0x04000000,0,0,0},	/* rank size */
	.EMI_CONF_VAL = 0x0, /* EMI_CONF_VAL */
	.CHN0_EMI_CONA_VAL = 0x0040c050,
	.CHN1_EMI_CONA_VAL = 0x0,
	.dram_cbt_mode_extern = CBT_R0_R1_NORMAL,    /* dram_cbt_mode_extern */
	.reserved = {0,0,0,0,0,0},          /* reserved 6 */
	.iLPDDRX_MODE_REG_5 = 0x00000000,   /* LPDDRX_MODE_REG5 */
	.PIN_MUX_TYPE = 0x0,                /* PIN_MUX_TYPE for tablet */
};

#endif

#elif SUPPORT_TYPE_PSRAM

/* default: 64Mb: 8MB */
EMI_SETTINGS default_emi_setting =
{
	.sub_version = 0x1,		/* sub_version */
	.type = 0x0008,			/* TYPE : 0x3:LP3, 0x5:LP4 0x8:PSRAM*/
	.id_length = 0,			/* EMMC ID/FW ID checking length */
	.fw_id_length = 0,		/* FW length */
	.ID = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0},	/* NAND_EMMC_ID */
	.fw_id = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0},	/* FW_ID */
	.EMI_CONA_VAL = 0x00005052,
	.EMI_CONH_VAL = 0x00000000,
	.DRAMC_ACTIME_UNION = {
		0x00000000,		/* U 00 */
		0x00000000,		/* U 01 */
		0x00000000,		/* U 02 */
		0x00000000,		/* U 03 */
		0x00000000,		/* U 04 */
		0x00000000,		/* U 05 */
		0x00000000,		/* U 06 */
		0x00000000,		/* U 07 */
	},
	.DRAM_RANK_SIZE = {0x00800000,0,0,0},   /* PSRAM size, not use*/
	.EMI_CONF_VAL = 0x0, /* EMI_CONF_VAL */
	.CHN0_EMI_CONA_VAL = 0x0,
	.CHN1_EMI_CONA_VAL = 0x0,
	.dram_cbt_mode_extern = CBT_R0_R1_NORMAL,    /* dram_cbt_mode_extern */
	.reserved = {0,0,0,0,0,0},          /* reserved 6 */
	.iLPDDRX_MODE_REG_5 = 0x00000000,   /* LPDDRX_MODE_REG5 */
	.PIN_MUX_TYPE = 0x0,                /* PIN_MUX_TYPE for tablet */
};


#endif
#endif /* __CUSTOM_EMI__ */

