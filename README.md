# *USE AT YOUR OWN RISK!*

# How to use
## Prepare
1. Open BambuStudio and Login account
2. Goto Devices Tab and view your machine status
3. Close BambuStudio and it will generate `BambuNetworkEngine.conf` under `X:\Users\[user_name]\AppData\Roaming\BambuStudio`
4. Download release archive of this project or just compile your own.
        

## Docker Setup

Quick and easy way to setup BambuSource2Raw is through the provided Docker builder.

```bash
git clone https://github.com/hisptoot/BambuSource2Raw
cd BambuSource2Raw/BambuSource2Raw/
docker build -t bblrtsp:v1 .
docker run --rm --name BambuRTSP-1 -p 8554:8554 -v "${HOME}/.config/BambuStudio:/bambu-bin/cfg:ro" bblrtsp:v1
```

## Linux
1. 
        apt-get install -y libcurl4
        
2. Download <https://johnvansickle.com/ffmpeg/releases/ffmpeg-release-amd64-static.tar.xz>

   Extract `ffmpeg` to `release` dir
    
3. Download <https://github.com/aler9/rtsp-simple-server/releases/download/v0.20.0/rtsp-simple-server_v0.20.0_linux_amd64.tar.gz>

   Extract `rtsp-simple-server` and `rtsp-simple-server.yml` to `release` dir

4. Download <https://upgrade-file.bambulab.cn/studio/plugins/01.03.00.02/linux_01.03.00.02.zip>

   Extract `libBambuSource.so` to `release` dir

5. Start
    1. Start in host shell:

            cd release-dir
            cp BambuNetworkEngine.conf .
            ./start_rtsp_feed.sh
        
    2. Start in docker:

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

3. Download <https://upgrade-file.bambulab.cn/studio/plugins/01.03.00.02/win_01.03.00.02.zip>
   
   Extract `BambuSource.dll` to `release` dir
        
4. start `start_rtsp_feed.bat`

5. Use [VLC](https://www.videolan.org/vlc/) or something else to view the live stream: `rtsp://127.0.0.1:8554/bbl` or `rtsp://[ip of pc]:8554/bbl`
    
# How to compile
## Linux
    apt-get install -y unzip libcurl4 libcurl4-openssl-dev 
    cd BambuSource2Raw
    make -f Makefile.linux

# Windows
1. Download and Install Windows Driver Kit Version 7.1.0 from <https://www.microsoft.com/en-us/download/details.aspx?id=11800>

   Open 'x64 Free Build Environment'

        cd /d [Project Root Dir]
        build

2. Download <https://www.gyan.dev/ffmpeg/builds/ffmpeg-git-full.7z>

   Extract `bin\ffmpeg.exe` to `win-build/amd64` dir
    
3. Download <https://github.com/aler9/rtsp-simple-server/releases/download/v0.20.0/rtsp-simple-server_v0.20.0_windows_amd64.zip>

   Extract `rtsp-simple-server.exe` and `rtsp-simple-server.yml` to `win-build/amd64` dir

4. Download <https://upgrade-file.bambulab.cn/studio/plugins/01.03.00.02/win_01.03.00.02.zip>

   Extract `BambuSource.dll` to `win-build/amd64` dir

5. Copy `start_rtsp_feed.bat` to `win-build/amd64` dir
        
# *Remark*
Currently login function is not implemented.

If bambusource2raw fails when starting stream, try to refresh token or relogin in BambuStudio and generate a new `BambuNetworkEngine.conf`.
