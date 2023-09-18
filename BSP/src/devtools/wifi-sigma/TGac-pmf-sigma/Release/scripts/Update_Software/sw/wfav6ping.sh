#echo $@
busybox ping -6 $@ &
echo PID=$! > /mnt/asec/pingpid.txt