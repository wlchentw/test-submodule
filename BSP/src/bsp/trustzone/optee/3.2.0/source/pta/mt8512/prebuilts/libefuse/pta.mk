ifeq ($(filter efuse, $(libnames)),)
$(call add-prebuilt-libname,efuse)
$(call add-libname-pta-obj,efuse,efuse_ta.o)
endif