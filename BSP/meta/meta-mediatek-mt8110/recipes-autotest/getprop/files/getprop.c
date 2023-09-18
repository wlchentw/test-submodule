#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define XSTR(s) STR(s)
#define STR(s) #s


int main(int argc, char*argv[])
{
	FILE *fp;
	char line[1024];
	char *str;
	char *token;

	if (argc == 1)
	{
		printf("[ro.mediatek.version.branch]: [%s]\n", "aud-ep-master-audio.sdk-mt8110.ep1");
		printf("[ro.build.type]: [%s]\n", "userdebug");
		printf("[ro.mediatek.version.release]: [%s]\n", "L2.GA");
		printf("[ro.hardware]: [%s]\n", "mt8512");
		printf("[ro.product.name]: [%s]\n", "mt8110");
		printf("[sys.boot_completed]: [%s]\n", "1");
		printf("[ro.mediatek.os]: [%s]\n", "yocto");
	}

	if (argc == 0)
		return 0;
	else if( argc != 2)
		return -1;

	fp = fopen("/tmp/met_log_d.conf", "r");
	if (strcmp(argv[1], "ro.build.type") == 0)
	{
		printf("userdebug");
	}
	else if (strcmp(argv[1], "ro.board.platform") == 0)
	{
		printf("%s", XSTR(CONFIG_MTK_PLATFORM));
	}
	else if (!strcmp(argv[1], "ro.mediatek.os"))
	{
		printf("%s\n", "yocto");
	}
	else if (!strcmp(argv[1], "ro.mediatek.version.branch"))
	{
		printf("%s\n", "aud-ep-master-audio.sdk-mt8110.ep1");
	}
	else if (!strcmp(argv[1], "ro.build.type"))
	{
		printf("%s\n", "userdebug");
	}
	else if (!strcmp(argv[1], "ro.mediatek.version.release"))
	{
		printf("%s\n", "1.0");
	}
	else if (!strcmp(argv[1], "ro.hardware"))
	{
		printf("%s\n", "mt8512");
	}
	else if (!strcmp(argv[1], "ro.product.name"))
	{
		printf("%s\n", "mt8110");
	}
	else if (!strcmp(argv[1], "sys.boot_completed"))
	{
		printf("%s\n", "1");
	}
	else if (fp) {
		do {
			memset(line, 0x0, 1024);
			str = fgets(line, 1023, fp);
			if (str == NULL) {
				break;
			}

			token = strtok(str, "=");
			if (token && strcmp(argv[1], token) == 0) {
				token = strtok(NULL, "\n");
				printf("%s", token);
			}
		} while (str);
		fclose(fp);
	} else {
		printf("\n");
	}

	return 0;
}