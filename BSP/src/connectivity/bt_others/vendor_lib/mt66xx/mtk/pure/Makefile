OBJS_libbluetooth_mtk_pure = \
    combo/mtk.o \
    combo/radiomgr.o \
    combo/radiomod.o \
    $(NULL)

#OBJS_libbluetoothem_mtk = \
#    $(PWD)/../../../../../../../src/connectivity/bt_vendor_lib/mtk/pure/combo/bt_em.o \
#    $(NULL)

#---------------------------------------------------------------------------
# $(BT_VCOM_RELAYER)
#     is used to define how to OPEN VCOM port.
# If VCOM port is opened default, no need to open by boots,
#     Set BT_VCOM_RELAYER=-DVCOM_OPENED
# If open VCOM port by set property "sys.usb.config",
#     Set BT_VCOM_RELAYER=-DOPEN_VCOM_BY_SET_PROP
#---------------------------------------------------------------------------
CCFLAGS = \
#    -Werror \
#    -Wall \
    $(LOCAL_CFLAGS) \
    $(BT_VCOM_RELAYER) \
    $(NULL)

LINKFLAGS = \
    $(NULL)

CGEN_PATH = $(PWD)/../../../../../../../src/bach/build.bach/work/custom_user/DEFAULT/linux-3.18.19/common/cgen
SO_INC = \
    -I ./inc \
    -I$(CGEN_PATH)/cfgdefault \
    -I$(CGEN_PATH)/cfgfileinc \
    -I$(CGEN_PATH)/inc \
    -DMTK_MT6630 \
    -DMTK_IVT_ONLY \
    $(NULL)

#---------------------------------------------------------------------------
ifeq ($(CC),)
ifeq ($(CROSS_COMPILE),)
    CC = gcc
    STRIP = strip
else
    CC = $(CROSS_COMPILE)-gcc
    STRIP = $(CROSS_COMPILE)-strip
endif
endif
#---------------------------------------------------------------------------
all: libbluetooth_mtk_pure.so
#libbluetoothem_mtk.so

%.o: %.c Makefile
	$(CC) -fPIC $(CCFLAGS) $(CFLAGS) $(INCLUDES) $(SO_INC) -c -o $@ $<

libbluetooth_mtk_pure.so: $(OBJS_libbluetooth_mtk_pure)
	$(CC) -shared $(OBJS_libbluetooth_mtk_pure) $(LINKFLAGS) $(SO_INC) -o libbluetooth_mtk_pure.so
#   $(STRIP) libbluetooth_mtk_pure.so

#libbluetoothem_mtk.so: $(OBJS_libbluetoothem_mtk)
#   $(CC) -shared $(OBJS_libbluetoothem_mtk) $(LINKFLAGS) $(SO_INC) -o libbluetoothem_mtk.so
#   $(STRIP) libbluetoothem_mtk.so

clean:
	rm -rf *.o

	rm -rf libbluetooth_mtk_pure.so
#   rm -rf libbluetoothem_mtk.so

#---------------------------------------------------------------------------
