#ifndef __CUSTOM_EMI__
#define __CUSTOM_EMI__

#include <platform/emi.h>

#if SUPPORT_TYPE_LPDDR4
EMI_SETTINGS default_emi_setting =
{
	.sub_version = 0x1,		/* sub_version */
	.type = 0x0003,         /* TYPE : 0x0:PC3, 0x1:PC4, 0x2:LP3, 0x3:LP4 */
	.id_length = 9,			/* EMMC ID/FW ID checking length */
	.fw_id_length = 0,		/* FW length */
	.ID = {0x15,0x01,0x00,0x44,0x48,0x36,0x44,0x4D,0x42,0x0,0x0,0x0,0x0,0x0,0x0,0x0},	/* NAND_EMMC_ID */
	.fw_id = {0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0},	/* FW_ID */
	.EMI_CONA_VAL = 0x1000a050,/*256Mbx16:0x1000a050,128Mbx16:0x10005050*/
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
	.CHN0_EMI_CONA_VAL = 0x0,
	.CHN1_EMI_CONA_VAL = 0x0,
	.dram_cbt_mode_extern = CBT_R0_R1_NORMAL,    /* dram_cbt_mode_extern */
	.reserved = {0,0,0,0,0,0},          /* reserved 6 */
	.iLPDDR3_MODE_REG_5 = 0x00000000,   /* LPDDR4_MODE_REG5 */
	.PIN_MUX_TYPE = 0x0,                /* PIN_MUX_TYPE for tablet */
};
#elif SUPPORT_TYPE_LPDDR3
EMI_SETTINGS default_emi_setting =
{
	.sub_version = 0x1,		/* sub_version */
	.type = 0x0002,         /* TYPE */
	.id_length = 9,			/* EMMC ID/FW ID checking length */
	.fw_id_length = 0,		/* FW length */
	.ID = {0x15,0x01,0x00,0x44,0x48,0x36,0x44,0x4D,0x42,0x0,0x0,0x0,0x0,0x0,0x0,0x0},	/* NAND_EMMC_ID */
	.fw_id = {0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0},	/* FW_ID */
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
	.DRAM_RANK_SIZE = {0x20000000,0,0,0},   /* rank size */
	.EMI_CONF_VAL = 0x0, /* EMI_CONF_VAL */
	.CHN0_EMI_CONA_VAL = 0x0,
	.CHN1_EMI_CONA_VAL = 0x0,
	.dram_cbt_mode_extern = CBT_R0_R1_NORMAL,    /* dram_cbt_mode_extern */
	.reserved = {0,0,0,0,0,0},          /* reserved 6 */
	.iLPDDR3_MODE_REG_5 = 0x00000000,   /* LPDDR4_MODE_REG5 */
	.PIN_MUX_TYPE = 0x0,                /* PIN_MUX_TYPE for tablet */
};
#elif SUPPORT_TYPE_PCDDR3
#if SUPPORT_PCDDR3_32BIT
EMI_SETTINGS default_emi_setting =
{
	.sub_version = 0x1,		/* sub_version */
	.type = 0x0000,         /* TYPE */
	.id_length = 9,			/* EMMC ID/FW ID checking length */
	.fw_id_length = 0,		/* FW length */
	.ID = {0x15,0x01,0x00,0x44,0x48,0x36,0x44,0x4D,0x42,0x0,0x0,0x0,0x0,0x0,0x0,0x0},	/* NAND_EMMC_ID */
	.fw_id = {0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0},	/* FW_ID */
	.EMI_CONA_VAL = 0x0000a052,/*256Mbx16 x2:0x0000a052*/
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
	.CHN0_EMI_CONA_VAL = 0x0,
	.CHN1_EMI_CONA_VAL = 0x0,
	.dram_cbt_mode_extern = CBT_R0_R1_NORMAL,    /* dram_cbt_mode_extern */
	.reserved = {0,0,0,0,0,0},          /* reserved 6 */
	.iLPDDR3_MODE_REG_5 = 0x00000000,   /* LPDDR4_MODE_REG5 */
	.PIN_MUX_TYPE = 0x0,                /* PIN_MUX_TYPE for tablet */
};
#else
EMI_SETTINGS default_emi_setting =
{
	.sub_version = 0x1,		/* sub_version */
	.type = 0x0000,         /* TYPE */
	.id_length = 9,			/* EMMC ID/FW ID checking length */
	.fw_id_length = 0,		/* FW length */
	.ID = {0x15,0x01,0x00,0x44,0x48,0x36,0x44,0x4D,0x42,0x0,0x0,0x0,0x0,0x0,0x0,0x0},	/* NAND_EMMC_ID */
	.fw_id = {0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0},	/* FW_ID */
	.EMI_CONA_VAL = 0x0000a050,
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
	.CHN0_EMI_CONA_VAL = 0x0,
	.CHN1_EMI_CONA_VAL = 0x0,
	.dram_cbt_mode_extern = CBT_R0_R1_NORMAL,    /* dram_cbt_mode_extern */
	.reserved = {0,0,0,0,0,0},          /* reserved 6 */
	.iLPDDR3_MODE_REG_5 = 0x00000000,   /* LPDDR4_MODE_REG5 */
	.PIN_MUX_TYPE = 0x0,                /* PIN_MUX_TYPE for tablet */
};
#endif
#elif SUPPORT_TYPE_PCDDR4
EMI_SETTINGS default_emi_setting =
{
	.sub_version = 0x1,		/* sub_version */
	.type = 0x0001,         /* TYPE */
	.id_length = 9,			/* EMMC ID/FW ID checking length */
	.fw_id_length = 0,		/* FW length */
	.ID = {0x15,0x01,0x00,0x44,0x48,0x36,0x44,0x4D,0x42,0x0,0x0,0x0,0x0,0x0,0x0,0x0},	/* NAND_EMMC_ID */
	.fw_id = {0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0},	/* FW_ID */
	.EMI_CONA_VAL = 0x0020a150, /*4Gbx16:0x0020a150,8Gbx16:0x0020f150*/
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
	.CHN0_EMI_CONA_VAL = 0x0,
	.CHN1_EMI_CONA_VAL = 0x0,
	.dram_cbt_mode_extern = CBT_R0_R1_NORMAL,    /* dram_cbt_mode_extern */
	.reserved = {0,0,0,0,0,0},          /* reserved 6 */
	.iLPDDR3_MODE_REG_5 = 0x00000000,   /* LPDDR4_MODE_REG5 */
	.PIN_MUX_TYPE = 0x0,                /* PIN_MUX_TYPE for tablet */
};
#endif
#endif /* __CUSTOM_EMI__ */

