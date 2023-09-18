PREFIX = ../install
CROSS = arm-linux-
CROSS_COMPILER = $(CROSS:%-=%)
ROOT = ../install/$(CROSS_COMPILER)
DEP_LIBS = 
INSTALL = _install
CLEAN =
NAME = nvram_custom
CODEBASE=DEFAULT
CONFIG = -I ./src \
	-I./CFG/$(PRJ_FILENAME)/cgen/cfgfileinc \
	-I./CFG/$(PRJ_FILENAME)/cgen/cfgdefault \
	-I./CFG/$(PRJ_FILENAME)/cgen/inc \
	-I./CFG/$(PRJ_FILENAME)/cgen/ \
	-I$(ROOT)/include/custom/$(PRJ_FILENAME)/cgen/cfgfileinc \
	-I$(ROOT)/include/custom/$(PRJ_FILENAME)/cgen/cfgdefault \
	-I$(ROOT)/include/custom/$(PRJ_FILENAME)/cgen/inc \
	-I$(ROOT)/include/custom/$(PRJ_FILENAME)/cgen \
	-I$(ROOT)/include/custom/$(CODEBASE)/cgen/cfgfileinc \
	-I$(ROOT)/include/custom/$(CODEBASE)/cgen/cfgdefault \
	-I$(ROOT)/include/custom/$(CODEBASE)/cgen/inc \
	-I$(ROOT)/include/custom/$(CODEBASE)/cgen
CFLAGS = -O2 -g -fPIC

ifeq ("$(COMBO_CHIP_ID)", "mt8167")
  CFLAGS += -DMTK_MT8167
else ifeq ("$(COMBO_CHIP_ID)", "mt6630")
  CFLAGS += -DMTK_MT6630
  CFLAGS += -D__MTK_MERGE_INTERFACE_SUPPORT__
endif

ifeq ("$(BOOTDEV_TYPE)", "emmc")
  CFLAGS += -DMTK_EMMC_SUPPORT
endif

LIB = lib$(NAME).a lib$(NAME).so
lib$(NAME).so:lib$(NAME).a

ALL = $(LIB)

include Makefile.config
include Makefile.template

_install:
	mkdir -p $(ROOT)/${base_libdir}
	cp lib$(NAME).so $(ROOT)/${base_libdir}
    
pack_rootfs:
# shared library
	mkdir -p $(PACK_TO)/${base_libdir}
	cp $(ROOT)/${base_libdir}/lib$(NAME).so $(PACK_TO)/${base_libdir}
	$(CROSS)strip $(PACK_TO)/${base_libdir}/lib$(NAME).so
	mkdir -p $(PACK_INITRAMFS_TO)/${base_libdir}
	cp $(ROOT)/${base_libdir}/lib$(NAME).so $(PACK_INITRAMFS_TO)/${base_libdir}
	$(CROSS)strip $(PACK_INITRAMFS_TO)/${base_libdir}/lib$(NAME).so
	ls -R $(PACK_TO)/${base_libdir}/lib$(NAME).so >> $(LIST)
	du -c -h $(PACK_TO)/${base_libdir}/lib$(NAME).so >> $(SIZE)



