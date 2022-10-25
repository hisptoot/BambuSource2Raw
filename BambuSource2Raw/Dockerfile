FROM ubuntu:18.04
RUN apt-get update && apt-get install -y curl
RUN mkdir -p /bambu-bin
RUN mkdir -p /bambu-bin/cfg
WORKDIR /bambu-bin
COPY bambusource2raw /bambu-bin/bambusource2raw
COPY start_rtsp_feed_docker.sh /bambu-bin/start_rtsp_feed_docker.sh
COPY ffmpeg /bambu-bin/ffmpeg
COPY rtsp-simple-server /bambu-bin/rtsp-simple-server
COPY rtsp-simple-server.yml /bambu-bin/rtsp-simple-server.yml
COPY libBambuSource.so /bambu-bin/libBambuSource.so
EXPOSE 8554
CMD ["/bin/bash", "/bambu-bin/start_rtsp_feed_docker.sh"]