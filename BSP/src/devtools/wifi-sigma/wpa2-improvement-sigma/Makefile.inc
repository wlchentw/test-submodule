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

# This is for WMM-PS
#for Ext TG
#CFLAGS = -g -O2 -D_REENTRANT -DWFA_WMM_PS_EXT -Wall -I../inc

## for WMM-AC, WMM-PS, Voice, PMF, TDLS Test-Bed 
#CFLAGS = -g -O2 -D_REENTRANT  -DWFA_WMM_PS_EXT -DWFA_WMM_AC -DWFA_VOICE_EXT -DWFA_STA_TB -Wall -I../inc

## for WMM-AC, WMM-PS Test-Bed
#CFLAGS = -g -O2 -D_REENTRANT  -DWFA_WMM_PS_EXT -DWFA_WMM_AC -DWFA_STA_TB -Wall -I../inc

## for non-Test-Bed WMM-AC, WMM-PS and Voice
#CFLAGS = -g -O2 -D_REENTRANT  -DWFA_WMM_PS_EXT -DWFA_WMM_AC -DWFA_VOICE_EXT -Wall -I../inc

## for PC-ENDPOINT
#CFLAGS = -g -O2 -D_REENTRANT  -DWFA_WMM_PS_EXT -DWFA_WMM_AC -DWFA_VOICE_EXT -DWFA_PC_CONSOLE -Wall -I../inc

## for PC-ENDPOINT No Voice
#CFLAGS = -g -O2 -D_REENTRANT  -DWFA_WMM_PS_EXT -DWFA_WMM_AC -DWFA_PC_CONSOLE -DWFA_STA_TB -Wall -I../inc

# This is for WPA2 as default
CFLAGS = -g -O2 -D_REENTRANT -DWFA_WMM_PS_EXT -DWFA_WMM_AC -DWFA_VOICE_EXT -DWFA_STA_TB -Wall -I../inc

#### Add MTK related config #####
CFLAGS += -static -no-pie -DMTK_CFG80211_SIGMA -DMTK_11N_SIGMA -DMTK_11N_SIGMA_LAST -DMTK_SPECIFIC_PING_SIGMA

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
