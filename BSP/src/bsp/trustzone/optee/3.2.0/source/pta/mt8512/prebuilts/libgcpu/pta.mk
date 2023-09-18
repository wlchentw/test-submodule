ifeq ($(filter gcpu, $(libnames)),)
$(call add-prebuilt-libname,gcpu)
$(call add-libname-pta-obj,gcpu,gcpu_drv.o)
endif
