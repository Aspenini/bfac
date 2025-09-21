@echo off
echo Building BFAC - Brainfuck Advanced Compiler...

REM Create distribution structure
mkdir dist 2>nul
mkdir dist\tools 2>nul

REM Build the compiler
echo Compiling bfac.exe...
tcc -o dist\bfac.exe bfac.c

if errorlevel 1 (
    echo Error: Failed to build compiler
    pause
    exit /b 1
)

REM Build the GUI
echo Compiling bfac-gui.exe...
tcc -mwindows -o dist\bfac-gui.exe bfac-gui.c -lcomdlg32

if errorlevel 1 (
    echo Error: Failed to build GUI
    pause
    exit /b 1
)

REM Copy tools to distribution
echo Copying Go tools to distribution...
copy tools\GoAsm.exe dist\tools\ >nul 2>&1
copy tools\GoLink.exe dist\tools\ >nul 2>&1  

REM Check if required tools exist
if not exist tools\GoAsm.exe (
    echo Error: GoAsm.exe not found in tools folder
    pause
    exit /b 1
)

if not exist tools\GoLink.exe (
    echo Error: GoLink.exe not found in tools folder  
    pause
    exit /b 1
)


echo.
echo SUCCESS! Built BFAC - Brainfuck Advanced Compiler:
echo   dist\bfac.exe      - Command-line compiler
echo   dist\bfac-gui.exe  - GUI compiler (user-friendly)
echo   dist\tools\        - Bundled Go tools (GoAsm, GoLink)
echo.
echo Command-line usage: dist\bfac.exe input.bf output.exe
echo GUI usage: Just run dist\bfac-gui.exe and use the interface!
echo.
echo BFAC creates native Windows PE executables with ZERO dependencies!
pause
