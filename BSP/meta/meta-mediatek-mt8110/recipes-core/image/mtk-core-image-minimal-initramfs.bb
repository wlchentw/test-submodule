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
		busybox \
		android-tools \
		mdpd3 \
		libion \
		mmtest \
		custom \
		${@bb.utils.contains("TEE_SUPPORT", "optee", "optee-client optee-services", "", d)}  \
		${@bb.utils.contains("TEE_TEST_SUPPORT", "optee", "optee-test", "", d)}  \
		i2c-tools \
		e2fsprogs \
		parted \
		"

PACKAGE_EXCLUDE_append  = " \
    udev-hwdb \
	"

IMAGE_ROOTFS_EXTRA_SPACE = "51200"

