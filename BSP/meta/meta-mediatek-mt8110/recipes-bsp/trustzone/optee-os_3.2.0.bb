inherit workonsrc
inherit deploy trustzone-build

DESCRIPTION = "OPTEE OS"
LICENSE = "BSD-2-Clause"
TZ_SRC = "${TOPDIR}/../src/bsp/trustzone"
WORKONSRC = "${TZ_SRC}/optee/3.2.0/optee_os"
WORKONSRC_BUILD = "${TZ_SRC}/optee/3.2.0/optee_os"
LIC_FILES_CHKSUM = "file://${S}/LICENSE;md5=69663ab153298557a59c67a60a743e5b"
OPTEEMACHINE = "mediatek"
OPTEEOUTPUTMACHINE = "mediatek"

OPTEE_ARCH = "${TEE_ARCH}"
TEE_TOOLCHAIN ?= "${HOST_PREFIX}"

TEE_OS_RAM_SIZE = "0x1d0000"

EXTRA_OEMAKE = "PLATFORM=${OPTEEMACHINE} \
                CFG_ARM64_core=y \
                CROSS_COMPILE64=${TEE_TOOLCHAIN} \
                NOWERROR=1 \
                ta-targets=ta_${OPTEE_ARCH} \
                LDFLAGS= \
                LIBGCC_LOCATE_CFLAGS=--sysroot=${STAGING_DIR_HOST} \
                CFLAGS='${CFLAGS} --sysroot=${STAGING_DIR_HOST}' \
                O=${WORKDIR}/out/${OPTEE_ARCH}-plat-${OPTEEOUTPUTMACHINE} \
               "

EXTRA_OEMAKE_append="PLATFORM_FLAVOR=${TZ_PLATFORM} \
                     CFG_TZDRAM_START=${TRUSTEDOS_ENTRYPOINT} \
                     CFG_TZDRAM_SIZE=${TRUSTEDOS_SIZE} \
                     CFG_SHMEM_START=${TRUSTEDOS_ENTRYPOINT}+CFG_TZDRAM_SIZE \
                     CFG_SHMEM_SIZE=${TRUSTEDOS_SHMEM_SIZE} \
                     CFG_TEE_RAM_VA_SIZE=${TEE_OS_RAM_SIZE} \
                     MTK_PROJECT=${MTK_PROJECT} \
                    "

do_compile () {
    oe_runmake all
}

do_install() {
    #install core on boot directory
    install -d ${D}${nonarch_base_libdir}/firmware/

    install -m 644 ${WORKDIR}/out/${OPTEE_ARCH}-plat-${OPTEEOUTPUTMACHINE}/core/*.bin ${D}${nonarch_base_libdir}/firmware/
    #install TA devkit
    install -d ${D}/usr/include/optee/export-user_ta/

    for f in  ${WORKDIR}/out/${OPTEE_ARCH}-plat-${OPTEEOUTPUTMACHINE}/export-ta_${OPTEE_ARCH}/* ; do
        cp -aR  $f  ${D}/usr/include/optee/export-user_ta/
    done
}

do_deploy() {
    install -d ${DEPLOYDIR}/optee
    for f in ${D}${nonarch_base_libdir}/firmware/*; do
        install -m 644 $f ${DEPLOYDIR}/optee/
    done
    install ${DEPLOYDIR}/optee/tee-pager.bin ${TZ_ASSEMBLE_OUT}/${TZ_RAW_BINARY}
    install ${DEPLOYDIR}/optee/tee-pager.bin ${TZ_ASSEMBLE_OUT}/${TRUSTEDOS_RAW_BINARY}
}

addtask deploy before do_build after do_install

FILES_${PN} = "${nonarch_base_libdir}/firmware/"
FILES_${PN}-dev = "/usr/include/optee"

INSANE_SKIP_${PN}-dev = "staticdev"
