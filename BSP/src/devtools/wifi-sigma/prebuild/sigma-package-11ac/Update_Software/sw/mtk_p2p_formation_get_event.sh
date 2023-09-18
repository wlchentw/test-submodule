#!/system/bin/sh

LOGF='/mnt/asec/mtk_p2p_formation.log'

rm ${LOGF}
echo ${*} | grep P2P_EVENT_FORMATION | cut -f3- -d' ' > ${LOGF}
kill -TERM ${PPID}
