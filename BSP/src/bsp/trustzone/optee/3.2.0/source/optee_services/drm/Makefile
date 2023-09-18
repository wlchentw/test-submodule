# If _HOST or _TA specific compilers are not specified, then use CROSS_COMPILE
HOST_CROSS_COMPILE ?= $(CROSS_COMPILE)
TA_CROSS_COMPILE ?= $(CROSS_COMPILE)
LIBDIR ?= $(LIBDIR)

.PHONY: all
all:
	$(MAKE) -C host CROSS_COMPILE="$(HOST_CROSS_COMPILE)" --no-builtin-variables

.PHONY: clean
clean:
	$(MAKE) -C host clean

.PHONY: install
install:
	$(MAKE) -C host install LIBDIR="$(LIBDIR)" INCDIR="$(INCDIR)"
