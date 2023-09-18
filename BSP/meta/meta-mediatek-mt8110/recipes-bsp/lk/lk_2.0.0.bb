inherit deploy extraexternalsrc lk-image hsm-sign-env

LICENSE = "MIT"
MTK_SRC = "${TOPDIR}/../src/bsp/lk"
LK_OUT = "${WORKDIR}/out"
LIC_FILES_CHKSUM = "file://${MTK_SRC}/LICENSE;md5=ba5e70cccfd7c167a0ace6a1eb1d5457"
SRC_URI = "file://gfh file://dev_info file://pbp file://key file://lk_dts file://dummy_img file://fit-lk"
#S = "${WORKDIR}"
DEPENDS += "u-boot-mkimage-native bc-native dtc-native"

TOOLCHAIN_PREFIX = "${TARGET_PREFIX}"
EXTERNALSRC = "${MTK_SRC}"
EXTERNALSRC_BUILD = "${MTK_SRC}"
LK_BINARY = "lk.bin"
LK_IMAGE = "bl2.img"
PACKAGE_ARCH = "${MACHINE_ARCH}"
DEPENDS += "libgcc"
GFH_DIR = "${WORKDIR}/gfh"
PBP_DIR = "${WORKDIR}/pbp"
KEY_DIR = "${WORKDIR}/key"
DTS_DIR = "${WORKDIR}/lk_dts"
DUMMY_IMG_DIR="${WORKDIR}/dummy_img"
PACKAGE_ARCH = "${MACHINE_ARCH}"
TOOLCHAIN = "gcc"


IC_NAME="$(echo ${TARGET_PLATFORM}|tr '[a-z]' '[A-Z]')"
DEV_INFO_HDR_TOOL = "${TOPDIR}/../src/bsp/scatter/scripts/dev-info-hdr-tool.py"

python __anonymous () {
    defaulttune = d.getVar('DEFAULTTUNE', True)
    d.setVar("LIBGCC", '${TOPDIR}/../prebuilt/bsp/lk/${DEFAULTTUNE}/libgcc.a')
}

do_configure () {
	:
}

do_genkey () {
	mkdir -p ${LK_OUT}/include
	if [ "${SECURE_BOOT_ENABLE}" = "yes" ]; then
		mkdir -p ${WORKDIR}/mykeys
		dtc -p 0x3ff ${DTS_DIR}/lk.dts -O dtb -o ${DTS_DIR}/lk.dtb
		cp ${MTK_KEY_DIR}/${VERIFIED_KEY}.crt ${WORKDIR}/mykeys/dev.crt
		cp ${MTK_KEY_DIR}/${VERIFIED_KEY}.pem ${WORKDIR}/mykeys/dev.key
		${HSM_ENV} HSM_KEY_NAME=${VERIFIED_KEY} uboot-mkimage -D "-I dts -O dtb -p 1024" -F -k ${WORKDIR}/mykeys -K ${DTS_DIR}/lk.dtb -r ${DUMMY_IMG_DIR}/fitImage
		OFF_DT_STRINGS="`fdtdump ${DTS_DIR}/lk.dtb | grep off_dt_strings | sed "s,^\/\/.*:\s*0x,,"`"
		SIZE_DT_STRINGS="`fdtdump ${DTS_DIR}/lk.dtb | grep size_dt_strings | sed "s,^\/\/.*:\s*0x,,"`"
		KEYNODE_LEN="`echo "obase=16;ibase=16;${OFF_DT_STRINGS} + ${SIZE_DT_STRINGS}" | bc`"
		python ${WORKDIR}/dev_info/dtb-transfer-array.py ${DTS_DIR}/lk.dtb ${DTS_DIR}/blob.h ${KEYNODE_LEN}
		cp ${DTS_DIR}/blob.h ${LK_OUT}/include/blob.h
		cp ${DTS_DIR}/lk.dtb ${TOPDIR}/lk.dtb
		rm -rf ${WORKDIR}/mykeys
	else
		cp ${DTS_DIR}/tmp_blob.txt ${LK_OUT}/include/blob.h
	fi
}

do_compile () {
	PATH=${TOPDIR}/../prebuilt/toolchain/aarch64-linux-android-4.9/bin/:$PATH \
	oe_runmake ARCH_arm64_TOOLCHAIN_PREFIX=aarch64-linux-android- \
	           NOECHO="" \
	           BUILDROOT=${LK_OUT} \
	           LIBGCC="" \
	           CFLAGS="" \
	           DEBUG=0 \
	           SECURE_BOOT_ENABLE=${SECURE_BOOT_ENABLE} \
	           SECURE_BOOT_TYPE=${SECURE_BOOT_TYPE} \
	           AVB_ENABLE_ANTIROLLBACK=${AVB_ENABLE_ANTIROLLBACK} \
	           AB_OTA_UPDATER=${AB_OTA_UPDATER} \
	           ${LK_PROJECT}
	# add filesize check
	start=$(grep -w _start -n ${LK_OUT}/build-${LK_PROJECT}/lk.elf.sym.sorted | \
			sed -E  's/(.*):([0-9a-fA-F]+)(.*)/\2/' | tr '[:lower:]' '[:upper:]')
	end=$(grep -w _end -n ${LK_OUT}/build-${LK_PROJECT}/lk.elf.sym.sorted | \
		  sed -E  's/(.*):([0-9a-fA-F]+)(.*)/\2/' | tr '[:lower:]' '[:upper:]')
	fs=`echo "obase=10;ibase=16; $end - $start" | bc`
	maxsize=$(printf '%d' ${LK_MAX_SIZE})

	if [ ${LK_MAX_SIZE} != "" ] && [ $fs -gt $maxsize ]; then
		  bberror "Little kernel image size overflow, please have a check. $fs > $maxsize"
	fi
}

do_buildclean () {
	PATH=${TOPDIR}/../prebuilt/toolchain/aarch64-linux-android-4.9/bin/:$PATH \
	oe_runmake ARCH_arm64_TOOLCHAIN_PREFIX=aarch64-linux-android- \
		   NOECHO="" \
		   BUILDROOT=${LK_OUT} \
		   LIBGCC=${LIBGCC} \
		   CFLAGS="" \
		   DEBUG=0 \
		   ${LK_PROJECT} clean
}

do_genheader () {
	if [ "${FIT_LK_IMAGE}" = "yes" ]; then
		gen_lk_fit_header
	else
		gen_lk_gfh_header
	fi
}

do_deploy () {
	install -d ${DEPLOYDIR}
	install ${LK_OUT}/build-${LK_PROJECT}/${LK_BINARY} ${DEPLOYDIR}/${LK_BINARY}
	install ${WORKDIR}/${LK_IMAGE} ${DEPLOYDIR}/${LK_IMAGE}
}

addtask genkey before do_compile after do_configure
addtask genheader before do_deploy after do_compile
addtask deploy before do_build after do_compile

