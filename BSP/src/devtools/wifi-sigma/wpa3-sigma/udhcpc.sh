#!/system/bin/sh
# 檢查是否有來自 udhcpc 的指令
[ -z "$1" ] && echo "Error: should be called from udhcpc" && exit 1
# 設定 ifconfig 參數
[ -n "$broadcast" ] && BROADCAST="broadcast $broadcast"
[ -n "$subnet" ] && NETMASK="netmask $subnet"

case "$1" in
  deconfig)
    ifconfig $interface 0.0.0.0
    ;;

  renew|bound)
    # 設定 IP
    ifconfig $interface $ip $BROADCAST $NETMASK
    # 設定 routing table
    if [ -n "$router" ] ; then
      echo "deleting routers"
      while route del default gw 0.0.0.0 dev $interface ; do
        :
      done
      for i in $router ; do
        route add default gw $i dev $interface
      done
    fi
    # 設定 dns server
    echo -n > "/etc/resolv.conf"
    [ -n "$domain" ] && echo search $domain >> "/etc/resolv.conf"
    for i in $dns ; do
      echo adding dns $i
      echo nameserver $i >> "/etc/resolv.conf"
    done
    ;;
esac

exit 0
