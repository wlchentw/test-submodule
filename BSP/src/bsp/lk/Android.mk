LOCAL_PATH := $(call my-dir)
LK_ROOT_DIR := $(PWD)
HOST_OS ?= $(shell uname | tr '[A-Z]' '[a-z]')
export HOST_OS

#TARGET_PLATFORM := mt8518

# LK build tools, directories setting
DTS_DIR := device/mediatek/${TARGET_BOARD_PLATFORM}/common/tools/bootloader/security/lk_dts
DUMMY_IMG_DIR := device/mediatek/${TARGET_BOARD_PLATFORM}/common/tools/bootloader/security/dummy_img
DEV_INFO_DIR := device/mediatek/${TARGET_BOARD_PLATFORM}/common/tools/bootloader/security/dev_info
UBOOT_MAKEIMAGE := prebuilts/u-boot/host/x86_64-linux/usr/bin/uboot-mkimage
PBP_DIR := device/mediatek/${TARGET_BOARD_PLATFORM}/common/tools/bootloader/security/pbp
GFH_DIR := device/mediatek/${TARGET_BOARD_PLATFORM}/common/tools/bootloader/security/gfh
KEY_DIR := device/mediatek/${TARGET_BOARD_PLATFORM}/common/tools/bootloader/security/key
DEV_INFO_HDR_TOOL := device/mediatek/${TARGET_BOARD_PLATFORM}/common/tools/bootloader/scatter

# LK make option
LK_MAKE_OPTION := $(if $(SHOW_COMMANDS),NOECHO=) $(if $(LK_CROSS_COMPILE),ARCH_arm64_TOOLCHAIN_PREFIX=$(LK_CROSS_COMPILE)) \
                  CFLAGS= DEBUG=0 SECURE_BOOT_ENABLE=${SECURE_BOOT_ENABLE} SECURE_BOOT_TYPE=${SECURE_BOOT_TYPE} \
                  AVB_ENABLE_ANTIROLLBACK=${AVB_ENABLE_ANTIROLLBACK} AB_OTA_UPDATER=${AB_OTA_UPDATER}

ifdef LK_PROJECT
    LK_DIR := $(LOCAL_PATH)

    ifeq ($(wildcard $(TARGET_PREBUILT_LK)),)
        TARGET_LK_OUT ?= $(if $(filter /% ~%,$(TARGET_OUT_INTERMEDIATES)),,$(LK_ROOT_DIR)/)$(TARGET_OUT_INTERMEDIATES)/BOOTLOADER_OBJ
        BUILT_LK_TARGET := $(TARGET_LK_OUT)/build-$(LK_PROJECT)/lk.bin

.KATI_RESTAT: $(BUILT_LK_TARGET)
$(BUILT_LK_TARGET): lk_clean lk_gen_key
	$(hide) mkdir -p $(dir $@)
	$(MAKE) -C $(LK_DIR) $(LK_MAKE_OPTION) $(LK_PROJECT)
###################################################################################
# Gen GFH header

	$(hide) cp ${LK_DIR}/build-${LK_PROJECT}/lk.bin ${LK_DIR}/build-${LK_PROJECT}/temp_gfh
	
ifeq ($(SECURE_BOOT_ENABLE), yes)
	$(hide) cp ${MTK_KEY_DIR}/${SBC_KEY}.pem ${KEY_DIR}/root_prvk.pem
endif#SECURE_BOOT_ENABLE

	python ${PBP_DIR}/pbp.py -g ${GFH_DIR}/gfh_conf.ini \
		-i ${KEY_DIR}/lk_key.ini -func sign \
		-o ${LK_DIR}/build-${LK_PROJECT}/temp_gfh ${LK_DIR}/build-${LK_PROJECT}/temp_gfh

ifneq ($(BOOTDEV_TYPE), nand)
	$(hide) cp -rf ${DEV_INFO_HDR_TOOL}/scripts .
	python ${DEV_INFO_HDR_TOOL}/scripts/dev-info-hdr-tool.py \
		${BOOTDEV_TYPE} ${LK_DIR}/build-${LK_PROJECT}/temp_gfh ${LK_DIR}/build-${LK_PROJECT}/lk.img
	$(hide) rm -rf scripts
else
	$(hide) cp ${LK_DIR}/build-${LK_PROJECT}/temp_gfh ${LK_DIR}/build-${LK_PROJECT}/lk.img
endif#BOOTDEV_TYPE
###################################################################################
	$(hide) cp -f $(LK_DIR)/build-$(LK_PROJECT)/lk.bin $(PRODUCT_OUT)/
	$(hide) cp -f $(LK_DIR)/build-$(LK_PROJECT)/lk.img $(PRODUCT_OUT)/
	$(hide) mv -f $(LK_DIR)/build-$(LK_PROJECT) $(PRODUCT_OUT)/build-$(LK_PROJECT)
	#$(hide) rm -rf ${LK_DIR}/include/blob.h

ifeq ($(BOOTDEV_TYPE), nand)
	cat ${PRODUCT_OUT}/MBR_NAND ${PRODUCT_OUT}/lk.img > ${PRODUCT_OUT}/mlk.img
endif
    else
        BUILT_LK_TARGET := $(TARGET_PREBUILT_LK)

	$(TARGET_PREBUILT_LK): $(BUILT_LK_TARGET)
		$(hide) mkdir -p $(dir $@)
		$(hide) cp -f $(BUILT_LK_TARGET) $@

    endif#TARGET_PREBUILT_LK

# Remove bootloader binary to trigger recompile when source changes
lk_clean:
	$(hide) rm -rf $(LK_DIR)/build-$(LK_PROJECT)
	$(hide) rm -rf $(PRODUCT_OUT)/build-$(LK_PROJECT)
	$(hide) rm -rf $(PRODUCT_OUT)/lk.bin
	$(hide) rm -rf $(PRODUCT_OUT)/lk.img

lk_gen_key:
ifeq ($(SECURE_BOOT_ENABLE), yes)
ifeq ($(STANDALONE_SIGN_PREPARE), yes)
	cp -f ${DTS_DIR}/standalone_dummy_blob.txt ${LK_DIR}/include/blob.h
else
	mkdir -p ${LK_DIR}/mykeys
	dtc -p 0x7ff ${DTS_DIR}/lk.dts -O dtb -o ${DTS_DIR}/lk.dtb
ifeq ($(shell test -f ${MTK_KEY_DIR}/daa_${VERIFIED_KEY}.crt && test -f ${MTK_KEY_DIR}/daa_${VERIFIED_KEY}.key && echo yes), yes)
	cp ${MTK_KEY_DIR}/daa_${VERIFIED_KEY}.crt ${LK_DIR}/mykeys/daa.crt
	cp ${MTK_KEY_DIR}/daa_${VERIFIED_KEY}.pem ${LK_DIR}/mykeys/daa.key
	echo "verified key and daa verified key are different!"
else
	cp ${MTK_KEY_DIR}/${VERIFIED_KEY}.crt ${LK_DIR}/mykeys/daa.crt
	cp ${MTK_KEY_DIR}/${VERIFIED_KEY}.pem ${LK_DIR}/mykeys/daa.key
	echo "verified key and daa verified key are the same!"
endif#shell
	${HSM_ENV} HSM_KEY_NAME=${VERIFIED_KEY} $(UBOOT_MAKEIMAGE) -D "-I dts -O dtb -p 1024" -F -k ${LK_DIR}/mykeys -K ${DTS_DIR}/lk.dtb -r ${DUMMY_IMG_DIR}/fitImage.daa
	rm -rf ${LK_DIR}/mykeys/*
	cp ${MTK_KEY_DIR}/${VERIFIED_KEY}.crt ${LK_DIR}/mykeys/dev.crt
	cp ${MTK_KEY_DIR}/${VERIFIED_KEY}.pem ${LK_DIR}/mykeys/dev.key
	${HSM_ENV} HSM_KEY_NAME=${VERIFIED_KEY} $(UBOOT_MAKEIMAGE) -D "-I dts -O dtb -p 1024" -F -k ${LK_DIR}/mykeys -K ${DTS_DIR}/lk.dtb -r ${DUMMY_IMG_DIR}/fitImage.dev
	python ${DEV_INFO_DIR}/dtb-transfer-array.py ${DTS_DIR}/lk.dtb ${DTS_DIR}/blob.h
	cp ${DTS_DIR}/blob.h ${LK_DIR}/include/blob.h
	rm -rf ${LK_DIR}/mykeys
endif#STANDALONE_SIGN_PREPARE
else
	cp -f ${DTS_DIR}/tmp_blob.txt ${LK_DIR}/include/blob.h
ifeq ($(STANDALONE_SIGN_PREPARE), yes)
	cp -f ${DTS_DIR}/standalone_dummy_blob.txt ${LK_DIR}/include/blob.h
endif
endif#SECURE_BOOT_ENABLE

.PHONY: lk lk_clean
droidcore: $(BUILT_LK_TARGET)
lk: $(BUILT_LK_TARGET)

endif#LK_PROJECT
