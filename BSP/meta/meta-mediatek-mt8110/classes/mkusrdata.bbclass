STATE_DIR_IMAGE_SIZE ?= "${MKUBIFS_USRDATA_IMAGE_SZ}"
STATE_DIR_IMAGE_PATH = "${DEPLOY_DIR_IMAGE}/userdata"
STATE_DIR_CONTENT = "${IMAGE_ROOTFS}/../usrdata"
#IMAGE_FSTYPES_NO_WHITESPACE="$(echo "${IMAGE_FSTYPES}" | tr -d '[:space:]')"
IMAGE_FSTYPES_NO_WHITESPACE="$(echo "${USERDATA_IMAGE_FSTYPES}" | tr -d '[:space:]')"
USERDATA_FS_PARTITION = "userdata_fs.ubi"
USERDATA_PARTITION = "userdata.ubi"
CRYPT_HOST_TOOL = "${TOPDIR}/../src/devtools/nfsb/crypt_tool/crypt_host_tool"
DEPENDS += "mtd-utils-native"
mk_userdata_image() {
	mkdir -p ${STATE_DIR_CONTENT}/tmp
	mkdir -p ${STATE_DIR_CONTENT}/etc
	mkdir -p ${STATE_DIR_CONTENT}/var
	mkdir -p ${STATE_DIR_CONTENT}/usr
	mkdir -p ${STATE_DIR_CONTENT}/usr/bin
	mkdir -p ${STATE_DIR_CONTENT}/usr/sbin
	mkdir -p ${STATE_DIR_CONTENT}/config

	if [ -d ${IMAGE_ROOTFS}/data ]; then
	  cp -a ${IMAGE_ROOTFS}/data/* ${STATE_DIR_CONTENT}
	  rm -rf ${IMAGE_ROOTFS}/data/*
	fi
	if test "${CONFIG_ICU_SUPPORT}" = "no"; then
		rm -fr ${IMAGE_ROOTFS}/usr/lib/libicudata.so.59.1
		rm -fr ${IMAGE_ROOTFS}/usr/lib/libicui18n.so.59.1
		rm -fr ${IMAGE_ROOTFS}/usr/lib/libicuuc.so.59.1
	fi

	rm -rf ${STATE_DIR_IMAGE_PATH}

	install -d ${STATE_DIR_CONTENT}

  for i in ${STATE_DIR_CONTENT}; do
    STATE_DIR_FILE_PATH=${STATE_DIR_IMAGE_PATH}/${i##${STATE_DIR_CONTENT}}
    install -d ${STATE_DIR_FILE_PATH}
    if [ "$(ls -A ${i})" ]; then
	cp -a ${i}/* ${STATE_DIR_FILE_PATH}
    else
	echo "${i} is empty"
    fi
  done

  if test "${IMAGE_FSTYPES_NO_WHITESPACE}" = "ubi"; then
	echo \[ubifs\] > ubinize.cfg
	echo mode=ubi >> ubinize.cfg
	echo image=${USERDATA_FS_PARTITION} >> ubinize.cfg
	echo vol_id=0 >> ubinize.cfg
	echo vol_size=${MKUBIFS_USRDATA_VOL_SZ}
	echo vol_type=dynamic >> ubinize.cfg
	echo vol_name=useradata >> ubinize.cfg
	echo vol_flags=autoresize >> ubinize.cfg
	dd if=/dev/zero of=${DEPLOY_DIR_IMAGE}/${USERDATA_FS_PARTITION} seek=${STATE_DIR_IMAGE_SIZE} count=0 bs=1k
	mkfs.ubifs -r ${STATE_DIR_FILE_PATH} ${MKUBIFS_USRDATA_ARGS} -o ${USERDATA_FS_PARTITION}
	ubinize -o ${USERDATA_PARTITION} ${UBINIZE_ARGS} ubinize.cfg
	cp ${USERDATA_PARTITION} ${DEPLOY_DIR_IMAGE}/${USERDATA_PARTITION}
  elif test "${IMAGE_FSTYPES_NO_WHITESPACE}" = "ext4"; then
	STATE_PARTITION="userdata.ext4"
	dd if=/dev/zero of=${DEPLOY_DIR_IMAGE}/${STATE_PARTITION} seek=${STATE_DIR_IMAGE_SIZE} count=${MKEXT4_USRDATA_CNT} bs=1k
	mkfs.ext4 -F -i 4096 -b 4096 ${DEPLOY_DIR_IMAGE}/${STATE_PARTITION} -d ${STATE_DIR_IMAGE_PATH}
	if test "${ENABLE_USRDATA_DM_CRYPT}" = "yes"; then
		mv ${DEPLOY_DIR_IMAGE}/${STATE_PARTITION} ${DEPLOY_DIR_IMAGE}/plain_${STATE_PARTITION}
		${CRYPT_HOST_TOOL} ${DM_CRYPT_ALG} `cat ${MTK_KEY_DIR}/${DM_CRYPT_KEY}` 0 0 ${DEPLOY_DIR_IMAGE}/plain_${STATE_PARTITION} ${DEPLOY_DIR_IMAGE}/${STATE_PARTITION} usrdata `cat ${MTK_KEY_DIR}/${DM_PROTECT_KEY}`
	fi
  else
	echo "No method to make ${IMAGE_FSTYPES_NO_WHITESPACE} type state image"
  fi
}

ROOTFS_POSTPROCESS_COMMAND += " mk_userdata_image;"


