do_configure_append() {
	if [ "${CONFIG_ROOTFS_CHECK}" = "no" ]; then
	sed -i -e 's/CONFIG_ROOTFS_CHECK=y/# CONFIG_ROOTFS_CHECK is not set/g' ${B}/.config
	fi
}
