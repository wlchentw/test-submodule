inherit workonsrc
inherit deploy

DESCRIPTION = "OPTEE TESTSUITE"
LICENSE = "BSD & GPLv2"
TZ_SRC = "${TOPDIR}/../src/bsp/trustzone"
WORKONSRC = "${TZ_SRC}/optee/3.2.0/source/optee_test"
WORKONSRC_BUILD = "${TZ_SRC}/optee/3.2.0/source/optee_test"
LIC_FILES_CHKSUM = "file://${S}/LICENSE.md;md5=daa2bcccc666345ab8940aab1315a4fa"
DEPENDS = "optee-client optee-os openssl"

OPTEE_CLIENT_EXPORT = "${STAGING_DIR_HOST}${prefix}"
OPTEE_OPENSSL_EXPORT = "${STAGING_INCDIR}"
TEEC_EXPORT         = "${STAGING_DIR_HOST}${prefix}"
TA_DEV_KIT_DIR      = "${STAGING_INCDIR}/optee/export-user_ta"

OPTEE_ARCH = "${TEE_ARCH}"
TEE_TOOLCHAIN ?= "${HOST_PREFIX}"

EXTRA_OEMAKE = " TA_DEV_KIT_DIR=${TA_DEV_KIT_DIR} \
                 OPTEE_CLIENT_EXPORT=${OPTEE_CLIENT_EXPORT} \
                 OPTEE_OPENSSL_EXPORT=${OPTEE_OPENSSL_EXPORT} \
                 TEEC_EXPORT=${TEEC_EXPORT} \
                 CROSS_COMPILE_HOST=${TARGET_PREFIX} \
                 CROSS_COMPILE_TA=${TEE_TOOLCHAIN} \
                 V=1 \
                 O=${WORKDIR}/out \
                 COMPILE_NS_USER=32 \
               "

do_compile() {
    # Top level makefile doesn't seem to handle parallel make gracefully
    oe_runmake xtest
    oe_runmake ta
}

do_install () {
    install -D -p -m0755 ${WORKDIR}/out/xtest/xtest ${D}${bindir}/xtest

    # install path should match the value set in optee-client/tee-supplicant
    # default TEEC_LOAD_PATH is /lib
    mkdir -p ${D}${nonarch_base_libdir}/optee_armtz/
    install -D -p -m0444 ${WORKDIR}/out/ta/*/*.ta ${D}${nonarch_base_libdir}/optee_armtz/
}

FILES_${PN} += "${nonarch_base_libdir}/optee_armtz/"

# Imports machine specific configs from staging to build
PACKAGE_ARCH = "${MACHINE_ARCH}"
