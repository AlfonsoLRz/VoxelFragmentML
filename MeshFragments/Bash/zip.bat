@echo off
cd %1
%2
for /r %%f in (*.stl) do (
	tar -cf "%%~nxf.zip" "%%~nxf"
)
for /r %%f in (*.stl) do (
	del "%%~nxf"
)
for /f %%i in ('cd') do set ADDRESS=%%~nxi
scp -pr -i "D:/ssh/id_rsa_alfonso_titan" -P "6922" "%1" "alfonso@titan.ujaen.es:/home/alfonso/Fragmentos/%ADDRESS%/"
pause
