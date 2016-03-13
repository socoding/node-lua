@echo off
REM lua dll is essential but not mime
if /i "%~2" == "lua"  (copy "%~1\deps\lua\lua.dll" "%~1" & exit)
if /i "%~2" == "mime" (copy "%~1\clib\mime\mime.dll" "%~1\clib\" & exit)
