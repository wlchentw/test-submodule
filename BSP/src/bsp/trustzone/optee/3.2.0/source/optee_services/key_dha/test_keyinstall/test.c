#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <string.h>
#include <tee_client_api.h>
#ifndef ALOGE
#define ALOGE printf
#endif

extern int write_key_to_device_API(int drmKeyId,unsigned char* kb,int len);
extern int read_key_from_device_API(int drmKeyId,unsigned char** kb,int* len);

#define TA_KEYINSATLL_UUID   {0x989850BF,0x4663,0x9DCD,{0x39,0x4C,0x07,0xA4,0x5F,0x46,0x33,0xD1}}
#define TZCMD_CRYPTO_TEST            10
static TEEC_Context teec_cxt_Keyinstall;
static TEEC_Session teec_sess_Keyinstall;
static bool isConnected = false;

int optee_keyinstall_close(void)
{
    if(isConnected)
    {
        TEEC_CloseSession(&teec_sess_Keyinstall);
        TEEC_FinalizeContext(&teec_cxt_Keyinstall);
        isConnected = false;
    }

    return 0;
}

bool optee_keyinstall_call(uint32_t cmd, TEEC_Operation *op)
{
    if(!op)
        return false;

    uint32_t err_origin;
    TEEC_Result res = TEEC_InvokeCommand(&teec_sess_Keyinstall, cmd, op, &err_origin);
    if (res != TEEC_SUCCESS) {
        ALOGE("[Test_KEYINSTALL:]TEEC_InvokeCommand(cmd=%d) failed with code: 0x%08x origin 0x%08x",
        cmd,res, err_origin);
    if (res == TEEC_ERROR_TARGET_DEAD) {
        optee_keyinstall_close();
    }
        return false;
    }

    return true;
}

int optee_keyinstall_connect(void){
    TEEC_Result res;
    TEEC_UUID uuid = TA_KEYINSATLL_UUID;
    uint32_t err_origin;

    //initialize context
    res = TEEC_InitializeContext(NULL, &teec_cxt_Keyinstall);
    if (res != TEEC_SUCCESS) {
        ALOGE("[Test_KEYINSTALL:]TEEC_InitializeContext failed with code 0x%x", res);
        return (int)res;
    }

    //open session
    res = TEEC_OpenSession(&teec_cxt_Keyinstall, &teec_sess_Keyinstall, &uuid, TEEC_LOGIN_PUBLIC,
            NULL, NULL, &err_origin);
    if (res != TEEC_SUCCESS) {
        ALOGE("[Test_KEYINSTALL:]TEEC_Opensession failed with code 0x%x origin 0x%x",
                res, err_origin);
        TEEC_CloseSession(&teec_sess_Keyinstall);
        return (int)res;
    }

    isConnected = true;
    return 0;
}



int main(int argc,char* argv[])
{
    unsigned char testkey1[] = "kkkkk";
    unsigned char testkey2[] = "mmmmm";
    ALOGE("test_key1: %s\n", testkey1);
    ALOGE("test_key2: %s\n", testkey2);

    //try to read key from null
    unsigned char* key1 = NULL;
    int len = 0;
    if(read_key_from_device_API(0,&key1, &len) < 0)
    {
        ALOGE("test_key1 keyId = 0 pass: this should be fail.\n");
    }
    else
    {
        ALOGE("Success to Read key.\n");
        ALOGE("test_key1,keyId = 0 read from block--key: %s\n", key1);
        ALOGE("test_key1,keyId = 0 read from block--len: %d\n", len);
        if(key1)
        {
            free(key1);
            key1 = NULL;
        }
    }

    //try to write key to block
    if(write_key_to_device_API(0,testkey1, 6) == 0)
    {
        ALOGE("test_key1 keyId = 0 pass: this should be pass.\n");
    }

    //try to read key from block
    if(read_key_from_device_API(0,&key1, &len) == 0)
    {
        ALOGE("test_key1 keyId = 0 pass: this should be pass.\n");
    }
    if(key1)
    {
        ALOGE("test_key1,keyId = 0 read from block--key: %s\n", key1);
        ALOGE("test_key1,keyId = 0 read from block--len: %d\n", len);
    }

    //free memory
    if(key1)
    {
        free(key1);
        key1 = NULL;
    }

    //test for key2
    if(read_key_from_device_API(7,&key1, &len) < 0)
    {
        ALOGE("test_key2 keyId = 7 pass: this should be fail.\n");
    }

    //try to write key to block
    if(write_key_to_device_API(7,testkey2, 6) == 0)
    {
        ALOGE("test_key2 keyId = 7 pass: this should be pass.\n");
    }

    //try to read key from block
    if(read_key_from_device_API(7,&key1, &len) == 0)
    {
        ALOGE("test_key2 keyId = 7 pass: this should be pass.\n");
    }
    if(key1)
    {
        ALOGE("test_key2,keyId = 7 read from block--key: %s\n", key1);
        ALOGE("test_key2,keyId = 7 read from block--len: %d\n", len);
    }

    //free memory
    if(key1)
    {
        free(key1);
        key1 = NULL;
    }

    //test for unexpected cases
    if(read_key_from_device_API(8,&key1, &len) < 0)
    {
        ALOGE("test_key10 keyId = 8 pass: Read-drmKeyId = 8.this should be fail.\n");
    }
    if(write_key_to_device_API(8,testkey1, 6) < 0)
    {
        ALOGE("test_key10 keyId = 8 pass: Write-drmKeyId = 8.this should be fail.\n");
    }
    if(read_key_from_device_API(-1,&key1, &len) < 0)
    {
        ALOGE("test_key10 keyId = 8 pass: Read-drmKeyId = -1.this should be fail.\n");
    }
    if(write_key_to_device_API(-1,testkey1, 6) < 0)
    {
        ALOGE("test_key10 keyId = 8 pass: Write-drmKeyId = -1.this should be fail.\n");
    }
    //free memory
    if(key1)
    {
        free(key1);
        key1 = NULL;
    }

    //test TZ crypto
    TEEC_Operation op;
    //connect Test TA
    if(!isConnected && optee_keyinstall_connect())
    {
        ALOGE("[Test_KEYINSTALL:]failed to connect test TA.\n");
        return -2;
    }

    memset(&op, 0, sizeof(op));
    op.paramTypes = TEEC_PARAM_TYPES(TEEC_NONE,
                                     TEEC_NONE,
                                     TEEC_NONE,
                                     TEEC_NONE);

    if(!optee_keyinstall_call(TZCMD_CRYPTO_TEST,&op))
    {
        ALOGE("[Test_KEYINSTALL:]get_drmkey:gen key failed.");
        return -3;
    }
    ALOGE("[Test_KEYINSTALL:]send cmd done.");

	return 0;
}