#include <stdio.h>
#include <fcntl.h>
#include <string.h>

#define PS_ADBD_FILE "/data/ps_adbd.txt"
#define PS_GVA_FILE "/data/ps_gva.txt"
#define PS_AVS_FILE "/data/ps_avs.txt"
#define LENGTH (128)
#define ADBD_SERVICE "/usr/bin/adbd"
#define CAST_SHELL_SERVICE "/chrome/cast_shell"
#define ALEXA_SERVICE "/usr/bin/alexaapp.sh"

int strfpos( const char *s1, const char *s2 )
{
	int pos = -1;
	int len2;
	if ( !(len2 = strlen(s2)) )
		return -1;
	for (;*s1;++s1)
	{
		pos++;
		if ( *s1 == *s2 && strncmp( s1, s2, len2 ) == 0 )
			return pos;
	}
	return -1;
}

int main(int argc, char *argv[])
{
	int n = 0;
	int adbd_prog_num = 0, avs_num = 0, gva_num = 0;
	char buf[LENGTH] = {0};

	system("ps | grep adbd > /data/ps_adbd.txt");
	FILE *fp = fopen (PS_ADBD_FILE, "rw");
	while(fgets(buf, LENGTH, fp) != NULL)
	{
		printf("string:%s\n", buf);
		n = strfpos(buf,ADBD_SERVICE);
		if (n > 0)
		{
			//printf("n = %d\n",n);
			adbd_prog_num++;
		}
	}
	if (adbd_prog_num > 0)
	{
		printf("adbd bootup complete success.\n");
	}
	fclose(fp);
	
#ifdef VA_SUPPORT_ALEXA	
	printf("avs project\n");
    system("ps | grep alexaapp > /data/ps_avs.txt");
	FILE *fp1 = fopen (PS_AVS_FILE, "rw");
	memset(buf, LENGTH, 0x00);
	n = 0;
	while(fgets(buf, LENGTH, fp1) != NULL)
	{
		printf("string:%s\n", buf);
		n = strfpos(buf,ALEXA_SERVICE);
		if (n > 0)
		{
			//printf("n = %d\n",n);
			avs_num++;
		}
	}
	if (avs_num > 0)
	{
		printf("alexaApp bootup complete success.\n");
	}
	
	if (appmainprog_num > 0 && avs_num > 0)
	{
		printf("appmainprog&alexaApp bootup complete success.\n");
	}
	else
	{
		printf("appmainprog&alexaApp bootup failed.\n");
	}
	fclose(fp1);
	system("rm /data/ps_avs.txt");
#endif

#ifdef VA_SUPPORT_GVA_SDK_SUPPORT || AUDIO_SUPPORT_C4A_SDK	
	printf("gva & c4a project\n");
    system("ps | grep cast_shell > /data/ps_gva.txt");
	FILE *fp2 = fopen (PS_GVA_FILE, "rw");
	memset(buf, LENGTH, 0x00);
	n = 0;
	while(fgets(buf, LENGTH, fp2) != NULL)
	{
		printf("string:%s\n", buf);
		n = strfpos(buf,CAST_SHELL_SERVICE);
		if (n > 0)
		{
			//printf("n = %d\n",n);
			gva_num++;
		}
	}
	if (gva_num > 0)
	{
		printf("cast_shell bootup complete success.\n");
	}
	
	if (appmainprog_num > 0 && gva_num > 0)
	{
		printf("appmainprog&cast_shell bootup complete success.\n");
	}
	else
	{
		printf("appmainprog&cast_shell bootup failed.\n");
	}
	fclose(fp2);
	system("rm /data/ps_gva.txt");
#endif

	system("rm /data/ps_adbd.txt");
	
	return 0;
}