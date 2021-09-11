#!/bin/sh
#set -euo pipefail
set -x

VERSION='8.0.4.1'
LOGFILE='/mnt/whale/service/algo/logs/logfile'
SYS_SN=$(cat /opt/sn)

function upload_log_druid() {
    payload_log="{\"name\":\"algo-metric\",\"data\":{\"camera_sn\":\"$SYS_SN\",\"time\":$(date +%s)000,\"action\":8,\"cv_message\":\"ota update failed\"}}"
    echo "$payload_log"
    /mnt/whale/tools/curl --location --request POST 'https://proxy.meetwhale.com/chunnel/create' --header 'Content-Type: application/json' --data-raw "$payload_log"
}

function copy_algo_files() {
    #backup
    cp /mnt/whale/service/algo/bin/check /mnt/whale/service/algo/bin/check.bak
    cp /mnt/whale/service/algo/config/service_default.json /mnt/whale/service/algo/config/service_default.json.bak

    cp -rvf ./files/mnt/whale/service/algo/* /mnt/whale/service/algo/
    cp -rvf ./files/mnt/whale/daemon/* /mnt/whale/daemon/

    echo "start copy alog files to algo directory"
    logdir=/mnt/whale/service/algo/logs
    if [ ! -d "$logdir" ]; then
        mkdir -p $logdir
    fi
    if [ ! -f "$logdir/algo.err.log" ]; then
        touch $logdir/algo.err.log
        touch $logdir/algo.out.log
    fi

    chmod +x /mnt/whale/service/algo/bin/algo-${VERSION}

    sync && sync && sleep 5

    ln -sf /mnt/whale/service/algo/bin/algo-${VERSION} /mnt/whale/service/algo/client
    export LD_LIBRARY_PATH=/mnt/whale/service/algo/lib:/mnt/whale/tools/lib:$LD_LIBRARY_PATH
}

echo client_sn:${SYS_SN} >/mnt/whale/service/algo/sys_sn
echo camera_sn:${SYS_SN} >/mnt/whale/service/algo/camera_sn

if [[ ! -d "/mnt/whale/service/algo/" ]]; then
    echo "/mnt/whale/service/algo don't exists"
    mkdir -p /mnt/whale/service/algo
fi

# upgrade
echo $(date "+%F %T") " ${VERSION} start updating ......" >>${LOGFILE}
copy_algo_files
echo ${VERSION} >/mnt/whale/service/algo/version
kill -9 $(pidof client)
echo $(date "+%F %T") " algo ${VERSION} update ok ......" >>${LOGFILE}
