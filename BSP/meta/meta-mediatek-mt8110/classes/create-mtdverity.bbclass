MTDVERITY_TOOL_DIR="${TOPDIR}/../src/devtools/nfsb/mtd_verity"
MTDVERITY_TOOL="${MTDVERITY_TOOL_DIR}/mtd_verity"
MTDVERITY_WORKING_PATH="${DEPLOY_DIR_IMAGE}"
OLD_ROOTFS_NAME="${IMAGE_NAME}.rootfs.${IMAGE_FSTYPES}"
NEW_ROOTFS_NAME="mtdverity_system.${IMAGE_FSTYPES}"
IMG_KEY_FILE = "${MTK_KEY_DIR}/${VERIFIED_KEY}.pem"

do_mtdverity_for_rootfs() {

       if [ "${SECURE_BOOT_ENABLE}" = "yes" ] && [ "${ENABLE_ROOTFS_CHECK}" = "yes" ]; then
			if [ "${STANDALONE_SIGN_PREPARE}" = "yes" ];then
				exit 0
			fi
                       ${MTDVERITY_TOOL} ${MTDVERITY_WORKING_PATH}/${OLD_ROOTFS_NAME} 1048576 1048576 10 ${IMG_KEY_FILE} ${MTDVERITY_WORKING_PATH}/${NEW_ROOTFS_NAME}
                       rm -f ${MTDVERITY_WORKING_PATH}/${OLD_ROOTFS_NAME}
                       mv ${MTDVERITY_WORKING_PATH}/${NEW_ROOTFS_NAME} ${MTDVERITY_WORKING_PATH}/${OLD_ROOTFS_NAME}
       else
              exit 0
       fi
}

#do_image_complete[postfuncs] += "do_mtdverity_for_rootfs"
do_rootfs[postfuncs] += "do_mtdverity_for_rootfs"
