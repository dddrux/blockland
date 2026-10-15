@echo off
rem Gera dist\setup.exe no Windows com 2 cliques: instala MSYS2 e Inno Setup (winget) se faltarem, compila o jogo e monta o instalador.
setlocal
cd /d "%~dp0.."
where winget >nul 2>&1
if errorlevel 1 goto nowinget
if not exist C:\msys64\usr\bin\bash.exe winget install -e --id MSYS2.MSYS2 --accept-package-agreements --accept-source-agreements
if not exist C:\msys64\usr\bin\bash.exe goto nomsys
call :findiscc
if defined ISCC goto build
winget install -e --id JRSoftware.InnoSetup --accept-package-agreements --accept-source-agreements
call :findiscc
if not defined ISCC goto noinno
:build
set MSYSTEM=MINGW64
set "ISCC_EXE=%ISCC%"
C:\msys64\usr\bin\bash.exe -lc "pacman -S --needed --noconfirm zip mingw-w64-x86_64-gcc mingw-w64-x86_64-SDL2 && cd \"$(cygpath -u '%CD%')\" && ISCC_EXE=\"$(cygpath -u \"$ISCC_EXE\")\" ./build-windows.sh --installer"
if errorlevel 1 goto fail
echo.
echo PRONTO: %CD%\dist\setup.exe
explorer "%CD%\dist"
pause
exit /b 0
:findiscc
set "ISCC="
if exist "%ProgramFiles(x86)%\Inno Setup 6\ISCC.exe" set "ISCC=%ProgramFiles(x86)%\Inno Setup 6\ISCC.exe"
if exist "%ProgramFiles%\Inno Setup 6\ISCC.exe" set "ISCC=%ProgramFiles%\Inno Setup 6\ISCC.exe"
if exist "%LOCALAPPDATA%\Programs\Inno Setup 6\ISCC.exe" set "ISCC=%LOCALAPPDATA%\Programs\Inno Setup 6\ISCC.exe"
exit /b 0
:nowinget
echo Precisa do winget (Windows 10/11 atualizado) ou instale MSYS2 e Inno Setup 6 manualmente.
goto fail
:nomsys
echo MSYS2 nao foi instalado.
goto fail
:noinno
echo Inno Setup 6 nao foi instalado.
:fail
echo FALHOU - veja as mensagens acima.
pause
exit /b 1
