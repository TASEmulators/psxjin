del /s ..\output\psxjin.zip
upx ..\output\psxjin-release.exe
IF ERRORLEVEL 1 IF NOT ERRORLEVEL 2 GOTO UPXFailed
cd ..\output
copy psxjin-release.exe psxjin.exe
..\win32\zip -X -9 -r ..\output\PSXjin.zip psxjin.exe bios\*.txt lua51.dll psxjin-instructions.txt plugins\*.dll
cd ..\win32
GOTO end

:UPXFailed
CLS
echo.
echo PSXJIN.EXE is either compiling or running.
echo Close it or let it finish before trying this script again.
pause

:end
