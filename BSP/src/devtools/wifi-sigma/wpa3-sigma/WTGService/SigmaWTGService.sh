#!/bin/sh -e
#
# Copyright (c) 2014 Wi-Fi Alliance
#
# Permission to use, copy, modify, and/or distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.
#
# THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
# WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
# SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER
# RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
# NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE
# USE OR PERFORMANCE OF THIS SOFTWARE.
#

 #  File: SigmaWTGService.sh - The script to control Sigma WTG Service
 #	 This Service start PC-Endpoint and all the required control agents(for Testbed STAs)
 # 	 based on the configuration file - SigmaWTG.conf
 #
 #  Revision History:
 #        2008/08/15  -- Initially created by Ankur Vachhani
 #        


CONF_FILE=/etc/SigmaWTG.conf
SERVICE=/usr/bin/SigmaWTG

case "$1" in
    start)
        echo "Starting Sigma WTG Service"
#	$SERVICE $CONF_FILE 2>&1 >/dev/null&
	$SERVICE $CONF_FILE 2>&1 &
        ;;
    restart|reload|force-reload)
        echo "Stopping Sigma WTG Service"
	/usr/bin/killall -9 $SERVICE wfa_dut wfa_ca >/dev/null 2>&1
        echo "Starting Sigma WTG Service"
	$SERVICE $CONF_FILE >/dev/null 2>&1 &
        exit 3
        ;;
    stop)
        echo "Stopping Sigma WTG Service"
	/usr/bin/killall -9 $SERVICE wfa_dut wfa_ca >/dev/null 2>&1
        ;;
    *)
    	# This case is called by /etc/init.d/rc.local on system bootup
        echo "Starting Sigma WTG Service..."
	$SERVICE $CONF_FILE >/dev/null 2>&1 &
        exit 0
        ;;
esac

exit 0