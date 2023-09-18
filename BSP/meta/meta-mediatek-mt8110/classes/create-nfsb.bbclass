NFSBVERITY="/sbin/veritysetup"
NFSB_TOOLS_DIR="${TOPDIR}/../src/devtools/nfsb"
MKNFSBIMG="${NFSB_TOOLS_DIR}/mknfsbimg3"
ZERO_PADDING_TOOL="${NFSB_TOOLS_DIR}/zero_padding.sh"
NFSB_WORKING_PATH="${IMGDEPLOYDIR}"
NFSB_BLOCK_SIZE="1024"
OLD_ROOTFS_NAME="${IMAGE_NAME}.rootfs.${IMAGE_FSTYPES}"
OLD_RECOVERY_ROOTFS_NAME="recovery.${IMAGE_FSTYPES}"
NEW_ROOTFS_NAME="nfsb_rootfs.${IMAGE_FSTYPES}"
NEW_RECOVERY_ROOTFS_NAME = "tmp_recovery.${IMAGE_FSTYPES}"
ZERO_PADDING_SIZE="1048576"
KEY_FILE_MOD="${NFSB_TOOLS_DIR}/rsa.key.pub_out"
KEY_FILE_PRI="${NFSB_TOOLS_DIR}/rsa.key.pri_out"
DST_IMG_KEY_FILE = "${TOPDIR}/../src/devtools/nfsb/rsa.key"

add_nfsb_for_rootfs() {

       mod_key=""
       pri_key=""
       SRC_IMG_KEY_FILE=""
       DM_VERITY_ALG=""

       if [ "${STANDALONE_SIGN_PREPARE}" = "yes" ]; then
           exit 0
       fi

       if [ "${ROOTFS_VERITY_KEY}" = "" ]; then
              SRC_IMG_KEY_FILE="${MTK_KEY_DIR}/${VERIFIED_KEY}.pem"
       else
              SRC_IMG_KEY_FILE="${MTK_KEY_DIR}/${ROOTFS_VERITY_KEY}.pem"
       fi

       if [ "${DM_NFSB_HASH_ALG}" = "" ]; then
              DM_VERITY_ALG="md5"
       else
              DM_VERITY_ALG="${DM_NFSB_HASH_ALG}"
       fi

       #backwards compatible for FORCE_DISABLE_DM_VERITY which only for NFSB actually
       if [ "${SECURE_BOOT_ENABLE}" = "yes" ] && [ "${SECURE_BOOT_TYPE}" = "" ]; then
              ENABLE_DM_NFSB="yes"
       fi
       if [ "${FORCE_DISABLE_DM_VERITY}" = "yes" ]; then
              ENABLE_DM_NFSB="no"
       fi

       if [ "${SECURE_BOOT_ENABLE}" = "yes" ] && [ "${ENABLE_DM_NFSB}" == "yes" ]; then
              if [ -e ${KEY_FILE_MOD} ]; then
                     mod_key=${KEY_FILE_MOD}
                     pri_key=${KEY_FILE_PRI}
              else
                     if [ -e ${SRC_IMG_KEY_FILE} ]; then
                         cp ${SRC_IMG_KEY_FILE} ${DST_IMG_KEY_FILE}
                         python ${TOPDIR}/../src/devtools/nfsb/pretreat-key.py ${TOPDIR} ${SRC_IMG_KEY_FILE}
                         mod_key=${KEY_FILE_MOD}
                         pri_key=${KEY_FILE_PRI}
                     else
                         bbfatal "${KEY_FILE_MOD} does not exist!"
                     fi
              fi
       else
              exit 0
       fi

       ${NFSBVERITY} --hash=${DM_VERITY_ALG} --no-superblock --data-block-size=${NFSB_BLOCK_SIZE} --hash-block-size=${NFSB_BLOCK_SIZE} format ${NFSB_WORKING_PATH}/${OLD_ROOTFS_NAME} ${NFSB_WORKING_PATH}/rootfs_hashes | tee ${NFSB_WORKING_PATH}/rootfs_table
       ${MKNFSBIMG} ${NFSB_WORKING_PATH}/${OLD_ROOTFS_NAME} ${NFSB_WORKING_PATH}/rootfs_hashes ${NFSB_WORKING_PATH}/${NEW_ROOTFS_NAME} ${NFSB_WORKING_PATH}/rootfs_table ${mod_key} ${pri_key};
       rm -f ${NFSB_WORKING_PATH}/${OLD_ROOTFS_NAME}
       mv ${NFSB_WORKING_PATH}/${NEW_ROOTFS_NAME} ${NFSB_WORKING_PATH}/${OLD_ROOTFS_NAME}
       ${ZERO_PADDING_TOOL} ${NFSB_WORKING_PATH}/${OLD_ROOTFS_NAME} ${ZERO_PADDING_SIZE}
       rm -f ${NFSB_WORKING_PATH}/rootfs_hashes
       rm -f ${NFSB_WORKING_PATH}/rootfs_table

       if [ -e ${NFSB_WORKING_PATH}/${OLD_RECOVERY_ROOTFS_NAME} ]; then
              ${NFSBVERITY} --hash=md5 --no-superblock --data-block-size=${NFSB_BLOCK_SIZE} --hash-block-size=${NFSB_BLOCK_SIZE} format ${NFSB_WORKING_PATH}/${OLD_RECOVERY_ROOTFS_NAME} ${NFSB_WORKING_PATH}/recovery_rootfs_hashes | tee ${NFSB_WORKING_PATH}/recovery_rootfs_table
              ${MKNFSBIMG} ${NFSB_WORKING_PATH}/${OLD_RECOVERY_ROOTFS_NAME} ${NFSB_WORKING_PATH}/recovery_rootfs_hashes ${NFSB_WORKING_PATH}/${NEW_RECOVERY_ROOTFS_NAME} ${NFSB_WORKING_PATH}/recovery_rootfs_table ${mod_key} ${pri_key};
              rm -f ${NFSB_WORKING_PATH}/${OLD_RECOVERY_ROOTFS_NAME}
              mv ${NFSB_WORKING_PATH}/${NEW_RECOVERY_ROOTFS_NAME} ${NFSB_WORKING_PATH}/${OLD_RECOVERY_ROOTFS_NAME}
              ${ZERO_PADDING_TOOL} ${NFSB_WORKING_PATH}/${OLD_RECOVERY_ROOTFS_NAME} ${ZERO_PADDING_SIZE}
              rm -f ${NFSB_WORKING_PATH}/recovery_rootfs_hashes
              rm -f ${NFSB_WORKING_PATH}/recovery_rootfs_table
       fi

       rm -f ${mod_key}
       rm -f ${pri_key}
}

IMAGE_POSTPROCESS_COMMAND += " add_nfsb_for_rootfs;"
