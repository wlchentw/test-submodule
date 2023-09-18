export V ?= 0

# If _HOST or _TA specific compilers are not specified, then use CROSS_COMPILE
HOST_CROSS_COMPILE ?= $(CROSS_COMPILE)
TA_CROSS_COMPILE ?= $(CROSS_COMPILE)
LIBDIR ?= $(LIBDIR)
BINDIR ?= $(BINDIR)
BINDIR ?= $(INCDIR)
TADIR ?= $(TADIR)

.PHONY: all
all:
	$(MAKE) -C host CROSS_COMPILE="$(HOST_CROSS_COMPILE)" --no-builtin-variables
	$(MAKE) -C ta CROSS_COMPILE="$(TA_CROSS_COMPILE)" LDFLAGS=""

.PHONY: clean
clean:
	$(MAKE) -C host clean
	$(MAKE) -C ta clean

.PHONY: install
install:
	$(MAKE) -C host install LIBDIR="$(LIBDIR)" BINDIR="$(BINDIR)" INCDIR="$(INCDIR)"
	$(MAKE) -C ta install TADIR="$(TADIR)"
