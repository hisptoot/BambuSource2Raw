# *USE AT YOUR OWN RISK!*

# How to use
## Prepare
1. Download release archive of this project or just compile your own.

## Linux
1. `apt-get install -y libcurl4`
        
2. Download <https://johnvansickle.com/ffmpeg/releases/ffmpeg-release-amd64-static.tar.xz>

   Extract `ffmpeg` to `release` dir
    
3. Download <https://github.com/aler9/rtsp-simple-server/releases/download/v0.20.0/rtsp-simple-server_v0.20.0_linux_amd64.tar.gz>

   Extract `rtsp-simple-server` and `rtsp-simple-server.yml` to `release` dir

4. Download <https://public-cdn.bambulab.cn/upgrade/studio/plugins/01.04.00.15/linux_01.04.00.15.zip>

   Extract `libBambuSource.so` to `release` dir

5. Get dev id and access code of the machine

   `./bambusource2raw list_dev -u <account_name> -p <password> -r <region: us cn>`

6. Generate `BambuNetworkEngine.conf`
    
    `./bambusource2raw gen_cfg -u <account_name> -p <password> -r <region: us cn> -d <dev_id>` 

7. Start
    ### For X1/X1C

    #### Start in host shell:

       cd release-dir
       cp BambuNetworkEngine.conf .
       ./start_rtsp_feed.sh
        
    #### Start in docker:

       mkdir bambu_cfg
       cp -f BambuNetworkEngine.conf bambu_cfg
            
       cd release-dir
       docker build -t bblrtsp:v1 .
       docker run --name BambuRTSP-1 -p 8554:8554 -v <FULL_PATH_OF_bambu_cfg_DIR>:/bambu-bin/cfg -d bblrtsp:v1

    ### For P1P

    #### Start in host shell:

     Modify `P1PIP` and `P1PACCESSCODE` in `start_rtsp_feed_p1p.sh` to the exact value

       cd release-dir
       cp BambuNetworkEngine.conf .
       ./start_rtsp_feed_p1p.sh
        
    #### Start in docker:

     Modify `P1PIP` and `P1PACCESSCODE` in `start_rtsp_feed_docker_p1p.sh` to the exact value

     Modify the entry of `Dockerfile` to `/bambu-bin/start_rtsp_feed_docker_p1p.sh`

       mkdir bambu_cfg
       cp -f BambuNetworkEngine.conf bambu_cfg
            
       cd release-dir
       docker build -t bblrtsp:v1 .
       docker run --name BambuRTSP-1 -p 8554:8554 -v <FULL_PATH_OF_bambu_cfg_DIR>:/bambu-bin/cfg -d bblrtsp:v1
            
## Windows
1. Download <https://www.gyan.dev/ffmpeg/builds/ffmpeg-git-full.7z>
   
   Extract `bin\ffmpeg.exe` to `release` dir
    
2. Download <https://github.com/aler9/rtsp-simple-server/releases/download/v0.20.0/rtsp-simple-server_v0.20.0_windows_amd64.zip>
   
   Extract `rtsp-simple-server.exe` and `rtsp-simple-server.yml` to `release` dir

3. Download <https://public-cdn.bambulab.cn/upgrade/studio/plugins/01.04.00.16/win_01.04.00.16.zip>
   
   Extract `BambuSource.dll` to `release` dir
        
4. Get dev id and access code of the machine

   `bambusource2raw.exe list_dev -u <account_name> -p <password> -r <region: us cn>`

5. Generate `BambuNetworkEngine.conf`
    
    `bambusource2raw.exe gen_cfg -u <account_name> -p <password> -r <region: us cn> -d <dev_id>` 

6. Put `BambuNetworkEngine.conf` in the same folder with `bambusource2raw.exe`

7. Start

    ### For X1/X1C

      start `start_rtsp_feed.bat`

    ### For P1P

      Modify `P1PIP` and `P1PACCESSCODE` in `start_rtsp_feed_p1p.bat` to the exact value

      start `start_rtsp_feed_p1p.bat`

## View live stream
Use [VLC](https://www.videolan.org/vlc/) or something else to view the live stream: `rtsp://127.0.0.1:8554/bbl` or `rtsp://[ip of pc]:8554/bbl`
    
# How to compile
## Linux
    apt-get install -y unzip libcurl4 libcurl4-openssl-dev 
    cd BambuSource2Raw
    make -f Makefile.linux

## Windows
1. Download and Install Windows Driver Kit Version 7.1.0 from <https://www.microsoft.com/en-us/download/details.aspx?id=11800>

   Open 'x64 Free Build Environment'

       cd /d [Project Root Dir]
       build

2. Download <https://www.gyan.dev/ffmpeg/builds/ffmpeg-git-full.7z>

   Extract `bin\ffmpeg.exe` to `win-build/amd64` dir
    
3. Download <https://github.com/aler9/rtsp-simple-server/releases/download/v0.20.0/rtsp-simple-server_v0.20.0_windows_amd64.zip>

   Extract `rtsp-simple-server.exe` and `rtsp-simple-server.yml` to `win-build/amd64` dir

4. Download <https://public-cdn.bambulab.cn/upgrade/studio/plugins/01.04.00.16/win_01.04.00.16.zip>

   Extract `BambuSource.dll` to `win-build/amd64` dir

5. Copy `start_rtsp_feed.bat` to `win-build/amd64` dir
        
# *Remark*
If bambusource2raw fails when starting stream, try to generate a new `BambuNetworkEngine.conf`.
