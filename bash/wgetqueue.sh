#!/bin/bash

shopt -s nullglob

QPATH=$HOME/.nyaqueue

mkdir -p "$QPATH"
mkdir -p "$QPATH/requests"

append_queue(){
    for a in "$@"
    do
        r=`mktemp --tmpdir="$QPATH/requests"`
        echo "$a" > "$r"
    done
}

start_daemon(){
     if [ -f "$QPATH/wgetqueue.pid" ]; then
        echo "Daemon is already running!"
        exit 1
    fi
    logger "Starting daemon"
    bash  wgetqueue.sh --daemon &
    echo "$!" > $QPATH/wgetqueue.pid
}

stop_daemon(){
    if [ ! -f "$QPATH/wgetqueue.pid" ]; then
        echo "Daemon is not running!"
        exit 1
    fi
    logger "Stopping wget daemon"
    kill -9 `cat $QPATH/wgetqueue.pid`
    rm "$QPATH/wgetqueue.pid"
}

daemon_loop(){
    logger "wget daemon is running"
    trap exit_trap 0
    while [ 1 ]
    do
        for a in "$QPATH/requests"/*
        do
            url=`cat "$a"`
            rm "$a"
            wget -c "$url"
        done

        sleep 1
    done
}

queue_main(){
 case "$1" in
        "start")
            start_daemon
            ;;
        "stop")
            stop_daemon
            ;;
        *)
            append_queue "$@"
            ;;
    esac
}

exit_trap(){
    logger "Exiting wgetqueue"
}



main(){
    if [[ "$1" != "--daemon" ]]; then
        queue_main $@
    else
        daemon_loop
    fi
        
}


tmp(){
    if [[ ! -f "$QPATH/running" ]]; then
        touch "$QPATH/running"
        trap "rm -rf '$QPATH/running'" 0
    fi 
}

main "$@"
