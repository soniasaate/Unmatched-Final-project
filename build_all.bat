@echo off
echo ========================================
echo Building Unmatched Project (with SFML 3)
echo ========================================

REM Create build directory
if not exist build mkdir build
cd build

REM Configure with CMake
echo Configuring CMake...
cmake .. -G "MinGW Makefiles"
if %errorlevel% neq 0 (
    echo [ERROR] CMake configuration failed
    pause
    exit /b 1
)

REM Build the project
echo Building project...
mingw32-make
if %errorlevel% neq 0 (
    echo [ERROR] Build failed
    pause
    exit /b 1
)

echo ========================================
echo Build completed successfully!
echo ========================================
echo To run the graphical version: build\unmatched_graphical.exe
echo To run the TUI version: build\unmatched_tui.exe
pause