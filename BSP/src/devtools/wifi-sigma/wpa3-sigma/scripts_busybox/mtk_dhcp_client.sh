#!/system/bin/sh
echo "mtk_dhcp_client.sh start DHCP client"
DHCP_CLIENT_APP_PATH=`cat mtk_ini.ini | grep DHCP_CLIENT_APP_PATH | cut -f2 -d'='`
CURRENT_PATH=`pwd`
echo "dhcp client path: "$DHCP_CLIENT_APP_PATH" interface: "$1

if [[ "$DHCP_CLIENT_APP_PATH" == *"busybox udhcpc"* ]]; then
	$DHCP_CLIENT_APP_PATH --timeout=3 --retries=10 -b -i $1 -S -s $CURRENT_PATH/udhcpc.sh -p /tmp/udhcpc-$1.pid
#	if [[ $? -ne 0 ]]; then
#		./$DHCP_CLIENT_APP_PATH --timeout=3 --retries=10 -b -i $1 -S -s $CURRENT_PATH/scripts_busybox/udhcpc.sh -p /tmp/udhcpc-$1.pid
#	fi
elif [[ "$DHCP_CLIENT_APP_PATH" == *"udhcpc"* ]]; then
	$DHCP_CLIENT_APP_PATH --timeout=3 --retries=10 -b -i $1 -S -s $CURRENT_PATH/udhcpc.sh -p /tmp/udhcpc-$1.pid
elif [[ "$DHCP_CLIENT_APP_PATH" == *"dhcpcd"* ]]; then
	$DHCP_CLIENT_APP_PATH $1
else
	echo $DHCP_CLIENT_APP_PATH" need to add cmd!!!!"
fi
