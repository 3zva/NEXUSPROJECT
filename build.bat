@echo off
echo Building Combined RecoilMaster native C++ app...
echo.

set "MINGW_ROOT=C:\msys64\mingw64"
set "QT_BIN=%MINGW_ROOT%\bin"
set "ASSET_SOURCE=%CD%\loader-assets"
set "OUTPUT_DIR=%USERPROFILE%\Desktop\NEXUS"
if exist "%QT_BIN%\cmake.exe" (
    echo Building native Recoil GUI...
    set "PATH=%QT_BIN%;%PATH%"
    "%QT_BIN%\cmake.exe" -S qt_recoil_gui -B qt_recoil_gui\build-mingw -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=%MINGW_ROOT% -DCMAKE_SUPPRESS_REGENERATION=ON
    if errorlevel 1 exit /b 1
    "%QT_BIN%\cmake.exe" --build qt_recoil_gui\build-mingw --config Release
    if errorlevel 1 exit /b 1
) else (
    echo Warning: Qt build tools were not found at %QT_BIN%.
)

REM Create final NEXUS output directory
if not exist "%OUTPUT_DIR%" mkdir "%OUTPUT_DIR%"
powershell -NoProfile -ExecutionPolicy Bypass -Command "$p=(Resolve-Path '%OUTPUT_DIR%\test-install' -ErrorAction SilentlyContinue).Path; if($p){Get-Process | Where-Object {$_.Path -and $_.Path.StartsWith($p, [System.StringComparison]::OrdinalIgnoreCase)} | Stop-Process -Force}" >nul 2>nul
del /Q "%OUTPUT_DIR%\*.exe" 2>nul
del /Q "%OUTPUT_DIR%\*.log" 2>nul
del /Q "%OUTPUT_DIR%\*.res" 2>nul
del /Q "%OUTPUT_DIR%\*.o" 2>nul
del /Q "%OUTPUT_DIR%\WebView2Loader.dll" 2>nul
del /Q "%OUTPUT_DIR%\firebase_config.json" 2>nul
del /Q "%OUTPUT_DIR%\nexus.ico" 2>nul
del /Q "%OUTPUT_DIR%\vision_overlay_config.txt" 2>nul
del /Q "%OUTPUT_DIR%\List A and B for Overlay window.txt" 2>nul
del /Q "%OUTPUT_DIR%\run_vision_tool.bat" 2>nul
if exist "%OUTPUT_DIR%\recoilmaster-main" rmdir /S /Q "%OUTPUT_DIR%\recoilmaster-main"
if exist "%OUTPUT_DIR%\configs" rmdir /S /Q "%OUTPUT_DIR%\configs"
if exist "%OUTPUT_DIR%\nexus-ui" rmdir /S /Q "%OUTPUT_DIR%\nexus-ui"
if exist "%OUTPUT_DIR%\recoil-ui" rmdir /S /Q "%OUTPUT_DIR%\recoil-ui"
if exist "%OUTPUT_DIR%\runtime" rmdir /S /Q "%OUTPUT_DIR%\runtime"
if exist "%OUTPUT_DIR%\test-install" rmdir /S /Q "%OUTPUT_DIR%\test-install"

REM Compile the C++ application
windres app.rc -O coff -o "%OUTPUT_DIR%\app_icon.o"
if errorlevel 1 exit /b 1
g++ -std=c++17 -O3 -s -fdata-sections -ffunction-sections -Wl,--gc-sections main.cpp "%OUTPUT_DIR%\app_icon.o" -Ithird_party\Microsoft.Web.WebView2\build\native\include -o "%OUTPUT_DIR%\Nexus Loader.exe" -mwindows -luser32 -lgdi32 -lcomctl32 -lcomdlg32 -lshell32 -lws2_32 -ladvapi32 -lwinmm -lwinhttp -lole32 -luuid
del /Q "%OUTPUT_DIR%\app_icon.o" 2>nul

if %ERRORLEVEL% EQU 0 (
    echo Build successful!
    echo Output: %OUTPUT_DIR%\Nexus Loader.exe

    if exist "%MINGW_ROOT%\include\opencv4\opencv2\opencv.hpp" (
        echo Building silent overlay...
        if not exist "%OUTPUT_DIR%\runtime" mkdir "%OUTPUT_DIR%\runtime"
        g++ -std=c++20 -O2 vision_automation_tool.cpp -o "%OUTPUT_DIR%\runtime\VisionAutomationTool.exe" -mwindows -I"%MINGW_ROOT%\include\opencv4" -L"%MINGW_ROOT%\lib" -ltesseract -lleptonica -lopencv_imgproc -lopencv_core -lgdi32 -luser32 -lshell32 -lwinhttp
        if errorlevel 1 exit /b 1
    ) else (
        echo Warning: OpenCV/Tesseract headers were not found. Overlay was not rebuilt.
    )
    
    REM Copy only the native app resources. Do not ship the old Python source or PyInstaller executable.
    echo.
    echo Copying UI resources...
    if exist recoilmaster-main (
        mkdir "%OUTPUT_DIR%\recoilmaster-main"
        if not exist "%OUTPUT_DIR%\runtime" mkdir "%OUTPUT_DIR%\runtime"
        copy recoilmaster-main\PeripheralCoreManager.html "%OUTPUT_DIR%\recoilmaster-main\" >nul
        copy recoilmaster-main\associated_icon.png "%OUTPUT_DIR%\recoilmaster-main\" >nul
        if exist "%ASSET_SOURCE%\firebase_config.json" copy "%ASSET_SOURCE%\firebase_config.json" "%OUTPUT_DIR%\runtime\" >nul
        copy third_party\Microsoft.Web.WebView2\runtimes\win-x64\native\WebView2Loader.dll "%OUTPUT_DIR%\runtime\" >nul
        if not exist "%OUTPUT_DIR%\runtime\VisionAutomationTool.exe" echo Warning: VisionAutomationTool.exe was not built.
        if exist qt_recoil_gui\build-mingw\NexusClient.exe (
            mkdir "%OUTPUT_DIR%\recoil-ui"
            copy qt_recoil_gui\build-mingw\NexusClient.exe "%OUTPUT_DIR%\recoil-ui\Nexus Recoil.exe" >nul
            if exist "%QT_BIN%\windeployqt.exe" "%QT_BIN%\windeployqt.exe" --release --compiler-runtime "%OUTPUT_DIR%\recoil-ui\Nexus Recoil.exe" >nul
            mkdir "%OUTPUT_DIR%\recoil-ui\config"
            if exist qt_recoil_gui\config\firebase_config.json copy qt_recoil_gui\config\firebase_config.json "%OUTPUT_DIR%\recoil-ui\config\" >nul
            if exist qt_recoil_gui\config\firebase_config.example.json copy qt_recoil_gui\config\firebase_config.example.json "%OUTPUT_DIR%\recoil-ui\config\" >nul
            if exist qt_recoil_gui\config\license_config.json copy qt_recoil_gui\config\license_config.json "%OUTPUT_DIR%\recoil-ui\config\" >nul
            if exist qt_recoil_gui\config\license_config.example.json copy qt_recoil_gui\config\license_config.example.json "%OUTPUT_DIR%\recoil-ui\config\" >nul
            if exist firebase_config.json copy firebase_config.json "%OUTPUT_DIR%\recoil-ui\config\firebase_config.json" >nul
            if exist "%ASSET_SOURCE%\firebase_config.json" copy "%ASSET_SOURCE%\firebase_config.json" "%OUTPUT_DIR%\recoil-ui\config\firebase_config.json" >nul
            if exist "Nexus Config.json" copy "Nexus Config.json" "%OUTPUT_DIR%\recoil-ui\Nexus Config.json" >nul
        ) else (
            echo Warning: native Qt Recoil GUI was not found. Build qt_recoil_gui first.
        )
        if exist "List A and B for Overlay window.txt" copy "List A and B for Overlay window.txt" "%OUTPUT_DIR%\runtime\" >nul
        if exist vision_overlay_config.txt copy vision_overlay_config.txt "%OUTPUT_DIR%\runtime\" >nul
        if exist "%ASSET_SOURCE%\login-gui-codex-package\html" (
            xcopy "%ASSET_SOURCE%\login-gui-codex-package\html" "%OUTPUT_DIR%\nexus-ui\html" /E /I /Y >nul
        ) else (
            echo Warning: Nexus login HTML assets were not found at %ASSET_SOURCE%\login-gui-codex-package\html.
            exit /b 1
        )
        if exist "%ASSET_SOURCE%\login-gui-codex-package\assets" (
            xcopy "%ASSET_SOURCE%\login-gui-codex-package\assets" "%OUTPUT_DIR%\nexus-ui\assets" /E /I /Y >nul
        ) else (
            echo Warning: Nexus login image assets were not found at %ASSET_SOURCE%\login-gui-codex-package\assets.
            exit /b 1
        )
        copy nexus-login-adapter.js "%OUTPUT_DIR%\nexus-ui\html\login-adapter.example.js" >nul
        if exist "%ASSET_SOURCE%\installer\nexus.ico" copy "%ASSET_SOURCE%\installer\nexus.ico" "%OUTPUT_DIR%\runtime\" >nul
        if exist "%ASSET_SOURCE%\installer\nexus-icon.png" copy "%ASSET_SOURCE%\installer\nexus-icon.png" "%OUTPUT_DIR%\nexus-ui\assets\" >nul
        xcopy recoilmaster-main\assets "%OUTPUT_DIR%\recoilmaster-main\assets" /E /I /Y >nul
        mkdir "%OUTPUT_DIR%\configs"
        copy recoilmaster-main\dist\configs\my_V5config.txt "%OUTPUT_DIR%\configs\" >nul
        echo Resources copied successfully!
    ) else (
        echo Warning: recoilmaster-main directory not found
    )
) else (
    echo Build failed!
)

echo.
echo Build complete. Run %OUTPUT_DIR%\Nexus Loader.exe to start the native app.
echo.
pause
