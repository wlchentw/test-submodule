# variable HAS_MTxxxx is set in has_platform.mk
LIB_MDP_TARGET :=
LINK_FLAGS := -Xlinker --unresolved-symbols=ignore-in-shared-libs

ifeq ($(HAS_MT2701), 1)
MDP_PLATFORM := mt2701
LIB_MDP_PROT_mt2701 := -lmdp_prot.mt2701
libmdp.$(MDP_PLATFORM).so: $(OBJECTS-libmdp.$(MDP_PLATFORM).so)
	$(CXX) -shared -pthread $(LINK_FLAGS) $(CFLAGS) $(CPPFLAGS) $(LIB_MDP_PQ_mt2701) $(LIB_MDP_PROT_mt2701) $(LDFLAGS) $^ -o $@

LIB_MDP_TARGET += libmdp.$(MDP_PLATFORM).so
endif

ifeq ($(HAS_MT8173), 1)
MDP_PLATFORM := mt8173
LIB_MDP_PROT_mt8173 := -lmdp_prot.mt8173
libmdp.$(MDP_PLATFORM).so: $(OBJECTS-libmdp.$(MDP_PLATFORM).so)
	$(CXX) -shared -pthread $(LINK_FLAGS) $(CFLAGS) $(CPPFLAGS) $(LIB_MDP_PQ_mt8173) $(LIB_MDP_PROT_mt8173) $(LDFLAGS) $^ -o $@

LIB_MDP_TARGET += libmdp.$(MDP_PLATFORM).so
endif

ifeq ($(HAS_MT2712), 1)
MDP_PLATFORM := mt2712
LIB_MDP_PROT_mt2712 := -lmdp_prot.mt2712
LIB_MDP_TILE_mt2712 :=

# use self built tile driver library if it is building
ifneq ($(OBJECTS-libmdp_prot.mt2712.so),)
LIB_MDP_PROT_mt2712 :=
LIB_MDP_TILE_mt2712 := libmdp_tile.mt2712.so
libmdp_tile.mt2712.so: $(OBJECTS-libmdp_prot.mt2712.so)
	$(CXX) -shared $(LINK_FLAGS) $(CFLAGS) $(CPPFLAGS) $(LDFLAGS) $^ -o $@
LIB_MDP_TARGET += libmdp_tile.$(MDP_PLATFORM).so
endif

libmdp.$(MDP_PLATFORM).so: $(OBJECTS-libmdp.$(MDP_PLATFORM).so) $(LIB_MDP_TILE_mt2712)
	$(CXX) -shared -pthread $(LINK_FLAGS) $(CFLAGS) $(CPPFLAGS) $(LIB_MDP_PQ_mt2712) $(LIB_MDP_PROT_mt2712) $(LDFLAGS) $^ -o $@

libcmdq.$(MDP_PLATFORM).so: $(OBJECTS-libcmdq.$(MDP_PLATFORM).so)
	$(CXX) -shared $(LINK_FLAGS) $(CFLAGS) $(CPPFLAGS) $(LDFLAGS) $^ -o $@

LIB_MDP_TARGET += libmdp.$(MDP_PLATFORM).so libcmdq.$(MDP_PLATFORM).so
endif

ifeq ($(HAS_MT6799), 1)
MDP_PLATFORM := mt6799
# MT6799 not build libmdp_prot.mt6799.so currently!!!
LIB_MDP_PROT_mt6799 :=
libmdp.$(MDP_PLATFORM).so: $(OBJECTS-libmdp.$(MDP_PLATFORM).so)
	$(CXX) -shared -pthread $(LINK_FLAGS) $(CFLAGS) $(CPPFLAGS) $(LIB_MDP_PQ_mt6799) $(LIB_MDP_PROT_mt6799) $(LDFLAGS) $^ -o $@

LIB_MDP_TARGET += libmdp.$(MDP_PLATFORM).so
endif

ifeq ($(HAS_MT8167), 1)
MDP_PLATFORM := mt8167
LIB_MDP_PROT_mt8167 := -lmdp_prot.mt8167
libmdp.$(MDP_PLATFORM).so: $(OBJECTS-libmdp.$(MDP_PLATFORM).so)
	$(CXX) -shared -pthread $(LINK_FLAGS) $(CFLAGS) $(CPPFLAGS) $(LIB_MDP_PQ_mt8167) $(LIB_MDP_PROT_mt8167) $(LDFLAGS) $^ -o $@

LIB_MDP_TARGET += libmdp.$(MDP_PLATFORM).so
endif

ifeq ($(HAS_MT8183), 1)
MDP_PLATFORM := mt8183
LIB_MDP_PROT_mt8183 := -lmdp_prot.mt8183
LIB_MDP_DEPEND_LIBS := -lion -lcutils -lutils
libmdp.$(MDP_PLATFORM).so: $(OBJECTS-libmdp.$(MDP_PLATFORM).so)
	$(CXX) -shared -pthread $(LINK_FLAGS) $(CFLAGS) $(CPPFLAGS) $(LIB_MDP_PQ_mt8183) $(LIB_MDP_PROT_mt8183) $(LIB_MDP_DEPEND_LIBS) $(LDFLAGS) $^ -o $@

LIB_MDP_TARGET += libmdp.$(MDP_PLATFORM).so
endif

ifeq ($(HAS_MT8512), 1)
MDP_PLATFORM := mt8512
LIB_MDP_PROT_mt8512 := -lmdp_prot.mt8512
#LIB_MDP_DEPEND_LIBS := -lion -lcutils -lutils
LIB_MDP_DEPEND_LIBS := -lion
libmdp.$(MDP_PLATFORM).so: $(OBJECTS-libmdp.$(MDP_PLATFORM).so)
	$(CXX) -shared -pthread $(CFLAGS) $(CPPFLAGS) $(LIB_MDP_PQ_mt8512) $(LIB_MDP_PROT_mt8512) $(LIB_MDP_DEPEND_LIBS) $(LDFLAGS) $^ -o $@
	#$(CXX) -shared -pthread $(LINK_FLAGS) $(CFLAGS) $(CPPFLAGS) $(LIB_MDP_PQ_mt8512) $(LIB_MDP_PROT_mt8512) $(LIB_MDP_DEPEND_LIBS) $(LDFLAGS) $^ -o $@

LIB_MDP_TARGET += libmdp.$(MDP_PLATFORM).so
endif
