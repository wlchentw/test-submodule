FILESEXTRAPATHS_append := ":${THISDIR}/files"
SRC_URI += " \
file://emi_container_mt8110.tag \
file://device.parameters.xml \
file://project.dat \
"

do_deploy_append () {
    install -d ${DEPLOYDIR}
    install -m 755 ${S}/../emi_container_${TARGET_PLATFORM_FLAVOR}.tag ${DEPLOYDIR}/emi.container.tag
	install -d ${DEPLOYDIR}/dev
	install -m 644 ${S}/../device.parameters.xml -t ${DEPLOYDIR}/dev/
	install -m 644 ${S}/../project.dat -t ${DEPLOYDIR}/dev/
	#install -m 644 ${S}/../ntx_bins -t ${DEPLOYDIR}/
	#cp -av ${S}/../ntx_bins ${DEPLOYDIR}/
}
