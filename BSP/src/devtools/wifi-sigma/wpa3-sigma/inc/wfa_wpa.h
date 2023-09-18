
#ifndef _WFA_WPA_SUPPLICANT_
#define _WFA_WPA_SUPPLICANT_

#include <stdint.h>     // For uint8_t, uint32_t, etc.
#include <netinet/in.h> // For INET_ADDRSTRLEN
#include <sys/param.h>  // for max path length
#include <net/ethernet.h>
#include <sys/un.h> //unix socket
#include "wfa_types.h"

#define WPS_BUF_LENGTH              1024
#define WPS_CMD_LENGTH              1024 

// Copied from net/if.h
#ifndef IFNAMSIZ
#define IFNAMSIZ 16
#endif

#ifndef ETH_ALEN
#define ETH_ALEN 6
#endif

#ifndef MAC_FMT
#define MAC_FMT "%02x:%02x:%02x:%02x:%02x:%02x"
#endif

#ifndef MAC_ARG
#define MAC_ARG(x) ((uint8_t*)(x))[0],((uint8_t*)(x))[1],((uint8_t*)(x))[2],((uint8_t*)(x))[3],((uint8_t*)(x))[4],((uint8_t*)(x))[5]
#endif

#define WPA_CMD_TIMEOUT			    (-2)
#define WPA_CMD_UNKNOWN_ERROR		(-3)

#define WPA_CHECKSUM 0x21221324

#define WFD_DEV_INFO_BITMASK_DEV_TYPE       0x0003
#define WFD_DEV_INFO_BITMASK_SO_COUPLE      0x0004
#define WFD_DEV_INFO_BITMASK_SN_COUPLE      0x0008
#define WFD_DEV_INFO_BITMASK_SESS_AVAIL     0x0030
#define WFD_DEV_INFO_BITMASK_WSD            0x0040
#define WFD_DEV_INFO_BITMASK_PC             0x0080
#define WFD_DEV_INFO_BITMASK_CP             0x0100
#define WFD_DEV_INFO_BITMASK_TIME_SYNC      0x0200
#define WFD_DEV_INFO_BITMASK_NON_AUD        0x0400
#define WFD_DEV_INFO_BITMASK_AUD_ONLY_SO    0x0800
#define WFD_DEV_INFO_BITMASK_TDLS_PER       0x1000
#define WFD_DEV_INFO_BITMASK_TDLS_PER_REIVK 0x2000
#define WFD_DEV_INFO_BITMASK_RSVD           0xC000

#define WFD_DEV_INFO_DEV_TYPE_SOURE             0
#define WFD_DEV_INFO_DEV_TYPE_PRIMARY_SINK      1
#define WFD_DEV_INFO_DEV_TYPE_SECONDARY_SINK    2
#define WFD_DEV_INFO_DEV_TYPE_DUAL              3
#define WFD_DEV_INFO_BITMASK_SESS_AVAIL_BIT1    0x10
#define WFD_DEV_INFO_BITMASK_SESS_AVAIL_BIT2    0x20

typedef enum {
	UN_SET,
	PBC,
	DIPLAY_PIN,
	KEY_PAD
} config_method_t;

typedef enum {
	DEVICE,
	GO,
	GC
} nego_role_t;

typedef struct {
    	uint32_t                     checksum;
    	char                         if_name[ IFNAMSIZ ]; //YF: It should be for cmd I/F
	int                          notif_sock;
	struct sockaddr_un           notif_local;
	int                          cmd_sock;
	struct sockaddr_un           cmd_local;
	pthread_t                    notif_thread;
	int                          notif_thread_cancel_request;
	config_method_t confMethod;
	char keypad[9]; //store peer's pin in string format ex: 12345670
	int go_intent; // store DUT's go_intent from UCC
	int bNegoDone; //store Go Nego Done or Not! 1 ==> Done, 0 ==> not yet
	nego_role_t role; // store DUT's role
	char ssid[32+1];// MAX SSID leng is 32	
	char group_id[WFA_P2P_GRP_ID_LEN];// store group id like "AA:BB:CC:DD:EE:FF DIRECT-XX"
	char                         p2p_if_name[ IFNAMSIZ ];
	int                          p2p_cmd_sock;
	struct sockaddr_un           p2p_cmd_local;
    int freq;

#ifdef MTK_P2P_SIGMA
	int    presistent_oper;

	int isOnInviteSession;   /* receive the invite request from peer*/
	char inviteDevAddr[20];

	int isRecvInviteRsp;     /* wait invite rsp from peer */
	int recvInviteRspStatus;
	int concurrencyOn;
	int isInviteReinvoke;

	char                         wlan_if_name[ IFNAMSIZ ];
    int                          wlan_cmd_sock;
    struct sockaddr_un           wlan_cmd_local;	
#endif
} wpa_private_t;


int checksum_ok
    ( wpa_private_t* P ///< [in] pointer to private wpa_supplicant context
    );

wpa_private_t *wpa_init(char* if_name);

int wpa_shutdown
    ( wpa_private_t *P
    );
	
int wpa_shutdown
    ( wpa_private_t * P
    );

int wpa_p2p_cmd_connect
    ( wpa_private_t* P
    , const char* if_name
    );

int wpa_start_cmd
    (
    char *if_name    
    , int sock
    , char *cmd
    , char *buffer
    , size_t len
    , uint8_t disp
    );

int wpa_get_if_by_role
    ( wpa_private_t * P
    , uint8_t* macbuf
    );

int wpa_get_mac
    ( wpa_private_t * P
    , uint8_t* macbuf
    );

int wpa_channel_to_freq
    ( int channel
    );
#endif //_WFA_WPA_SUPPLICANT_
