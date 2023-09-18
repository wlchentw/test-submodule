/* An example test TA implementation.
 */

#ifndef __TRUSTZONE_TA_DRMKEY__
#define __TRUSTZONE_TA_DRMKEY__

#define TZ_TA_DRMKEY_UUID   {0x989850BF,0x4663,0x9DCD,{0x39,0x4C,0x07,0xA4,0x5F,0x46,0x33,0xD2}}

/* Data Structure for DRMKEY TA */
/* You should define data structure used both in REE/TEE here
   N/A for Test TA */

/* Command for Test TA */
#define TZCMD_DRMKEY_INSTALL            0
#define TZCMD_DRMKEY_QUERY              1
#define TZCMD_DRMKEY_GEN_KEY            2
#define TZCMD_DRMKEY_INIT_ENV           3
#endif /* __TRUSTZONE_TA_DRMKEY__ */
