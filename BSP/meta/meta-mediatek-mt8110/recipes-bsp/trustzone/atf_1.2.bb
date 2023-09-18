inherit extraexternalsrc
inherit deploy trustzone-build

DESCRIPTION = "ARM trusted firmware"
LICENSE = "BSD"
# code src may be change in trustzone-build.bb to adapt license
MTK_SRC = "${TOPDIR}/../src/bsp/trustzone/atf"
ATF_VER = '1.2'
CHIP_TYPE = ''
LIC_FILES_CHKSUM = "file://${B}/v${ATF_VER}/${CHIP_TYPE}/license.md;md5=829bdeb34c1d9044f393d5a16c068371"
EXTERNALSRC = "${MTK_SRC}"
EXTERNALSRC_BUILD = "${MTK_SRC}"
PACKAGE_ARCH = "${MACHINE_ARCH}"
ATF_OUT = "${WORKDIR}/out"
ATF_OUT_BINARY = "${ATF_OUT}/${ATF_INSTALL_MODE}/bl31.bin"
DEPLOY_DIR_IMAGE = "${DEPLOY_DIR}/images/${MACHINE}/trustzone"
UBOOT_ARCH ?= "arm64"

EXTRA_OEMAKE = "'CROSS_COMPILE=aarch64-linux-android-'	\
		'KERNEL_ARCH=${KERNEL_ARCH}' \
		'ATF_UBOOT_ARCH=${UBOOT_ARCH}' \
		'PLAT=${MTK_MACH_TYPE}'	\
		'MACH_TYPE=${MTK_MACH_TYPE}'	\
		'SECURE_OS=${TEE_SUPPORT}'	\
		'BUILD_BASE=${ATF_OUT}'"

CFLAGS[unexport] = "1"
LDFLAGS[unexport] = "1"
AS[unexport] = "1"
LD[unexport] = "1"
do_compile[nostamp] = "1"

do_compile () {
        PATH=${TOPDIR}/../prebuilt/toolchain/aarch64-linux-android-4.9/bin/:$PATH \
        oe_runmake -C ${B}/v${ATF_VER}/${CHIP_TYPE} -f ${B}/v${ATF_VER}/${CHIP_TYPE}/Makefile
}

do_deploy () {
	install -d ${DEPLOYDIR}
	install ${ATF_OUT_BINARY} ${DEPLOYDIR}/${ATF_RAW_BINARY}
	install ${ATF_OUT_BINARY} ${DEPLOYDIR}/${ATF_SIGNED_BINARY}
}

addtask deploy before do_build after do_install
