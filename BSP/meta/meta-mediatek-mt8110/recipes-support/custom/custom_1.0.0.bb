#Basic Configuration
DESCRIPTION = "Customization."
SECTION = "base"
LICENSE = "MediaTekProprietary"
LIC_FILES_CHKSUM = "file://LICENSE;md5=e1696b147d49d491bcb4da1a57173fff"
MTK_SRC = "${TOPDIR}/../src/support/libnvram_custom/CFG"

WORKONSRC = "${MTK_SRC}"

inherit deploy workonsrc

PRJ_FILENAME = "${MTK_PROJECT}"

#Parameters passed to do_compile()
EXTRA_OEMAKE = "'CROSS=${TARGET_PREFIX}'\
                'PRJ_FILENAME=${PRJ_FILENAME}'"
ALLOW_EMPTY_${PN} = "1"

FILES_${PN}-dev = "*"
FILES_${PN}  = "*"
do_install () {
	oe_runmake install ROOT=${D}

	if test -d "${WORKDIR}/${MTK_NVRAM_PROJECT}/cgen"; then
		mkdir -p ${D}/include/custom/${MTK_PROJECT}
		cp -rf ${WORKDIR}/${MTK_NVRAM_PROJECT}/cgen ${D}/include/custom/${MTK_PROJECT}
	fi

	install -d ${D}${includedir}
	cp -af ${D}/include/* ${D}${includedir}
	rm -rf ${D}/include

}

addtask nvramclean
do_nvramclean () {
	oe_runmake clean
}
