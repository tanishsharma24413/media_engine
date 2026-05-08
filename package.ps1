# package.ps1 — Tanish Player Release Packager
# This script bundles the built executable with its dependencies into a 'dist' folder.

$ProjectRoot = Get-Location
$BuildDir = Join-Path $ProjectRoot "build"
$DistDir = Join-Path $ProjectRoot "dist"
$ExePath = Join-Path $BuildDir "app\Release\TanishPlayer.exe"

if (-not (Test-Path $ExePath)) {
    Write-Error "Error: TanishPlayer.exe not found at $ExePath. Please build the project in Release mode first."
    Write-Host "Try: cmake --build build --config Release --target TanishPlayer"
    exit 1
}

# 1. Create fresh dist folder
if (Test-Path $DistDir) { Remove-Item -Recurse -Force $DistDir }
New-Item -ItemType Directory -Path $DistDir | Out-Null

# 2. Copy Executable
Copy-Item $ExePath $DistDir

# 3. Copy License & Assets
Copy-Item (Join-Path $ProjectRoot "LICENSE") $DistDir
Copy-Item (Join-Path $ProjectRoot "logo.png") $DistDir

# 4. Copy FFmpeg DLLs (if they exist)
# Note: You need to update the $FFmpegBin path to your local FFmpeg bin folder.
$FFmpegBin = "C:\ffmpeg\bin" 
if (Test-Path $FFmpegBin) {
    Write-Host "Copying FFmpeg DLLs from $FFmpegBin..."
    Get-ChildItem -Path $FFmpegBin -Filter "*.dll" | Copy-Item -Destination $DistDir
} else {
    Write-Warning "FFmpeg DLLs not found at $FFmpegBin. The player may not run without them."
}

# 5. Copy SDL2 DLLs (if you used vcpkg, they are usually in build/app/Release)
$SdlDll = Join-Path $BuildDir "app\Release\SDL2.dll"
if (Test-Path $SdlDll) {
    Copy-Item $SdlDll $DistDir
}

Write-Host "`nSuccessfully prepared release in: $DistDir"
Write-Host "You can now zip this folder and share it with your users!"
