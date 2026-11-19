@echo off
setlocal
pushd "%~dp0.." || exit /b 1
if not defined VSCMD_VER (
    for /f "usebackq tokens=*" %%i in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "vsdir=%%i"
    call :compiler || exit /b 2
)
if /I not "%VSCMD_ARG_TGT_ARCH%"=="x64" (
    echo Abre o Developer Command Prompt x64 do Visual Studio.
    exit /b 2
)
if not exist bin mkdir bin
set "work=%TEMP%\circe-build-%RANDOM%-%RANDOM%"
mkdir "%work%" || exit /b 1
set "base=%CD%"
pushd "%work%"
set "opts=/nologo /std:c++20 /O2 /EHsc /MT /utf-8 /DNOMINMAX /DUNICODE /D_UNICODE /D_WIN32_WINNT=0x0A00"
rc /nologo /c 65001 /I "%base%\instalador" /fo idioma.res "%base%\instalador\idioma.rc" || goto failed
cl %opts% /LD "%base%\instalador\idioma.cpp" idioma.res /Fe:"%base%\bin\CIRCEIdioma.dll" /link /IMPLIB:idioma.lib user32.lib || goto failed
rc /nologo /I "%base%\src" /fo circe.res "%base%\src\circe.rc" || goto failed
cl %opts% "%base%\src\app.cpp" "%base%\src\language_win.cpp" "%base%\src\platform.cpp" "%base%\src\panel.cpp" circe.res /Fe:"%base%\bin\CIRCE.exe" /link /SUBSYSTEM:WINDOWS /MANIFEST:NO user32.lib kernel32.lib gdi32.lib shell32.lib advapi32.lib comdlg32.lib comctl32.lib ole32.lib uuid.lib dwmapi.lib uxtheme.lib || goto failed
cl %opts% "%base%\native\window.cpp" "%base%\native\gravar.cpp" /Fe:"%base%\bin\CIRCEJanela.exe" /link /SUBSYSTEM:WINDOWS user32.lib shell32.lib gdi32.lib runtimeobject.lib d3d11.lib dxgi.lib ole32.lib uuid.lib advapi32.lib || goto failed
set "result=0"
goto cleanup
:failed
set "result=1"
:cleanup
popd
rmdir /s /q "%work%"
popd
exit /b %result%
:compiler
if not defined vsdir exit /b 1
call "%vsdir%\VC\Auxiliary\Build\vcvars64.bat"
exit /b %ERRORLEVEL%
