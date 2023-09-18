CRIPTION = "Combo FW"
LICENSE = "MediaTekProprietary"
LIC_FILES_CHKSUM = "file://LICENSE;md5=fe6c5551532af70327222041e18e4e5f"
INSANE_SKIP_${PN} += "installed-vs-shipped"
FILES_${PN} += "/lib/firmware/soc2_1_ram_mcu_2_1_hdr.bin"
FILES_${PN} += "/lib/firmware/soc2_1_ram_bt_2_1_hdr.bin"
FILES_${PN} += "/lib/firmware/soc2_1_patch_mcu_2_1_hdr.bin"
FILES_${PN} += "/lib/firmware/soc2_1_ram_wifi_2_1_hdr.bin"
FILES_${PN} += "/lib/firmware/WIFI_RAM_CODE_soc2_1_2_1.bin"

inherit workonsrc
WORKONSRC = "${TOPDIR}/../src/connectivity/bin/mt6631"

do_install () {
	install -d ${D}/lib/firmware
	install -m 0755 ${S}/soc2_1_ram_mcu_2_1_hdr.bin ${D}/lib/firmware
	install -m 0755 ${S}/soc2_1_ram_bt_2_1_hdr.bin ${D}/lib/firmware
	install -m 0755 ${S}/soc2_1_patch_mcu_2_1_hdr.bin ${D}/lib/firmware
	install -m 0755 ${S}/soc2_1_ram_wifi_2_1_hdr.bin ${D}/lib/firmware
	install -m 0755 ${S}/WIFI_RAM_CODE_soc2_1_2_1.bin ${D}/lib/firmware
}
