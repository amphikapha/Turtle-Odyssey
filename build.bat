@echo off
echo ================================
echo Building Turtle Odyssey
echo ================================

REM Load .env file
if exist "..\.env" (
    for /f "usebackq tokens=1,2 delims==" %%a in ("..\ .env") do (
        set %%a=%%b
    )
)

REM Check if variable exists
if "%VCPKG_TOOLCHAIN%"=="" (
    echo ERROR: VCPKG_TOOLCHAIN not set in .env
    echo Create .env with:
    echo VCPKG_TOOLCHAIN=C:/Users/ananz/vcpkg/scripts/buildsystems/vcpkg.cmake
    pause
    exit /b 1
)

echo Using vcpkg toolchain:
echo %VCPKG_TOOLCHAIN%

REM Create build directory
if not exist build mkdir build
cd build

echo.
echo Generating project files...
echo Note: Using vcpkg toolchain

cmake .. -DCMAKE_TOOLCHAIN_FILE=%VCPKG_TOOLCHAIN%

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo ERROR: CMake generation failed!
    echo.
    echo If you see "Could not find package" errors, you need to:
    echo 1. Install vcpkg
    echo 2. Install required libs:
    echo    vcpkg install glfw3:x64-windows glm:x64-windows assimp:x64-windows openal-soft:x64-windows libsndfile:x64-windows freetype:x64-windows
    echo 3. Run: vcpkg integrate install
    pause
    exit /b 1
)

echo.
echo Building project...
cmake --build . --config Release

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo ERROR: Build failed!
    pause
    exit /b 1
)

echo.
echo ================================
echo Build successful!
echo ================================
echo Executable location: build\Release\TurtleOdyssey.exe
pause
