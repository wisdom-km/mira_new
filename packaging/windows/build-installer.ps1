# build-installer: PowerShell build script for the DirectorDesk Windows packaging module.
# This file owns project behavior only; keep platform and dependency boundaries explicit.

$ErrorActionPreference = "Stop"
# Build flow: read the version from CMakeLists.txt, configure Release with the pinned
# vcpkg toolchain, stage runtime assets, then emit the portable archive and Inno Setup installer.
$Root = Resolve-Path (Join-Path $PSScriptRoot "..\..")
$BuildDir = Join-Path $Root "build\windows-release"
$StageDir = Join-Path $Root "packaging\stage\DirectorDesk"
$DistDir = Join-Path $Root "dist"
$VcpkgRoot = if ($env:VCPKG_ROOT) { $env:VCPKG_ROOT } else { "C:\Users\19612\vcpkg" }

function Read-ProjectVersion {
    $text = Get-Content -Path (Join-Path $Root "CMakeLists.txt") -Raw
    $match = [regex]::Match($text, 'project\s*\(\s*DirectorDesk\s+VERSION\s+([0-9]+\.[0-9]+\.[0-9]+)')
    if (-not $match.Success) {
        throw "无法从 CMakeLists.txt 读取 project(... VERSION)"
    }
    return $match.Groups[1].Value
}

function Find-Vcvars {
    $fallback = "G:\BaseWare\VisualStudio\VC\Auxiliary\Build\vcvars64.bat"
    if (Test-Path $fallback) { return $fallback }
    $vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    if (Test-Path $vswhere) {
        $install = & $vswhere -latest -products * `
            -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 `
            -property installationPath
        if ($install) {
            $candidate = Join-Path $install "VC\Auxiliary\Build\vcvars64.bat"
            if (Test-Path $candidate) { return $candidate }
        }
    }
    return $null
}

function Find-Tool([string]$name, [string]$fallback) {
    $cmd = Get-Command $name -ErrorAction SilentlyContinue
    if ($cmd) { return $cmd.Source }
    if ($fallback -and (Test-Path $fallback)) { return $fallback }
    return $null
}

function Find-Iscc {
    $paths = @(
        "${env:ProgramFiles(x86)}\Inno Setup 6\ISCC.exe",
        "${env:ProgramFiles}\Inno Setup 6\ISCC.exe",
        "$env:LOCALAPPDATA\Programs\Inno Setup 6\ISCC.exe"
    )
    foreach ($path in $paths) {
        if (Test-Path $path) { return $path }
    }
    $cmd = Get-Command iscc -ErrorAction SilentlyContinue
    if ($cmd) { return $cmd.Source }
    return $null
}

$AppVersion = Read-ProjectVersion
$Vcvars = Find-Vcvars
$CMake = Find-Tool "cmake.exe" "G:\BaseWare\VisualStudio\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
$Ninja = Find-Tool "ninja.exe" "G:\BaseWare\VisualStudio\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe"
if (-not $Vcvars) { throw "找不到 vcvars64.bat" }
if (-not $CMake) { throw "找不到 cmake" }
if (-not $Ninja) { throw "找不到 ninja" }

$configure = @"
call "$Vcvars" || exit /b 1
set "VCPKG_ROOT=$VcpkgRoot"
"$CMake" -S "$Root" -B "$BuildDir" -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE="$VcpkgRoot\scripts\buildsystems\vcpkg.cmake" -DCMAKE_MAKE_PROGRAM="$Ninja"
if errorlevel 1 exit /b 1
"$CMake" --build "$BuildDir"
if errorlevel 1 exit /b 1
"@
$bat = Join-Path $env:TEMP "dd-build-release.bat"
Set-Content -Path $bat -Value $configure -Encoding ASCII
& cmd /c $bat
if ($LASTEXITCODE -ne 0) { throw "Release 构建失败" }

$exe = Join-Path $BuildDir "DirectorDesk.exe"
if (-not (Test-Path $exe)) { throw "找不到 $exe" }

if (Test-Path $StageDir) { Remove-Item $StageDir -Recurse -Force }
New-Item -ItemType Directory -Path $StageDir | Out-Null
New-Item -ItemType Directory -Path $DistDir -Force | Out-Null

Copy-Item $exe $StageDir
Get-ChildItem $BuildDir -Filter *.dll | ForEach-Object { Copy-Item $_.FullName $StageDir }
Copy-Item (Join-Path $BuildDir "shaders") (Join-Path $StageDir "shaders") -Recurse
Copy-Item (Join-Path $BuildDir "examples") (Join-Path $StageDir "examples") -Recurse
Copy-Item (Join-Path $Root "LICENSE") $StageDir
Copy-Item (Join-Path $Root "README.md") $StageDir
Copy-Item (Join-Path $Root "docs\USER-GUIDE.md") $StageDir
Copy-Item (Join-Path $Root "docs\THIRD_PARTY.md") $StageDir
New-Item -ItemType Directory -Path (Join-Path $StageDir "img") | Out-Null
Copy-Item (Join-Path $Root "img\dog.png") (Join-Path $StageDir "img\dog.png")

$zip = Join-Path $DistDir "DirectorDesk-$AppVersion-windows-x64.zip"
if (Test-Path $zip) { Remove-Item $zip -Force }
Compress-Archive -Path (Join-Path $StageDir "*") -DestinationPath $zip -Force

$iscc = Find-Iscc
if (-not $iscc) { throw "找不到 Inno Setup ISCC.exe，请先安装 JRSoftware.InnoSetup" }
$iss = Join-Path $PSScriptRoot "DirectorDesk.iss"
& $iscc "/DAppVersion=$AppVersion" $iss
if ($LASTEXITCODE -ne 0) { throw "Inno Setup 编译失败" }

Write-Host "Installer and zip written to $DistDir (version $AppVersion)"
Get-ChildItem $DistDir | Format-Table Name, Length
