require u-boot-common_${PV}.inc

SRC_URI += "file://default-gcc.patch \
            file://0001-rsa-add-sha-256-384-512-rsa-2048-3072-4096-algorithm.patch \
            file://0002-rsa-add-sha-256-384-512-rsassa-pss-2048-3072-4096-al.patch \
            file://0003-rsa-extend-bits-of-n0-inverse.patch \
            file://0001-libfdt-fix-fdt_find_region-incorrectly-adding-parent.patch \
            file://0004-security-boot-u-boot-hsm-support.patch \
"

SUMMARY = "U-Boot bootloader image creation tool"
DEPENDS = "openssl"

EXTRA_OEMAKE_class-target = 'CROSS_COMPILE="${TARGET_PREFIX}" CC="${CC} ${CFLAGS} ${LDFLAGS}" HOSTCC="${BUILD_CC} ${BUILD_CFLAGS} ${BUILD_LDFLAGS}" STRIP=true V=1'
EXTRA_OEMAKE_class-native = 'CC="${BUILD_CC} ${BUILD_CFLAGS} ${BUILD_LDFLAGS}" HOSTCC="${BUILD_CC} ${BUILD_CFLAGS} ${BUILD_LDFLAGS}" STRIP=true V=1'
EXTRA_OEMAKE_class-nativesdk = 'CROSS_COMPILE="${HOST_PREFIX}" CC="${CC} ${CFLAGS} ${LDFLAGS}" HOSTCC="${BUILD_CC} ${BUILD_CFLAGS} ${BUILD_LDFLAGS}" STRIP=true V=1'

do_compile () {
	oe_runmake sandbox_defconfig

	# Disable CONFIG_CMD_LICENSE, license.h is not used by tools and
	# generating it requires bin2header tool, which for target build
	# is built with target tools and thus cannot be executed on host.
	sed -i "s/CONFIG_CMD_LICENSE=.*/# CONFIG_CMD_LICENSE is not set/" .config

	oe_runmake cross_tools NO_SDL=1
}

do_install () {
	install -d ${D}${bindir}
	install -m 0755 tools/mkimage ${D}${bindir}/uboot-mkimage
	ln -sf uboot-mkimage ${D}${bindir}/mkimage
}

BBCLASSEXTEND = "native nativesdk"
