DESCRIPTION = "poweron"
LICENSE = "MediaTekProprietary"

APPS_SRC = "${TOPDIR}/../meta/meta-mediatek-mt8110/recipes-autotest/poweron/files"
LIC_FILES_CHKSUM = "file://COPYING;md5=751419260aa954499f7abaabaa882bbe"

inherit workonsrc systemd

WORKONSRC = "${APPS_SRC}"

SYSTEMD_PACKAGES = "${PN}"

do_compile() {
	make VA_SUPPORT_GVA_SDK=${VA_SUPPORT_GVA_SDK} AUDIO_SUPPORT_C4A_SDK=${AUDIO_SUPPORT_C4A_SDK} VA_SUPPORT_ALEXA=${VA_SUPPORT_ALEXA}
}

do_install() {
   install -d ${D}${bindir}
   install -m 755 ${S}/poweron ${D}${bindir}
}

FILES_${PN} += "${D}${bindir}/poweron"
INSANE_SKIP_${PN} += "already-stripped ldflags"
INSANE_SKIP_${PN} += "installed-vs-shipped"
FILES_${PN}-dev = ""
