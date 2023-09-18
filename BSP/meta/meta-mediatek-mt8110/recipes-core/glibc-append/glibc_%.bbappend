FILESEXTRAPATHS_append := ":${THISDIR}/${PN}"

SRC_URI_append = " \
                   file://hook-printf-and-vprintf-with-syslog.patch \
                 "

