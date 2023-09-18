DESCRIPTION = "Energy Aware assist"
LICENSE = "MediaTekProprietary"
LIC_FILES_CHKSUM = "file://LICENSE;md5=e1696b147d49d491bcb4da1a57173fff"

S = "${WORKDIR}"
SRC_URI = "file://energy-aware-control.service \
           file://ea-init \
           file://LICENSE \
"

inherit systemd
RDEPENDS_energy-aware += "bash"
FILES_${PN} += " \
    ${systemd_unitdir}/system/energy-aware-control.service \
    "

SYSTEMD_SERVICE_${PN} = "energy-aware-control.service"

do_install_append() {
    if ${@bb.utils.contains('DISTRO_FEATURES','systemd','true','false',d)}; then
        install -d ${D}${systemd_unitdir}/system ${D}${datadir}/energy-aware
        install -m 0755 ${S}/ea-init ${D}${datadir}/energy-aware
        install -m 644 ${WORKDIR}/energy-aware-control.service ${D}${systemd_unitdir}/system/energy-aware-control.service
    fi
}
