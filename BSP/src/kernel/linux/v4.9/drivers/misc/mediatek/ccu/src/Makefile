#
# Copyright (C) 2016 MediaTek Inc.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2 as
# published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#

#obj-y += $(subst ",,$(CONFIG_MTK_PLATFORM))/
#obj-$(CONFIG_ARCH_MT6799) += 1.0/

#subdir-ccflags-y += -DMTK_VPU_EMULATOR
subdir-ccflags-y += -DMTK_VPU_KERNEL
subdir-ccflags-y += -DCCU_AF_ENABLE

platform = $(subst ",,$(CONFIG_MTK_PLATFORM))

obj-y += $(subst ",,$(platform))/

ifneq (,$(filter $(platform),mt6757 mt6799))
    obj-y += 1.0/
else ifneq (,$(filter $(platform),mt6758 mt6763 mt6765 mt8518))
    obj-y += 1.1/
else
    obj-y += 2.0/
endif
