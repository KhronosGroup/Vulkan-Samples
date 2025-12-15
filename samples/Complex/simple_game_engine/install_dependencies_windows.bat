@echo off
REM Install script for Simple Game Engine dependencies on Windows
REM This script installs all required dependencies for building the Simple Game Engine

echo Installing Simple Game Engine dependencies for Windows...

:: Check if vcpkg is installed
where vcpkg >nul 2>nul
if %ERRORLEVEL% neq 0 (
    echo vcpkg not found. Please install vcpkg first.
    echo Visit https://github.com/microsoft/vcpkg for installation instructions.
    echo Typically, you would:
    echo 1. git clone https://github.com/Microsoft/vcpkg.git
    echo 2. cd vcpkg
    echo 3. .\bootstrap-vcpkg.bat
    echo 4. Add vcpkg to your PATH
    exit /b 1
)

:: Enable binary caching for vcpkg
echo Enabling binary caching for vcpkg...
set VCPKG_BINARY_SOURCES=clear;files,%TEMP%\vcpkg-cache,readwrite

:: Create cache directory if it doesn't exist
if not exist %TEMP%\vcpkg-cache mkdir %TEMP%\vcpkg-cache

:: Install all dependencies at once using vcpkg with parallel installation
echo Installing all dependencies...
vcpkg install --triplet=x64-windows --x-manifest-root=%~dp0 --feature-flags=binarycaching,manifests --x-install-root=%VCPKG_INSTALLATION_ROOT%/installed

:: Remind about Vulkan SDK
echo.
echo Don't forget to install the Vulkan SDK from https://vulkan.lunarg.com/
echo.

echo All dependencies have been installed successfully!
echo You can now use CMake to build your Vulkan project.
echo.
echo Example CMake command:
echo cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=[path\to\vcpkg]\scripts\buildsystems\vcpkg.cmake
echo cmake --build build

exit /b 0
