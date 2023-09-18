DESCRIPTION = "MTK eFuse Writer"
LICENSE = "MediaTekProprietary"
LIC_FILES_CHKSUM = "file://LICENSE;md5=e1696b147d49d491bcb4da1a57173fff"

inherit workonsrc
WORKONSRC = "${TOPDIR}/../src/support/efuse_writer"

DEPENDS += "${@bb.utils.contains('TEE_SUPPORT', 'mtee', 'tzapp', '' ,d)}"
RDEPENDS_${PN} += "${@bb.utils.contains('TEE_SUPPORT', 'mtee', 'tzapp', '' ,d)}"
DEPENDS += "${@bb.utils.contains('TEE_SUPPORT', 'optee', 'optee-services', '' ,d)}"
RDEPENDS_${PN} += "${@bb.utils.contains('TEE_SUPPORT', 'optee', 'optee-services', '' ,d)}"

do_compile() {
	if test "${TEE_SUPPORT}" = "optee" ;then
        oe_runmake LDFLAGS='${LDFLAGS} -ltz_efuse'
	else
        oe_runmake LDFLAGS='${LDFLAGS} -ltz_efuse -ltz_uree'
	fi
}

do_install() {
	install -d ${D}${bindir}
	install -m 0755 ewriter ${D}${bindir}/
}
