#include <string.h>
#include <sys/time.h>
#include "ta_drmkey.h"
#include "keyblock.h"
#include "keyblock_protect.h"
#include <tee_client_api.h>
#include "drm_utils.h"
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

//temp dir for install key
#define DRMKEY_LINUX
#ifdef DRMKEY_ANDROID
#define KB_MIX_PATH         "/data/vendor/key_provisioning/KB_MIX"
#define KB_INSTALL_LOG_PATH         "/data/vendor/key_provisioning/drm_key_install.log"
#elif defined(DRMKEY_LINUX)
#define KB_MIX_PATH         "/data/vendor/key_provisioning/KB_MIX"
#define KB_INSTALL_LOG_PATH         "/data/vendor/key_provisioning/drm_key_install.log"
#endif

int free_keyblock(void* kb)
{
    if(kb)
        free(kb);
    return 0;
}

static int drmkey_remove_file(char *filename)
{
    if (remove(filename) != 0)
    {
        ALOGE("[drmkey_ca:]remove file faield: %s\n", filename);
        return -1;
    }

    return 0;
}

extern int errno;
int write_buff_to_file(unsigned char* buff,unsigned int length,char* path)
{
	int written = 0;
    if((buff == NULL) || (path == NULL))
    {
        ALOGE("[drmkey_ca:] invalid input for writing.");
        return -1;
    }

    FILE* file = fopen(path,"wb");
	if(file == NULL)
	{
		 ALOGE("[drmkey_ca:]Open file Error: %s\n,error = %d.", path, errno);
		 return -1;
	}
	written = fwrite(buff,1,length,file);
	fflush(file);
	if(written != (int)length)
	{
		 ALOGE("[drmkey_ca:]Write file Error: %s\n", path);
		 fclose(file);
		 return -2;
	}
	fclose(file);
	return 0;
}

int read_file_to_buff(unsigned char** buff,unsigned int* length,char* path)
{
    struct stat f_info;
    int readLen = 0;
    FILE* file;
    
    if((length == NULL) || (path == NULL) || (buff == NULL))
    {
        ALOGE("[drmkey_ca:] invalid input for reading.");
        return -1;
    }

    file = fopen(path,"rb");
    if(file == NULL)
    {
        ALOGE("[drmkey_ca:] Open file Error: %s\n", path);
        return -1;
    }
    if(stat(path,&f_info) != 0)
    {
        ALOGE("[drmkey_ca:] stat file_info Error: %s\n", path);
        fclose(file);
        return -2;
    }
    *buff = (unsigned char*)malloc(f_info.st_size);
    if(*buff == NULL)
    {
        ALOGE("[drmkey_ca:]read file to buffer:Allocate memory failed.\n");
        fclose(file);
        return -3;
    }
    *length = f_info.st_size;
    readLen = fread(*buff,1,*length,file);
    if(readLen != f_info.st_size)
	{
		 ALOGE("[drmkey_ca:]read file Error: %s\n", path);
         free(*buff);
         *buff = NULL; 
		 fclose(file);
		 return -4;
	}
    fclose(file);
    return 0;
}

static TEEC_Context teec_cxt_drmkey;
static TEEC_Session teec_sess_drmkey;
static bool isConnected = false;

int optee_keymanager_close(void)
{
    if(isConnected)
    {
        TEEC_CloseSession(&teec_sess_drmkey);
        TEEC_FinalizeContext(&teec_cxt_drmkey);
        isConnected = false;
    }

    return 0;
}

bool optee_keymanager_call(uint32_t cmd, TEEC_Operation *op)
{
    if(!op)
        return false;

    uint32_t err_origin;
    TEEC_Result res = TEEC_InvokeCommand(&teec_sess_drmkey, cmd, op, &err_origin);
    if (res != TEEC_SUCCESS) {
        ALOGE("[drmkey_ca:]TEEC_InvokeCommand(cmd=%d) failed with code: 0x%08x origin 0x%08x",
        cmd,res, err_origin);
    if (res == TEEC_ERROR_TARGET_DEAD) {
        optee_keymanager_close();
    }
        return false;
    }

    return true;
}

extern unsigned char Input_Ekkb_pub[];
extern unsigned char InputPkb[];
int init_drmkey_env(unsigned char *Input_Ekkb_pub,unsigned char *InputPkb)
{
    TEEC_Operation op;
    DRMKEY_ENV_T* env;

    if(!Input_Ekkb_pub || !Input_Ekkb_pub){
        ALOGE("[drmkey_ca:][init_drmkey_env]: input params is invalid.\n");
        return -1;
    }

    env=(DRMKEY_ENV_T*)malloc(sizeof(DRMKEY_ENV_T));
    if(!env){
        ALOGE("[drmkey_ca:][init_drmkey_env]: malloc memory failed.\n");
        return -2;
    }
    memcpy(env->_Ekkb_pub_,Input_Ekkb_pub,256);
    memcpy(env->_Pkb_,InputPkb,129);

    //set params
    memset(&op, 0, sizeof(op));
    op.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_INPUT,
                                     TEEC_NONE,
                                     TEEC_NONE,
                                     TEEC_NONE);
    op.params[0].tmpref.buffer = env;
    op.params[0].tmpref.size = sizeof(DRMKEY_ENV_T);

    if(!optee_keymanager_call(TZCMD_DRMKEY_INIT_ENV,&op)){
       ALOGE("[drmkey_ca:][init_drmkey_env]: init failed.\n");
       free(env);
       return -3;
    }
    free(env);

    return 0;
}

int optee_keymanager_connect(void){
    TEEC_Result res;
    TEEC_UUID uuid = TZ_TA_DRMKEY_UUID;
    uint32_t err_origin;

    //initialize context
    res = TEEC_InitializeContext(NULL, &teec_cxt_drmkey);
    if (res != TEEC_SUCCESS) {
        ALOGE("[drmkey_ca:]TEEC_InitializeContext failed with code 0x%x", res);
        return (int)res;
    }

    //open session
    res = TEEC_OpenSession(&teec_cxt_drmkey, &teec_sess_drmkey, &uuid, TEEC_LOGIN_PUBLIC,
            NULL, NULL, &err_origin);
    if (res != TEEC_SUCCESS) {
        ALOGE("[drmkey_ca:]TEEC_Opensession failed with code 0x%x origin 0x%x",
                res, err_origin);
        return (int)res;
    }

    isConnected = true;
    return 0;
}

int process_encrypt_key_block(unsigned char *encryptKbk, unsigned int inLength, unsigned char **installLog)
{
    TEEC_Result res;
    TEEC_Operation op;
    TEEC_SharedMemory share_mem;
    bool ret = false;
    uint32_t installLogSize = INSTALL_LOG_SIZE;

    //if not connected,try to connect it
    if(!isConnected && optee_keymanager_connect())
    {
        ALOGE("[drmkey_ca:]failed to connect drmkey TA.\n");
        return -1;
    }
    
    //init ENV
    if(init_drmkey_env(Input_Ekkb_pub,InputPkb) < 0){
        ALOGE("[drmkey_ca:]Failed to init ENV in connection.");
        return -1;
    }

    if((encryptKbk == NULL) ||(inLength > MAX_KEYBLOCK_SIZE))
    {
        ALOGE("[drmkey_ca:]invalid input.\n");
        return -2;
    }
    //register shared_mem
    memset(&share_mem,0,sizeof(TEEC_SharedMemory));
    share_mem.size = inLength + installLogSize;
    share_mem.flags = TEEC_MEM_INPUT | TEEC_MEM_OUTPUT;
    res = TEEC_AllocateSharedMemory(&teec_cxt_drmkey, &share_mem);
    if(res != TEEC_SUCCESS){
        ALOGE("[drmkey_ca:]TEEC_AllocateSharedMemory failed with code 0x%x", res);
        return -3;
    }
    memset(share_mem.buffer,0,share_mem.size);
    memcpy(share_mem.buffer,encryptKbk,inLength);

    //set params
    memset(&op, 0, sizeof(op));
    op.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_PARTIAL_INPUT,
                                     TEEC_MEMREF_PARTIAL_OUTPUT,
                                     TEEC_NONE,
                                     TEEC_NONE);

    op.params[0].memref.parent = &share_mem;
    op.params[0].memref.offset = 0;
    op.params[0].memref.size = inLength;
    op.params[1].memref.parent = &share_mem;
    op.params[1].memref.offset = inLength;
    op.params[1].memref.size = installLogSize;

    *installLog = (unsigned char *)malloc(installLogSize);
    if(!(*installLog)){
        ALOGE("[drmkey_ca:]malloc log buffer failed.\n");
        TEEC_ReleaseSharedMemory(&share_mem);
        return -4;
    }
    
    ret = optee_keymanager_call(TZCMD_DRMKEY_INSTALL,&op);
    memcpy(*installLog,(void*)((char*)share_mem.buffer + inLength),installLogSize);
    TEEC_ReleaseSharedMemory(&share_mem);
    if(ret == false){
        ALOGE("[drmkey_ca:]install DRMKEY failed.\n");
        return -5;
    }else{
        return 0;  
    }
}


// Key manager gen key
int get_drmkey(unsigned int keyID)
{
    TEEC_Operation op;

    if(keyID >= DRM_KEY_MAX){
        ALOGE("[drmkey_ca:]get_drmkey:invalid keyId.\n");
        return -1;
    }
    
    //we have get encrypt key;send it to TA
    if(!isConnected && optee_keymanager_connect())
    {
        ALOGE("[drmkey_ca:]get_drmkey:failed to connect drmkey TA.\n");
        return -2;
    }

    memset(&op, 0, sizeof(op));
    op.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INPUT,
                                     TEEC_NONE,
                                     TEEC_NONE,
                                     TEEC_NONE);
    op.params[0].value.a = keyID;
    op.params[1].value.b = 0;

    if(!optee_keymanager_call(TZCMD_DRMKEY_GEN_KEY,&op))
    {
        ALOGE("[drmkey_ca:]get_drmkey:gen key failed.");
        return -3;
    }

    return 0;
}

/*
count ---key count, set by TA
keytype ---result of DRMKEY ARRAY, should be NULL if no key installed
user need to free it after using
*/
int query_drmkey(unsigned int *count, unsigned int **keytype)
{
    TEEC_Operation op;
    unsigned int result_key[DRM_KEY_MAX] = {0};
    unsigned int result_key_size = DRM_KEY_MAX * sizeof(unsigned int);

    if(!count || !keytype){
        ALOGE("[drmkey_ca:]Error! invaid input pramas for querying drmkey!\n");
        return -1;
    }

    if(!isConnected && optee_keymanager_connect())
    {
        ALOGE("[drmkey_ca:]failed to connect drmkey TA.\n");
        return -1;
    }

    //set params
    memset(&op, 0, sizeof(op));
    op.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_OUTPUT,
                                     TEEC_VALUE_OUTPUT,
                                     TEEC_NONE,
                                     TEEC_NONE);

    op.params[0].tmpref.buffer = result_key;
    op.params[0].tmpref.size = result_key_size;


    if(!optee_keymanager_call(TZCMD_DRMKEY_QUERY,&op)){
        ALOGE("[drmkey_ca:] query_drmkey failed.");
        return -1;
    }

    *keytype = malloc(result_key_size);
    if(*keytype == NULL){
        ALOGE("[drmkey_ca:] allocate memory failed.");
        return -2;
    }

    memcpy(*keytype,result_key,result_key_size);
    *count = op.params[1].value.a;

    return 0;
}

int install_KB_MIX(void)
{
    int ret = 0;
    unsigned char* kb_mix = NULL;
    unsigned char* kb_install_log = NULL;
    unsigned int   kb_mix_len = 0;

    ret = read_file_to_buff(&kb_mix,&kb_mix_len,KB_MIX_PATH);
    if(ret != 0)
    {
        ALOGE("[drmkey_ca:]read_file_to_buff failed!\n");
        return -1;
    }
    
    ret =  process_encrypt_key_block(kb_mix,kb_mix_len,&kb_install_log);
    free_keyblock((void*)kb_mix);
    if(ret != 0)
    {
        ALOGE("[drmkey_ca:]failed to process encrypt keyblock,ret = %d.\n",ret);
        ret = write_buff_to_file(kb_install_log,INSTALL_LOG_SIZE,KB_INSTALL_LOG_PATH);
        if(ret < 0){
            //just warning
            ALOGE("[drmkey_ca:]failed to write installation log,ret = %d.\n",ret);
        }
        free_keyblock((void*)kb_install_log);
        return -1;
    }
    free_keyblock((void*)kb_install_log);

    if(drmkey_remove_file(KB_MIX_PATH) != 0)
        ALOGE("[drmkey_ca:][install_KB_MIX] drmkey_remove_file() faield %s\n", KB_MIX_PATH);

    ALOGE("[drmkey_ca:]install_KB_MIX end!\n");
    return 0;

}

int install_KB_MIX_API(unsigned char* buff,unsigned int len)
{
    int ret = 0;
    ret = write_buff_to_file(buff,len,KB_MIX_PATH);
    if(ret != 0) return -1;
    return install_KB_MIX();
}
