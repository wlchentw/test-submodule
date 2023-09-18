inherit workonsrc
inherit trustzone-build
inherit systemd

DESCRIPTION = "OPTEE CLIENT"
LICENSE = "BSD-2-Clause & MediaTekProprietary"
TZ_SRC = "${TOPDIR}/../src/bsp/trustzone"
WORKONSRC = "${TZ_SRC}/optee/3.2.0/source/optee_client"
WORKONSRC_BUILD = "${TZ_SRC}/optee/3.2.0/source/optee_client"
LIC_FILES_CHKSUM = "file://${S}/LICENSE;md5=69663ab153298557a59c67a60a743e5b"

EXTRA_OEMAKE_append = "O=${WORKDIR}/out"

SYSTEMD_PACKAGS = "${PN}"
SYSTEMD_SERVICE_${PN} = "tee-supplicant.service"
FILES_${PN} += "${D}${systemd_unitdir}/system/tee-supplicant.service"

do_install() {
    oe_runmake install

    install -D -p -m0755 ${WORKDIR}/out/export/bin/tee-supplicant ${D}${bindir}/tee-supplicant

    install -D -p -m0644 ${WORKDIR}/out/export/lib/libteec.so.1.0 ${D}${libdir}/libteec.so.1.0
    ln -sf libteec.so.1.0 ${D}${libdir}/libteec.so
    ln -sf libteec.so.1.0 ${D}${libdir}/libteec.so.1

    cp -a ${WORKDIR}/out/export/include ${D}/usr/

    cp -a ${S}/tee-supplicant.service ${WORKDIR}
    sed -i -e s:/etc:${sysconfdir}:g \
           -e s:/usr/bin:${bindir}:g \
              ${WORKDIR}/tee-supplicant.service

    install -d ${D}${systemd_unitdir}/system
    install -D -p -m0644 ${WORKDIR}/tee-supplicant.service ${D}${systemd_unitdir}/system/tee-supplicant.service
}

