SUMMARY = "LIBION"
DESCRIPTION = "LIBION"

FILESEXTRAPATHS_append := ":${THISDIR}/files"

LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://NOTICE;md5=c1a3ff0b97f199c7ebcfdd4d3fed238e"

SRC_URI = " git://android.googlesource.com/platform/system/core;protocol=https \
	    file://0001-libion-Add-makefile.patch \
	    file://0002-libion-Add-MediaTek-ION-special-heap-and-command.patch \
	    file://0003-libion-fix-MUSL-libc-build-error.patch \
	    file://0004-libion-sync-struct-defination.patch \
"

SRCREV = "30f991f251940be3ed11566fb71139852286f68a"

S = "${WORKDIR}/git"

do_compile() {
	cd ${S}/libion && make
}

do_install() {
	cd ${S}/libion && make install DESTDIR="${D}" INCDIR="${includedir}" \
	SRCDIR="${S}" LIBDIR="${libdir}"
}

ALLOW_EMPTY_${PN} = "1"
FILES_${PN} = "${libdir} ${includedir}"
FILES_${PN}-dev = ""
INSANE_SKIP_${PN} += "ldflags"
INSANE_SKIP_${PN}-dev += "ldflags"

