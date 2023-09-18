SUMMARY = "A small image just capable of allowing a device to boot."

IMAGE_INSTALL = "packagegroup-core-boot ${ROOTFS_PKGMANAGE_BOOTSTRAP} ${CORE_IMAGE_EXTRA_INSTALL}"

IMAGE_LINGUAS = " "

LICENSE = "MIT"

inherit core-image
inherit create-mtdverity
inherit create-sparse-image

inherit mkusrdata

IMAGE_FEATURES_append = " \
	${@bb.utils.contains('ENABLE_ROOTFS_CHECK', 'yes', 'read-only-rootfs', '' ,d)} \
	"

IMAGE_INSTALL_append = " \
		${@bb.utils.contains('SUPPORT_MTK_BT', 'true', 'bluetooth bluetooth-vendor', '', d)} \
		${@bb.utils.contains('COMBO_CHIP_ID', 'mt6631', 'mtkcombotool mt6631-wifi-fw \
		mt66xx-wmt-drv mt66xx-wmt-cdev-wifi mt66xx-wifi-drv mt66xx-bt-drv mtkwifitesttool', '', d)} \
		wireless-tools \
		wpa-supplicant \
		alsa-utils \
		busybox \
		android-tools \
		energy-aware \
		evtest \
		ewriter \
		mdpd3 \
		libion \
		mtk-ovl-adapter \
		mmtest \
		custom \
		libnvramcustom \
		nvram \
		${@bb.utils.contains("TEE_SUPPORT", "optee", "optee-client optee-services", "", d)}  \
		${@bb.utils.contains("TEE_TEST_SUPPORT", "optee", "optee-test", "", d)}  \
		i2c-tools \
		iperf2 \
		memtester \
		parted \
		${@base_contains("KEYPAD_SERVICE_ENABLE", "true", "keypad", "", d)} \
		${@base_contains("NATA_AUTOTEST_ENABLE", "true", "autotest poweron getprop", "", d)} \
		"

PACKAGE_EXCLUDE_append  = " \
    udev-hwdb \
	"

IMAGE_ROOTFS_EXTRA_SPACE = "204800"

