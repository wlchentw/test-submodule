inherit workonsrc

DESCRIPTION = "Mediatek Modified Bluetooth"
LICENSE = "Apache-2.0 & MediaTekProprietary"
BLUETOOTH_TOOL_DIR = "${TOPDIR}/../src/connectivity/bt_others/bluetooth_tool"
LIC_FILES_CHKSUM = "file://${BLUETOOTH_TOOL_DIR}/LICENSE;md5=1bdf4d859920d6689a2e65700ad2c9a9"
DEPENDS += "alsa-lib zlib ${@bb.utils.contains('LICENSE_FLAGS_AAC', 'yes', 'aac', '' ,d)} ${@bb.utils.contains('BLE_MESH_ENABLE', 'yes', 'ble-mesh', '' ,d)} ${@bb.utils.contains('SUPPORT_MTK_BT_STEREO', 'yes', 'stereo-player', '' ,d)} bluetooth-vendor"
WORKONSRC = "${TOPDIR}/../src/connectivity/bt_others/bluetooth_tool"

inherit systemd
SYSTEMD_PACKAGES = "${PN}"

SYSTEMD_SERVICE_${PN} = "${@bb.utils.contains('IS_ROOT', 'yes', '76xx_root_', '${BT_LAUNCHER_SUFFIX}' ,d)}btservice.service"
FILES_${PN} += "${systemd_unitdir}/system/${@bb.utils.contains('IS_ROOT', 'yes', '76xx_root_', '${BT_LAUNCHER_SUFFIX}' ,d)}btservice.service"

do_compile() {
        echo $PWD
        echo bluetooth start compile
        echo ${WORKONSRC}
        export ENABLE_SYS_LOG="no"
        echo bluetooth start compile and sys log:$ENABLE_SYS_LOG

        if ${@bb.utils.contains('LICENSE_FLAGS_AAC', 'yes', 'true', 'false' ,d)}; then
            export SUPPORT_AAC="yes"
        else
            export SUPPORT_AAC="no"
        fi
        export SUPPORT_SPP="no"
        export SUPPORT_HIDH="no"
        export SUPPORT_HIDD="no"
        export SUPPORT_GATT="yes"
        export SUPPORT_AVRCP="yes"
        export SUPPORT_A2DP_SRC="yes"
        export SUPPORT_A2DP_ADEV="yes"
        export SUPPORT_A2DP_SINK="yes"
        export SUPPORT_HFP_CLIENT="yes"
        export SUPPORT_BT_WIFI_RATIO_SETTING="yes"
        export SUPPORT_DISPATCH_A2DP_WITH_PLAYBACK="yes"
        if ${@bb.utils.contains('BLE_MESH_ENABLE', 'yes', 'true', 'false' ,d)}; then
            export SUPPORT_BLE_MESH="yes"
            export SUPPORT_BLE_MESH_HEARTBEAT="yes"
        else
            export SUPPORT_BLE_MESH="no"
            export SUPPORT_BLE_MESH_HEARTBEAT="no"
        fi
        if ${@bb.utils.contains('SUPPORT_MTK_BT_STEREO', 'yes', 'true', 'false' ,d)}; then
            export SUPPORT_STEREO="yes"
        else
            export SUPPORT_STEREO="no"
        fi
        echo support stereo ? ${SUPPORT_STEREO}
        if ${@bb.utils.contains('SUPPORT_MTK_BT_MULTI_POINT', 'yes', 'true', 'false' ,d)}; then
            export SUPPORT_MULTI_POINT="yes"
        else
            export SUPPORT_MULTI_POINT="no"
        fi
        FOR_BT_VENDOR="no"
        cd ${WORKONSRC}/script
        echo MTK_BT_C4A = ${MTK_BT_C4A}
        /bin/bash generate_environment.sh ${COMBO_CHIP_ID} ${FOR_BT_VENDOR}
        if [ $? -ne 0 ]; then
            echo bluetooth generate environment fail!!
            exit 1
        fi
        cd ${S}
        echo support aac codec ? ${SUPPORT_AAC}
		echo STAGING_DIR_HOST=${STAGING_DIR_HOST}
        /bin/bash ${S}/script/yocto_build_bluetooth.sh ${TOPDIR} ${COMBO_CHIP_ID} ${MTK_PROJECT} ${STAGING_DIR_HOST} ${MTK_BT_C4A}
        if [ $? -ne 0 ]; then
            echo bluetooth compile fail!!
            exit 1
        fi
        echo bluetooth end compile
}

do_install() {
    install -d ${D}${libdir}
    install -m 755 ${WORKONSRC}/prebuilts/lib/* ${D}${libdir}/

    install -d ${D}/usr/bin
    install -m 0755 ${WORKONSRC}/prebuilts/bin/* ${D}/usr/bin/

    install -d ${D}/data/misc/bluedroid
    install -m 0644 ${WORKONSRC}/prebuilts/conf/* ${D}/data/misc/bluedroid/

    if ${@bb.utils.contains('DISTRO_FEATURES','systemd','true','false',d)}; then
        install -d ${D}${systemd_unitdir}/system/
        install -m 0644 ${B}/${@bb.utils.contains('IS_ROOT', 'yes', '76xx_root_', '${BT_LAUNCHER_SUFFIX}' ,d)}btservice.service ${D}${systemd_unitdir}/system
    fi
    /bin/bash ${S}/script/yocto_clean_bluetooth.sh ${TOPDIR} ${COMBO_CHIP_ID}
}

FILES_${PN} += "/data/misc/bluedroid ${libdir}"
FILES_${PN}-dev = ""
INSANE_SKIP_${PN} += "ldflags"
