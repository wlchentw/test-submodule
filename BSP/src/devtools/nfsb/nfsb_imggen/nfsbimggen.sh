#!/bin/bash

TOOLDIR=$(cd `dirname $0`; pwd)
NFSBVERITY="/sbin/veritysetup"
NFSB_TOOLS_DIR="${TOOLDIR}"
MKNFSBIMG="${NFSB_TOOLS_DIR}/mknfsbimg3"
ZERO_PADDING_TOOL="${NFSB_TOOLS_DIR}/zero_padding.sh"
NFSB_BLOCK_SIZE="1024"
ZERO_PADDING_SIZE="1048576"
KEY_FILE_MOD="${NFSB_TOOLS_DIR}/rsa.key.pub_out"
KEY_FILE_PRI="${NFSB_TOOLS_DIR}/rsa.key.pri_out"
NFSB_WORKING_PATH="${NFSB_TOOLS_DIR}"

KEY_FILE_PATH=$1
SRC_FILE_PATH=$2
DST_FILE_PATH=$3

mod_key=${KEY_FILE_MOD}
pri_key=${KEY_FILE_PRI}

python ${NFSB_TOOLS_DIR}/pretreat-key.py ${NFSB_WORKING_PATH} ${KEY_FILE_PATH}

${NFSBVERITY} --hash=sha256 --no-superblock --data-block-size=${NFSB_BLOCK_SIZE} --hash-block-size=${NFSB_BLOCK_SIZE} format ${SRC_FILE_PATH} ${NFSB_WORKING_PATH}/rootfs_hashes | tee ${NFSB_WORKING_PATH}/rootfs_table
${MKNFSBIMG} ${SRC_FILE_PATH} ${NFSB_WORKING_PATH}/rootfs_hashes ${DST_FILE_PATH} ${NFSB_WORKING_PATH}/rootfs_table ${mod_key} ${pri_key};
${ZERO_PADDING_TOOL} ${DST_FILE_PATH} ${ZERO_PADDING_SIZE}
rm -f ${NFSB_WORKING_PATH}/rootfs_hashes
rm -f ${NFSB_WORKING_PATH}/rootfs_table
rm -f ${mod_key}
rm -f ${pri_key}
