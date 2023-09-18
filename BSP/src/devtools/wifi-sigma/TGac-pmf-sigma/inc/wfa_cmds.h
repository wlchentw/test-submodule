/****************************************************************************
 *  (c) Copyright 2007 Wi-Fi Alliance.  All Rights Reserved
 *
 *
 *  LICENSE
 *
 *  License is granted only to Wi-Fi Alliance members and designated
 *  contractors ($B!H(BAuthorized Licensees$B!I(B)..AN  Authorized Licensees are granted
 *  the non-exclusive, worldwide, limited right to use, copy, import, export
 *  and distribute this software:
 *  (i) solely for noncommercial applications and solely for testing Wi-Fi
 *  equipment; and
 *  (ii) solely for the purpose of embedding the software into Authorized
 *  Licensee$B!G(Bs proprietary equipment and software products for distribution to
 *  its customers under a license with at least the same restrictions as
 *  contained in this License, including, without limitation, the disclaimer of
 *  warranty and limitation of liability, below..AN  The distribution rights
 *  granted in clause
 *  (ii), above, include distribution to third party companies who will
 *  redistribute the Authorized Licensee$B!G(Bs product to their customers with or
 *  without such third party$B!G(Bs private label. Other than expressly granted
 *  herein, this License is not transferable or sublicensable, and it does not
 *  extend to and may not be used with non-Wi-Fi applications..AN  Wi-Fi Alliance
 *  reserves all rights not expressly granted herein..AN
 *.AN
 *  Except as specifically set forth above, commercial derivative works of
 *  this software or applications that use the Wi-Fi scripts generated by this
 *  software are NOT AUTHORIZED without specific prior written permission from
 *  Wi-Fi Alliance.
 *.AN
 *  Non-Commercial derivative works of this software for internal use are
 *  authorized and are limited by the same restrictions; provided, however,
 *  that the Authorized Licensee shall provide Wi-Fi Alliance with a copy of
 *  such derivative works under a perpetual, payment-free license to use,
 *  modify, and distribute such derivative works for purposes of testing Wi-Fi
 *  equipment.
 *.AN
 *  Neither the name of the author nor "Wi-Fi Alliance" may be used to endorse
 *  or promote products that are derived from or that use this software without
 *  specific prior written permission from Wi-Fi Alliance.
 *
 *  THIS SOFTWARE IS PROVIDED BY WI-FI ALLIANCE "AS IS" AND ANY EXPRESS OR
 *  IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 *  OF MERCHANTABILITY, NON-INFRINGEMENT AND FITNESS FOR A.AN PARTICULAR PURPOSE,
 *  ARE DISCLAIMED. IN NO EVENT SHALL WI-FI ALLIANCE BE LIABLE FOR ANY DIRECT,
 *  INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 *  (INCLUDING, BUT NOT LIMITED TO, THE COST OF PROCUREMENT OF SUBSTITUTE
 *  GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 *  HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 *  STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE) ARISING IN ANY WAY OUT OF
 *  THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE
 ******************************************************************************
 *     Revision History:
 *     02/11/2008   Change encrytype from 8 bytes to 9, BUG #5.
 */


/*
 * wfa_cmds.h:
 *   definitions of command types.
 */
#ifndef _WFA_CMDS_H
#define _WFA_CMDS_H

typedef struct _tg_ping_start
{
    char dipaddr[WFA_IP_V6_ADDR_STR_LEN];  /* destination/remote ip address */
    int  frameSize;
    int  frameRate;
    int  duration;
    int  type;
	int  qos;
#ifdef MTK_HS20_SIGMA
	int	 dscp;
#endif
} tgPingStart_t;

typedef struct ca_sta_set_ip_config
{
    char intf[WFA_IF_NAME_LEN];
    int isDhcp;
	int type; // 1 for Ipv4, 2 for Ipv6
    char ipaddr[WFA_IP_ADDR_STR_LEN];
    char mask[WFA_IP_MASK_STR_LEN];
    char defGateway[WFA_IP_ADDR_STR_LEN];
    char pri_dns[WFA_IP_ADDR_STR_LEN];
    char sec_dns[WFA_IP_ADDR_STR_LEN];
} caStaSetIpConfig_t;

typedef struct ca_sta_verify_ip_connection
{
   char dipaddr[WFA_IP_ADDR_STR_LEN];
   int timeout;
} caStaVerifyIpConnect_t;

typedef struct ca_sta_set_encryption
{
   char intf[WFA_IF_NAME_LEN];
   char ssid[WFA_SSID_NAME_LEN];
   int encpType;
   char keys[4][32];  /* 26 hex */
   int activeKeyIdx;
} caStaSetEncryption_t;

typedef enum wfa_enableType
{
   eEnable = 1,
   eDisable
} wfaEnableType;

typedef struct ca_sta_set_mode
{
   char intf[WFA_IF_NAME_LEN];
   char ssid[WFA_SSID_NAME_LEN];
   char mode;
   int encpType;
   int channel;
   char keys[4][32];  /* 26 hex */
   int activeKeyIdx;
} caStaSetMode_t;

typedef struct ca_sta_set_psk
{
   char intf[WFA_IF_NAME_LEN];
   char ssid[WFA_SSID_NAME_LEN];
   BYTE passphrase[64];
   char keyMgmtType[WFA_KEYMGMTTYPE_LEN];  /* WPA-PSK */
   int encpType;    /* TKIP    */
   int pmf;               /* PMF enable or disable */
#if defined(MTK_HS20_SIGMA)
	int prefer;
	char prog[16];
#endif
} caStaSetPSK_t;

typedef struct ca_sta_set_eaptls
{
   char intf[WFA_IF_NAME_LEN];
   char ssid[WFA_SSID_NAME_LEN];
   char keyMgmtType[WFA_KEYMGMTTYPE_LEN];
   char encrptype[WFA_ENCRYPTYPE_LEN];
   char trustedRootCA[128];
   char clientCertificate[128];
   int pmf;               /* PMF enable or disable */
#if defined(MTK_HS20_SIGMA)
	int prefer;
	char prog[16];
#endif   
} caStaSetEapTLS_t;

typedef struct ca_sta_set_eapttls
{
   char intf[WFA_IF_NAME_LEN];
   char ssid[WFA_SSID_NAME_LEN];
   char username[32];
   char passwd[16];
   char keyMgmtType[WFA_KEYMGMTTYPE_LEN];
   char encrptype[WFA_ENCRYPTYPE_LEN];
   char trustedRootCA[32];
   char clientCertificate[32];
   int pmf;               /* PMF enable or disable */
#if defined(MTK_HS20_SIGMA)
	int prefer;
	char prog[16];
#endif   
} caStaSetEapTTLS_t;

typedef struct ca_sta_set_eapsim
{
   char intf[WFA_IF_NAME_LEN];
   char ssid[WFA_SSID_NAME_LEN];
   char username[32];
   char passwd[96];
   char keyMgmtType[WFA_KEYMGMTTYPE_LEN];
   char encrptype[WFA_ENCRYPTYPE_LEN];
   char	tripletCount;
   char tripletSet[3][64];
   int pmf;               /* PMF enable or disable */
} caStaSetEapSIM_t;

typedef struct ca_sta_set_eappeap
{
   char intf[WFA_IF_NAME_LEN];
   char ssid[WFA_SSID_NAME_LEN];
   char username[32];
   char passwd[16];
   char keyMgmtType[WFA_KEYMGMTTYPE_LEN];
   char encrptype[WFA_ENCRYPTYPE_LEN];
   char trustedRootCA[32];
   char innerEAP[16];
   int peapVersion;
   int pmf;               /* PMF enable or disable */
} caStaSetEapPEAP_t;

typedef struct ca_sta_set_eapfast
{
   char intf[WFA_IF_NAME_LEN];
   char ssid[WFA_SSID_NAME_LEN];
   char username[32];
   char passwd[16];
   char keyMgmtType[WFA_KEYMGMTTYPE_LEN];
   char encrptype[WFA_ENCRYPTYPE_LEN];
   char trustedRootCA[32];
   char innerEAP[16];
   char	validateServer;
   char pacFileName[32];
   int pmf;               /* PMF enable or disable */
} caStaSetEapFAST_t;

typedef struct ca_sta_set_eapaka
{
   char intf[WFA_IF_NAME_LEN];
   char ssid[WFA_SSID_NAME_LEN];
   char username[32];
   char passwd[96];
   char keyMgmtType[WFA_KEYMGMTTYPE_LEN];
   char encrptype[WFA_ENCRYPTYPE_LEN];
   char	tripletCount;
   char tripletSet[3][96];
   int pmf;               /* PMF enable or disable */
} caStaSetEapAKA_t;

enum sectype {
   SEC_TYPE_PSK = 1,
   SEC_TYPE_EAPTLS,
   SEC_TYPE_EAPTTLS,
   SEC_TYPE_EAPPEAP,
   SEC_TYPE_EAPSIM,
   SEC_TYPE_EAPFAST,
   SEC_TYPE_EAPAKA,
};

typedef struct ca_sta_set_security
{
   BYTE type; /* PSK, EAPx */
   char ssid[WFA_SSID_NAME_LEN];
   char keyMgmtType[WFA_KEYMGMTTYPE_LEN];
   char encrptype[WFA_ENCRYPTYPE_LEN];
   int  pmf;
   union _security
   {
      char                 passphrase[64];       /* PSK */
      caStaSetEapTLS_t     tls;
      caStaSetEapTTLS_t    ttls;
      caStaSetEapSIM_t     sim;
      caStaSetEapPEAP_t    peap;
      caStaSetEapAKA_t     aka;
      caStaSetEapFAST_t    fast;
   } secu;
} caStaSetSecurity_t;

typedef struct ca_sta_set_systime
{
   BYTE month;
   BYTE date;
   WORD year;
   BYTE hours;
   BYTE minutes;
   BYTE seconds;
} caStaSetSystime_t;

enum {
   PMF_TYPE_DISASSOC = 1,
   PMF_TYPE_DEAUTH,
   PMF_TYPE_SAQUERY,
   PMF_TYPE_AUTH,
   PMF_TYPE_ASSOCREQ,
   PMF_TYPE_REASSOCREQ,
};

enum {
   PMF_PROT_CORRECTKEY = 1,
   PMF_PROT_INCORRECTKEY,
   PMF_PROT_UNPROTECTED,
};

typedef struct pmf_frame
{
   BYTE type;
   BYTE protected;
   char sender[8]; /* ap or sta */
   char bssid[18]; /* ap mac addr */
   char staid[18]; /* sta mac addr */
} pmfFrame_t;

enum {
   TDLS_TYPE_DISCOVERY = 1,
   TDLS_TYPE_SETUP,
   TDLS_TYPE_TEARDOWN,
   TDLS_TYPE_CHANNELSWITCH,
   TDLS_TYPE_NULLFRAME,
};

typedef struct tdls_frame
{
   BYTE type;
   char peer[18];
   int timeout;
   int switchtime;
   int channel;
   char offset[4]; /* 20 or 40 Mhz */
   int status;     /* status code */
   int reason;     /* reason code */
   char bssid[18];
} tdlsFrame_t;

enum {
   VENT_TYPE_NEIGREQ = 1,
   VENT_TYPE_TRANSMGMT,
};

typedef struct vent_frame
{
   BYTE type;
   char ssid[WFA_SSID_NAME_LEN];
} ventFrame_t;

enum {
    FM_TYPE_PMF = 1,
    FM_TYPE_TDLS,
    FM_TYPE_VENT,
};

typedef struct ca_sta_sendframe
{
   BYTE frame;
   union _frametype
   {
       pmfFrame_t pmf;
       tdlsFrame_t tdls;
       ventFrame_t vent;
   } frameType;
} caStaSendFrame_t;

#ifdef WFA_STA_TB
typedef enum wfa_supplicant_names
{
   eWindowsZeroConfig = 1,
   eMarvell,
   eIntelProset,
   eWpaSupplicant,
   eCiscoSecureClient,
   eOpen1x,
   eDefault
} wfaSupplicantName;


typedef enum wfa_preambleType
{
   eLong = 1,
   eShort
} wfaPreambleType;

typedef enum wfa_WirelessMode
{
   eModeB = 1,
   eModeBG,
   eModeA,
   eModeABG,
   eModeAN,
   eModeGN,
   eModeNL,
} wfaWirelessMode;

typedef enum wfa_reset_prog
{
   eResetProg11n =1,
} wfaResetProg;

typedef enum wfa_tdlsMode
{
   eDef = 0,
   eHiLoMac = 1,
   eExistLink,
   eAPProhibit,
   eWeakSec,
   eIgnChnlSWProh,  /* if it is present, ignore channel switch prohibit */
} wfaTDLSMode;

#if defined MTK_HS20_SIGMA // R2
typedef enum wfa_hs20r2ProvProt
{
	eOMADM = 1,
	eSOAP  = 2,
} wfaHs20r2ProvProt;
#endif


typedef struct ca_sta_preset_parameters
{
   char intf[WFA_IF_NAME_LEN];
#if defined MTK_HS20_SIGMA // R2
   char prog[WFA_HS20_PROGRAM_LEN];
   unsigned int ppsmo;
   char filetype[WFA_HS20_R2_FILETYPE_LEN];
   char filename[WFA_HS20_R2_FILENAME_LEN];
   char filepath[WFA_HS20_R2_FILEPATH_LEN];
   wfaHs20r2ProvProt provprot;
   unsigned int qosmap;
   char certname[WFA_HS20_R2_FILENAME_LEN];
   char certfp[64];
#endif
   wfaSupplicantName supplicant;
   BYTE rtsFlag;
   WORD rtsThreshold;
   BYTE fragFlag;
   WORD fragThreshold;
   BYTE preambleFlag;
   wfaPreambleType preamble;
   BYTE modeFlag;
   wfaWirelessMode wirelessMode;
   BYTE psFlag;
   BYTE legacyPowerSave;
   BYTE wmmFlag;
   BYTE wmmState;
   BYTE reset;
   BYTE ht;    // temperary for high throughput
   BYTE ftoa;
   BYTE ftds;
   BYTE activescan;
//   BYTE ignChSwitchProh;
   BYTE tdls;
   BYTE tdlsMode;
} caStaPresetParameters_t;

typedef struct ca_sta_set_11n
{
   char intf[WFA_IF_NAME_LEN];
   BOOL _40_intolerant;
   BOOL addba_reject;
   BOOL ampdu;
   BOOL amsdu;
   BOOL greenfield;
   BOOL sgi20;
   unsigned short stbc_rx;
   unsigned short smps;
   char width[8];
   char mcs_fixedrate[4];
   BOOL mcs32;
   BOOL rifs_test;
   unsigned char txsp_stream;
   unsigned char rxsp_stream;
} caSta11n_t;


#define WFA_ENABLED          1
#define WFA_OPTIONAL         1
#define WFA_DISABLED         0
#define WFA_REQUIRED         2
#define WFA_F_REQUIRED       3            /* forced required */
#define WFA_F_DISABLED       4            /* forced disabled */
#define WFA_INVALID_BOOL 0xFF

typedef struct ca_sta_set_wireless
{
   char intf[WFA_IF_NAME_LEN];
   char band [8];
   BOOL rts_force;
   BOOL bw_sgnl;
   BOOL dyn_bw_sgnl;
#define NOACK_BE       0
#define NOACK_BK       1
#define NOACK_VI       2
#define NOACK_VO       3
   unsigned char noAck[4];
} caStaSetWireless_t;

typedef struct ca_sta_send_addba
{
   char intf[WFA_IF_NAME_LEN];
   unsigned short tid;
} caStaSetSendADDBA_t;

typedef struct ca_sta_set_rifs
{
   char intf [WFA_IF_NAME_LEN];
   unsigned int action;

} caStaSetRIFS_t;

typedef struct ca_sta_send_coexist_mgmt
{
   char intf[WFA_IF_NAME_LEN];
   char type[16];
   char value[16];
} caStaSendCoExistMGMT_t;
#endif


#ifdef MTK_P2P_SIGMA
/* P2P */
/*
typedef struct ca_sta_get_p2p_dev_address
{
   char intf[WFA_IF_NAME_LEN];
} caStaGetP2pDevAddress_t;
*/
typedef struct ca_sta_set_p2p
{
   char intf[WFA_IF_NAME_LEN];

   unsigned char listen_chn_flag;
   WORD listen_chn;

   unsigned char p2p_mode_flag;
   char p2p_mode[16];

   unsigned char presistent_flag;
   int presistent;

   unsigned char intra_bss_flag;
   int intra_bss;

   unsigned char noa_duration_flag;
   int noa_duration;

   unsigned char noa_interval_flag;
   int noa_interval;

   unsigned char noa_count_flag;
   int noa_count;

	unsigned char concurrency_flag;
	int concurrency;

	unsigned char p2p_invitation_flag;
	int p2p_invitation;


	unsigned char bcn_int_flag;
	int bcn_int;

	unsigned char ext_listen_time_int_flag;
	int ext_listen_time_int;

	unsigned char ext_listen_time_period_flag;
	int ext_listen_time_period;

	unsigned char discoverability_flag;
	int discoverability;


	unsigned char service_discovry_flag;
	int service_discovery;

	unsigned char crossconnection_flag;
	int crossconnection;

	unsigned char p2pmanaged_flag;
	int p2pmanaged;

	unsigned char go_apsd_flag;
	int go_apsd;


} caStaSetP2p_t;

typedef struct ca_sta_p2p_connect
{
   char intf[WFA_IF_NAME_LEN];
   char grpid[WFA_P2P_GRP_ID_LEN];
   char devId[WFA_P2P_DEVID_LEN];
} caStaP2pConnect_t;

typedef struct ca_sta_start_auto_go
{
   char intf[WFA_IF_NAME_LEN];
   WORD oper_chn;
   unsigned char ssid_flag;
   char ssid[WFA_SSID_NAME_LEN];
} caStaStartAutoGo_t;


typedef struct ca_sta_p2p_start_grp_formation
{
   char intf[WFA_IF_NAME_LEN];
   char devId[WFA_P2P_DEVID_LEN];
   WORD intent_val;
   WORD init_go_neg;
   unsigned char oper_chn_flag;
   WORD oper_chn;
   unsigned char ssid_flag;
   char ssid[WFA_SSID_NAME_LEN];
} caStaP2pStartGrpForm_t;

typedef struct ca_sta_p2p_dissolve
{
   char intf[WFA_IF_NAME_LEN];
   char grpId[WFA_P2P_GRP_ID_LEN];
} caStaP2pDissolve_t;

typedef struct ca_sta_send_p2p_inv_req
{
   char intf[WFA_IF_NAME_LEN];
   char devId[WFA_P2P_DEVID_LEN];
   char grpId[WFA_P2P_GRP_ID_LEN];
   int reinvoke;
} caStaSendP2pInvReq_t;

typedef struct ca_sta_accept_p2p_inv_req
{
   char intf[WFA_IF_NAME_LEN];
   char devId[WFA_P2P_DEVID_LEN];
   char grpId[WFA_P2P_GRP_ID_LEN];
   int reinvoke;
} caStaAcceptP2pInvReq_t;


typedef struct ca_sta_send_p2p_prov_dis_req
{
   char intf[WFA_IF_NAME_LEN];
   char confMethod[16];
   char devId[WFA_P2P_DEVID_LEN];
} caStaSendP2pProvDisReq_t;

typedef struct ca_sta_set_wps_pbc
{
   char intf[WFA_IF_NAME_LEN];
   unsigned char grpid_flag;
   char grpId[WFA_P2P_GRP_ID_LEN];
} caStaSetWpsPbc_t;

typedef struct ca_sta_wps_read_pin
{
   char intf[WFA_IF_NAME_LEN];
   unsigned char grpid_flag;
   char grpId[WFA_P2P_GRP_ID_LEN];
} caStaWpsReadPin_t;

typedef struct ca_sta_wps_read_label
{
   char intf[WFA_IF_NAME_LEN];
   unsigned char grpid_flag;
   char grpId[WFA_P2P_GRP_ID_LEN];
} caStaWpsReadLabel_t;

typedef struct ca_sta_wps_enter_pin
{
   char intf[WFA_IF_NAME_LEN];
   char wpsPin[WFA_WPS_PIN_LEN];
   unsigned char grpid_flag;
   char grpId[WFA_P2P_GRP_ID_LEN];
} caStaWpsEnterPin_t;

typedef struct ca_sta_get_psk
{
   char intf[WFA_IF_NAME_LEN];
   char grpId[WFA_P2P_GRP_ID_LEN];
} caStaGetPsk_t;

typedef struct ca_sta_get_p2p_ip_config
{
   char intf[WFA_IF_NAME_LEN];
   char grpId[WFA_P2P_GRP_ID_LEN];
} caStaGetP2pIpConfig_t;

typedef struct ca_sta_send_service_discovery_req
{
   char intf[WFA_IF_NAME_LEN];
   char devId[WFA_P2P_DEVID_LEN];
} caStaSendServiceDiscoveryReq_t;

typedef struct ca_sta_send_p2p_presence_req
{
   char intf[WFA_IF_NAME_LEN];
   long long int duration;
   long long int interval;
} caStaSendP2pPresenceReq_t;

typedef struct ca_sta_add_arp_table_entry
{
   char intf[WFA_IF_NAME_LEN];
   char macaddress [WFA_MAC_ADDR_STR_LEN];
   char ipaddress [WFA_MAC_ADDR_STR_LEN];
} caStaAddARPTableEntry_t;

typedef struct ca_sta_block_icmp_reponse
{
   char intf[WFA_IF_NAME_LEN];
   char ipaddress [WFA_MAC_ADDR_STR_LEN];
   char grpId[WFA_P2P_GRP_ID_LEN];
} caStaBlockICMPResponse_t;


typedef struct ca_sta_set_sleep
{
   char intf[WFA_IF_NAME_LEN];
   char grpId[WFA_P2P_GRP_ID_LEN];
} caStaSetSleep_t;

typedef struct ca_sta_set_opportunistic_ps
{
   char intf[WFA_IF_NAME_LEN];
   int ctwindow;
   char grpId[WFA_P2P_GRP_ID_LEN];
} caStaSetOpprPs_t;

/* P2P */

#endif  /* MTK_P2P_SIGMA */

#ifdef MTK_HS20_SIGMA

typedef struct ca_sta_reset_parm
{
	char intf[WFA_IF_NAME_LEN];
	char hs2_cache_profile[32];
	char arp[WFA_IP_ADDR_STR_LEN];
} caStaResetParm_t;

typedef struct ca_sta_get_key
{
	char intf[WFA_IF_NAME_LEN];
	char keyType[WFA_KEY_TYPE_LEN];
} caStaGetKey_t;

#define BSSID_LIST_MAX_LEN		12

typedef struct ca_sta_bssid_pool
{
	char intf[WFA_IF_NAME_LEN];
	char bssid_filter;
	char bssid_list[BSSID_LIST_MAX_LEN][18];		// Support 12 sets
} caStaBssidPool_t;

typedef struct ca_dev_send_frame
{
	char prog[WFA_HS20_PROGRAM_LEN];
	char intf[WFA_IF_NAME_LEN];
	char dest[WFA_MAC_ADDR_STR_LEN];
	char type[WFA_HS20_TYPE_LEN];
	char framename[WFA_HS20_FRAMENAME_LEN];
	char SenderMac[WFA_MAC_ADDR_STR_LEN];
	int  DestIP[WFA_IP_ADDR_STR_LEN];
	int  fg_ANQP_QUERY_LIST;
	int  fg_ANQP_CAPABILITY_LIST;
	int  fg_ANQP_VENUE_NAME_INFORMATION;
	int  fg_ANQP_EMERGENCY_CALL_NUMBER_INFORMATION;
	int  fg_ANQP_NETWORK_AUTHENTICATION_TYPE_INFORMATION;
	int  fg_ANQP_ROAMING_CONSORTIUM_LIST;
	int  fg_ANQP_IP_ADDRESS_TYPE_AVAILABILITY_INFORMATION;
	int  fg_ANQP_NAI_REALM_LIST;
	int  fg_ANQP_3GPP_CELLULAR_NETWORK_INFORMATION;
	int  fg_ANQP_AP_GEOSPATIAL_LOCATION;
	int  fg_ANQP_AP_CIVIC_LOCATION;
	int  fg_ANQP_AP_LOCATION_PUBLIC_IDENTIFIER_URI;
	int  fg_ANQP_DOMAIN_NAME_LIST;
	int  fg_ANQP_EMERGENCY_ALERT_IDENTIFIER_URI;
	int  fg_ANQP_EMERGENCY_NAI;
	int  fg_ANQP_VENDOR_SPECIFIC_LIST;
	int  fg_HS20_QUERY_LIST;
	int  fg_HS20_CAPABILITY_LIST;
	int  fg_HS20_OPERATOR_FRIENDLY_NAME;
	int  fg_HS20_WAN_METRICS;
	int  fg_HS20_CONNECTION_CAPABILITY;
	int  fg_HS20_NAI_HOME_REALM_QUERY;
	int  fg_HS20_OP_CLASS;
	int  fg_HS20_OSU_PROVIDER_LIST;
	int  fg_HS20_ICON_REQ;
	char hs20_icon_req[WFA_HS20_ICON_LEN];
} caDevSendFrame_t;

typedef struct ca_sta_add_credential
{
	char intf[WFA_IF_NAME_LEN];
	char type[WFA_HS20_TYPE_LEN];
	char username[WFA_HS20_LEN_32];
	char password[WFA_HS20_LEN_256];
	char imsi[WFA_HS20_LEN_32];
	char plmn_mnc[WFA_HS20_LEN_32];
	char plmn_mcc[WFA_HS20_LEN_32];
	char root_ca[WFA_HS20_LEN_32];
	char realm[WFA_HS20_LEN_32];
	int  prefer;
	char fqdn[WFA_HS20_LEN_32];
	char clientCA[WFA_HS20_LEN_32];
} caStaAddCredential_t;

typedef struct ca_Sta_Scan
{
	char intf[WFA_IF_NAME_LEN];
	char hessid[WFA_MAC_ADDR_STR_LEN];
	char accs_net_type[3];
} caStaScan_t;


typedef struct ca_Dev_Set_Parameter
{
	char intf[WFA_IF_NAME_LEN];
	int  clearARP;
} caDevSetParameter_t;


typedef struct ca_Sta_OSU
{
	char intf[WFA_IF_NAME_LEN];
	char fname[WFA_HS20_LEN_64];
} caStaOSU_t;


typedef struct ca_Sta_Policy_Update
{
	char intf[WFA_IF_NAME_LEN];
	int  PolicyUpdate;
	int  Timeout;
} caStaPolicyUpdate_t;



#endif /* MTK_HS20_SIGMA */


typedef struct ca_sta_set_uapsd
{
   char intf[WFA_IF_NAME_LEN];
   char ssid[WFA_SSID_NAME_LEN];
   int maxSPLength;
   BYTE acBE;
   BYTE acBK;
   BYTE acVI;
   BYTE acVO;
   int  type;
   char peer[18];
} caStaSetUAPSD_t;

typedef struct ca_sta_set_ibss
{
   char intf[WFA_IF_NAME_LEN];
   char ssid[WFA_SSID_NAME_LEN];
   int channel;
   int encpType;
   char keys[4][32];
   int activeKeyIdx;
} caStaSetIBSS_t;

typedef struct sta_upload
{
   int type;
   int next;     /* sequence number, 0 is the last one */
} caStaUpload_t;

typedef struct sta_debug_set
{
   unsigned short level;
   unsigned short state;
} staDebugSet_t;
typedef struct config
{
   BYTE wmm;
   int  rts_thr ;
   int  frag_thr ;
} wmmconf_t;

typedef struct wmm_tsinfo
{
   unsigned int Reserved1 :1;
   unsigned int TID       :4;
   unsigned int direction :2;
   unsigned int dummy1    :1;
   unsigned int dummy2    :1;
   unsigned int Reserved2 :1;
   unsigned int PSB       :1;
   unsigned int UP        :3;
   unsigned int infoAck   :2;
   unsigned int Reserved4 :1;
   unsigned int Reserved5 :6;
   unsigned int bstSzDef :1;
}wmmtsinfo_t;

typedef struct wmm_tspec
{
   wmmtsinfo_t      tsinfo;
   BOOL Fixed;//The MSDU Fixed Bit
   unsigned short size;//The MSDU Size
   unsigned short maxsize;//Maximum MSDU Size
   unsigned int   min_srvc;//The minimum Service Interval
   unsigned int   max_srvc;//The maximum Service Interval
   unsigned int inactivity;//Inactivity Interval
   unsigned int suspension;//The Suspension Interval
   unsigned int srvc_strt_tim;//The Service Start Time
   unsigned int mindatarate;//The Minimum Data Rate
   unsigned int meandatarate;//The Minimum Data Rate
   unsigned int peakdatarate;//The Minimum Data Rate
   unsigned int burstsize;//The Maximum Burst Size
   unsigned int delaybound;//The Delay Bound
   unsigned int PHYrate;//The minimum PHY Rate
   float sba;//The Surplus Bandwidth Allownce
   unsigned short medium_time;//The medium time
}wmmtspec_t;

typedef struct wmmac_addts
{
   BYTE       dialog_token;
   BYTE       accesscat;
   wmmtspec_t tspec;
} wmmacadd_t;

typedef struct ca_sta_set_wmm
{
   char intf[WFA_IF_NAME_LEN];
   BYTE group;
   BYTE action;
#ifdef WFA_WMM_AC
   BYTE       send_trig;
   char       dipaddr[WFA_IP_ADDR_STR_LEN];
   BYTE       trig_ac;
#endif

   union _action
   {
       wmmconf_t   config;
       wmmacadd_t  addts;
       BYTE        delts;
   } actions;
} caStaSetWMM_t;

typedef struct ca_sta_set_pwrsave
{
   char intf[WFA_IF_NAME_LEN];
#ifdef MTK_P2P_SIGMA
   char mode[10];
#else
   char mode[8];
#endif
} caStaSetPwrSave_t;

typedef struct ca_sta_reset_default
{
   char intf[WFA_IF_NAME_LEN];
   char prog[8];
} caStaResetDefault_t;

typedef struct ca_dev_info
{
   BYTE fw;
} caDevInfo_t;

typedef struct ca_sta_associate
{
   char ssid[WFA_SSID_NAME_LEN];
   char bssid[18];
} caStaAssociate_t;

typedef enum wfa_onoffType
{
   WFA_OFF = 0,
   WFA_ON = 1,
} wfaOnOffType;

typedef struct ca_sta_set_radio
{
   wfaOnOffType mode;
} caStaSetRadio_t;

typedef struct ca_sta_rfeature
{
   char prog[8];
   wfaEnableType uapsd;
   char peer[18]; /* peer mac addr */
   wfaEnableType tpktimer;
#if 0 //defined MTK_HS20_SIGMA // R2   
   unsigned int policyupdate;
   char username[128];
   char password[128];
#endif

   wfaEnableType chSwitch;
   short offChNum;
   short secChOffset;

} caStaRFeat_t;


typedef struct dut_commands
{
   char intf[WFA_IF_NAME_LEN];
   union _cmds
   {
       int streamId;
       int iftype;
       tgProfile_t profile;
       tgPingStart_t startPing;
       char resetProg[16];
       char macaddr[18];
       caStaAssociate_t assoc;
       caStaSetIpConfig_t ipconfig;
       caStaVerifyIpConnect_t verifyIp;
       caStaSetEncryption_t wep;
       caStaSetPSK_t        psk;
       caStaSetEapTLS_t     tls;
       caStaSetEapTTLS_t    ttls;
       caStaSetEapSIM_t     sim;
       caStaSetEapPEAP_t    peap;
       caStaSetEapAKA_t     aka;
       caStaSetEapFAST_t    fast;
       caStaSetSecurity_t   setsec;
       caStaSetUAPSD_t      uapsd;
       caStaSetIBSS_t       ibss;
       caStaUpload_t        upload;
       caStaSetWMM_t        setwmm;
       staDebugSet_t        dbg;
       caDevInfo_t          dev;
       caStaSendFrame_t     sf;
       caStaSetRadio_t      sr;
       caStaRFeat_t         rfeat;
       int type;
       caStaSetWireless_t	setWireless;
   } cmdsu;
} dutCommand_t;


extern int buildCommandProcessTable(void);

#endif