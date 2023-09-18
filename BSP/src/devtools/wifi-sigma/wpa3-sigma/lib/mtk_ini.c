#include "iniparser.h"
#include "wfa_portall.h"
#include "mtk_ini.h"

void mtk_parser_ini(void)
{
    dictionary *ini;
    char *ini_tmp = NULL;
    ini = iniparser_load("./mtk_ini.ini");
    extern char ctrl_if[];

    if (NULL == ini) {
        printf ("Cannot open the ini file, Please add mtk_ini.ini together with wfa_dut. \r\n");
        strcpy(WFA_STAUT_IF, "wlan0");
        strcpy(WFA_STAUT_IF_P2P, "p2p0");
#ifdef MTK_P2P_SIGMA
        strcpy(DHCPD_DEF_STATIC_IP_ADDR, DEFAULT_DHCPD_IP);
        strcpy(DHCPD_IP_POOL_S, DEFAULT_DHCPD_IP_POOL_S);
        strcpy(DHCPD_IP_POOL_E, DEFAULT_DHCPD_IP_POOL_E);
#endif
        return;

    }

#ifdef MTK_CFG80211_SIGMA
    /* WFA_STAUT_IF defined in wfa_portall.h */
    ini_tmp = iniparser_getstring(ini, "Common:WFA_STAUT_IF", "wlan0");
    strcpy(WFA_STAUT_IF, ini_tmp);
    printf ("Read WFA_STAUT_IF from ini file, WFA_STAUT_IF = %s.\r\n", WFA_STAUT_IF);


#ifdef MTK_11N_AP_SIGMA
    /* WFA_STAUT_IF_AP defined in wfa_portall.h */
    ini_tmp = iniparser_getstring(ini, "Common:WFA_STAUT_IF_AP", "ap0");
    strcpy(WFA_STAUT_IF_AP, ini_tmp);
    printf ("Read WFA_STAUT_IF_AP from ini file, WFA_STAUT_IF_AP = %s.\r\n", WFA_STAUT_IF_AP);
#endif

    /* WFA_STAUT_IF_P2P defined in wfa_portall.h */
    ini_tmp = iniparser_getstring(ini, "Common:WFA_STAUT_IF_P2P", "p2p0");
    strcpy(WFA_STAUT_IF_P2P, ini_tmp);
    printf ("Read WFA_STAUT_IF_P2P from ini file, WFA_STAUT_IF_P2P = %s.\r\n", WFA_STAUT_IF_P2P);
#endif
    ini_tmp = iniparser_getstring(ini, "Common:DHCPD_LEASE_FILE_PATH", DEFAULT_DHCPD_LEASE_FILE_PATH);
    strcpy(DHCPD_LEASE_FILE_PATH, ini_tmp);
    printf ("Read DHCPD_LEASE_FILE_PATH from ini file, DHCPD_LEASE_FILE_PATH = %s.\r\n", DHCPD_LEASE_FILE_PATH);

    ini_tmp = iniparser_getstring(ini, "Common:DHCPD_CONFIG_FILE_PATH", DEFAULT_DHCPD_CONFIG_FILE_PATH);
    strcpy(DHCPD_CONFIG_FILE_PATH, ini_tmp);
    printf ("Read DHCPD_CONFIG_FILE_PATH from ini file, DHCPD_CONFIG_FILE_PATH = %s.\r\n", DHCPD_CONFIG_FILE_PATH);

#ifdef MTK_P2P_SIGMA
    ini_tmp = iniparser_getstring(ini, "P2P:DHCPD_DEF_STATIC_IP_ADDR", DEFAULT_DHCPD_IP);
    strcpy(DHCPD_DEF_STATIC_IP_ADDR, ini_tmp);
    printf ("Read DHCPD_DEF_STATIC_IP_ADDR from ini file, DHCPD_DEF_STATIC_IP_ADDR = %s.\r\n", DHCPD_DEF_STATIC_IP_ADDR);

    ini_tmp = iniparser_getstring(ini, "P2P:DHCPD_IP_POOL_S", DEFAULT_DHCPD_IP_POOL_S);
    strcpy(DHCPD_IP_POOL_S, ini_tmp);
    printf ("Read DHCPD_IP_POOL_S from ini file, DHCPD_IP_POOL_S = %s.\r\n", DHCPD_IP_POOL_S);

    ini_tmp = iniparser_getstring(ini, "P2P:DHCPD_IP_POOL_E", DEFAULT_DHCPD_IP_POOL_E);
    strcpy(DHCPD_IP_POOL_E, ini_tmp);
    printf ("Read DHCPD_IP_POOL_E from ini file, DHCPD_IP_POOL_E = %s.\r\n", DHCPD_IP_POOL_E);
#endif

    ini_tmp = iniparser_getstring(ini, "Common:MTK_WPA_CLI_OUTPUT_BUFFER_PATH", DEFAULT_MTK_WPA_CLI_OUTPUT_BUFFER_PATH);
    strcpy(MTK_WPA_CLI_OUTPUT_BUFFER_PATH, ini_tmp);
    printf ("Read MTK_WPA_CLI_OUTPUT_BUFFER_PATH from ini file, MTK_WPA_CLI_OUTPUT_BUFFER_PATH = %s.\r\n", MTK_WPA_CLI_OUTPUT_BUFFER_PATH);

#ifdef ANDROID_AOSP
    ini_tmp = iniparser_getstring(ini, "ANDROID:WPA_SUPPLICANT_CTRL_IFACE_PATH", DEFAULT_WPA_SUPPLICANT_CTRL_IFACE_PATH);
    strcpy(WPA_SUPPLICANT_CTRL_IFACE_PATH, ini_tmp);
    printf ("Read WPA_SUPPLICANT_CTRL_IFACE_PATH from ini file, WPA_SUPPLICANT_CTRL_IFACE_PATH = %s.\r\n", WPA_SUPPLICANT_CTRL_IFACE_PATH);
    sprintf(ctrl_if, "-p %s", WPA_SUPPLICANT_CTRL_IFACE_PATH);
    printf ("ctrl_if = %s.\r\n", ctrl_if);
#else
    sprintf(ctrl_if, "-p/tmp/wpa_supplicant");
#endif

#ifdef MTK_11N_AP_SIGMA
    ini_tmp = iniparser_getstring(ini, "AP:WFA_AP_CONF_PATH", "./hostapd.conf");
    strcpy(WFA_AP_CONF_PATH, ini_tmp);
    printf ("Read WFA_AP_CONF_PATH from ini file, WFA_AP_CONF_PATH = %s.\r\n", WFA_AP_CONF_PATH);

    ini_tmp = iniparser_getstring(ini, "AP:WFA_AP_BIN_PATH", "hostapd");
    strcpy(WFA_AP_BIN_PATH, ini_tmp);
    printf ("Read WFA_AP_BIN_PATH from ini file, WFA_AP_BIN_PATH = %s.\r\n", WFA_AP_BIN_PATH);
#endif


    iniparser_freedict(ini);

}
