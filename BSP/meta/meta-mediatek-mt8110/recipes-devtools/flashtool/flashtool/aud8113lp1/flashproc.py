# SPDX-License-Identifier:	MediaTekProprietary

#!/usr/bin/python
import os

procedures = {
    # product   : fastboot args
    'DEFAULT'   : [['daWait'],
                    ['fbWait'],
    #                ['fastboot', 'erase', 'mmc0'],
                    ['fastboot', 'flash', 'mmc0', 'MBR_EMMC'],
                    ['fastboot', 'flash', 'mmc0boot0', 'bl2.img'],
                    ['fastboot', 'flash', 'UBOOT', 'u-boot-mtk-fit.bin'],
                    ['fastboot', 'flash', 'tee_a', 'tee.img'],
                    ['fastboot', 'flash', 'boot_a', 'boot.img'],
                    ['fastboot', 'flash', 'system_a', 'rootfs.img'],
                    ['fastboot', 'flash', 'userdata', 'userdata.ext4'],
                    ]
}

userprocedures = {
    # product   : fastboot args
    'DEFAULT'   : [['daWait'],
                    ['fbWait'],
                    ['fastboot', 'erase', 'system_a'],
                    ['fastboot', 'flash', 'system_a', 'rootfs.img'] ]
}

bootprocedures = {
    # product   : fastboot args
    'DEFAULT'   : [['daWait'],
                    ['fbWait'],
    #                ['fastboot', 'erase', 'mmc0'],
                    ['fastboot', 'flash', 'mmc0', 'MBR_EMMC'],
                    ['fastboot', 'flash', 'mmc0boot0', 'bl2.img'],
                    ['fastboot', 'flash', 'tee_a', 'tee.img'],
                    ['fastboot', 'flash', 'boot_a', 'boot.img'] ]
}

testprocedures = {
    # product   : fastboot args
    'DEFAULT'   : [['daWait'],
                    ['fbWait'],
                    ['fastboot', 'flash', 'mmc0', 'MBR_EMMC'],
                    ['fastboot', 'erase', 'mmc0boot0'],
                    ['fastboot', 'erase', 'UBOOT'],
                    ['fastboot', 'erase', 'tee_a'],
                    ['fastboot', 'erase', 'boot_a'],
                    ['fastboot', 'erase', 'system_a'],
                    ['fastboot', 'erase', 'userdata'],
                    ['fastboot', 'flash', 'mmc0boot0', 'bl2.img'],
                    ['fastboot', 'flash', 'UBOOT', 'u-boot-mtk-fit.bin'],
                    ['fastboot', 'flash', 'tee_a', 'tee.img'],
                    ['fastboot', 'flash', 'boot_a', 'boot.img'],
                    ['fastboot', 'flash', 'system_a', 'rootfs.img'],
                    ['fastboot', 'flash', 'userdata', 'userdata.ext4'],
                    ['fastboot', 'flash', 'hwcfg', 'ntx_hwconfig.bin'],
                    ['fastboot', 'flash', 'ntxfw', 'ntxfw_EA0T04_v1.bin'],
                    ]
}
u1procedures = {
    # product   : fastboot args
    'DEFAULT'   : [['daWait'],
                    ['fbWait'],
                    ['fastboot', 'flash', 'mmc0', 'MBR_EMMC'],
                    ['fastboot', 'erase', 'mmc0boot0'],
                    ['fastboot', 'erase', 'UBOOT'],
                    ['fastboot', 'erase', 'tee_a'],
                    ['fastboot', 'erase', 'boot_a'],
                    ['fastboot', 'erase', 'waveform'],
                    ['fastboot', 'erase', 'hwcfg'],
                    ['fastboot', 'erase', 'ntxfw'],
                    ['fastboot', 'erase', 'system_a'],
                    ['fastboot', 'erase', 'userdata'],
                    ['fastboot', 'flash', 'mmc0boot0', 'bl2.img'],
                    ['fastboot', 'flash', 'UBOOT', 'u-boot-mtk-fit.bin'],
                    ['fastboot', 'flash', 'tee_a', 'tee.img'],
                    ['fastboot', 'flash', 'boot_a', 'boot.img'],
                    ['fastboot', 'flash', 'waveform', 'waveform.img'],
                    ['fastboot', 'flash', 'system_a', 'rootfs.img'],
                    ['fastboot', 'flash', 'userdata', 'userdata.ext4'],
                    ['fastboot', 'flash', 'hwcfg', 'ntx_hwconfig.bin'],
                    ['fastboot', 'flash', 'ntxfw', 'ntxfw.bin'],
                    ]
}
u2procedures = {
    # product   : fastboot args
    'DEFAULT'   : [['daWait'],
                    ['fbWait'],
                    ['fastboot', 'flash', 'mmc0', 'MBR_EMMC'],
                    ['fastboot', 'erase', 'mmc0boot0'],
                    ['fastboot', 'erase', 'UBOOT'],
                    ['fastboot', 'erase', 'tee_a'],
                    ['fastboot', 'erase', 'boot_a'],
                    ['fastboot', 'erase', 'ntxfw'],
                    ['fastboot', 'erase', 'system_a'],
                    ['fastboot', 'erase', 'userdata'],
                    ['fastboot', 'flash', 'mmc0boot0', 'bl2.img'],
                    ['fastboot', 'flash', 'UBOOT', 'u-boot-mtk-fit.bin'],
                    ['fastboot', 'flash', 'tee_a', 'tee.img'],
                    ['fastboot', 'flash', 'boot_a', 'boot.img'],
                    ['fastboot', 'flash', 'system_a', 'rootfs.img'],
                    ['fastboot', 'flash', 'userdata', 'userdata.ext4'],
                    ['fastboot', 'flash', 'ntxfw', 'ntxfw.bin'],
                    ]
}



# return procedure list
def getFlashProc(product):
    try:
        ret = procedures[product.upper()]
        return ret
    except Exception, e:
        return None


def getFlashUserProc(product):
    try:
        ret = userprocedures[product.upper()]
        return ret
    except Exception, e:
        return None

def getFlashBootProc(product):
    try:
        ret = bootprocedures[product.upper()]
        return ret
    except Exception, e:
        return None

def getFlashTestProc(product):
    try:
        ret = testprocedures[product.upper()]
        return ret
    except Exception, e:
        return None

def getFlashU1Proc(product):
    try:
        ret = u1procedures[product.upper()]
        return ret
    except Exception, e:
        return None

	
def getFlashU2Proc(product):
    try:
        ret = u2procedures[product.upper()]
        return ret
    except Exception, e:
        return None
	
