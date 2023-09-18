#ifndef __KEYINSTALL_CA__
#define __KEYINSTALL_CA__

int write_key_to_device_API(int drmKeyId,unsigned char* kb,int len);
int read_key_from_device_API(int drmKeyId,unsigned char** kb,int* len);

#endif