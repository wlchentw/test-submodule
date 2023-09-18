#pragma once

//#define AVB_ENABLE_DEVICE_STATE_CHANGE 1
#define FORCE_DISABLE_FLASH_CHECK 1
#define INDEX_RPMB_UNLOCK 1
#define AVB_DEVICE_UNLOCK 0x5a

void cmd_continue_boot(const char *arg, void *data, unsigned sz);
void cmd_download(const char *arg, void *data, unsigned sz);
void cmd_download_boot(const char *arg, void *data, unsigned sz);
void cmd_download_tz(const char *arg, void *data, unsigned sz);
void cmd_erase(const char *arg, void *data, unsigned sz);
void cmd_dump(const char *arg, void *data, unsigned sz);
void cmd_flash(const char *arg, void *data, unsigned sz);
void cmd_flash_img(const char *arg, void *data, unsigned sz);
void cmd_flash_sparse_img(const char *arg, void *data, unsigned sz);
void cmd_getvar(const char *arg, void *data, unsigned sz);
void cmd_reboot(const char *arg, void *data, unsigned sz);
void cmd_reboot_bootloader(const char *arg, void *data, unsigned sz);
void cmd_reboot_recovery(const char *arg, void *data, unsigned sz);
#ifdef AVB_ENABLE_DEVICE_STATE_CHANGE
void cmd_flashing(const char *arg, void *data, unsigned sz);
int cmd_flash_lock_state(const char *arg);
#endif
void cmd_efuse_read(const char *arg, void *data, unsigned sz);
void cmd_efuse_write(const char *arg, void *data, unsigned sz);
void cmd_wifimac_write(const char *arg, void *data, unsigned sz);
void cmd_wifimac_read(const char *arg, void *data, unsigned sz);
void cmd_btmac_write(const char *arg, void *data, unsigned sz);
void cmd_btmac_read(const char *arg, void *data, unsigned sz);
void cmd_ethmac_write(const char *arg, void *data, unsigned sz);
void cmd_ethmac_read(const char *arg, void *data, unsigned sz);
void cmd_lkver_read(const char *arg, void *data, unsigned sz);
void cmd_led_ctrl(const char *arg, void *data, unsigned sz);
void cmd_poweroff(const char *arg, void *data, unsigned sz);
