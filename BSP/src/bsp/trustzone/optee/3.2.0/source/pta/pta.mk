define my-dir
$(patsubst %/,%,$(dir $(lastword $(MAKEFILE_LIST))))
endef

define all-pta-mk-under
$(wildcard $1/*/pta.mk)
endef

define _add-prebuilt-libname
libdirs += $(call my-dir)
libnames += $1
endef

define add-prebuilt-libname
$(eval $(call _add-prebuilt-libname, $1))
endef

define add-pta-src
$(eval LDADD := $(LDADD) $(out-dir)$(subst ..,,$(libdir)/$(strip $(subst .c,.o,$1))))
endef

# $1: libname
# $2: object file in libname
define _add-libname-pta-obj
my-path := $(call my-dir)
$(out-dir)$$(subst ..,,$$(my-path))/$$(strip $2): $(call my-dir)/lib$$(strip $1).a
	echo $$@
	$(q)mkdir -p $$(dir $$@)
	$(q)cd $$(dir $$@); $(AR$(sm)) x $$(abspath $$<) $2

libdeps += $(out-dir)$$(subst ..,,$$(my-path))/$$(strip $2)
LDADD := $(LDADD) $(out-dir)$$(subst ..,,$$(my-path))/$$(strip $2)
endef

define add-libname-pta-obj
$(eval $(call _add-libname-pta-obj,$1,$2))
endef

base-prefix = core-lib/
BUILD_OPTEE_OS_LIB := mk/lib.mk

include $(call all-pta-mk-under, $(call my-dir))

