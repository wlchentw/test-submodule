# If wts is going to be run on ARM platform, please uncomment the line below
#make clean;make TOOL_CHAIN=CORTEX_A9 TOOL_VERSION=P2P

# Build for host machine by default, instead of ARM platform.
make clean;make TOOL_VERSION=P2P
