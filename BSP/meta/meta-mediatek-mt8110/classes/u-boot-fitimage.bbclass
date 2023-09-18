
python __anonymous () {
        depends = d.getVar("DEPENDS", True)
        depends = "%s u-boot-mkimage-native" % depends
        d.setVar("DEPENDS", depends)
}

#
# Emit the fitImage ITS header
#
fitimage_emit_fit_header() {
        cat << EOF >> ${WORKDIR}/fit-image.its
/dts-v1/;

/ {
        description = "U-Boot fitImage for ${DISTRO_NAME}/${PV}/${MACHINE}";
        #address-cells = <1>;
EOF
}

#
# Emit the fitImage section bits
#
# $1 ... Section bit type: imagestart - image section start
#                          confstart  - configuration section start
#                          sectend    - section end
#                          fitend     - fitimage end
#
fitimage_emit_section_maint() {
        case $1 in
        imagestart)
                cat << EOF >> ${WORKDIR}/fit-image.its

        images {
EOF
        ;;
        confstart)
                cat << EOF >> ${WORKDIR}/fit-image.its

        configurations {
EOF
        ;;
        sectend)
                cat << EOF >> ${WORKDIR}/fit-image.its
        };
EOF
        ;;
        fitend)
                cat << EOF >> ${WORKDIR}/fit-image.its
};
EOF
        ;;
        esac
}

#
# Emit the fitImage ITS u-boot section
#
# $1 ... Path to u-boot image
# $2 ... Compression type
fitimage_emit_section_uboot() {

        uboot_csum="sha256"

        cat << EOF >> ${WORKDIR}/fit-image.its
                kernel@1 {
                        description = "U-Boot";
                        data = /incbin/("${1}");
                        type = "kernel";
                        arch = "${BL33_ARCH}";
                        os = "linux";
                        compression = "${2}";
                        load = <${UBOOT_FIT_LOADADDRESS}>;
                        entry = <${UBOOT_FIT_ENTRYPOINT}>;
                        hash@1 {
                                algo = "${uboot_csum}";
                        };
                };
EOF
}

#
# Emit the fitImage ITS DTB section
#
# $1 ... Image counter
# $2 ... Path to DTB image
fitimage_emit_section_dtb() {

        dtb_csum="sha256"

        cat << EOF >> ${WORKDIR}/fit-image.its
		        fdt@1 {
		                description = "sig blob for u-boot verified boot";
		                data = /incbin/("${1}");
		                type = "kernel";
		                arch = "arm";
		                os = "linux";
		                compression = "${2}";
		                load = <${UBOOT_DTB_LOADADDRESS}>;
		                entry = <${UBOOT_DTB_LOADADDRESS}>;
		                hash@1 {
		                        algo = "${dtb_csum}";
		                };
		        };
EOF
}

#
# Emit the fitImage ITS configuration section
#
# $1 ... u-boot image ID
fitimage_emit_section_config() {

        conf_csum="sha256,rsa2048"
        conf_key_name="dev"

        conf_desc="${MTK_PROJECT} configuration"

        uboot_line="kernel = \"kernel@1\";"
        if [ "${SECURE_BOOT_ENABLE}" = "yes" ]; then
            fdt_line="fdt = \"fdt@1\";"
        else
            fdt_line=""
        fi

        cat << EOF >> ${WORKDIR}/fit-image.its
                default = "conf@1";
                conf@1 {
                        description = "${conf_desc}";
                        ${uboot_line}
                        ${fdt_line}
                        signature@1 {
                                algo = "${conf_csum}";
                                key-name-hint="${conf_key_name}";
                                sign-images="fdt","kernel";
                        };
                };
EOF
}

do_assemble_fitimage() {

                rm -f ${WORKDIR}/fit-image.its

                fitimage_emit_fit_header

                #
                # Step 1: Prepare a u-boot image section.
                #
                fitimage_emit_section_maint imagestart

                fitimage_emit_section_uboot ${UBOOT_OUT}/${UBOOT_BINARY} ${UBOOT_COMPRESS}

                if [ "${SECURE_BOOT_ENABLE}" = "yes" ]; then
                    fitimage_emit_section_dtb ${UBOOT_OUT}/sig_blob.dtb none
                fi

                fitimage_emit_section_maint sectend

                #
                # Step 2: Prepare a configurations section
                #
                fitimage_emit_section_maint confstart

                fitimage_emit_section_config

                fitimage_emit_section_maint sectend

                fitimage_emit_section_maint fitend

                #
                # Step 3: Assemble the image
                #
                dtc -p 0x3ff ${WORKDIR}/u-boot.dts -O dtb -o ${UBOOT_OUT}/sig_blob.dtb
                ${UBOOT_OUT}/tools/mkimage -f ${WORKDIR}/fit-image.its ${UBOOT_OUT}/${UBOOT_FIT_IMAGE}

                if [ "${SECURE_BOOT_ENABLE}" = "yes" ]; then
				if [ "${STANDALONE_SIGN_PREPARE}" = "yes" ]; then
					exit 0
				fi
                        mkdir -p ./mykeys
                        cp ${MTK_KEY_DIR}/${VERIFIED_KEY}.crt ./mykeys/dev.crt
                        cp ${MTK_KEY_DIR}/${VERIFIED_KEY}.pem ./mykeys/dev.key
						${UBOOT_OUT}/tools/mkimage -D "-I dts -O dtb -p 1024" -k ./mykeys -f ${WORKDIR}/fit-image.its -K ${UBOOT_OUT}/u-boot.dtb -r ${UBOOT_OUT}/${UBOOT_FIT_IMAGE}
						cat ${UBOOT_OUT}/u-boot-nodtb.bin ${UBOOT_OUT}/u-boot.dtb >${UBOOT_OUT}/${UBOOT_BINARY}
                        ${UBOOT_OUT}/tools/mkimage -D "-I dts -O dtb -p 1024" -k ./mykeys -f ${WORKDIR}/fit-image.its -K ${UBOOT_OUT}/sig_blob.dtb -r ${UBOOT_OUT}/${UBOOT_FIT_IMAGE}
                fi
}

addtask assemble_fitimage before do_install after do_compile

