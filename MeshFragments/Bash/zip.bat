@echo off
cd %1
%2
for /r %%f in (*.%3) do (
	zip -q -9 "%%~nxf.zip" "%%~nxf"
)
for /r %%f in (*.%3) do (
	del "%%~nxf"
)
pause
