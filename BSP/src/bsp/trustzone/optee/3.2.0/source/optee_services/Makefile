export V ?= 0

LIBDIR := $(LIBDIR)
BINDIR := $(BINDIR)
INCDIR := $(INCDIR)
TADIR := $(TADIR)

SERVICE_LIST := $(subst /,,$(dir $(wildcard */Makefile)))

.PHONY: all
all: services prepare-for-rootfs

.PHONY: clean
clean: services-clean prepare-for-rootfs-clean

services:
	@for service in $(SERVICE_LIST); do \
		$(MAKE) -C $$service CROSS_COMPILE="$(HOST_CROSS_COMPILE)" || exit -1; \
	done

services-clean:
	@for service in $(SERVICE_LIST); do \
		$(MAKE) -C $$service clean || exit -1; \
	done

prepare-for-rootfs: services
	@echo "Nothing to do"

prepare-for-rootfs-clean:
	@rm -rf $(TA_OUTPUT_DIR)

.PHONY: install
install:
	@for service in $(SERVICE_LIST); do \
		$(MAKE) -C $$service install LIBDIR="${LIBDIR}" BINDIR="${BINDIR}" INCDIR="${INCDIR}" TADIR="$(TADIR)" || exit -1; \
	done
