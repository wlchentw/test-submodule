bin_PROGRAMS = wmt_loader wmt_launcher wmt_loopback wmt_concurrency stp_dump3
wmt_loader_SOURCES = combo_loader/yocto_loader.c
wmt_loopback_SOURCES = src/wmt_loopback.c
wmt_launcher_SOURCES = src/yocto_stp_uart_launcher.c
wmt_launcher_LDADD = -lpthread

power_on_wifi_SOURCES = src/power_on_wifi.c

wmt_concurrency_SOURCES = src/yocto_wmt_concurrency.c

stp_dump3_SOURCES = src/stp_dump/os_linux.c \
                   src/stp_dump/eloop.c \
                   src/stp_dump/stp_dump.c \
                   src/stp_dump/bt_fw_logger.c \
                   src/stp_dump/wifi_fw_logger.c
stp_dump3_LDADD = -lpthread