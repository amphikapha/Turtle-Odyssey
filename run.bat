@echo off
echo ================================
echo Running Turtle Odyssey
echo ================================
echo.

REM Check if exe exists
if exist build\Release\TurtleOdyssey.exe (
    cd build\Release
    TurtleOdyssey.exe
    cd ..\..
) else if exist build\Debug\TurtleOdyssey.exe (
    cd build\Debug
    TurtleOdyssey.exe
    cd ..\..
) else (
    echo ERROR: Game executable not found!
    echo Please build the project first using build.bat
    echo.
    pause
)
