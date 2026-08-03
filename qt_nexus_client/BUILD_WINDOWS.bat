@echo off
setlocal

REM Set QTDIR before running this script, for example:
REM set QTDIR=C:\Qt\6.8.3\msvc2022_64

if "%QTDIR%"=="" (
  echo ERROR: QTDIR is not set.
  echo Example: set QTDIR=C:\Qt\6.8.3\msvc2022_64
  exit /b 1
)

if not exist "%QTDIR%\bin\windeployqt.exe" (
  echo ERROR: windeployqt.exe was not found under %QTDIR%\bin
  exit /b 1
)

cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DCMAKE_PREFIX_PATH="%QTDIR%"
if errorlevel 1 exit /b 1

cmake --build build --config Release
if errorlevel 1 exit /b 1

"%QTDIR%\bin\windeployqt.exe" --release --compiler-runtime build\Release\NexusClient.exe
if errorlevel 1 exit /b 1

if not exist build\Release\config mkdir build\Release\config
if exist config\firebase_config.json copy /Y config\firebase_config.json build\Release\config\firebase_config.json >nul

echo.
echo Build complete:
echo %CD%\build\Release\NexusClient.exe
endlocal
