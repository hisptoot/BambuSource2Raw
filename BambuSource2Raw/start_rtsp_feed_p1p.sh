#!/bin/bash
SERVER_PROCESS=rtsp-simple-server
SOURCE_PROCESS=bambusource2raw
SERVER_PID=`pidof $SERVER_PROCESS`
SOURCE_PID=`pidof $SOURCE_PROCESS`
P1PIP=192.168.12.34
P1PACCESSCODE=12345678

if [[ -z "${SERVER_PID// }" ]]; then
    echo "starting rtsp-simple-server"
    ./rtsp-simple-server 2>&1 > /dev/null &
    echo "wait for 5 seconds"
    sleep 5
fi

if [[ -n "${SOURCE_PID// }" ]]; then
    echo "bambusource2raw already running"
    exit 1
fi

echo "starting bambusource2raw"
./bambusource2raw start_stream_local -s $P1PIP -a $P1PACCESSCODE | ./ffmpeg -i - -c copy -f rtsp rtsp://127.0.0.1:8554/bbl