inherit externalsrc
inherit deploy trustzone-build

DESCRIPTION = "ARM trusted firmware"
LICENSE = "BSD-3-Clause & MediaTekProprietary"
# code src may be change in trustzone-build.bb to adapt license
MTK_SRC = "${TOPDIR}/../src/bsp/trustzone/atf"
ATF_VER = '1.6'
CHIP_TYPE = ''
LIC_FILES_CHKSUM = "file://${B}/v${ATF_VER}/${CHIP_TYPE}/license.rst;md5=e927e02bca647e14efd87e9e914b2443"
EXTERNALSRC = "${MTK_SRC}"
EXTERNALSRC_BUILD = "${MTK_SRC}"
PACKAGE_ARCH = "${MACHINE_ARCH}"
ATF_OUT = "${WORKDIR}/out"
ATF_OUT_BINARY = "${ATF_OUT}/${ATF_INSTALL_MODE}/bl31.bin"
DEPLOY_DIR_IMAGE = "${DEPLOY_DIR}/images/${MACHINE}/trustzone"
TBASE_VER ?= "none"
BL33_ARCH ?= "${KERNEL_ARCH}"

EXTRA_OEMAKE = "'CROSS_COMPILE=${TOPDIR}/../prebuilt/toolchain/aarch64-linux-android-4.9/bin/aarch64-linux-android-'	\
		'MTK_BL33_PROJECT=${BL33_PROJECT}' \
		'MTK_BL33_ARCH=${BL33_ARCH}'	\
		'KERNEL_ARCH=${KERNEL_ARCH}'	\
		'PLAT=${MTK_MACH_TYPE}'		\
		'MACH_TYPE=${MTK_MACH_TYPE}'	\
		'SECURE_OS=${TEE_SUPPORT}'	\
		'SECURE_OS_ARCH=${TEE_ARCH}' \
		'DEBUG=${@bb.utils.contains("ATF_INSTALL_MODE", "debug", "1", "0", d)}'	\
		'BUILD_BASE=${ATF_OUT}'		\
		${@oe.utils.conditional('TEE_SUPPORT', 'tbase', 'TBASE_VER=${TBASE_VER}', '', d)}"

CFLAGS[unexport] = "1"
LDFLAGS[unexport] = "1"
AS[unexport] = "1"
LD[unexport] = "1"
do_compile[nostamp] = "1"

do_compile () {
	oe_runmake -C ${B}/v${ATF_VER}/${CHIP_TYPE} -f ${B}/v${ATF_VER}/${CHIP_TYPE}/Makefile
}

do_deploy () {
	install -d ${DEPLOYDIR}
	install ${ATF_OUT_BINARY} ${DEPLOYDIR}/${ATF_RAW_BINARY}
	install ${ATF_OUT_BINARY} ${DEPLOYDIR}/${ATF_SIGNED_BINARY}
}

addtask populate_lic before do_install after do_compile
addtask deploy before do_build after do_install
