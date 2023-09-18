
#include <common.h>			// gd

#include "ntx_hwconfig.h"

const char *gszWifi[]={
    "No","AW-GH381","AW-GH321","GB9619","PW621","WC160","WC121","WC121A2",
    "RTL8189","AP6476","NC","RTL8821CS","RTL8822BS","RTL8192","RTL8723DS","CYW43455",
    "88W8987",
};

const char gszNtxHwCfgMagic[]="HW CONFIG ";// hw config tool magic .

const char * gszPCBA[]={ 
	"E60800","E60810","E60820","E90800","E90810", //  0~4 
	"E60830","E60850","E50800","E50810","E60860", //  5~9
	"E60MT2","E60M10","E60610","E60M00","E60M30", // 10~14
	"E60620","E60630","E60640","E50600","E60680", // 15~19
	"E60610C","E60610D","E606A0","E60670","E606B0", // 20~24
	"E50620","Q70Q00","E50610","E606C0","E606D0", // 25~29
	"E606E0","E60Q00","E60Q10","E60Q20","E606F0",	// 30~34
	"E606F0B","E60Q30","E60QB0","E60QC0","A13120",// 35~39
	"E60Q50","E606G0","E60Q60","E60Q80","A13130",// 40~44
	"E606H2","E60Q90","ED0Q00","E60QA0","E60QD0",// 45~49
	"E60QF0","E60QH0","E60QG0","H70000","ED0Q10",// 50~54 
	"E70Q00","H40000","NC","E60QJ0","E60QL0",// 55~59
	"E60QM0","E60QK0","E70S00","T60Q00","C31Q00", // 60~64
	"E60QN0","E60U00","E70Q10","E60QP0","E60QQ0", // 65~69
	"E70Q20","T05R00","M31Q00","E60U10","E60K00", // 70~74
	"E80K00","E70Q30","EA0Q00","E60QR0","ED0R00", // 75~79.
	"E60QU0","E60U20","M35QE0","E60QT0","E70Q50", // 80~84.
	"T60U00","E60QV0","E70K00","T60P00","TA0P00", // 85~89.
	"MXXQ4X","E60P20","T60P10","E60K10","EA0P10", // 90~94.
	"E60P40","E70P10","E70P20","E80P00","E70P20", // 95~99.
	"E60P50","E70K10","E70P50","E60K20", // 100~104
};

const static unsigned char gszNtxBinMagicA[4]={0xff,0xf5,0xaf,0xff};
const unsigned int gRamSize[7]={128,64,256,512,1024,2048,4096};


volatile unsigned char *gpbRAM_TopAddr;
volatile NTX_HWCONFIG *gptNtxHwCfg=0 ;
volatile unsigned long gdwRAM_TotalSize,gdwRAM_ReservedSize;

const static char gszKParamName_HwCfg[]="hwcfg";
const static char gszKParamName_waveform[]="waveform";
const static char gszKParamName_ntxfw[]="ntxfw";
unsigned long gdwNtxHwCfgSize = 0 ;


#ifndef _NTX_HIDDEN_MEM //[
typedef struct tagNtxHiddenMem {
	// private :
	const char *pszName; 		// binary data name .
	unsigned long dwLoadSectNo;	// the sector numbers loaded from emmc .
	// public :
	unsigned long dwLoadSects; 	// total sectors loaded from emmc .
	unsigned char *pbMemStart; 	// binary data ptr in ram for program using .
	unsigned long dwMemSize; 	// binary data size in ram for program using .
	unsigned long dwIdx; 		// loaded index .
	int iIsEnabled; 			// active flag .
} NtxHiddenMem;
#endif //] _NTX_HIDDEN_MEM

static unsigned long gdwNtxHiddenMemIdx;
static NtxHiddenMem *gptNtxHiddenMemInfoA[4];


static NtxHiddenMem gtNtxHiddenMem_ntxfw = {
	.pszName = gszKParamName_ntxfw ,
	//.dwLoadSectNo = SD_OFFSET_SECS_NTXFW,
};

static NtxHiddenMem gtNtxHiddenMem_HwCfg = {
	.pszName = gszKParamName_HwCfg ,
	//.dwLoadSectNo = SD_OFFSET_SECS_HWCFG, //mtk is a partition
	.dwMemSize = 110,
};

static unsigned long _get_ramsize(unsigned char **O_ppbRamStart)
{
	DECLARE_GLOBAL_DATA_PTR;
	if(O_ppbRamStart) {
		*O_ppbRamStart = (unsigned char*)gd->bd->bi_dram[0].start;
	}

	return (unsigned long)(gd->bd->bi_dram[0].size);
}


static int _read_mmc(int I_iSDDevNum,unsigned char *O_pbBuf,
		unsigned long I_dwBinSectorNum,unsigned long I_dwBinReadSectors)
{
	int iRet = 0;
	char cCmdA[128+1];

	static int giCurSDDevNum=-1;

	if(giCurSDDevNum!=I_iSDDevNum) {
		sprintf(cCmdA,"mmc dev %d",I_iSDDevNum);
		iRet=run_command(cCmdA,0);
		giCurSDDevNum = I_iSDDevNum;
	}

	printf("[NTX] %s(): read to %p\n",__FUNCTION__,O_pbBuf);

	sprintf(cCmdA,"mmc read 0x%x 0x%x 0x%x",(unsigned)O_pbBuf,
			(unsigned)(I_dwBinSectorNum),(unsigned)I_dwBinReadSectors);
	printf("%s\n",cCmdA);
	iRet=run_command(cCmdA, 0);//

	return iRet;
}

static unsigned long _load_ntx_bin_header(int I_iSDDevNum,unsigned long I_dwBinSectorNum,
		unsigned char *O_pbBuf,unsigned long I_dwBufSize)
{
	unsigned long dwBinSize = 0;
	unsigned char *pbMagic;
	
	//ASSERT(I_dwBufSize>=512);
	//printf("I_iSDDevNum:%d ,I_dwBinSectorNum: %ul \n",I_iSDDevNum,I_dwBinSectorNum);
	_read_mmc(I_iSDDevNum,O_pbBuf,(unsigned long)(I_dwBinSectorNum),1) ;
	
	pbMagic = O_pbBuf+ 496;
	if( gszNtxBinMagicA[0]==pbMagic[0]&&gszNtxBinMagicA[1]==pbMagic[1]&&
		gszNtxBinMagicA[2]==pbMagic[2]&&gszNtxBinMagicA[3]==pbMagic[3]) 
	{
		dwBinSize = *((unsigned long *)(pbMagic+8));
		printf("[NTX] Find ntxbin magic \n");
	}
	else {
		printf("pbMagic:%x_%x_%x_%x\n",pbMagic[0],pbMagic[1],pbMagic[2],pbMagic[3]);
		printf("gszNtxBinMagicA:%x_%x_%x_%x\n",gszNtxBinMagicA[0],gszNtxBinMagicA[1],gszNtxBinMagicA[2],gszNtxBinMagicA[3]);
		printf("binary magic @ sector no. %lu not found !\n",I_dwBinSectorNum);
	}

	return dwBinSize; 
}


static void _load_ntx_bin(int I_iSDDevNum,unsigned long I_dwBinSectorNum,
		unsigned long I_dwBinSectorsToLoad,unsigned char *O_pbBuf,unsigned long I_dwBufSize)
{
	if(I_dwBufSize<(I_dwBinSectorsToLoad*512)) {
		printf("%s() : buffer size not enough (buffer size %d should >= %d)!\n",
				__FUNCTION__,(int)I_dwBufSize,(int)(I_dwBinSectorsToLoad*512));
		return ;
	}
	
	_read_mmc(I_iSDDevNum,O_pbBuf,I_dwBinSectorNum,I_dwBinSectorsToLoad);		 
}


static unsigned char * NtxHiddenMem_get_topaddr(void)
{
	if(0==gpbRAM_TopAddr) {
		gdwRAM_TotalSize = _get_ramsize((unsigned char **)&gpbRAM_TopAddr);
		printf("ram p=%p,size=%u\n",gpbRAM_TopAddr,(unsigned)gdwRAM_TotalSize);
		gpbRAM_TopAddr += gdwRAM_TotalSize;
		gpbRAM_TopAddr -= 0x700000; 	// MTK kernel reserve some space , gpbRAM_TopAddr = 0x7F300000;
		gdwRAM_ReservedSize = 0;
		gdwNtxHiddenMemIdx = 0;
	}
	return (unsigned char *)gpbRAM_TopAddr;
}

void NtxHiddenMem_append_kcmdline(unsigned long I_ulCmdlineBufSize,char *O_pcCmdlineBufA)
{
	unsigned long dwTemp,dwTemp2;
	char *pcTemp,*pcTemp2;
	char cAppendStr[128]={0};
	int i;
	int iTotalItems;

	iTotalItems = sizeof(gptNtxHiddenMemInfoA)/sizeof(gptNtxHiddenMemInfoA[0]);

	if(O_pcCmdlineBufA) 
	{
		for (i=0;i<iTotalItems;i++)
		{
			if(!gptNtxHiddenMemInfoA[i]) {
				break;
			}

			//printf("[%d]append kcmdline for \"%s\" \n",i,gptNtxHiddenMemInfoA[i]->pszName);
			if(gptNtxHiddenMemInfoA[i]->iIsEnabled) {
				sprintf(cAppendStr," %s_p=0x%x %s_sz=%d",
					gptNtxHiddenMemInfoA[i]->pszName,
					(unsigned)(gptNtxHiddenMemInfoA[i]->pbMemStart),
					gptNtxHiddenMemInfoA[i]->pszName,
					(int)gptNtxHiddenMemInfoA[i]->dwMemSize);
				//printf("%s\n",cAppendStr);

				if(strlen(cAppendStr)+strlen(O_pcCmdlineBufA)<I_ulCmdlineBufSize) 
				{
					strcat(O_pcCmdlineBufA,cAppendStr);
					//printf("out=\"%s\"\n",O_pcCmdlineBufA);
				}
				else {
					printf("%s(%d):cmdline buffer not enought !!\n",__FILE__,__LINE__);
				}
			}
			else {
				printf("%s mem disabled or not avalible !!\n",gptNtxHiddenMemInfoA[i]->pszName);
				sprintf(cAppendStr," %s=bypass",
					gptNtxHiddenMemInfoA[i]->pszName);

				if(strlen(cAppendStr)+strlen(O_pcCmdlineBufA)<I_ulCmdlineBufSize) 
				{
					strcat(O_pcCmdlineBufA,cAppendStr);
					//printf("out=\"%s\"\n",O_pcCmdlineBufA);
				}
				else {
					printf("%s(%d):cmdline buffer not enought !!\n",__FILE__,__LINE__);
				}
			}
		}
	}
	else {
		printf("cmdline buffer not avalible !!\n");
	}

}

static unsigned char * NtxHiddenMem_load_ntxbin(NtxHiddenMem *IO_ptNtxHiddenMem,unsigned long *O_pdwBinSize)
{
	int iLoadDeviceNum ;
	unsigned long dwChk;
	unsigned long dwBinSectsToLoad;
	unsigned long dwBinBytesToLoad;
	unsigned char *pbRetAddr=0;
	int iSkipLoadBin = 0;
	unsigned char bSecBufferA[512] ={0};

	if(IO_ptNtxHiddenMem->pbMemStart && IO_ptNtxHiddenMem->dwMemSize) {
		printf("\"%s\" loaded already !!\n",IO_ptNtxHiddenMem->pszName);
		if(O_pdwBinSize) {
			*O_pdwBinSize = IO_ptNtxHiddenMem->dwMemSize;
		}
		return IO_ptNtxHiddenMem->pbMemStart;
	}

	//cAppendStr[0] = '\0';
	NtxHiddenMem_get_topaddr();
//	iLoadDeviceNum = GET_ISD_NUM();
	iLoadDeviceNum = 0;
	do {

		dwChk = _load_ntx_bin_header(iLoadDeviceNum,\
				IO_ptNtxHiddenMem->dwLoadSectNo,(unsigned char *)bSecBufferA,512);

		if(!dwChk && IO_ptNtxHiddenMem->dwMemSize) {
			dwChk = IO_ptNtxHiddenMem->dwMemSize;
			iSkipLoadBin = 1;
		}

		if(dwChk>0) {

			if(dwChk>=(gdwRAM_TotalSize-gdwRAM_ReservedSize)) {
				printf("ERROR : bin size (%d) bigger than RAM size(%d-%d) !!!\n",
						(int)dwChk,(int)gdwRAM_TotalSize,(int)gdwRAM_ReservedSize);
				break;
			}

			if(gdwNtxHiddenMemIdx!=0) {
				if(IO_ptNtxHiddenMem->dwLoadSectNo<=gptNtxHiddenMemInfoA[gdwNtxHiddenMemIdx-1]->dwLoadSectNo) {
					printf("[WARNING] Binaries load sequence should Lo->Hi !\n");
				}
				else 
				if( (gptNtxHiddenMemInfoA[gdwNtxHiddenMemIdx-1]->dwLoadSectNo+\
						gptNtxHiddenMemInfoA[gdwNtxHiddenMemIdx-1]->dwLoadSects) >= 
						IO_ptNtxHiddenMem->dwLoadSectNo ) 
				{
					printf("skip load \"%s\" because it will overwrite \"%s\" \n",
							IO_ptNtxHiddenMem->pszName,gptNtxHiddenMemInfoA[gdwNtxHiddenMemIdx-1]->pszName);
					break ;
				}
			}

			//dwBinSectsToLoad = (dwChk&0x1ff)?(dwChk>>9)+1:dwChk>>9;
			dwBinSectsToLoad = (dwChk>>9)+1;
			dwBinBytesToLoad = dwBinSectsToLoad<<9;
			gpbRAM_TopAddr -= dwBinBytesToLoad;
			if(!iSkipLoadBin) {
				_load_ntx_bin(iLoadDeviceNum,IO_ptNtxHiddenMem->dwLoadSectNo+1,
					dwBinSectsToLoad,(unsigned char *)gpbRAM_TopAddr,dwBinBytesToLoad);
				pbRetAddr = (unsigned char *)gpbRAM_TopAddr;
			}


			gdwRAM_ReservedSize += dwBinBytesToLoad;
			if(gdwNtxHiddenMemIdx<sizeof(gptNtxHiddenMemInfoA)/sizeof(gptNtxHiddenMemInfoA[0])) {
				gptNtxHiddenMemInfoA[gdwNtxHiddenMemIdx] = IO_ptNtxHiddenMem;

				gptNtxHiddenMemInfoA[gdwNtxHiddenMemIdx]->pbMemStart = (unsigned char *)gpbRAM_TopAddr;
				gptNtxHiddenMemInfoA[gdwNtxHiddenMemIdx]->dwMemSize = dwChk;
				gptNtxHiddenMemInfoA[gdwNtxHiddenMemIdx]->dwIdx = gdwNtxHiddenMemIdx;
				gptNtxHiddenMemInfoA[gdwNtxHiddenMemIdx]->iIsEnabled = 1;

				gptNtxHiddenMemInfoA[gdwNtxHiddenMemIdx]->dwLoadSects = dwBinSectsToLoad;

#if 0 //[ debug informations .
				printf("[%d]%s added\n",gdwNtxHiddenMemIdx,gptNtxHiddenMemInfoA[gdwNtxHiddenMemIdx]->pszName);
				printf(" mem start=%p\n",gptNtxHiddenMemInfoA[gdwNtxHiddenMemIdx]->pbMemStart);
				printf(" mem size=%d\n",gptNtxHiddenMemInfoA[gdwNtxHiddenMemIdx]->dwMemSize);
#endif //] debug informations .

				gdwNtxHiddenMemIdx++;
			}
			else {
				printf("Hidden memory out of range (must < %d)\n",
						sizeof(gptNtxHiddenMemInfoA)/sizeof(gptNtxHiddenMemInfoA[0]));
			}

			if(O_pdwBinSize) {
				*O_pdwBinSize = dwChk;
			}
		}
		else {
			printf("\"%s\" not exist !!\n",IO_ptNtxHiddenMem->pszName);
		}
	} while(0);

	return pbRetAddr;
}


void _load_ntxfw(int nOffset){

	printf("[NTX][%s_%d]  nOffset: %d \n",__FUNCTION__,__LINE__,nOffset);
	gtNtxHiddenMem_ntxfw.dwLoadSectNo = nOffset ;
	NtxHiddenMem_load_ntxbin(&gtNtxHiddenMem_ntxfw,0);

}


void _load_isd_hwconfig(int nOffset)
{
	printf("[NTX][%s_%d]  nOffset: %d \n",__FUNCTION__,__LINE__,nOffset);
	if(gptNtxHwCfg) {
		return ;
	}
	else
	{
		gtNtxHiddenMem_HwCfg.dwLoadSectNo = nOffset ;
		gptNtxHwCfg = (NTX_HWCONFIG *)NtxHiddenMem_load_ntxbin(&gtNtxHiddenMem_HwCfg,&gdwNtxHwCfgSize);
		if(!gptNtxHwCfg) {
			gptNtxHwCfg = (NTX_HWCONFIG *)gtNtxHiddenMem_HwCfg.pbMemStart;
			if( 'H'==gptNtxHwCfg->m_hdr.cMagicNameA[0] && 
					'W'==gptNtxHwCfg->m_hdr.cMagicNameA[1] &&
					' '==gptNtxHwCfg->m_hdr.cMagicNameA[2] &&
					'C'==gptNtxHwCfg->m_hdr.cMagicNameA[3] &&
					'O'==gptNtxHwCfg->m_hdr.cMagicNameA[4] &&
					'N'==gptNtxHwCfg->m_hdr.cMagicNameA[5] &&
					'F'==gptNtxHwCfg->m_hdr.cMagicNameA[6] &&
					'I'==gptNtxHwCfg->m_hdr.cMagicNameA[7] &&
					'G'==gptNtxHwCfg->m_hdr.cMagicNameA[8] 	)
			{
				printf("use hwconfig pass from boot_package . \n");
			}
			else {
				gptNtxHwCfg = 0;
			}
		}
	}

	if(gptNtxHwCfg)
	{
		printf("\n hwcfgp=%p,pcb=%d,customer=%d,SysPartType=%d\n\n",gptNtxHwCfg,\
		gptNtxHwCfg->m_val.bPCB,gptNtxHwCfg->m_val.bCustomer,gptNtxHwCfg->m_val.bSysPartType);

		if (7==gptNtxHwCfg->m_val.bRamSize) {
			// RamSize N/C . 
		}
		else
		if (gptNtxHwCfg->m_val.bRamSize>=sizeof(gRamSize)/sizeof(gRamSize[0])) {
			printf("[Warning] RamSize table must be updated !\n");
		}
		else 
		if ( gRamSize[gptNtxHwCfg->m_val.bRamSize]*1024*1024 < gdwRAM_TotalSize) {
			printf("Detect Ram TotalSize(%d) > HwCfg Ram Size(%d), use HwCfg Ram setting\n",
					(int)gdwRAM_TotalSize,
					(int)(gRamSize[gptNtxHwCfg->m_val.bRamSize]*1024*1024));
			gdwRAM_TotalSize = (unsigned long)gRamSize[gptNtxHwCfg->m_val.bRamSize]*1024*1024;
		}
	}

	if(gdwRAM_TotalSize<=512*1024*1024) {
		//run_command("setenv cma 32M",0);
		// disable (ray)
		//setenv("cma","32M");
	}

	if( (2==gptNtxHwCfg->m_val.bUIStyle) ) {
    	// Android .
		if ( (gptNtxHwCfg->m_val.bEPD_Flags& 0x08) ||  (gdwRAM_TotalSize>1024*1024*1024)) {
    		//  CFA enabled || RAM size > 1GB .
			// disable (ray)
			//setenv("vmalloc","400M");
		}
		else if( gdwRAM_TotalSize==1024*1024*1024 ) {
			// disable (ray)
			//setenv("vmalloc","320M");
		}
	}
	printf("gptNtxHwCfg->m_val.bWifi ＝ %d , String = %s\n", gptNtxHwCfg->m_val.bWifi, gszWifi[gptNtxHwCfg->m_val.bWifi]);

	if (gptNtxHwCfg->m_val.bWifi != 0) {
		// disable (ray)
		//setenv("Wifi", gszWifi[gptNtxHwCfg->m_val.bWifi]);
	}

}
