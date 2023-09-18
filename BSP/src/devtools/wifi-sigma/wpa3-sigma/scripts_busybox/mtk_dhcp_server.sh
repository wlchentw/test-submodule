#!/system/bin/sh
echo "mtk_dhcp_server.sh start DHCP server"
DHCP_SERVER_APP_PATH=`cat mtk_ini.ini | grep DHCP_SERVER_APP_PATH | cut -f2 -d'='`
DHCPD_LEASE_FILE_PATH=`cat mtk_ini.ini | grep DHCPD_LEASE_FILE_PATH | cut -f2 -d'='`
DHCPD_CONFIG_FILE_PATH=`cat mtk_ini.ini | grep DHCPD_CONFIG_FILE_PATH | cut -f2 -d'='`

echo "dhcp server path: "$DHCP_SERVER_APP_PATH" interface: "$1

if [[ "$DHCP_SERVER_APP_PATH" == *"udhcpd"* ]]; then
	echo "interface $1" > $DHCPD_CONFIG_FILE_PATH.tmp
	cat $DHCPD_CONFIG_FILE_PATH >> $DHCPD_CONFIG_FILE_PATH.tmp
	touch $DHCPD_LEASE_FILE_PATH;sync;sync;echo \"\" > $DHCPD_LEASE_FILE_PATH; $DHCP_SERVER_APP_PATH -fS $DHCPD_CONFIG_FILE_PATH.tmp &
elif [[ "$DHCP_SERVER_APP_PATH" == *"dnsmasq"* ]]; then
	$DHCP_SERVER_APP_PATH --no-resolv --no-poll --pid-file --dhcp-range=$1,$1,1h
elif [[ "$DHCP_SERVER_APP_PATH" == *"dhcpd"* ]]; then
	touch $DHCPD_LEASE_FILE_PATH;sync;sync;echo \"\" > $DHCPD_LEASE_FILE_PATH; $DHCP_SERVER_APP_PATH -cf $DHCPD_CONFIG_FILE_PATH -lf $DHCPD_LEASE_FILE_PATH $1 &
elif [[ "$DHCP_SERVER_APP_PATH" == *"ndc"* ]]; then
	$DHCP_SERVER_APP_PATH tether start $DHCPD_IP_POOL_S $DHCPD_IP_POOL_E
else
	echo $DHCP_SERVER_APP_PATH need to add cmd!!!!
fi
