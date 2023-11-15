#!/bin/bash
SERVER_PROCESS=rtsp-simple-server
SOURCE_PROCESS=bambusource2raw
SERVER_PID=`pidof $SERVER_PROCESS`
SOURCE_PID=`pidof $SOURCE_PROCESS`
P1PIP=192.168.12.34
P1PACCESSCODE=12345678

function start_rtsp_server() {
    if [[ -z "${SERVER_PID// }" ]]; then
        echo "starting rtsp-simple-server"
        pushd /bambu-bin/
        /bambu-bin/rtsp-simple-server 2>&1 > /dev/null &
        popd
        echo "wait for 5 seconds"
        sleep 5
    fi
}

start_rtsp_server

if [[ -n "${SOURCE_PID// }" ]]; then
    echo "bambusource2raw already running"
    exit 1
fi

echo "starting bambusource2raw"
pushd /bambu-bin/cfg
while :
do
    /bambu-bin/bambusource2raw start_stream_local -s $P1PIP -a $P1PACCESSCODE | /bambu-bin/ffmpeg -hide_banner -loglevel error -fflags nobuffer -flags low_delay -analyzeduration 10 -probesize 3200 -i - -c copy -f rtsp rtsp://127.0.0.1:8554/bbl
    sleep 30
    start_rtsp_server
done
popd
