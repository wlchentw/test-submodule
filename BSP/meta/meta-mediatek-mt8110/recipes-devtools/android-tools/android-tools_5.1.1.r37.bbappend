FILESEXTRAPATHS_append := ":${THISDIR}/${PN}"

SRC_URI = " \
    git://${CORE_REPO};name=core;protocol=https;nobranch=1;destsuffix=git/system/core \
    git://${EXTRAS_REPO};name=extras;protocol=https;nobranch=1;destsuffix=git/system/extras \
    git://${LIBHARDWARE_REPO};name=libhardware;protocol=https;nobranch=1;destsuffix=git/hardware/libhardware \
    git://${LIBSELINUX_REPO};name=libselinux;protocol=https;nobranch=1;destsuffix=git/external/libselinux \
    git://${BUILD_REPO};name=build;protocol=https;nobranch=1;destsuffix=git/build \
    file://remove-selinux-android.patch \
    file://use-capability.patch \
    file://use-local-socket.patch \
    file://preserve-ownership.patch \
    file://mkbootimg-Add-dt-parameter-to-specify-DT-image.patch \
    file://remove-bionic-android.patch \
    file://define-shell-command.patch \
    file://implicit-declaration-function-strlcat-strlcopy.patch \
    file://fix-big-endian-build.patch \
    file://0001-add-base64-implementation.patch \
    file://0002-adb-Musl-fixes.patch \
    file://0003-adb-force-root.patch \
    file://0004-adb-enable-reboot.patch \
    file://android-tools-adbd.service \
    file://.gitignore;subdir=git \
    file://adb.mk;subdir=${BPN} \
    file://adbd.mk;subdir=${BPN} \
    file://ext4_utils.mk;subdir=${BPN} \
    file://fastboot.mk;subdir=${BPN} \
    file://mkbootimg.mk;subdir=${BPN} \
"

SRCREV_core = "81df1cc77722000f8d0025c1ab00ced123aa573c"
SRCREV_extras = "8e9fa76016a34ec53b83157032e38fad06af1135"
SRCREV_libhardware = "64a2b7534486c509d0e13b91b12d52b42166080e"
SRCREV_libselinux = "6608a1875b07370733d0f93f3a52febcef3442bf"
SRCREV_build = "04f82d5e3692231eb8bfaa57712841bfd76daa12"
