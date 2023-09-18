PREFIX   = ../../../install
CROSS    = arm-linux-
ROOT     = $(PREFIX)/$(CROSS:%-=%)

DEFAULT = "DEFAULT"

all:

	@bash -c 'if [ -e $(PRJ_FILENAME)/Makefile ]; then \
				make -C $(PRJ_FILENAME) CROSS=$(CROSS) PROJECT=$(PROJECT) all; \
				if [ $$? != 0 ]; then exit $$?; fi \
                        fi;'
	@bash -c 'if [ -e $(DEFAULT)/Makefile ]; then \
				make -C $(DEFAULT) CROSS=$(CROSS) PROJECT=$(PROJECT) all; \
				if [ $$? != 0 ]; then exit $$?; fi \
                        fi;'			

clean:
	@bash -c 'if [ -e $(PRJ_FILENAME)/Makefile ]; then \
				make -C $(PRJ_FILENAME) clean; \
				if [ $$? != 0 ]; then exit $$?; fi \
                        fi;'
	@bash -c 'if [ -e $(DEFAULT)/Makefile ]; then \
				make -C $(DEFAULT) clean; \
				if [ $$? != 0 ]; then exit $$?; fi \
                        fi;'
	rm -rf rootfs
	rm -rf initramfs

install:
	@bash -c 'if [ -e $(PRJ_FILENAME)/Makefile ]; then \
				make -C $(PRJ_FILENAME) CROSS=$(CROSS) ROOT=$(ROOT) PREFIX=$(PREFIX) install; \
				if [ $$? != 0 ]; then exit $$?; fi \
                        fi;'
	@bash -c 'if [ -e $(DEFAULT)/Makefile ]; then \
				make -C $(DEFAULT) CROSS=$(CROSS) ROOT=$(ROOT) PREFIX=$(PREFIX) install; \
				if [ $$? != 0 ]; then exit $$?; fi \
                        fi;'	

pack_rootfs:
	@bash -c 'if [ -e $(PRJ_FILENAME)/Makefile ]; then \
				make -C $(PRJ_FILENAME) CROSS=$(CROSS) PACK_TO=$(PACK_TO) PACK_INITRAMFS_TO=$(PACK_INITRAMFS_TO) PROJECT=$(PROJECT) pack_rootfs; \
				if [ $$? != 0 ]; then exit $$?; fi \
                        fi;'
	@bash -c 'if [ -e $(DEFAULT)/Makefile ]; then \
				make -C $(DEFAULT) CROSS=$(CROSS) PACK_TO=$(PACK_TO) PACK_INITRAMFS_TO=$(PACK_INITRAMFS_TO) PROJECT=$(PROJECT) pack_rootfs; \
				if [ $$? != 0 ]; then exit $$?; fi \
                        fi;'
