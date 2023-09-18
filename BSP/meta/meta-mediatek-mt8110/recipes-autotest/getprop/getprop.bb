DESCRIPTION = "getprop"
LICENSE = "MediaTekProprietary"

APPS_SRC = "${TOPDIR}/../meta/meta-mediatek-mt8110/recipes-autotest/getprop/files"
LIC_FILES_CHKSUM = "file://COPYING;md5=751419260aa954499f7abaabaa882bbe"

inherit workonsrc systemd

WORKONSRC = "${APPS_SRC}"

SYSTEMD_PACKAGES = "${PN}"

do_compile() {
	make
}

do_install() {
   install -d ${D}${bindir}
   install -m 755 ${S}/getprop ${D}${bindir}
}

FILES_${PN} += "${D}${bindir}/getprop"
INSANE_SKIP_${PN} += "already-stripped ldflags"
INSANE_SKIP_${PN} += "installed-vs-shipped"
FILES_${PN}-dev = ""
