DESCRIPTION = "keypad"
LICENSE = "MediaTekProprietary"
LIC_FILES_CHKSUM = "file://COPYING;md5=751419260aa954499f7abaabaa882bbe"
APPS_SRC = "${TOPDIR}/../meta/meta-mediatek-mt8110/recipes-autotest/keypad/files"

inherit workonsrc deploy systemd

WORKONSRC = "${APPS_SRC}"

SYSTEMD_PACKAGES = "${PN}"
SYSTEMD_SERVICE_${PN} = "Keypad.service"

do_compile() {
	make
}

do_install() {
   install -d ${D}${bindir}
   install -d ${D}${systemd_unitdir}/system
   
   install -m 755 ${S}/keypad ${D}${bindir}
   install -m 755 ${S}/Keypad.service ${D}${systemd_unitdir}/system
   install -m 755 ${S}/keypad.sh ${D}/${bindir}
}

FILES_${PN} += "${systemd_unitdir}/system/Keypad.service"
INSANE_SKIP_${PN} += "already-stripped ldflags"
FILES_${PN}-dev = ""
