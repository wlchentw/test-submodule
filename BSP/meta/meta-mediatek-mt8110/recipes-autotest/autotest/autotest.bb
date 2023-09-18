inherit deploy extraexternalsrc

DESCRIPTION = "Mediatek autotest"
LICENSE = "MediaTekProprietary"
LIC_FILES_CHKSUM = "file://COPYING;md5=751419260aa954499f7abaabaa882bbe"

BBCLASSEXTEND = "native nativesdk"

SRC_URI = " \
	file://flashimg-init-emmc-china.bat \
	file://COPYING \
	"

S = "${WORKDIR}"
ALLOW_EMPTY_${PN} = "1"
do_deploy () {
	if [ ${MTK_PROJECT} = "aud8110p1" ] || [ ${MTK_PROJECT} = "aud8110m1-emmc" ] || [ ${MTK_PROJECT} = "aud8110p2-d1" ] || [ ${MTK_PROJECT} = "aud8110p2-d2" ]; then
		install -d ${DEPLOYDIR}
		install -m 755 ${S}/flashimg-init-emmc-china.bat -t ${DEPLOYDIR}
		mv ${DEPLOYDIR}/flashimg-init-emmc-china.bat ${DEPLOYDIR}/flashimg-init.bat
	fi	 
}

addtask deploy before do_build after do_compile