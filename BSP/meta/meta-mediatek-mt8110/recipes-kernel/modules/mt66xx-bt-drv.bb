DESCRIPTION = "Mediatek mt66xx BT Driver"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://COPYING;md5=d7810fab7487fb0aad327b76f1be7cd7"
DEPENDS = "linux-mtk-extension mt66xx-wmt-drv"
MT66xx_CONNECTIVITY_SRC = "${TOPDIR}/../src/kernel/modules/connectivity"

inherit workonsrc
WORKONSRC = "${MT66xx_CONNECTIVITY_SRC}/bt_driver/mt66xx"
MODULE_NAME = "wmt_cdev_bt"
LINUX_SRC = "${TOPDIR}/tmp/work/${MACHINE_ARCH}${TARGET_VENDOR}-${TARGET_OS}/${PREFERRED_PROVIDER_virtual/kernel}/4.9-${PR}/${PREFERRED_PROVIDER_virtual/kernel}-4.9/"
LDFLAGS = "-L ${RECIPE_SYSROOT}"
TOOLCHAIN = "gcc"
export KERNEL_VER = "v`echo "${PREFERRED_VERSION_linux-mtk-extension}" | cut -d "%" -f 1`"

do_compile() {
	if test "${COMBO_CHIP_ID}" = "mt6631"; then
		echo mt8512 bt driver start compile
		echo ${WORKONSRC}
		echo ${KERNEL_VER} ZQ-kernel_version
		if test "${KERNEL_ARCH}" = "arm64"; then
			make bt_drv TOPDIR=${TOPDIR} MODULE_NAME=${MODULE_NAME} LINUX_SRC=${LINUX_SRC} TARGET_OS=${TARGET_OS} TARGET_VENDOR=${TARGET_VENDOR} ARCH=arm64 CROSS_COMPILE=aarch64-poky-linux- PLATFORM=MT8512_YOCTO CONNECTIVITY_SRC=${MT66xx_CONNECTIVITY_SRC}
		else
			make bt_drv TOPDIR=${TOPDIR} MODULE_NAME=${MODULE_NAME} LINUX_SRC=${LINUX_SRC} TARGET_OS=${TARGET_OS} TARGET_VENDOR=${TARGET_VENDOR} ARCH=arm CROSS_COMPILE=arm-poky-linux-gnueabi- PLATFORM=MT8512_YOCTO CONNECTIVITY_SRC=${MT66xx_CONNECTIVITY_SRC}
		fi
		echo mt8512 bt driver end compile
	fi
}

do_install() {
	echo ${D}
	if test "${COMBO_CHIP_ID}" = "mt6631"; then
		install -d ${D}/lib/modules/mt66xx/
		install -m 0644 ${S}/${MODULE_NAME}.ko ${D}/lib/modules/mt66xx/
	fi
}

FILES_${PN} += "/lib/modules/mt66xx"
INSANE_SKIP_${PN} += "already-stripped"
FILES_${PN}-dev = ""