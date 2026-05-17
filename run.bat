@echo off
chcp 65001 >nul
title TilemapViewer - Build and Run

echo ============================================
echo   TilemapViewer - Build ^& Run
echo ============================================
echo.

set PROJECT_DIR=%~dp0
cd /d "%PROJECT_DIR%"

:: Tạo build folder nếu chưa có
if not exist "build" mkdir build

:: Xóa cache nếu CMakeLists.txt mới hơn CMakeCache.txt
:: (dùng xcopy /d để so sánh timestamp)
if exist "build\CMakeCache.txt" (
    xcopy /d /y "CMakeLists.txt" "build\_cml_check_\" >nul 2>&1
    if errorlevel 1 (
        echo [2/4] CMakeLists.txt da thay doi - xoa cache cu...
        del /f /q "build\CMakeCache.txt" >nul 2>&1
        rmdir /s /q "build\CMakeFiles" >nul 2>&1
    ) else (
        echo [2/4] CMake da configure, bo qua...
        rmdir /s /q "build\_cml_check_" >nul 2>&1
        goto BUILD
    )
    rmdir /s /q "build\_cml_check_" >nul 2>&1
)

:: CMake configure
echo [2/4] CMake configure...
cd build
cmake .. -G "MinGW Makefiles"
if errorlevel 1 (
    echo [LOI] CMake configure that bai!
    cd ..
    pause
    exit /b 1
)
cd ..

:BUILD
echo [3/4] Building...
cd build
mingw32-make -j4
if errorlevel 1 (
    echo.
    echo [LOI] Build that bai!
    cd ..
    pause
    exit /b 1
)
cd ..

:: Copy tất cả PNG vào build
echo [4/4] Copy assets...
for %%f in ("%PROJECT_DIR%*.png") do (
    copy /y "%%f" "%PROJECT_DIR%build\" >nul
)

:: Copy toàn bộ thư mục hinh anh vào build
if exist "%PROJECT_DIR%hinh anh" (
    echo Dang copy thu muc hinh anh...
    xcopy /e /i /y "%PROJECT_DIR%hinh anh" "%PROJECT_DIR%build\hinh anh" >nul
) else (
    echo [CANH BAO] Khong tim thay thu muc hinh anh goc!
)

:: Copy toàn bộ thư mục audio vào build (bao gồm cả file .ogg và .wav)
if exist "%PROJECT_DIR%audio" (
    echo Đang copy thu muc audio...
    xcopy /e /i /y "%PROJECT_DIR%audio" "%PROJECT_DIR%build\audio" >nul
) else (
    echo [CANH BAO] Khong tim thay thu muc audio goc!
)
echo.
echo [OK] Build thanh cong!
echo.
cd build
tilemap.exe
cd ..

echo.
pause