inherit deploy u-boot-fitimage

UBOOT_SRC = "${TOPDIR}/../src/bsp/u-boot"
UBOOT_OUT = "${WORKDIR}/out"
LIC_FILES_CHKSUM = "file://${UBOOT_SRC}/Licenses/README;md5=30503fd321432fc713238f582193b78e"
SRC_URI = "file://u-boot.dts "
SECTION = "bootloaders"
PROVIDES = "virtual/bootloader"
DEPENDS += "bc-native dtc-native libgcc lk"

LICENSE = "GPLv2+"

PACKAGE_ARCH = "${MACHINE_ARCH}"

PROJECT = "mtk"

# Some versions of u-boot use .bin and others use .img.  By default use .bin
# but enable individual recipes to change this value.
UBOOT_SUFFIX ??= "bin"
UBOOT_IMAGE ?= "u-boot-${PROJECT}.${UBOOT_SUFFIX}"
UBOOT_BINARY ?= "u-boot-dtb.${UBOOT_SUFFIX}"
UBOOT_FIT_IMAGE ?= "u-boot-mtk-fit.${UBOOT_SUFFIX}"
UBOOT_MAKE_TARGET ?= "all"
UBOOT_FIT_LOADADDRESS ?="0x44e00000"

# EXTRA_OEMAKE = 'CROSS_COMPILE=${TOPDIR}/../prebuilt/toolchain/aarch64-linux-android-4.9/bin/aarch64-linux-android- KBUILD_OUTPUT="${UBOOT_OUT}" V=1'
EXTRA_OEMAKE = 'CROSS_COMPILE="${TARGET_PREFIX}" KBUILD_OUTPUT="${UBOOT_OUT}" V=1'

python __anonymous () {
	tee_loadaddress = int(d.getVar('UBOOT_FIT_LOADADDRESS', True), 16)
	dtb_loadaddress = tee_loadaddress - 0x10000
	dtb_loadaddress_str = hex(dtb_loadaddress).replace('L', '')
	d.setVar('UBOOT_DTB_LOADADDRESS', dtb_loadaddress_str)
}

do_compile () {
	cd ${UBOOT_SRC}
	oe_runmake mrproper
	oe_runmake ${UBOOT_BUILD_DEFCONFIG}
	oe_runmake -f Makefile
}

do_deploy () {
	install -d ${DEPLOYDIR}
	install -m 755 ${UBOOT_OUT}/${UBOOT_BINARY} -t ${DEPLOYDIR}
	install -m 755 ${UBOOT_OUT}/${UBOOT_FIT_IMAGE} -t ${DEPLOYDIR}
}

addtask deploy before do_build after do_assemble_fitimage

