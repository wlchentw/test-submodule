inherit workonsrc

DESCRIPTION = "MTK Bluetooth Vendor_lib Picus Boots"
LICENSE = "MediaTekProprietary"
LIC_FILES_CHKSUM = "file://LICENSE;md5=1bdf4d859920d6689a2e65700ad2c9a9"
DEPENDS += "${@bb.utils.contains_any('COMBO_CHIP_ID', 'MT7663 mt7663 mt7668 mt7662', '', 'zlib', d)}"
DEPENDS += "${@bb.utils.contains_any('BT_NVRAM_ENABLE', 'no', '', 'nvram', d)}"

WORKONSRC = "${TOPDIR}/../src/connectivity/bt_others/bluetooth_tool"

do_compile() {
    echo $PWD
    echo bluetooth vendor start compile
    echo ${WORKONSRC}

    export ENABLE_SYS_LOG="no"
    export MTK_BT_VCOM_OPENED=${BT_BOOTS_VCOM_OPENED}
    if ${@bb.utils.contains_any('TARGET_PLATFORM', 'mt8133 mt8512a mt8110', 'true', 'false' ,d)}; then
        export USB_UDC_TYPE="SSUSB"
    else
        export USB_UDC_TYPE="MUSB"
    fi
    FOR_BT_VENDOR="yes"
    cd ${WORKONSRC}/script
    /bin/bash generate_environment.sh ${COMBO_CHIP_ID} ${FOR_BT_VENDOR}
    if [ $? -ne 0 ]; then
        echo bluetooth generate environment fail!!
        exit 1
    fi
    cd ${S}
	echo vendor lib STAGING_DIR_HOST=${STAGING_DIR_HOST}
    /bin/bash ${S}/script/yocto_build_vendor.sh ${TOPDIR} ${COMBO_CHIP_ID} ${MTK_PROJECT} ${STAGING_DIR_HOST} ${BT_NVRAM_ENABLE}
    if [ $? -ne 0 ]; then
        echo bluetooth vendor compile fail!!
        exit 1
    fi
    echo bluetooth vendor end compile
}

do_install() {
    install -d ${D}${libdir}
    install -m 755 ${WORKONSRC}/vendor_prebuilts/lib/* ${D}${libdir}/

    install -d ${D}/usr/bin
    install -m 0755 ${WORKONSRC}/vendor_prebuilts/bin/* ${D}/usr/bin/

    /bin/bash ${S}/script/yocto_clean_vendor.sh ${TOPDIR} ${COMBO_CHIP_ID}
}

FILES_${PN} += "${libdir}"
FILES_${PN}-dev = ""
INSANE_SKIP_${PN} += "ldflags"