#!/bin/bash
set -ueo pipefail
RTSP_URL="rtsp://127.0.0.1:8554/bbl"
cd /bambu-bin
rtsp-simple-server 2>&1 > /dev/null &
sleep 10s
cd cfg
echo "Starting stream at: ${RTSP_URL}"
bambusource2raw | ffmpeg -hide_banner -loglevel error -i - -c copy -f rtsp "${RTSP_URL}"
