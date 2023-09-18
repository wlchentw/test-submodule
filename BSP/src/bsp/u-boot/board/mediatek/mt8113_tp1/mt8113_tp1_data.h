#ifndef MT8113_TP1_H//[
#define MT8113_TP1_H
typedef struct {
	unsigned int magic_number_begin;
	unsigned int dram_size;
	unsigned char efuse_data[512];
	unsigned int magic_number_end;
	unsigned int rpmb_key_status;
	unsigned int powerkey_status;
	unsigned int usb_status;
	unsigned int upkey_status;
	unsigned int downkey_status;
} BOOT_ARGUMENT_T;

#define BOOT_ARGUMENT_MAGIC 0x504c504c

#define BOOT_ARGUMENT_LOCATION	0x40000100

extern BOOT_ARGUMENT_T *gpt_boot_args;

#define E70T04 'o'

#endif //] MT8113_TP1_H

