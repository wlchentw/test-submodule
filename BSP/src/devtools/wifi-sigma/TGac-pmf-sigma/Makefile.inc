##### Change the following for your environment: ############ 
#***************** cortex-a9-rhel4 PATH  ********************
#CROSS_COMPILER=${PWD}/../../vendor/mediatek/c4a/prebuilts/toolchain/armv7a/bin/armv7a-cros-linux-gnueabi-



#CC=$(CROSS_COMPILER)gcc
#AR=$(CROSS_COMPILER)ar
LIB=lib
DUT=dut
CA=ca
UCC=ucc
MAKE=make

MTK_P2P_CFLAGS := -DMTK_P2P_SUPPLICANT -DMTK_P2P_SIGMA
MTK_HS20_CFLAGS := -DMTK_HS20_SIGMA
MTK_AC_CFLAGS := -DMTK_AC_SIGMA -DMTK_PMF_SIGMA
MTK_TDLS_CFLAGS:= -DMTK_TDLS_SIGMA
EXTRA_MTK_CFLAGS=-DMTK_PLATFORM -DMTK_SPECIFIC_PING_SIGMA

### Modify here for different project
MTK_CFLAGS := $(MTK_AC_CFLAGS) $(MTK_HS20_CFLAGS) $(MTK_P2P_CFLAGS) $(MTK_TDLS_CFLAGS) $(EXTRA_MTK_CFLAGS)

### XML FLAGS
XML_FLAGS := -I/usr/include/libxml2 -L/usr/lib -lxml2 -lz -liconv -lm -L/bin -lxml2-2

# This is for WMM-PS
#for Ext TG
#CFLAGS = -g -O2 -D_REENTRANT -DWFA_WMM_PS_EXT -Wall -I../inc

## for WMM-AC, WMM-PS, Voice, PMF, TDLS Test-Bed
CFLAGS = -g -O2 -D_REENTRANT  -DWFA_WMM_PS_EXT -DWFA_WMM_AC -DWFA_VOICE_EXT -DWFA_STA_TB $(MTK_CFLAGS) -Wall -I../inc

## for WMM-AC, WMM-PS Test-Bed
#CFLAGS = -g -O2 -D_REENTRANT  -DWFA_WMM_PS_EXT -DWFA_WMM_AC -DWFA_STA_TB -Wall -I../inc

## for non-Test-Bed WMM-AC, WMM-PS and Voice
#CFLAGS = -g -O2 -D_REENTRANT  -DWFA_WMM_PS_EXT -DWFA_WMM_AC -DWFA_VOICE_EXT $(MTK_CFLAGS) -Wall -I../inc

## for PC-ENDPOINT
#CFLAGS = -g -O2 -D_REENTRANT  -DWFA_WMM_PS_EXT -DWFA_WMM_AC -DWFA_VOICE_EXT -DWFA_PC_CONSOLE -Wall -I../inc

## for PC-ENDPOINT No Voice
#CFLAGS = -g -O2 -D_REENTRANT  -DWFA_WMM_PS_EXT -DWFA_WMM_AC -DWFA_PC_CONSOLE -Wall -I../inc

# This is for WPA2 as default
#CFLAGS = -g -O2 -D_REENTRANT -Wall -I../inc

DUTLIBS = ../lib/libwfa_dut.a -lpthread
CALIBS = ../lib/libwfa_ca.a -lpthread
RANLIB = ranlib

LIBWFA_NAME_DUT = libwfa_dut.a
LIBWFA_NAME_CA = libwfa_ca.a
LIBWFA_NAME = libwfa.a

LIB_OBJS = wfa_sock.o wfa_tg.o wfa_cs.o wfa_ca_resp.o wfa_tlv.o wfa_typestr.o wfa_cmdtbl.o wfa_cmdproc.o wfa_miscs.o wfa_thr.o wfa_wmmps.o

LIB_OBJS_DUT = wfa_sock.o wfa_tlv.o wfa_cs.o wfa_cmdtbl.o wfa_tg.o wfa_miscs.o wfa_thr.o wfa_wmmps.o

LIB_OBJS_CA = wfa_sock.o wfa_tlv.o wfa_ca_resp.o wfa_cmdproc.o wfa_miscs.o wfa_typestr.o

CLEANFILES = core core.* *.core.* *.o *.a
