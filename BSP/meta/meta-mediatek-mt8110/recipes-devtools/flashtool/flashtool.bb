DESCRIPTION = "Mediatek FlashScript"
LICENSE = "MediaTekProprietary"
LIC_FILES_CHKSUM = "file://${WORKDIR}/README;md5=ecf62296074513b19f1c6e1ae1bd704a"
SRC_URI = "file://README file://flashimage.py file://${MACHINE}/flashproc.py file://../ntx_bins"
BBCLASSEXTEND += "native"

inherit deploy

do_deploy () {
	install -d ${DEPLOYDIR}
	install -m 755 ${WORKDIR}/flashimage.py -t ${DEPLOYDIR}
	install -m 755 ${WORKDIR}/${MACHINE}/flashproc.py -t ${DEPLOYDIR}
	#echo "KERNEL_SINGLE_IMG_DTBS=${KERNEL_SINGLE_IMG_DTBS}"
	for kernel_dtb in ${KERNEL_SINGLE_IMG_DTBS}
	do
		
		#echo "kernel_dtb=${kernel_dtb}"
		dtb_base_name="$(basename "${kernel_dtb}" .dtb)"

		# for flashimage.py ,cannot import file name with '-' , so we need replace it by '_' .
		dtb_base_name_no_dash="$(echo "${dtb_base_name}"|sed -e "s/-/_/g")"


		#
		# prepare flashproc_xxx.py and flashimage_xxx.py .
		#
		#echo "DEPLOYDIR=${DEPLOYDIR}"
		ls ${DEPLOYDIR}
		cp ${DEPLOYDIR}/flashproc.py ${DEPLOYDIR}/flashproc_${dtb_base_name_no_dash}.py
		sed -i "s/boot\.img/fitImage-${dtb_base_name}.img/g" "${DEPLOYDIR}/flashproc_${dtb_base_name_no_dash}.py"

		cp ${DEPLOYDIR}/flashimage.py ${DEPLOYDIR}/flashimage_${dtb_base_name_no_dash}.py
		sed -i "s/flashproc/flashproc_${dtb_base_name_no_dash}/g" "${DEPLOYDIR}/flashimage_${dtb_base_name_no_dash}.py"

		#install -m 755 ${WORKDIR}/flashimage.py ${DEPLOYDIR}/flashimage_${dtb_base_name_no_dash}.py
		#install -m 755 ${WORKDIR}/${MACHINE}/flashproc.py ${DEPLOYDIR}/flashproc_${dtb_base_name_no_dash}.py

		#
		# prepare netronix binaries . 
		# 
		#install -d "${DEPLOYDIR}/${dtb_base_name}"
		#echo "S=${S},WORKDIR=${WORKDIR},MACHINE=${MACHINE}"
		#ls -l ${WORKDIR}/ntx_bins/${dtb_base_name}/waveform.img

		cp -L -v ${WORKDIR}/ntx_bins/${dtb_base_name}/waveform.img ${DEPLOYDIR}/waveform_${dtb_base_name_no_dash}.img
		sed -i "s/waveform\.img/waveform_${dtb_base_name_no_dash}.img/g" "${DEPLOYDIR}/flashproc_${dtb_base_name_no_dash}.py"

		cp -L -v ${WORKDIR}/ntx_bins/${dtb_base_name}/ntx_hwconfig.bin ${DEPLOYDIR}/ntx_hwconfig_${dtb_base_name_no_dash}.bin
		sed -i "s/ntx_hwconfig\.bin/ntx_hwconfig_${dtb_base_name_no_dash}.bin/g" "${DEPLOYDIR}/flashproc_${dtb_base_name_no_dash}.py"

		cp -L -v ${WORKDIR}/ntx_bins/${dtb_base_name}/ntxfw.bin ${DEPLOYDIR}/ntxfw_${dtb_base_name_no_dash}.bin
		sed -i "s/ntxfw\.bin/ntxfw_${dtb_base_name_no_dash}.bin/g" "${DEPLOYDIR}/flashproc_${dtb_base_name_no_dash}.py"

		# Generate bin header for ntxfw bins
		gen_ntx_bin_header.sh ${DEPLOYDIR}/ntxfw_${dtb_base_name_no_dash}.bin
		
	done
}

addtask deploy before do_build after do_compile
