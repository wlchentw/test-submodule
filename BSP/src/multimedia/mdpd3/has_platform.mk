#   For master branch, it's simple to build all platforms, no matter
# what the PACKAGE_ARCH is, no matter what the OS it is.
#   For customer branch, we can specify a platform to build.

# 64 bit CPUs list
# They may be built as 32 or 64 bit libraries.
HAS_MT8173 := 0
HAS_MT2712 := 0
HAS_MT6799 := 0
HAS_MT8167 := 0
HAS_MT8183 := 0
HAS_MT8512 := 1

# 32 bit CPUs list
# They only can be built as 32 bit libraries.
HAS_MT2701 := 0
