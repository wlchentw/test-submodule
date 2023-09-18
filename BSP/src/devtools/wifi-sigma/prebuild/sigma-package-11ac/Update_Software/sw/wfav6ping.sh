#echo $@
busybox ping -6 $@ &
echo PID=$! > /tmp/asec/pingpid.txt