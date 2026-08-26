@echo off
echo ========================================
echo Installing SFML for Windows (MSYS2)...
echo ========================================

REM Check if MSYS2 is installed
if not exist "C:\msys64" (
    echo [ERROR] MSYS2 not found at C:\msys64
    echo Please install MSYS2 from: https://www.msys2.org/
    pause
    exit /b 1
)

REM Install SFML via MSYS2
echo Installing SFML 3.0.0...
C:\msys64\usr\bin\bash.exe -lc "pacman -S --noconfirm mingw-w64-ucrt-x86_64-sfml"

if %errorlevel% neq 0 (
    echo [ERROR] Failed to install SFML
    pause
    exit /b 1
)

echo ========================================
echo SFML installed successfully!
echo ========================================
pause