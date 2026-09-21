# PowerShell deployment script for RathonWare
# This script builds the application in Release mode, runs windeployqt to collect DLL dependencies,
# and compiles the installer using Inno Setup (ISCC.exe) if installed.

$ErrorActionPreference = "Stop"

Write-Host "=== Starting RathonWare Build & Package Process ===" -ForegroundColor Cyan

# 1. Clean and build in Release mode
Write-Host "`n[Step 1] Building in Release mode with CMake..." -ForegroundColor Yellow
if (-not (Test-Path "build")) {
    New-Item -ItemType Directory -Path "build" | Out-Null
}

cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release

# 2. Prepare deployment directory
Write-Host "`n[Step 2] Preparing 'dist' directory..." -ForegroundColor Yellow
if (Test-Path "dist") {
    Remove-Item -Recurse -Force "dist"
}
New-Item -ItemType Directory -Path "dist" | Out-Null

# Locate the compiled executable
# Depending on the CMake generator, the binary could be in build/, build/Release/, or build/src/
$ExePaths = @(
    "build/RathonWare.exe",
    "build/Release/RathonWare.exe",
    "build/src/RathonWare.exe",
    "build/src/Release/RathonWare.exe"
)

$ExePath = $null
foreach ($path in $ExePaths) {
    if (Test-Path $path) {
        $ExePath = $path
        break
    }
}

if ($null -eq $ExePath) {
    Write-Error "Could not find compiled RathonWare.exe. Please ensure the build succeeded."
}

Write-Host "Found compiled executable at: $ExePath" -ForegroundColor Green
Copy-Item $ExePath "dist/RathonWare.exe"

# 3. Run windeployqt
Write-Host "`n[Step 3] Running windeployqt to package dependencies..." -ForegroundColor Yellow
# Try to find windeployqt in PATH
$Windeployqt = Get-Command windeployqt -ErrorAction SilentlyContinue | Select-Object -ExpandProperty Source

if ($null -eq $Windeployqt) {
    # Try looking in common Qt installations
    $QtPaths = Get-ChildItem "C:\Qt\6.*\*\bin\windeployqt.exe" -ErrorAction SilentlyContinue
    if ($QtPaths) {
        $Windeployqt = $QtPaths[0].FullName
    }
}

if ($null -eq $Windeployqt) {
    Write-Host "WARNING: windeployqt.exe was not found in PATH or standard C:\Qt directories." -ForegroundColor Red
    Write-Host "Please make sure Qt 6 bin directory is in your PATH and run this script again." -ForegroundColor Red
    Write-Host "Otherwise, dependencies will not be copied." -ForegroundColor Red
} else {
    Write-Host "Using windeployqt at: $Windeployqt" -ForegroundColor Green
    # Run windeployqt on the executable in dist folder
    & $Windeployqt --qmldir qml/ dist/RathonWare.exe
    # Also deploy to build directory for local development testing
    & $Windeployqt --qmldir qml/ $ExePath
}

# 4. Compile Installer using Inno Setup
Write-Host "`n[Step 4] Compiling Installer with Inno Setup..." -ForegroundColor Yellow
$Iscc = Get-Command iscc -ErrorAction SilentlyContinue | Select-Object -ExpandProperty Source

if ($null -eq $Iscc) {
    # Try looking in common Inno Setup installation paths
    $InnoPaths = @(
        "$env:LOCALAPPDATA\Programs\Inno Setup 6\ISCC.exe",
        "C:\Program Files (x86)\Inno Setup 6\ISCC.exe",
        "C:\Program Files\Inno Setup 6\ISCC.exe",
        "C:\Program Files (x86)\Inno Setup 5\ISCC.exe"
    )
    foreach ($path in $InnoPaths) {
        if (Test-Path $path) {
            $Iscc = $path
            break
        }
    }
}

if ($null -eq $Iscc) {
    Write-Host "WARNING: Inno Setup Compiler (ISCC.exe) was not found." -ForegroundColor Red
    Write-Host "Please install Inno Setup (https://jrsoftware.org/isdl.php) to compile the installer." -ForegroundColor Red
    Write-Host "The app dependencies are packaged in 'dist/' and can be run directly from there." -ForegroundColor Green
} else {
    Write-Host "Using Inno Setup Compiler at: $Iscc" -ForegroundColor Green
    & $Iscc installer.iss
    Write-Host "`nSUCCESS! The installer has been created at build/RathonWareSetup.exe" -ForegroundColor Green
}
