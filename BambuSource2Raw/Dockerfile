FROM debian:latest as builder
RUN set -e; apt-get update; apt-get install -y curl libcurl4 unzip libcurl4-openssl-dev build-essential; \
  apt-get clean; rm -rf /var/cache/apt;
RUN mkdir -p /app/build
WORKDIR /app
COPY Makefile.linux ./
ENV outdir="./build"
ENV MAKEFILES="./Makefile.linux"
RUN make libBambuSource.so
RUN make ffmpeg
RUN make rtsp-simple-server
COPY *.c *.h *.cpp ./
RUN make bambusource2raw

FROM debian:latest as runner
COPY --from=builder /app/build/* /usr/local/bin/
COPY --from=builder /app/build/*.yml /bambu-bin/
RUN set -e; apt-get update; apt-get install -y libcurl4 curl; \
  apt-get clean; rm -rf /var/cache/apt;
COPY docker_entrypoint.sh /usr/local/bin
EXPOSE 8554
ENTRYPOINT ["docker_entrypoint.sh"]
