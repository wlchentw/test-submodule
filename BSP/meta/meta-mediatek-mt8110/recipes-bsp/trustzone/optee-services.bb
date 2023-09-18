inherit workonsrc
inherit trustzone-build

SUMMARY = "OP-TEE services"
LICENSE = "BSD-2-Clause & MediaTekProprietary"
LIC_FILES_CHKSUM = "file://${S}/LICENSE;md5=cb3d385c0d64a57fad9b9e0e9029f607"

DEPENDS = "optee-client optee-os "

TZ_SRC = "${TOPDIR}/../src/bsp/trustzone"
WORKONSRC = "${TZ_SRC}/optee/3.2.0/source/optee_services"

OPTEE_CLIENT_EXPORT = "${STAGING_DIR_HOST}${prefix}"
TEEC_EXPORT = "${STAGING_DIR_HOST}${prefix}"
TA_DEV_KIT_DIR = "${STAGING_INCDIR}/optee/export-user_ta"

INSANE_SKIP_${PN} = "installed-vs-shipped"
SOLIBS = ".so"
FILES_SOLIBSDEV = ""
FILES_${PN} += "${nonarch_base_libdir}/optee_armtz/"

OPTEE_ARCH = "${TEE_ARCH}"
TEE_TOOLCHAIN ?= "${HOST_PREFIX}"

EXTRA_OEMAKE = " TA_DEV_KIT_DIR=${TA_DEV_KIT_DIR} \
                 OPTEE_CLIENT_EXPORT=${OPTEE_CLIENT_EXPORT} \
                 TEEC_EXPORT=${TEEC_EXPORT} \
                 HOST_CROSS_COMPILE=${TARGET_PREFIX} \
                 TA_CROSS_COMPILE=${TEE_TOOLCHAIN} \
                 V=1 \
               "

do_compile() {
    oe_runmake
}

do_install () {
    install -d ${D}${libdir}
    install -d ${D}${bindir}
    install -d ${D}${includedir}
    install -d ${S}/out/ta
    oe_runmake install LIBDIR="${D}${libdir}" BINDIR="${D}${bindir}" INCDIR="${D}${includedir}" TADIR="${S}/out/ta"

    mkdir -p ${D}${nonarch_base_libdir}/optee_armtz
    install -D -p -m0444 ${S}/out/ta/* ${D}${nonarch_base_libdir}/optee_armtz
}