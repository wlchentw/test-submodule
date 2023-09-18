FILESEXTRAPATHS_prepend := "${THISDIR}/${PN}:"

SRC_URI_append = "file://0001-fix-compiling-mtd_probe.c-fail-and-including-mtd_pro.patch"
