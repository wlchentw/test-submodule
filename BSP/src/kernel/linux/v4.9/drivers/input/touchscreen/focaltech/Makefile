#
# Makefile for the input core drivers.
#

# Each configuration option enables a list of files.
obj-$(CONFIG_TOUCH_FOCALTECH)  += focaltech_ts.o
#focaltech_ts-objs += focaltech.o focaltech_ctl.o focaltech_ex_fun.o
focaltech_ts-objs := focaltech_core.o
focaltech_ts-objs += focaltech_ctl.o
focaltech_ts-objs += focaltech_ex_fun.o
focaltech_ts-objs += focaltech_flash.o
#focaltech_ts-y    += focaltech.o focaltech_ctl.o focaltech_ex_fun.o
