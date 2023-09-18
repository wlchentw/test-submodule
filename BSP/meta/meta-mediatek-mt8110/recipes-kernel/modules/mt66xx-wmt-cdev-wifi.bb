DESCRIPTION = "Mediatek MT66xx WiFi Driver"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://COPYING;md5=d7810fab7487fb0aad327b76f1be7cd7"
DEPENDS += "mt66xx-wmt-drv"
MT66xx_SRC = "${TOPDIR}/../src/kernel/modules/connectivity"

inherit workonsrc
WORKONSRC = "${MT66xx_SRC}/wlan_driver/adapter_mt66xx/"

MODULE_NAME = "wmt_chrdev_wifi"
LINUX_SRC = "${TOPDIR}/tmp/work/${MACHINE_ARCH}${TARGET_VENDOR}-${TARGET_OS}/${PREFERRED_PROVIDER_virtual/kernel}/4.9-${PR}/${PREFERRED_PROVIDER_virtual/kernel}-4.9/"
LDFLAGS = "-L ${RECIPE_SYSROOT}"
TOOLCHAIN = "gcc"

do_buildclean() {
}

do_configure() {
}

do_compile() {
	if test "${COMBO_CHIP_ID}" = "mt6631"; then
		echo mt66xx wifi start compile
		echo ${WORKONSRC}
		echo ${S}

		if test "${KERNEL_ARCH}" = "arm64"; then
		cd ${S} && make -f Makefile.ce TOPDIR=${TOPDIR} MODULE_NAME=${MODULE_NAME} DRIVER_DIR=${S} LINUX_SRC=${LINUX_SRC} PACKAGE_ARCH=${PACKAGE_ARCH} TARGET_OS=${TARGET_OS} TARGET_VENDOR=${TARGET_VENDOR} ARCH=arm64 CROSS_COMPILE=aarch64-poky-linux-
		else
		cd ${S} && make -f Makefile.ce TOPDIR=${TOPDIR} MODULE_NAME=${MODULE_NAME} DRIVER_DIR=${S} LINUX_SRC=${LINUX_SRC} PACKAGE_ARCH=${PACKAGE_ARCH} TARGET_OS=${TARGET_OS} TARGET_VENDOR=${TARGET_VENDOR} ARCH=arm CROSS_COMPILE=arm-poky-linux-gnueabi-
		fi

		echo mt66xx wifi end compile
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
