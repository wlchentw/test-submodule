DESCRIPTION = "MediaTek MDP daemon"
LICENSE = "MediaTekProprietary"
LIC_FILES_CHKSUM = "file://NOTICE;md5=e1696b147d49d491bcb4da1a57173fff"

inherit workonsrc
WORKONSRC = "${TOPDIR}/../src/multimedia/mdpd3"

DEPENDS = "libmdppq"
DEPENDS += "libmdp-prot"
#DEPENDS += "libcutils"
#DEPENDS += "libutils"
DEPENDS += "libion"
RDEPENDS_${PN} = "libgcc"
RDEPENDS_${PN} += "libstdc++"
RDEPENDS_${PN} += "libmdppq"
RDEPENDS_${PN} += "libmdp-prot"

inherit pkgconfig

do_configure() {
        :
}

do_compile() {
	oe_runmake \
		PACKAGE_ARCH="${PACKAGE_ARCH}"  \
		CFLAGS="${CFLAGS}" \
		LDFLAGS="${LDFLAGS}"
}

do_install() {
	oe_runmake \
		PREFIX="${prefix}" DESTDIR="${D}" PACKAGE_ARCH="${PACKAGE_ARCH}" INCDIR="${includedir}" SRCDIR="${S}" install
}

inherit systemd

SYSTEMD_PACKAGES = "${PN}"
SYSTEMD_SERVICE_${PN} = "mdpd.service"
FILES_${PN} += "${systemd_unitdir}/system/mdpd.service"

do_install_append() {
    if ${@bb.utils.contains('DISTRO_FEATURES','systemd','true','false',d)}; then
        install -d ${D}${systemd_unitdir}/system/
        install -m 0644 ${B}/v4l2_mdpd/lib/systemd/system/mdpd.service ${D}${systemd_unitdir}/system
    fi
	install -d ${D}${includedir}
	install -d ${D}${includedir}/platform/
	install -m 444 ${S}v4l2_mdpd/vpu/include/platform/debug.h ${D}${includedir}/platform/debug.h
	install -m 444 ${S}v4l2_mdpd/vpu/include/compiler.h ${D}${includedir}/compiler.h
	install -m 444 ${S}v4l2_mdpd/vpu/include/debug.h ${D}${includedir}/debug.h
	install -m 444 ${S}v4l2_mdpd/vpu/app/mdp/mdp_debug.h ${D}${includedir}/mdp_debug.h
	install -m 444 ${S}v4l2_mdpd/vpu/app/mdp/mdp_ipi.h ${D}${includedir}/mdp_ipi.h
	install -m 444 ${S}libmdp/if/mdp_lib_if.h ${D}${includedir}/mdp_lib_if.h
	install -m 444 ${S}libmdp/dpframework/include/DpConfig.h ${D}${includedir}/DpConfig.h
	install -m 444 ${S}libmdp/dpframework/include/DpDataType.h ${D}${includedir}/DpDataType.h
	install -m 444 ${S}libmdp/dpframework/include/DpIspDataType.h ${D}${includedir}/DpIspDataType.h
	install -m 444 ${S}libmdp/dpframework/include/DpIspStream.h ${D}${includedir}/DpIspStream.h
	install -m 444 ${S}libmdp/dpframework/include/DpIspDataType.h ${D}${includedir}/DpIspDataType.h
	install -m 444 ${S}libmdp/dpframework_prot/include/mt8183/tpipe_config.h ${D}${includedir}/tpipe_config.h
	install -m 444 ${S}kernel-headers/mt8183/cmdq_v3_def.h ${D}${includedir}/cmdq_v3_def.h
	install -m 444 ${S}kernel-headers/mt8183/cmdq_v3_event_common.h ${D}${includedir}/cmdq_v3_event_common.h
	install -m 444 ${S}kernel-headers/mt8183/cmdq_v3_subsys_common.h ${D}${includedir}/cmdq_v3_subsys_common.h
	install -m 444 ${S}kernel-headers/mt8183/cmdq_engine.h ${D}${includedir}/cmdq_engine.h
	rm -fr ${D}/usr/lib/libmdp.mt2701.so
	rm -fr ${D}/usr/lib/libmdp.mt2712.so
	rm -fr ${D}/usr/lib/libmdp.mt8173.so
	rm -fr ${D}/usr/lib/libmdp.mt8167.so
	rm -fr ${D}/usr/lib/libmdp_tile.mt2712.so
}

INSANE_SKIP_${PN} += "already-stripped"
FILES_${PN}-dev = "dev-elf"
FILES_${PN} += "${libdir} ${includedir}"

SECURITY_CFLAGS_pn-${PN} = "${SECURITY_NO_PIE_CFLAGS}"
