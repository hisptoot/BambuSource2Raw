@ECHO OFF
SET ServerProcess=rtsp-simple-server.exe
SET SourceProcess=bambusource2raw.exe

TASKLIST | FINDSTR /I "%SourceProcess%"
IF ERRORLEVEL 1 (GOTO :StartRtspServer) ELSE (GOTO :ErrorSourceExist)

:StartRtspServer
TASKLIST | FINDSTR /I "%ServerProcess%"
IF ERRORLEVEL 1 (start "rtsp server" /B rtsp-simple-server.exe) ELSE (GOTO :StartStream)
timeout /t 5 /nobreak

:StartStream
bambusource2raw.exe | ffmpeg -fflags nobuffer -flags low_delay -analyzeduration 10 -probesize 3200 -i - -c copy -f rtsp rtsp://127.0.0.1:8554/bbl
GOTO :EOF

:ErrorSourceExist
ECHO "bambusource2raw already running"
:EOF
pause
