FILESEXTRAPATHS_append := ":${THISDIR}/${PN}"

DEPENDS += "grep-native"
FPGA_PROJ="$(echo ${MTK_PROJECT}| grep fpga -o)"

SRC_URI_append = " \
	file://expand-fs-to-whole-partition-eMMC.rules \
"