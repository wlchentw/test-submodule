DESCRIPTION = "This module serves the common part driver of connectivity"
LICENSE = "MediaTekProprietary"
LIC_FILES_CHKSUM = "file://LICENSE;md5=e1696b147d49d491bcb4da1a57173fff"
INSANE_SKIP_${PN} += "installed-vs-shipped"

inherit workonsrc
WORKONSRC = "${TOPDIR}/../src/connectivity/combo_tool/6631_combo_tool"

inherit autotools

inherit systemd

SYSTEMD_PACKAGES = "${PN}"
SYSTEMD_SERVICE_${PN} = "wmtd.service launcher.service stp_dump.service mt66xx_drv_insmod.service"
FILES_${PN} += "${systemd_unitdir}/system/wmtd.service"
FILES_${PN} += "${systemd_unitdir}/system/launcher.service"
FILES_${PN} += "${systemd_unitdir}/system/stp_dump.service"
FILES_${PN} += "${systemd_unitdir}/system/mt66xx_drv_insmod.service"
FILES_${PN} += "/etc/mt66xx_drv_insmod.sh"
FILES_${PN} += "/lib/firmware/WMT_SOC.cfg"
FILES_${PN} += "/lib/firmware/WMT_STEP.cfg"

do_compile () {
}

do_install_append() {
    install -d ${D}/lib/firmware
    install -m 0755 ${S}/cfg_folder/WMT_SOC.cfg ${D}/lib/firmware
    install -m 0755 ${S}/cfg_folder/WMT_STEP_CONNAC.cfg ${D}/lib/firmware/WMT_STEP.cfg
    if ${@bb.utils.contains('DISTRO_FEATURES','systemd','true','false',d)}; then
        install -d ${D}${systemd_unitdir}/system/
        install -m 0644 ${B}/wmtd.service ${D}${systemd_unitdir}/system
        install -m 0644 ${B}/launcher.service ${D}${systemd_unitdir}/system
        install -m 0644 ${B}/stp_dump.service ${D}${systemd_unitdir}/system
        install -m 0644 ${B}/mt66xx_drv_insmod.service ${D}${systemd_unitdir}/system
    fi

    install -d ${D}/etc
    install -m 755 ${WORKONSRC}/mt6631_drv_insmod.sh ${D}/etc/mt66xx_drv_insmod.sh
}

