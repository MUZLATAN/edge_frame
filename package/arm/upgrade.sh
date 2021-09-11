#!/usr/bin/env bash
# set -xeuo pipefail
set -x

VERSION='99.1.0.9'
LOGFILE='/home/nvidia/FLOW/logfile'
SYS_SN=$(get_sn)

function remove_old_otad() {
    if [[ -f "/etc/supervisor/conf.d/otad.conf" ]]; then
        echo "**************remove otad.conf***************************"
        mv /etc/supervisor/conf.d/otad.conf /etc/supervisor/conf.d/otad.conf.bak
        supervisorctl update
    fi
}

function upload_log_druid() {
    payload_log="{\"name\":\"algo-metric\",\"data\":{\"camera_sn\":\"$SYS_SN\",\"time\":$(date +%s)000,\"action\":8,\"cv_message\":\"ota update failed\"}}"
    echo "$payload_log"
    curl --location --request POST 'https://proxy.meetwhale.com/chunnel/create' --header 'Content-Type: application/json' --data-raw "$payload_log"
}

function switch_startup_service() {
    /home/nvidia/FLOW/bin/service stop
}

function copy_algo_files() {
    #backup
    cp /home/nvidia/FLOW/bin/check /home/nvidia/FLOW/bin/check.bak
    cp /home/nvidia/FLOW/bin/service /home/nvidia/FLOW/bin/service.bak
    cp /home/nvidia/FLOW/config/service_default.json /home/nvidia/FLOW/config/service_default.json.bak

    cp -rvf ./files/home/nvidia/FLOW/* /home/nvidia/FLOW
    cp -rvf ./files/etc/* /etc/

    cp ./files/etc/cvlib.sh /etc/profile.d/
    chmod +x /etc/profile.d/cvlib.sh

    echo "start copy alog files to algo directory"
    logdir=/home/nvidia/FLOW/logs
    if [ ! -d "$logdir" ]; then
        mkdir -p $logdir
    fi
    if [ ! -f "$logdir/algo.err.log" ]; then
        touch $logdir/algo.err.log
        touch $logdir/algo.out.log
    fi

    chmod +x /home/nvidia/FLOW/bin/algo-${VERSION}

    sync

    ln -sf /home/nvidia/FLOW/bin/algo-${VERSION} /home/nvidia/FLOW/client
    chown -R whale:whale /home/nvidia/FLOW
    ldconfig
}

# this upgrade don't use it for safety, because of update ota
function generate_algo_dir() {
    if [[ ! -d "/home/whale/algo" ]]; then
        cp -rvf /home/nvidia/FLOW* /home/whale/algo
        if [ -d "/home/nvidia/FLOW" ]; then
            mv /home/nvidia/FLOW /home/nvidia/FLOW.bak
        fi
        chown -R whale:whale /home/whale/algo/

        touch /home/whale/algo/logs/algo.err.log
        touch /home/whale/algo/logs/algo.out.log

        #add ld_library_path and crontab task
        touch /etc/ld.so.conf.d/algo.conf
        echo "/home/whale/algo/lib" >/etc/ld.so.conf.d/algo.conf
        echo "----------------------------1"
        crontab_task=$(cat /var/spool/cron/crontabs/root | grep /home/whale/algo/bin/check)
        echo "----------------------------check value: " "X${crontab_task}"
        if [ "X" = "X${crontab_task}" ]; then
            echo "set check crontab task"
            echo "*/1 * * * * /home/whale/algo/bin/check" >>/var/spool/cron/crontabs/root
        fi

        echo $(date "+%F %T") "${VERSION} start update, first copy algo directory ."
        echo $(date "+%F %T") "${VERSION} start update, first copy algo directory ." >>${LOGFILE}
    fi
}

## check depend on package
#"************check current version depend on**************************************"
function check_depend_lib() {
    tengineFile=/home/nvidia/FLOW/lib/libtengine.so
    gprFile=/home/nvidia/FLOW/lib/libgpr.so.7
    tengineFileSize=$(du $tengineFile | awk '{print $1}')

    if [ -e /home/nvidia/FLOW/download_lib_failed ]; then
        upload_log_druid
    elif [ "$tengineFileSize" -le 88300 -o ! -e $tengineFile -o ! -e $gprFile ]; then
        newTengineLib=https://whale-cv-edge.oss-cn-shanghai.aliyuncs.com/algo-lib-1023fc5c21ef53041c851b8a776ea900.tar.gz
        libMd5code=1023fc5c21ef53041c851b8a776ea900
        libPath=algo-lib-1023fc5c21ef53041c851b8a776ea900.tar.gz
        wget $newTengineLib -O $libPath
        fileCheckCode=$(md5sum $libPath | awk '{print $1}')
        if [ $fileCheckCode != $libMd5code ]; then
            upload_log_druid
            echo " check depend lib error" >>${LOGFILE}
            touch /home/nvidia/FLOW/download_lib_failed
        else
            tar xzvf $libPath -C /home/nvidia/FLOW/
            echo $(date "+%F %T") " tar depend lib ok." $downloadPath >>${LOGFILE}
        fi

    fi
}

#"************check current version depend on**************************************"
## check depend on package

#**************************************************"
remove_old_otad
check_depend_lib
#**************************************************"

# stop FileTrans
pid=$(ps -ef | grep FLOW/bin/FileTrans | grep -v grep | awk '{print $2}')
kill -9 $pid

if [[ ! -d "/home/nvidia/FLOW/" ]]; then
    echo "/home/nvidia/FLOW don't exists"
    mkdir -p /home/nvidia/FLOW
    copy_algo_files

    # update should be able to restart related service
    # so no need restart
    supervisorctl update
else
    # upgrade
    echo "start updating ..."
    copy_algo_files
    echo ${VERSION} >/home/nvidia/FLOW/version

    if [ ! -e /home/nvidia/FLOW/switch ]; then
        switch_startup_service
        touch /home/nvidia/FLOW/switch
        echo $(date "+%F %T") "first switch the new ota policy.." >>${LOGFILE}
    fi

    supervisorctl update
    supervisorctl restart algo
    echo $(date "+%F %T") " algo ${VERSION} update ok ......" >>${LOGFILE}

fi
