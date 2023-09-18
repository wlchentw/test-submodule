CROSS = arm-linux-
CROSS_COMPILER = $(CROSS:%-=%)
ROOT = ../install/$(CROSS_COMPILER)
CFLAGS = -O2 -g $(DEF_CFLAGS) $(THUMB) $(OFLAGS) -fPIC
DEP_LIBS = nvram_custom
BACH_BUILD_DEP = libnvram_custom

all:
	make -f Makefile.libnvram
	make -f Makefile.libfile_op
	make -f Makefile.nvram_daemon
	make -f Makefile.APDB
	make -f Makefile.NvRAMUtils
	make -f Makefile.wfbt_wr

clean:
	make -f Makefile.libnvram clean
	make -f Makefile.libfile_op clean
	make -f Makefile.nvram_daemon clean
	make -f Makefile.APDB clean
	make -f Makefile.NvRAMUtils clean
	make -f Makefile.wfbt_wr clean

install:
	make -f Makefile.libnvram install
	make -f Makefile.libfile_op install
	make -f Makefile.nvram_daemon install
	make -f Makefile.APDB install
	make -f Makefile.NvRAMUtils install
	make -f Makefile.wfbt_wr install
	
pack_rootfs:
	mkdir -p $(PACK_TO)/${base_libdir}
	cp -af $(ROOT)/${base_libdir}/libfile_op.so $(PACK_TO)/${base_libdir}
	$(CROSS)strip $(PACK_TO)/${base_libdir}/libfile_op.so
	cp -af $(ROOT)/${base_libdir}/libnvram.so $(PACK_TO)/${base_libdir}
	$(CROSS)strip $(PACK_TO)/${base_libdir}/libnvram.so
	cp -af $(ROOT)/${base_libdir}/libNvRAMUtils.so $(PACK_TO)/${base_libdir}
	$(CROSS)strip $(PACK_TO)/${base_libdir}/libNvRAMUtils.so
	mkdir -p $(PACK_TO)/sbin
	cp -f $(ROOT)/sbin/nvram_daemon $(PACK_TO)/sbin/
	$(CROSS)strip $(PACK_TO)/sbin/nvram_daemon
	cp -f $(ROOT)/sbin/wfbt_wr $(PACK_TO)/sbin/
	$(CROSS)strip $(PACK_TO)/sbin/wfbt_wr
	mkdir -p $(PACK_INITRAMFS_TO)/${base_libdir}
	cp -af $(ROOT)/${base_libdir}/libfile_op.so $(PACK_INITRAMFS_TO)/${base_libdir}
	$(CROSS)strip $(PACK_INITRAMFS_TO)/${base_libdir}/libfile_op.so
	cp -af $(ROOT)/${base_libdir}/libnvram.so $(PACK_INITRAMFS_TO)/${base_libdir}
	$(CROSS)strip $(PACK_INITRAMFS_TO)/${base_libdir}/libnvram.so
	cp -af $(ROOT)/${base_libdir}/libNvRAMUtils.so $(PACK_INITRAMFS_TO)/${base_libdir}
	$(CROSS)strip $(PACK_INITRAMFS_TO)/${base_libdir}/libNvRAMUtils.so
	mkdir -p $(PACK_INITRAMFS_TO)/sbin
	cp -f $(ROOT)/sbin/nvram_daemon $(PACK_INITRAMFS_TO)/sbin/
	$(CROSS)strip $(PACK_INITRAMFS_TO)/sbin/nvram_daemon
	cp -f $(ROOT)/sbin/wfbt_wr $(PACK_INITRAMFS_TO)/sbin/
	$(CROSS)strip $(PACK_INITRAMFS_TO)/sbin/wfbt_wr