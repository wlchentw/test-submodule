# clear all the related variables to avoid duplicated usage
# because this mk file may be included multiple times
# according to the target archtecture.
OBJS :=
INCLUDES :=
MDP_LIB_OBJS :=
MDP_CFLAGS :=
PQ_SUPPORT :=
TILE_SOURCE :=
include libmdp/$(MDP_PLATFORM)/rules_$(MDP_PLATFORM).mk
OBJS := $(MDP_LIB_OBJS)
$(eval $$(OBJS): CPPFLAGS := $(CC_OPTION) $(MDP_CFLAGS) -fPIC -Iv4l2_mdpd/vpu/include -Iv4l2_mdpd/vpu/platform/include $$(INCLUDES) -include v4l2_mdpd/vpu/include/compiler.h)
OBJECTS-libmdp.$(MDP_PLATFORM).so := $(OBJS)

ifeq ($(PQ_SUPPORT), 1)
LIB_MDP_PQ_$(MDP_PLATFORM) := -lmdppq
endif

OBJS :=
INCLUDES :=
CMDQ_LIG_OBJS :=
MDP_CFLAGS :=
-include libcmdq/rules_$(MDP_PLATFORM).mk
OBJS := $(CMDQ_LIB_OBJS)
$(eval $$(OBJS): CPPFLAGS := $(CC_OPTION) $(MDP_CFLAGS) -fPIC -Iv4l2_mdpd/vpu/include  $$(INCLUDES) -include v4l2_mdpd/vpu/include/compiler.h)
OBJECTS-libcmdq.$(MDP_PLATFORM).so := $(OBJS)
