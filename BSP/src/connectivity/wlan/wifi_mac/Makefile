#
# Makefile for libwifitest
#

# definitions
# ------------------------------------------------------------------------------
CFLAGS				 = -pipe -g -Os $(EXTRA_CFLAGS)

# library info
# ------------------------------------------------------------------------------

# wifitest tool info
PROGRAM				 = wifi_bt_mac_write
# objects
# ------------------------------------------------------------------------------

# compiling and linking
# ------------------------------------------------------------------------------
all: $(PROGRAM)
clean:
	-rm -f $(OBJS) $(LIB) $(PROGRAM)
$(PROGRAM): main.c
	$(CC) -L . -lm -o $@ $<
