#ifndef __MT_SCP_H__
#define __MT_SCP_H__

#define SCPSYS_PART_NAME	"SCPSYS"
#define CMSYS_RESET_CTL		0x0
#define CPU_RST_SW		0x1

#define CMSYS_CLKGAT_CTL	0x8
#define CPUCK_EN		0x1

#define MAX_SCPSYS_SIZE         0x20000
/* scp firmware information */
#define PART_HDR_DATA_SIZE	512
#define PART_MAGIC		0x58881688
#define PART_EXT_MAGIC		0x58891689
union  fm_hdr_t {
	struct {
		unsigned int magic;        /* partition magic */
		unsigned int dsize;        /* partition data size */
		char         name[32];     /* partition name */
		unsigned int maddr;        /* partition memory address */
		unsigned int mode;
		/* extension */
		unsigned int ext_magic;    /* always EXT_MAGIC */
		/* header size: 512 bytes currently,may extend in the future */
		unsigned int hdr_size;
		unsigned int hdr_version;  /* see HDR_VERSION */
		unsigned int img_type;
		unsigned int img_list_end;
		unsigned int align_size;
		unsigned int dsize_extend;
		unsigned int maddr_extend;
	} info;
	unsigned char data[PART_HDR_DATA_SIZE];
};

void start_scpsys(void);
void stop_scpsys(void);
int load_scpsys(void);

#endif

