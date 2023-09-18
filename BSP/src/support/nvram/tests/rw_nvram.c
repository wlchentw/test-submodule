#include <stdio.h>
#include <string.h>

#include "NvRAMUtils.h"

int main(int argc, char* argv[])
{
    struct WIFI_CFG_STRUCT WIFI;
    struct BT_CFG_STRUCT BT;

    int w = 0;
    int b = 0;

    memset(&WIFI, 0, sizeof(struct WIFI_CFG_STRUCT));
    memset(&BT, 0, sizeof(struct BT_CFG_STRUCT));

	// read mac
    if(r_WiFi_CFG(&WIFI)) {
        fprintf(stderr,"WiFi MAC: %x:%x:%x:%x:%x:%x\n",
                WIFI.mac[0], WIFI.mac[1], WIFI.mac[2], WIFI.mac[3], WIFI.mac[4], WIFI.mac[5]);
        //modify mac
        //WIFI.mac[0] = 0x1;
        //WIFI.mac[1] = 0x1;
        //WIFI.mac[2] = 0x1;
        //WIFI.mac[3] = 0x1;
        //WIFI.mac[4] = 0x1;
        //WIFI.mac[5] = 0x1;
		// write & read mac
        if(w_WiFi_CFG(&WIFI) && r_WiFi_CFG(&WIFI)) {
            w = 1;
            fprintf(stderr,"WiFi MAC: %x:%x:%x:%x:%x:%x\n",
                    WIFI.mac[0], WIFI.mac[1], WIFI.mac[2], WIFI.mac[3], WIFI.mac[4], WIFI.mac[5]);
        }
    }

    if(r_BT_CFG(&BT)) {
        fprintf(stderr,"BT MAC: %x:%x:%x:%x:%x:%x\n",
                BT.mac[0], BT.mac[1], BT.mac[2], BT.mac[3], BT.mac[4], BT.mac[5]);

        //BT.mac[0] = 0x1;
        //BT.mac[1] = 0x1;
        //BT.mac[2] = 0x1;
        //BT.mac[3] = 0x1;
        //BT.mac[4] = 0x1;
        //BT.mac[5] = 0x1;

        if(w_BT_CFG(&BT) && r_BT_CFG(&BT)) {
            b = 1;
            fprintf(stderr,"BT MAC: %x:%x:%x:%x:%x:%x\n",
                    BT.mac[0], BT.mac[1], BT.mac[2], BT.mac[3], BT.mac[4], BT.mac[5]);
        }
    }

    if(b == 1 && w == 1) {
        b_CFG();
    }


	r_XOCAP();

	char SN[SN_LEN + 1];
	char PCBSN[PCBSN_LEN + 1];

	memset(SN, 0, SN_LEN + 1);
	memset(PCBSN, 0, PCBSN_LEN + 1);


	if(w_SN("12345678901234567890ABC") && r_SN(SN)) {
		printf("%s\n", SN);
	}

	if(w_PCBSN("123456789012345678") && r_PCBSN(PCBSN)) {
		printf("%s\n", PCBSN);
	}
    return 0;
}
