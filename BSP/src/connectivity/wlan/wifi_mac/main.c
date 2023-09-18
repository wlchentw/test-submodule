
/******************************************************************************
*                         C O M P I L E R   F L A G S
*******************************************************************************
*/

/******************************************************************************
*                    E X T E R N A L   R E F E R E N C E S
*******************************************************************************
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/sendfile.h>
#include <fcntl.h>
#include <linux/types.h>
#include <limits.h>
#include <getopt.h>
#include <errno.h>
#include <time.h>

/******************************************************************************
*                              C O N S T A N T S
*******************************************************************************
*/
/* !defined(ANDROID) */


/******************************************************************************
*                             D A T A   T Y P E S
*******************************************************************************
*/

/******************************************************************************
*                                 M A C R O S
*******************************************************************************
*/

/******************************************************************************
*                   F U N C T I O N   D E C L A R A T I O N S
*******************************************************************************
*/

/******************************************************************************
*                            P U B L I C   D A T A
*******************************************************************************
*/

/******************************************************************************
*                           P R I V A T E   D A T A
*******************************************************************************
*/
char EEPROM_PATH[64] = "/data/misc/connectivity/EEPROM_MT7668.bin";

/******************************************************************************
*                              F U N C T I O N S
*******************************************************************************
*/
int main(int argc, char *argv[]) {
	FILE *fh = NULL, *frandom = NULL;
	long random_value = 0;
	int rc;
	unsigned char uacEEPROMImage[6];

	fh = fopen(EEPROM_PATH, "rb+");
	if(fh != NULL) {
		/*Check WIFI Mac address*/
		fseek(fh, 0x4, SEEK_SET);
		fread(uacEEPROMImage, sizeof(unsigned char), 6, fh);
		if(uacEEPROMImage[0] == 0 && uacEEPROMImage[1] == 0 &&
			uacEEPROMImage[2] == 0 && uacEEPROMImage[3] == 0 &&
			uacEEPROMImage[4] == 0 && uacEEPROMImage[5] == 0)
		{
			//time(&timep);
			/*Get Random value*/
			frandom = fopen("/dev/urandom", "rb");
			if (frandom == NULL) {
				printf("Could not open /dev/urandom.\n");
				return -1;
			}
			
			rc = fread(&random_value, 1, sizeof(long), frandom);
			fclose(frandom);
	
			//random_value = random();
			printf("Time:%x\n", random_value);
			uacEEPROMImage[0] = 0x00;
			uacEEPROMImage[1] = 0x08;
			uacEEPROMImage[2] = 0x22;
			uacEEPROMImage[3] = random_value >> 16;
			uacEEPROMImage[4] = random_value >> 8;
			uacEEPROMImage[5] = random_value;

			fseek(fh, 0x4, SEEK_SET);
			fwrite(uacEEPROMImage, sizeof(unsigned char), 6, fh);
		}

		/*Check BT Mac Address*/
		fseek(fh, 0x384, SEEK_SET);
		fread(uacEEPROMImage, sizeof(unsigned char), 6, fh);
		if(uacEEPROMImage[0] == 0 && uacEEPROMImage[1] == 0 &&
			uacEEPROMImage[2] == 0 && uacEEPROMImage[3] == 0 &&
			uacEEPROMImage[4] == 0 && uacEEPROMImage[5] == 0)
		{
			if (random_value == 0) {
				frandom = fopen("/dev/urandom", "rb");
				if (frandom == NULL) {
					printf("Could not open /dev/urandom.\n");
					return -1;
				}
			
				rc = fread(&random_value, 1, sizeof(long), frandom);
				fclose(frandom);
			
				uacEEPROMImage[0] = 0x00;
				uacEEPROMImage[1] = 0x08;
				uacEEPROMImage[2] = 0x22;
				uacEEPROMImage[3] = random_value >> 16;
				uacEEPROMImage[4] = random_value >> 8;
				uacEEPROMImage[5] = random_value;
			}
			else {
				uacEEPROMImage[0] = 0x00;
				uacEEPROMImage[1] = 0x08;
				uacEEPROMImage[2] = 0x22;
				uacEEPROMImage[3] = random_value >> 16;
				uacEEPROMImage[4] = random_value >> 8;
				uacEEPROMImage[5] = random_value + 1;
			}

			fseek(fh, 0x384, SEEK_SET);
			fwrite(uacEEPROMImage, sizeof(unsigned char), 6, fh);
		}

		fclose(fh);
	}
	return 0;
}
