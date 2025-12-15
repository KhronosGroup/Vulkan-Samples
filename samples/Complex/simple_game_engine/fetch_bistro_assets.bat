@echo off
setlocal enabledelayedexpansion

REM Fetch the Bistro example assets into the desired assets directory.
REM Default target when run from attachments\simple_engine: Assets\bistro
REM Usage:
REM   fetch_bistro_assets.bat [target-dir]
REM Example:
REM   fetch_bistro_assets.bat

set REPO_SSH=git@github.com:gpx1000/bistro.git
set REPO_HTTPS=https://github.com/gpx1000/bistro.git

if "%~1"=="" (
    set "TARGET_DIR=Assets\bistro"
) else (
    set "TARGET_DIR=%~1"
)

REM Ensure parent directory exists (avoid trailing backslash quoting issue by appending a dot)
for %%I in ("%TARGET_DIR%") do set "PARENT=%%~dpI"
if not exist "%PARENT%." mkdir "%PARENT%."

REM If directory exists and is a git repo, update it; otherwise clone it
if exist "%TARGET_DIR%\.git" (
    echo Updating existing bistro assets in %TARGET_DIR%
    pushd "%TARGET_DIR%" >nul 2>nul
    git pull --ff-only
    if errorlevel 1 (
        echo ERROR: Failed to update repository at %TARGET_DIR%.
        popd >nul 2>nul
        endlocal & exit /b 1
    )
    popd >nul 2>nul
) else (
    echo Cloning bistro assets into %TARGET_DIR%
    REM Try SSH first; fall back to HTTPS on failure
    git clone --depth 1 "%REPO_SSH%" "%TARGET_DIR%" 1>nul 2>nul
    if errorlevel 1 (
        echo SSH clone failed, trying HTTPS
        git clone --depth 1 "%REPO_HTTPS%" "%TARGET_DIR%"
        if errorlevel 1 (
            echo ERROR: Failed to clone repository via HTTPS into %TARGET_DIR%.
            endlocal & exit /b 1
        )
    )
)

REM If git-lfs is available, ensure LFS content is pulled (ignore failures)
if exist "%TARGET_DIR%\.git" (
    pushd "%TARGET_DIR%" >nul 2>nul
    git lfs version >nul 2>nul
    if not errorlevel 1 (
        git lfs install --local >nul 2>nul
        git lfs pull || rem ignore
    )
    popd >nul 2>nul
)

echo Bistro assets ready at: %TARGET_DIR%
endlocal & exit /b 0
