#include "crypt_tool.h"

/*
Customized interface
MTK highly recommend that not embeds DM_PROTECT_KEY in crypttool,
but stores it in emmc and get it though get_dm_protect_key_str at runtime.
*/
const char* get_dm_protect_key_str(void)
{
	return NULL;
}

