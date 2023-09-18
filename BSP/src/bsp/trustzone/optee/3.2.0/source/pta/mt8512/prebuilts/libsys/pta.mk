ifeq ($(filter sys, $(libnames)),)
$(call add-prebuilt-libname,sys)
$(call add-libname-pta-obj,sys,intl.o)
endif
