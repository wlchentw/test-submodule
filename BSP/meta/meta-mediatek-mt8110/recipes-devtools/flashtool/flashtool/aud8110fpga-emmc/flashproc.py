#!/usr/bin/python
import os

procedures = {
    # product   : fastboot args
    'DEFAULT'   : [['daWait'],
                    ['fbWait'],
                    ['fastboot', 'erase', 'mmc0'],
                    ['fastboot', 'flash', 'mmc0', 'MBR_EMMC'],
                    ['fastboot', 'flash', 'mmc0boot0', 'bl2.img'],
                    ['fastboot', 'flash', 'tee_a', 'tee.img'],
                    ['fastboot', 'flash', 'boot_a', 'boot.img'],
                    ['fastboot', 'flash', 'system_a', 'rootfs.img'],
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
                    ['fastboot', 'erase', 'mmc0'],
                    ['fastboot', 'flash', 'mmc0', 'MBR_EMMC'],
                    ['fastboot', 'flash', 'mmc0boot0', 'bl2.img'],
                    ['fastboot', 'flash', 'tee_a', 'tee.img'],
                    ['fastboot', 'flash', 'boot_a', 'boot.img'] ]
}

testprocedures = {
    # product   : fastboot args
    'DEFAULT'   : [['daWait'],
                    ['fbWait'],
                    ['fastboot', 'erase', 'mmc0boot0'],
                    ['fastboot', 'erase', 'tee_a'],
                    ['fastboot', 'erase', 'boot_a'],
                    ['fastboot', 'erase', 'system_a'],
                    ['fastboot', 'flash', 'mmc0boot0', 'bl2.img'],
                    ['fastboot', 'flash', 'tee_a', 'tz.img'],
                    ['fastboot', 'flash', 'boot_a', 'boot.img'],
                    ['fastboot', 'flash', 'system_a', 'rootfs.img'] ]
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
