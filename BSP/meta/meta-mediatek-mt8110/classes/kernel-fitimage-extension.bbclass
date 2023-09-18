inherit kernel-uboot-extension hsm-sign-env

python __anonymous () {
    kerneltype = d.getVar('KERNEL_IMAGETYPE', True)
    if kerneltype == 'fitImage':
        depends = d.getVar("DEPENDS", True)
        depends = "%s u-boot-mkimage-native lz4-native dtc-native" % depends
        d.setVar("DEPENDS", depends)

        # Override KERNEL_IMAGETYPE_FOR_MAKE variable, which is internal
        # to kernel.bbclass . We have to override it, since we pack zImage
        # (at least for now) into the fitImage .
        kernelarch = d.getVar('KERNEL_ARCH', True)
        if kernelarch == 'arm':
                d.setVar("KERNEL_IMAGETYPE_FOR_MAKE", "zImage")
        elif kernelarch == 'arm64':
                d.setVar("KERNEL_IMAGETYPE_FOR_MAKE", "Image")
        else:
                print("please set KERNEL_ARCH variable.")
                sys.exit(1)
        image = d.getVar('INITRAMFS_IMAGE', True)
        if image:
            d.appendVarFlag('do_assemble_fitimage', 'depends', ' ${INITRAMFS_IMAGE}:do_image_complete')
}

#
# Emit the fitImage ITS header
#
fitimage_emit_fit_header() {
		its_file="$1"
        cat << EOF >> "${its_file}"
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
# $2 ... its file name .
fitimage_emit_section_maint() {
		its_file="${2}"
        case $1 in
        imagestart)
                cat << EOF >> "${its_file}"

        images {
EOF
        ;;
        confstart)
                cat << EOF >> "${its_file}"

        configurations {
                default = "conf@1";
EOF
        ;;
        sectend)
                cat << EOF >> "${its_file}"
        };
EOF
        ;;
        fitend)
                cat << EOF >> "${its_file}"
};
EOF
        ;;
        esac
}

#
# Emit the fitImage ITS kernel section
#
# $1 ... Image counter
# $2 ... Path to kernel image
# $3 ... Compression type
# $4 ... its file name 
fitimage_emit_section_kernel() {
		its_file="${4}"

        kernel_csum="sha256"

        ENTRYPOINT=${UBOOT_ENTRYPOINT}
        if test -n "${UBOOT_ENTRYSYMBOL}"; then
                ENTRYPOINT=`${HOST_PREFIX}nm ${S}/vmlinux | \
                        awk '$3=="${UBOOT_ENTRYSYMBOL}" {print $1}'`
        fi

        cat << EOF >> "${its_file}"
                kernel@${1} {
                        description = "Linux kernel";
                        data = /incbin/("${2}");
                        type = "kernel";
                        arch = "${UBOOT_ARCH}";
                        os = "linux";
                        compression = "${3}";
                        load = <${UBOOT_LOADADDRESS}>;
                        entry = <${ENTRYPOINT}>;
                        hash@1 {
                                algo = "${kernel_csum}";
                        };
                };
EOF
}

#
# Emit the fitImage ITS DTB section
#
# $1 ... Image counter
# $2 ... Path to DTB image
# $3 ... its file name
fitimage_emit_section_dtb() {
		its_file="${3}"

        dtb_csum="sha256"

        cat << EOF >> "${its_file}"
                fdt@${1} {
						#description = "Flattened Device Tree blob";
                        description = "$(basename "${2}")";
                        data = /incbin/("${2}");
                        type = "flat_dt";
                        arch = "${UBOOT_ARCH}";
                        compression = "none";
                        load = <${DTB_LOADADDRESS}>;
                        hash@1 {
                                algo = "${dtb_csum}";
                        };
                };
EOF
}

#
# Emit the fitImage ITS configuration section
#
# $1 ... Linux kernel ID
# $2 ... DTB image ID
# $3 ... Cnfig ID
# $4 ... its file name
fitimage_emit_section_config() {
		its_file="${4}"

        conf_csum="sha256,rsa2048"
        conf_key_name="dev"

        # Test if we have any DTBs at all
        if [ -z "${2}" ] ; then
                conf_desc="Boot Linux kernel"
                fdt_line=""
        else
                conf_desc="Boot Linux kernel with FDT blob"
                fdt_line="fdt = \"fdt@${2}\";"
        fi
        kernel_line="kernel = \"kernel@${1}\";"

        cat << EOF >> "${its_file}"
                conf@${3} {
                        description = "${conf_desc}";
                        ${kernel_line}
                        ${fdt_line}
                        signature@1 {
                                algo = "${conf_csum}";
                                key-name-hint="${conf_key_name}";
                                sign-images="fdt","kernel";
                        };
                };
EOF
}
DEFAULT_ITS_FILE="fit-image.its"
DEFAULT_ITS_BASE_FILE="fit-image-base.its"
do_assemble_fitimage() {
        cd ${B}
        if test "x${KERNEL_IMAGETYPE}" = "xfitImage" ; then
                kernelcount=1
                dtbcount=""
                rm -f "${DEFAULT_ITS_FILE}"

                fitimage_emit_fit_header "${DEFAULT_ITS_FILE}"

                #
                # Step 1: Prepare a kernel image section.
                #
                fitimage_emit_section_maint imagestart "${DEFAULT_ITS_FILE}"

                uboot_prep_kimage
                fitimage_emit_section_kernel "${kernelcount}" linux.bin "${linux_comp}" "${DEFAULT_ITS_FILE}"

				cp "${DEFAULT_ITS_FILE}" "${DEFAULT_ITS_BASE_FILE}"

                #
                # Step 2: Prepare a DTB image section
                #
                if test -n "${KERNEL_DEVICETREE}"; then
                        dtbcount=1
                        for DTB in ${KERNEL_DEVICETREE}; do
                                if echo ${DTB} | grep -q '/dts/'; then
                                        bbwarn "${DTB} contains the full path to the the dts file, but only the dtb name should be used."
                                        DTB=`basename ${DTB} | sed 's,\.dts$,.dtb,g'`
                                fi

                                # go through device tree blob dir for 32/64 bits kernel
                                DTB_PATH="arch/${ARCH}/boot/dts/mediatek/${DTB}"
                                if [ ! -e "${DTB_PATH}" ]; then
                                        DTB_PATH="arch/${ARCH}/boot/dts/${DTB}"
                                        if [ ! -e "${DTB_PATH}" ]; then
                                                DTB_PATH="arch/${ARCH}/boot/${DTB}"
                                        fi
                                fi

                                fitimage_emit_section_dtb ${dtbcount} ${DTB_PATH} "${DEFAULT_ITS_FILE}"
                                dtbcount=`expr ${dtbcount} + 1`
                        done
                fi

                fitimage_emit_section_maint sectend "${DEFAULT_ITS_FILE}"

                # Force the first Kernel and DTB in the default config
				dtb_total=$(expr ${dtbcount} - 1)
                #kernelcount=1
                #dtbcount=1

                bbwarn "making fitimage configurations , dtbcount=${dtbcount}"
				
				fitimage_emit_section_maint confstart "${DEFAULT_ITS_FILE}"
                #
                # Step 3: Prepare configurations section
                #
				for dtb_id in $(seq 1 1 ${dtb_total}); do  

					fitimage_emit_section_config ${kernelcount} ${dtb_id} ${dtb_id} "${DEFAULT_ITS_FILE}"

				done
				fitimage_emit_section_maint sectend "${DEFAULT_ITS_FILE}"
				fitimage_emit_section_maint fitend "${DEFAULT_ITS_FILE}"

                #
                # Step 4: Assemble the image
                #

                ${HSM_ENV} HSM_KEY_NAME=${VERIFIED_KEY} uboot-mkimage -f "${DEFAULT_ITS_FILE}" arch/${ARCH}/boot/fitImage

                if [ "${SECURE_BOOT_ENABLE}" = "yes" ] && [ "${STANDALONE_SIGN_PREPARE}" != "yes" ]; then
                        mkdir -p ./mykeys
                        cp ${MTK_KEY_DIR}/${VERIFIED_KEY}.crt ./mykeys/dev.crt
                        cp ${MTK_KEY_DIR}/${VERIFIED_KEY}.pem ./mykeys/dev.key
                        ${HSM_ENV} HSM_KEY_NAME=${VERIFIED_KEY} uboot-mkimage -D "-I dts -O dtb -p 1024" -k ./mykeys -f "${DEFAULT_ITS_FILE}" -r arch/${ARCH}/boot/fitImage
                fi


				#
				# build fitimages with different dtbs .
				# 
                if test -n "${KERNEL_SINGLE_IMG_DTBS}"; then
                        dtbcount=1
                        DTB_NAMES="$(cat "${B}/.config"|grep CONFIG_BUILD_ARM_DTB_OVERLAY_IMAGE_NAMES|sed "s/CONFIG_BUILD_ARM_DTB_OVERLAY_IMAGE_NAMES=\"//"|sed "s/\".*$//")"
                        #echo "DTB_NAMES=\"${DTB_NAMES}\""
                        for DTB_NAME in ${DTB_NAMES}; do
                                DTB="/mediatek/${DTB_NAME}.dtb"
                                if echo ${DTB} | grep -q '/dts/'; then
                                        bbwarn "${DTB} contains the full path to the the dts file, but only the dtb name should be used."
                                        DTB=`basename ${DTB} | sed 's,\.dts$,.dtb,g'`
                                fi
								
								dtb_base_name="$(basename "${DTB}"|sed -e "s/\.dtb$//")"
								tmp_its_file="fitImage-${dtb_base_name}.its"
								cp "${DEFAULT_ITS_BASE_FILE}" "${tmp_its_file}"
                                # go through device tree blob dir for 32/64 bits kernel
                                DTB_PATH="arch/${ARCH}/boot/dts/mediatek/${DTB}"
                                if [ ! -e "${DTB_PATH}" ]; then
                                        DTB_PATH="arch/${ARCH}/boot/dts/${DTB}"
                                        if [ ! -e "${DTB_PATH}" ]; then
                                                DTB_PATH="arch/${ARCH}/boot/${DTB}"
                                        fi
                                fi

                                fitimage_emit_section_dtb ${dtbcount} ${DTB_PATH} "${tmp_its_file}"
								fitimage_emit_section_maint sectend "${tmp_its_file}"
								fitimage_emit_section_maint confstart "${tmp_its_file}"
								fitimage_emit_section_config ${kernelcount} 1 1 "${tmp_its_file}"
								fitimage_emit_section_maint sectend "${tmp_its_file}"
								fitimage_emit_section_maint fitend "${tmp_its_file}"
								#
								# Assemble the image
								#

								${HSM_ENV} HSM_KEY_NAME=${VERIFIED_KEY} uboot-mkimage -f "${tmp_its_file}" arch/${ARCH}/boot/fitImage-${dtb_base_name}.img

				                if [ "${SECURE_BOOT_ENABLE}" = "yes" ] && [ "${STANDALONE_SIGN_PREPARE}" != "yes" ]; then
			                        mkdir -p ./mykeys
						            cp ${MTK_KEY_DIR}/${VERIFIED_KEY}.crt ./mykeys/dev.crt
									cp ${MTK_KEY_DIR}/${VERIFIED_KEY}.pem ./mykeys/dev.key
									${HSM_ENV} HSM_KEY_NAME=${VERIFIED_KEY} uboot-mkimage -D "-I dts -O dtb -p 1024" -k ./mykeys -f "${tmp_its_file}" -r arch/${ARCH}/boot/fitImage-${dtb_base_name}.img
								fi
						done
				fi

        fi
}

addtask assemble_fitimage before do_install after do_compile


do_prepare_eink_lib() {
	#echo "Gallen : prepare_eink_lib "
	echo "  S=\"${S}\" "
	if [ ! -e "${S}/drivers/misc/mediatek/cfa/EInk_Kaleido_render.o_shipped" ] && [ -e "${S}/drivers/misc/mediatek/cfa" ] ;then
		echo "\"EInk_Kaleido_render.o_shipped\" not exist , creating default for EC070KH2 ..."
		cd "${S}/drivers/misc/mediatek/cfa"
		if [ $? = 0 ];then
			if [ -e "EInk_Kaleido_EC070KH2_render.o_shipped" ];then
				ln -s EInk_Kaleido_EC070KH2_render.o_shipped EInk_Kaleido_render.o_shipped
			fi
			cd -
		fi
	fi
}

addtask prepare_eink_lib before do_compile 

kernel_do_deploy[vardepsexclude] = "DATETIME"
kernel_do_deploy_append() {
        # Update deploy directory
        if test "x${KERNEL_IMAGETYPE}" = "xfitImage" ; then
                cd ${B}
                echo "Copying "${DEFAULT_ITS_FILE}" source file..."
                its_base_name="${KERNEL_IMAGETYPE}-its-${PV}-${PR}-${MACHINE}-${DATETIME}"
                its_symlink_name=${KERNEL_IMAGETYPE}-its-${MACHINE}
                install -m 0644 "${DEFAULT_ITS_FILE}" ${DEPLOYDIR}/${its_base_name}.its
                linux_bin_base_name="${KERNEL_IMAGETYPE}-linux.bin-${PV}-${PR}-${MACHINE}-${DATETIME}"
                linux_bin_symlink_name=${KERNEL_IMAGETYPE}-linux.bin-${MACHINE}
                install -m 0644 linux.bin ${DEPLOYDIR}/${linux_bin_base_name}.bin
                if test -n "${RECOVERY_KERNEL_DEVICETREE}"; then
                        find arch/${ARCH}/boot/dts -name "${RECOVERY_KERNEL_DEVICETREE}" -exec install -m 0644 {} ${DEPLOYDIR}/${RECOVERY_KERNEL_DEVICETREE} \;
                fi

				
				# install fitimages with dtbs ..
				DTB_NAMES="$(cat "${B}/.config"|grep CONFIG_BUILD_ARM_DTB_OVERLAY_IMAGE_NAMES|sed "s/CONFIG_BUILD_ARM_DTB_OVERLAY_IMAGE_NAMES=\"//"|sed "s/\".*$//")"
				for DTB_NAME in ${DTB_NAMES}; do
					DTB="/mediatek/${DTB_NAME}.dtb"
					if echo ${DTB} | grep -q '/dts/'; then
						bbwarn "${DTB} contains the full path to the the dts file, but only the dtb name should be used."
						DTB=`basename ${DTB} | sed 's,\.dts$,.dtb,g'`
					fi
						
					dtb_base_name="$(basename "${DTB}"|sed -e "s/\.dtb$//")"
					tmp_its_file="fitImage-${dtb_base_name}.its"
					install -m 0644 "${tmp_its_file}" ${DEPLOYDIR}/
					install -m 0644 "arch/${ARCH}/boot/fitImage-${dtb_base_name}.img" ${DEPLOYDIR}/
				done

                cd ${DEPLOYDIR}
                ln -nfs fitImage boot.img
        fi
}
