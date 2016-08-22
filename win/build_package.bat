@echo off
if NOT EXIST C:\gtk3\bin (
    echo packge needs GTK+-3.0 or above located in C:\gtk3 directory
    goto :end
)
REM borrowed from vcvars32.bat
for %%f in (HKLM\SOFTWARE HKCU\SOFTWARE HKLM\SOFTWARE\Wow6432Node HKCU\SOFTWARE\Wow6432Node) do (
	for /F "tokens=1,2*" %%i in ('reg query "%%f\Microsoft\VisualStudio\SxS\VS7" /v "10.0"') DO (
		if "%%i"=="10.0" (
			SET "VS10DIR=%%k"
			goto :found
		)
	)
)
echo package needs Visual Studio 10 to build
goto :end
:found

@call "%VS10DIR%\VC\bin\vcvars32.bat"
msbuild /p:Configuration=Release reminderer.vcxproj
echo invoking iexpress...
iexpress /N iexpress.sed
:end
pause
