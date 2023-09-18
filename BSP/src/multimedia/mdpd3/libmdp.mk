# variable HAS_MTxxxx is set in has_platform.mk

ifeq ($(HAS_MT2701), 1)
MDP_PLATFORM := mt2701
include libmdp.platform.mk
endif

ifeq ($(HAS_MT8173), 1)
MDP_PLATFORM := mt8173
include libmdp.platform.mk
endif

ifeq ($(HAS_MT2712), 1)
MDP_PLATFORM := mt2712
include libmdp.platform.mk
ifeq ($(TILE_SOURCE), 1)
include libmdp/dpframework_prot/rules_mt2712.mk
endif
endif

ifeq ($(HAS_MT6799), 1)
MDP_PLATFORM := mt6799
include libmdp.platform.mk
endif

ifeq ($(HAS_MT8167), 1)
MDP_PLATFORM := mt8167
include libmdp.platform.mk
endif

ifeq ($(HAS_MT8183), 1)
MDP_PLATFORM := mt8183
include libmdp.platform.mk
endif

ifeq ($(HAS_MT8512), 1)
MDP_PLATFORM := mt8512
include libmdp.platform.mk
endif
