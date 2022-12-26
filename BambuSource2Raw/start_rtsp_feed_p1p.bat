@ECHO OFF
SET ServerProcess=rtsp-simple-server.exe
SET SourceProcess=bambusource2raw.exe
SET P1PIP=192.168.12.34
SET P1PACCESSCODE=12345678

TASKLIST | FINDSTR /I "%SourceProcess%"
IF ERRORLEVEL 1 (GOTO :StartRtspServer) ELSE (GOTO :ErrorSourceExist)

:StartRtspServer
TASKLIST | FINDSTR /I "%ServerProcess%"
IF ERRORLEVEL 1 (start "rtsp server" /B rtsp-simple-server.exe) ELSE (GOTO :StartStream)
timeout /t 5 /nobreak

:StartStream
bambusource2raw.exe start_stream_local -s %P1PIP% -a %P1PACCESSCODE% | ffmpeg -i - -c copy -f rtsp rtsp://127.0.0.1:8554/bbl
GOTO :EOF

:ErrorSourceExist
ECHO "bambusource2raw already running"
:EOF
pause