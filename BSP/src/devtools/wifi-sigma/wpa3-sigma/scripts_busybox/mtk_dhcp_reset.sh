#!/system/bin/sh
echo "mtk_dhcp_reset.sh reset DHCP "

DHCP_SERVER_APP_PATH=`cat mtk_ini.ini | grep DHCP_SERVER_APP_PATH | cut -f2 -d'='`
if [[ "$DHCP_SERVER_APP_PATH" == *"busybox udhcpd"* ]]; then
	killall busybox
elif [[ "$DHCP_SERVER_APP_PATH" == *"udhcpd"* ]]; then
	killall udhcpd
elif [[ "$DHCP_SERVER_APP_PATH" == *"ndc"* ]]; then
	ndc tether stop
elif [[ "$DHCP_SERVER_APP_PATH" == *"dnsmasq"* ]]; then
	killall dnsmasq
else
	echo "need reset cmd in mtk_dhcp_reset.sh !"
fi
DHCP_CLIENT_APP_PATH=`cat mtk_ini.ini | grep DHCP_CLIENT_APP_PATH | cut -f2 -d'='`
if [[ "$DHCP_CLIENT_APP_PATH" == *"busybox udhcpc"* ]]; then
	killall busybox
elif [[ "$DHCP_CLIENT_APP_PATH" == *"udhcpc"* ]]; then
	killall udhcpc
elif [[ "$DHCP_CLIENT_APP_PATH" == *"dhcpcd"* ]]; then
	busybox killall dhcpcd
else
	echo "need reset cmd in mtk_dhcp_reset.sh !"
fi
