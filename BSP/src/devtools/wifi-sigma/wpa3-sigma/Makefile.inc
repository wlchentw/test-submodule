#
# Copyright (c) 2014 Wi-Fi Alliance
# 
# Permission to use, copy, modify, and/or distribute this software for any 
# purpose with or without fee is hereby granted, provided that the above 
# copyright notice and this permission notice appear in all copies.
# 
# THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES 
# WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF 
# MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY 
# SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER 
# RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
# NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE
# USE OR PERFORMANCE OF THIS SOFTWARE.
#

##### Change the following for your environment: ############ 
#************* For WFD Cert Sigma ***************
ifeq "$(TOOL_VERSION)" "WFD"
	CFLAGS = -g -O2 -D_REENTRANT  -DWFA_WMM_PS_EXT -DWFA_WMM_AC -DWFA_VOICE_EXT -DWFA_STA_TB -Wall -I../inc -DMTK_P2P_SIGMA -DMTK_WFD_SIGMA 
endif

#************* For P2P Cert Sigma ***************
ifeq "$(TOOL_VERSION)" "P2P"
	CFLAGS = -g -O2 -D_REENTRANT  -DWFA_WMM_PS_EXT -DWFA_WMM_AC -DWFA_VOICE_EXT -DWFA_STA_TB -Wall -I../inc -DMTK_P2P_SIGMA
endif

#************* For TGn/TGac/ Cert Sigma ***************
ifeq "$(TOOL_VERSION)" "STA"
	CFLAGS = -g -O2 -D_REENTRANT  -DWFA_WMM_PS_EXT -DWFA_WMM_AC -DWFA_VOICE_EXT -DWFA_STA_TB -Wall -I../inc
endif

#************* For PMF/TGn/TGac/ Cert Sigma ***************
#ifeq "$(TOOL_VERSION)" "STA_PMF"
	CFLAGS = -g -O2 -D_REENTRANT  -DWFA_WMM_PS_EXT -DWFA_WMM_AC -DWFA_VOICE_EXT -DWFA_STA_TB -Wall -I../inc -DPMF_SIGMA
#endif

#************* For TGn/TGac/ AP Cert Sigma ***************
ifeq "$(TOOL_VERSION)" "AP"
	CFLAGS = -g -O2 -D_REENTRANT  -DWFA_WMM_PS_EXT -DWFA_WMM_AC -DWFA_VOICE_EXT -DWFA_STA_TB -Wall -I../inc -DPMF_SIGMA -DMTK_11N_AP_SIGMA
endif

#************** For WMMPS Cert Sigma ****************
ifeq "$(TOOL_VERSION)" "WMMPS"
	CFLAGS = -g -O2 -D_REENTRANT  -DWFA_WMM_PS_EXT -DWFA_VOICE_EXT -DWFA_STA_TB -Wall -I../inc -DPMF_SIGMA
endif

############################################################


#***************** CFG80211 ARCHITETURE  ********************

CFLAGS += -DMTK_CFG80211_SIGMA
############################################################


##### Change the following for your environment: ############ 

#***************** PLATFORM ANDROID    ********************
ifeq "$(ANDROID_PLATFORM)" "Y"
	CFLAGS += -DMTK_ANDROID_SIGMA
endif

ifeq "$(ANDROID_AOSP)" "Y"
CFLAGS += -DANDROID_AOSP -DCTRL_IFACE_PATH=\"/data/misc/wifi/sockets\"
endif

############################################################

##### Change the following for your environment: ############ 
#***************** cortex-a9-rhel4 PATH  ********************
ifeq "$(TOOL_CHAIN)" "CORTEX_A9"
    #CROSS_COMPILER=/mtkoss/gnuarm/vfp_4.5.1_2.6.27_cortex-a9-rhel4/i686/bin/armv7a-mediatek451_001_vfp-linux-gnueabi-
    #CROSS_COMPILER=cortexa7-vfp-neon-poky-linux-
    #CROSS_COMPILER=/mtkoss/gnuarm/neon_4.8.2_2.6.35_cortex-a9-ubuntu/x86_64/bin/armv7a-mediatek482_001_neon-linux-gnueabi-
endif

ifeq "$(PLATFORM)" "BDP"
    CFLAGS += -DMTK_BDP_SIGMA
endif

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
#CFLAGS = -g -O2 -D_REENTRANT -Wall -I../inc


DUTLIBS = ../lib/libwfa_dut.a -lpthread
CALIBS = ../lib/libwfa_ca.a -lpthread
RANLIB = ranlib

LIBWFA_NAME_DUT = libwfa_dut.a
LIBWFA_NAME_CA = libwfa_ca.a
LIBWFA_NAME = libwfa.a

LIB_OBJS = wfa_sock.o wfa_tg.o wfa_cs.o wfa_ca_resp.o wfa_tlv.o wfa_typestr.o wfa_cmdtbl.o wfa_cmdproc.o wfa_miscs.o wfa_thr.o wfa_wmmps.o iniparser.o dictionary.o mtk_ini.o

LIB_OBJS_DUT = wfa_sock.o wfa_tlv.o wfa_cs.o wfa_cmdtbl.o wfa_tg.o wfa_miscs.o wfa_thr.o wfa_wmmps.o iniparser.o dictionary.o mtk_ini.o

LIB_OBJS_CA = wfa_sock.o wfa_tlv.o wfa_ca_resp.o wfa_cmdproc.o wfa_miscs.o wfa_typestr.o iniparser.o dictionary.o mtk_ini.o

CLEANFILES = core core.* *.core.* *.o *.a
