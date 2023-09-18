inherit kernel externalsrc kernel-fitimage-extension recovery-kernel-fitimage

DEPENDS_append_aarch64 = " libgcc"
KERNEL_CC_append_aarch64 = " ${TOOLCHAIN_OPTIONS}"
KERNEL_LD_append_aarch64 = " ${TOOLCHAIN_OPTIONS}"

LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://COPYING;md5=d7810fab7487fb0aad327b76f1be7cd7"
EXTERNALSRC = "${KERNEL_SRC}"
STAGING_KERNEL_DIR = "${KERNEL_SRC}"
LINUX_VERSION_EXTENSION = "-custom"
KERNEL_EXTRA_ARGS = "dtbs ${KERNEL_DEVICETREE}"
KERNEL_CONFIG_COMMAND = "oe_runmake_call -C ${S} O=${B} ${KBUILD_DEFCONFIG}"

DST_IMG_KEY_FILE = "${TOPDIR}/../src/devtools/nfsb/rsa.key"
MTD_DST_IMG_KEY_FILE = "${TOPDIR}/../src/devtools/nfsb/mtd_verity/rsa.key"

do_configure_prepend() {
	install -d ${TMPDIR}/work-shared/${MACHINE}
	ln -nfs ${STAGING_KERNEL_DIR} ${TMPDIR}/work-shared/${MACHINE}/kernel-source
}

ntx_get_ver_str() {
	SRC_DIR="$1"
	wr_ver_file="$2"
	WORK_DIR="$(pwd)"
	cd "${SRC_DIR}"
	if [ $? = 0 ];then
		if [ "$(git status -s |grep ".[MDTAC][[:blank:]]")" ];then
			stat="M$(date +%m%d%H%M%S)"
		else
			stat="B$(date +%m%d%H%M%S)"
		fi
		rev_string="$(git log -1 --pretty=format:%h-%cI)"
		rev_and_date="$(echo "${rev_string}"|sed -e "s/T.*$//")"
		rev_times="$(echo "${rev_string}"|sed -e "s/^.*T//"|sed -e s/+.*$//)"
		rev="$(echo "${rev_and_date}"|awk -F- '{print $1}')"
		rev_year="$(echo "${rev_and_date}"|awk -F- '{print $2}')"
		rev_month="$(echo "${rev_and_date}"|awk -F- '{print $3}')"
		rev_day="$(echo "${rev_and_date}"|awk -F- '{print $4}')"
		rev_hour="$(echo "${rev_times}"|awk -F: '{print $1}')"
		rev_min="$(echo "${rev_times}"|awk -F: '{print $2}')"
		rev_sec="$(echo "${rev_times}"|awk -F: '{print $3}')"
		ver="${rev}-${rev_year}${rev_month}${rev_day}T${rev_hour}${rev_min}${rev_sec}-${stat}"
		cd "${WORK_DIR}"
		scm_info=1
	else 
		echo "\"${SRC_DIR}\" not exist !" 1>&2
		scm_info=0
	fi

	if [ ${scm_info} = 0 ];then
		ver="non-scm-$(date +%m%d%H%M%S)"
	fi

	if [ -z "${wr_ver_file}" ];then
		echo "$ver"
	else
		echo "$ver" > "${wr_ver_file}"
	fi

	return 0
}

do_compile_prepend() {

       SRC_IMG_KEY_FILE=""

       if [ "${ROOTFS_VERITY_KEY}" = "" ]; then
              SRC_IMG_KEY_FILE="${MTK_KEY_DIR}/${VERIFIED_KEY}.pem"
       else
              SRC_IMG_KEY_FILE="${MTK_KEY_DIR}/${ROOTFS_VERITY_KEY}.pem"
       fi

       #backwards compatible for FORCE_DISABLE_DM_VERITY which only for NFSB actually
       if [ "${SECURE_BOOT_ENABLE}" = "yes" ] && [ "${SECURE_BOOT_TYPE}" = "" ]; then
              ENABLE_DM_NFSB="yes"
       fi
       if [ "${FORCE_DISABLE_DM_VERITY}" = "yes" ]; then
              ENABLE_DM_NFSB="no"
       fi
       echo "SECURE_BOOT_ENABLE is ${SECURE_BOOT_ENABLE},SECURE_BOOT_TYPE is ${SECURE_BOOT_TYPE}"
       if [ "${SECURE_BOOT_ENABLE}" = "yes" ] && [ "${SECURE_BOOT_TYPE}" = "avb" ]; then
              ENABLE_DM_VERITY="yes"
       fi
       echo "ENABLE_DM_VERITY is ${ENABLE_DM_VERITY}"
       if [ "${SECURE_BOOT_ENABLE}" = "yes" ] && [ "${ENABLE_DM_NFSB}" = "yes" ]; then
              if ! grep -Fxq "CONFIG_DM_NFSB=y" ${B}/.config ; then
                     echo "Error: If SECURE_BOOT_ENABLE is set to yes,kernel must be configed CONFIG_DM_NFSB=y"
                     exit 1
              fi

              if test -e ${SRC_IMG_KEY_FILE}; then
                     cp ${SRC_IMG_KEY_FILE} ${DST_IMG_KEY_FILE}
                     python ${TOPDIR}/../src/devtools/nfsb/pretreat-key.py ${TOPDIR} ${KERNEL_SRC}
              fi
       else
              if grep -Fxq "CONFIG_DM_NFSB=y" ${B}/.config ; then
                     echo "Error: If SECURE_BOOT_ENABLE is set to no,kernel must be configed CONFIG_DM_NFSB=n"
                     exit 1
              fi
       fi

       if [ "${SECURE_BOOT_ENABLE}" = "yes" ] && [ "${ENABLE_DM_VERITY}" = "yes" ]; then
            if ! grep -Fxq "CONFIG_DM_VERITY=y" ${B}/.config ; then
                     echo "Error: If SECURE_BOOT_ENABLE is set to yes,kernel must be configed CONFIG_DM_VERITY=y"
                     exit 1
            fi
       fi

       if [ "${ENABLE_MTD_VERITY}" = "yes" ]; then
           echo "ENABLE_MTD_VERITY is abandoned, plese use ENABLE_ROOTFS_CHECK in [project].config and CONFIG_ROOTFS_CHECK in kernel config"
           exit 1
       fi

       if [ "${SECURE_BOOT_ENABLE}" = "yes" ] && [ "${ENABLE_ROOTFS_CHECK}" = "yes" ]; then
              if ! grep -Fxq "CONFIG_ROOTFS_CHECK=y" ${B}/.config ; then
                     echo "Error: If SECURE_BOOT_ENABLE and ENABLE_ROOTFS_CHECK is set to yes,kernel must be configed CONFIG_ROOTFS_CHECK=y"
                     exit 1
              fi

              if test -e ${SRC_IMG_KEY_FILE}; then
                     cp ${SRC_IMG_KEY_FILE} ${MTD_DST_IMG_KEY_FILE}
                     python ${TOPDIR}/../src/devtools/nfsb/mtd_verity/pretreat-key.py ${TOPDIR} ${KERNEL_SRC}
              fi
       else
              if grep -Fxq "CONFIG_ROOTFS_CHECK=y" ${B}/.config ; then
                     echo "Error: If SECURE_BOOT_ENABLE and ENABLE_ROOTFS_CHECK is set to no,kernel must be configed CONFIG_ROOTFS_CHECK=n"
                     exit 1
              fi
       fi

       if [ "${ENABLE_USRDATA_DM_CRYPT}" = "yes" ]; then
              if ! grep -Fxq "CONFIG_DM_CRYPT=y" ${B}/.config ; then
                     echo "Error: If ENABLE_USRDATA_DM_CRYPT is set to yes,kernel must be configed CONFIG_DM_CRYPT=y"
                     exit 1
              fi
       fi

	   ntx_get_ver_str "${KERNEL_SRC}" "${S}/.ntx_version"
	   cp "${S}/.ntx_version" "${B}/.ntx_version"
	   NTX_VER_STR="$(cat ${B}/.ntx_version)"
	   echo "NTX_VER_STR=\"${NTX_VER_STR}\",S=\"${S}\",D=\"${D}\",B=\"${B}\"" 

}

do_populate_sysroot(){
:
}

do_package(){
:
}

do_package_qa(){
:
}

do_package_write_rpm(){
:
}

