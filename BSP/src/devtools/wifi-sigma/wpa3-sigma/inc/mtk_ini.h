#ifndef _MTK_PARSER_INI_
#define _MTK_PARSER_INI_

#define DEFAULT_DHCPD_IP "192.168.5.1"
#define DEFAULT_DHCPD_IP_POOL_S "192.168.5.100"
#define DEFAULT_DHCPD_IP_POOL_E "192.168.5.200"

#define DEFAULT_DHCPD_LEASE_FILE_PATH "/tmp/udhcpd.leases"
#define DEFAULT_DHCPD_CONFIG_FILE_PATH "/data/misc/wifi/wts/udhcpd.conf"

#ifdef ANDROID_AOSP
#define DEFAULT_WPA_SUPPLICANT_CTRL_IFACE_PATH "/data/misc/wifi/sockets"
#endif

#define DEFAULT_MTK_WPA_CLI_OUTPUT_BUFFER_PATH "/tmp/tmp.txt"

void mtk_parser_ini(void);

#endif //_MTK_PARSER_INI_
