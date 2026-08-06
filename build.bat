@echo off
echo Building NEXUS native C++ app...
echo.

set "MINGW_ROOT=C:\msys64\mingw64"
set "QT_BIN=%MINGW_ROOT%\bin"
set "ASSET_SOURCE=%CD%\loader-assets"
set "OUTPUT_DIR=%USERPROFILE%\Desktop\NEXUS"
if exist "%QT_BIN%\cmake.exe" (
    echo Building native NEXUS client...
    set "PATH=%QT_BIN%;%PATH%"
    "%QT_BIN%\cmake.exe" -S qt_nexus_client -B qt_nexus_client\build-mingw -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=%MINGW_ROOT% -DCMAKE_SUPPRESS_REGENERATION=ON
    if errorlevel 1 exit /b 1
    "%QT_BIN%\cmake.exe" --build qt_nexus_client\build-mingw --config Release
    if errorlevel 1 exit /b 1
) else (
    echo Warning: Qt build tools were not found at %QT_BIN%.
)

REM Create final NEXUS output directory
powershell -NoProfile -ExecutionPolicy Bypass -Command "$p=(Resolve-Path '%OUTPUT_DIR%' -ErrorAction SilentlyContinue).Path; if($p){Get-Process | Where-Object {$_.Path -and $_.Path.StartsWith($p, [System.StringComparison]::OrdinalIgnoreCase)} | Stop-Process -Force}" >nul 2>nul
if exist "%OUTPUT_DIR%" rmdir /S /Q "%OUTPUT_DIR%"
mkdir "%OUTPUT_DIR%"

REM Compile the C++ application
windres app.rc -O coff -o "%OUTPUT_DIR%\app_icon.o"
if errorlevel 1 exit /b 1
g++ -std=c++17 -O3 -s -fdata-sections -ffunction-sections -Wl,--gc-sections main.cpp "%OUTPUT_DIR%\app_icon.o" -Ithird_party\Microsoft.Web.WebView2\build\native\include -o "%OUTPUT_DIR%\Nexus Loader.exe" -mwindows -luser32 -lgdi32 -lcomctl32 -lcomdlg32 -lshell32 -lws2_32 -ladvapi32 -lwinmm -lwinhttp -lole32 -luuid
del /Q "%OUTPUT_DIR%\app_icon.o" 2>nul

if %ERRORLEVEL% EQU 0 (
    echo Build successful!
    echo Output: %OUTPUT_DIR%\Nexus Loader.exe

    if exist "%MINGW_ROOT%\include\opencv4\opencv2\opencv.hpp" (
        echo Building NEXUS runtime helper...
        if not exist "%OUTPUT_DIR%\runtime" mkdir "%OUTPUT_DIR%\runtime"
        g++ -std=c++20 -O2 nexus_runtime_helper.cpp -o "%OUTPUT_DIR%\runtime\NEXUS Runtime Helper.exe" -mwindows -I"%MINGW_ROOT%\include\opencv4" -L"%MINGW_ROOT%\lib" -ltesseract -lleptonica -lopencv_imgproc -lopencv_core -lgdi32 -luser32 -lshell32 -lwinhttp
        if errorlevel 1 exit /b 1
        g++ -std=c++20 -O2 -shared nexus_runtime_helper.cpp -o "%OUTPUT_DIR%\runtime\NEXUSRuntimeHelper.dll" -I"%MINGW_ROOT%\include\opencv4" -L"%MINGW_ROOT%\lib" -ltesseract -lleptonica -lopencv_imgproc -lopencv_core -lgdi32 -luser32 -lshell32 -lwinhttp
        if errorlevel 1 exit /b 1
        powershell -NoProfile -ExecutionPolicy Bypass -Command "$bin='%MINGW_ROOT%\bin'; $out='%OUTPUT_DIR%\runtime'; $names=@('libtesseract*.dll','libleptonica*.dll','libopencv_core*.dll','libopencv_imgproc*.dll','libgcc_s_seh-1.dll','libstdc++-6.dll','libwinpthread-1.dll','zlib1.dll','libpng*.dll','libjpeg*.dll','libtiff*.dll','libwebp*.dll','libopenjp2*.dll','libarchive*.dll','libcurl*.dll','libcrypto*.dll','libssl*.dll','liblzma*.dll','libzstd*.dll','libbrotli*.dll','libiconv*.dll','libintl*.dll','libsharpyuv*.dll','libgif*.dll'); foreach($pattern in $names){ Get-ChildItem -LiteralPath $bin -Filter $pattern -File -ErrorAction SilentlyContinue | Copy-Item -Destination $out -Force }"
        if errorlevel 1 exit /b 1
    ) else (
        echo Warning: OpenCV/Tesseract headers were not found. NEXUS runtime helper was not rebuilt.
    )
    
    REM Copy only the native app resources. Do not ship the old Python source or PyInstaller executable.
    echo.
    echo Copying UI resources...
    if exist nexus-runtime-core (
        mkdir "%OUTPUT_DIR%\nexus-runtime-core"
        if not exist "%OUTPUT_DIR%\runtime" mkdir "%OUTPUT_DIR%\runtime"
        copy nexus-runtime-core\NexusRuntimeCore.html "%OUTPUT_DIR%\nexus-runtime-core\" >nul
        copy nexus-runtime-core\associated_icon.png "%OUTPUT_DIR%\nexus-runtime-core\" >nul
        if exist "%ASSET_SOURCE%\firebase_config.json" copy "%ASSET_SOURCE%\firebase_config.json" "%OUTPUT_DIR%\runtime\" >nul
        copy third_party\Microsoft.Web.WebView2\runtimes\win-x64\native\WebView2Loader.dll "%OUTPUT_DIR%\runtime\" >nul
        if not exist "%OUTPUT_DIR%\runtime\NEXUSRuntimeHelper.dll" echo Warning: NEXUSRuntimeHelper.dll was not built.
        del /Q "%OUTPUT_DIR%\runtime\NEXUS Runtime Helper.exe" 2>nul
        if exist runtime\native-detector (
            xcopy runtime\native-detector "%OUTPUT_DIR%\runtime\native-detector" /E /I /Y >nul
            del /Q "%OUTPUT_DIR%\runtime\native-detector\R6NativeDetector.exe" 2>nul
            del /Q "%OUTPUT_DIR%\runtime\native-detector\current_native_detector_pid.txt" 2>nul
            del /Q "%OUTPUT_DIR%\runtime\native-detector\detector_status.json" 2>nul
        )
        if exist qt_nexus_client\build-mingw\NexusClient.exe (
            mkdir "%OUTPUT_DIR%\nexus-client"
            copy qt_nexus_client\build-mingw\NexusClient.exe "%OUTPUT_DIR%\nexus-client\Nexus Client.exe" >nul
            if exist "%QT_BIN%\windeployqt.exe" "%QT_BIN%\windeployqt.exe" --release --compiler-runtime "%OUTPUT_DIR%\nexus-client\Nexus Client.exe" >nul
            mkdir "%OUTPUT_DIR%\nexus-client\config"
            if exist qt_nexus_client\config\firebase_config.json copy qt_nexus_client\config\firebase_config.json "%OUTPUT_DIR%\nexus-client\config\" >nul
            if exist qt_nexus_client\config\firebase_config.example.json copy qt_nexus_client\config\firebase_config.example.json "%OUTPUT_DIR%\nexus-client\config\" >nul
            if exist qt_nexus_client\config\license_config.json copy qt_nexus_client\config\license_config.json "%OUTPUT_DIR%\nexus-client\config\" >nul
            if exist qt_nexus_client\config\license_config.example.json copy qt_nexus_client\config\license_config.example.json "%OUTPUT_DIR%\nexus-client\config\" >nul
            if exist firebase_config.json copy firebase_config.json "%OUTPUT_DIR%\nexus-client\config\firebase_config.json" >nul
            if exist "%ASSET_SOURCE%\firebase_config.json" copy "%ASSET_SOURCE%\firebase_config.json" "%OUTPUT_DIR%\nexus-client\config\firebase_config.json" >nul
            if exist "Nexus Config.json" copy "Nexus Config.json" "%OUTPUT_DIR%\nexus-client\Nexus Config.json" >nul
        ) else (
            echo Warning: native NEXUS client was not found. Build qt_nexus_client first.
        )
        if exist "nexus_runtime_helper_words.txt" copy "nexus_runtime_helper_words.txt" "%OUTPUT_DIR%\runtime\" >nul
        if exist nexus_runtime_helper_config.txt copy nexus_runtime_helper_config.txt "%OUTPUT_DIR%\runtime\" >nul
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
        xcopy nexus-runtime-core\assets "%OUTPUT_DIR%\nexus-runtime-core\assets" /E /I /Y >nul
        mkdir "%OUTPUT_DIR%\configs"
        copy nexus-runtime-core\dist\configs\nexus_runtime_config.txt "%OUTPUT_DIR%\configs\" >nul
        echo Resources copied successfully!
    ) else (
        echo Warning: nexus-runtime-core directory not found
    )
) else (
    echo Build failed!
)

echo.
echo Build complete. Run %OUTPUT_DIR%\Nexus Loader.exe to start the native app.
echo.
pause
