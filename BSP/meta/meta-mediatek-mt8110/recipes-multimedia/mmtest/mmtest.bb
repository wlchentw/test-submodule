DESCRIPTION = "mmtest"
LICENSE = "MediaTekProprietary"

APPS_SRC = "${TOPDIR}/../src/multimedia/mmtest"
LIC_FILES_CHKSUM = "file://NOTICE;md5=e1696b147d49d491bcb4da1a57173fff"

inherit package pkgconfig workonsrc autotools

WORKONSRC = "${APPS_SRC}"

do_compile() {
	oe_runmake
}

do_install() {
	oe_runmake \
		PREFIX="${prefix}" DESTDIR="${D}" MTK_PROJECT="${MTK_PROJECT}" install
}

FILES_${PN} += " /data/* "
INSANE_SKIP_${PN} += "already-stripped ldflags"
INSANE_SKIP_${PN} += "installed-vs-shipped"
FILES_${PN}-dev = ""
