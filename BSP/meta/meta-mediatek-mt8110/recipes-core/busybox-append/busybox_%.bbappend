FILESEXTRAPATHS_append := ":${THISDIR}/${PN}"

SRC_URI_append = " \
                   file://change-default-config-file-path.patch \
                   file://syslog-startup.conf \
				   "

