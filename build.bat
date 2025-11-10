@echo off
echo ================================
echo Building Turtle Odyssey
echo ================================

REM Create build directory
if not exist build mkdir build
cd build

REM Generate project files
echo.
echo Generating project files...
echo Note: Using vcpkg toolchain
cmake .. -DCMAKE_TOOLCHAIN_FILE=C:/Users/mayma/Documents/year3/game/vcpkg/scripts/buildsystems/vcpkg.cmake

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo ERROR: CMake generation failed!
    echo.
    echo If you see "Could not find package" errors, you need to:
    echo 1. Install vcpkg
    echo 2. Install libraries: vcpkg install glfw3:x64-windows glm:x64-windows assimp:x64-windows
    echo 3. Run: vcpkg integrate install
    echo.
    echo Or specify vcpkg toolchain manually:
    echo cmake .. -DCMAKE_TOOLCHAIN_FILE=C:/dev/vcpkg/scripts/buildsystems/vcpkg.cmake
    pause
    exit /b 1
)

REM Build the project
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
echo.
echo Executable location: build\Release\TurtleOdyssey.exe
echo.
echo To run the game:
echo   cd build\Release
echo   TurtleOdyssey.exe
echo.
pause
