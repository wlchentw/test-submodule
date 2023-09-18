FILESEXTRAPATHS_prepend := "${THISDIR}/${PN}/${MTK_PROJECT}:"

do_install_append() {
    ln -sf ../proc/self/mounts ${D}${sysconfdir}/mtab
}
