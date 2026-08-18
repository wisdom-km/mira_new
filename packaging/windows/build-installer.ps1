$ErrorActionPreference = "Stop"
$Root = Resolve-Path (Join-Path $PSScriptRoot "..\..")
$BuildDir = Join-Path $Root "build\windows-release"
$StageDir = Join-Path $Root "packaging\stage\DirectorDesk"
$DistDir = Join-Path $Root "dist"
$VcpkgRoot = if ($env:VCPKG_ROOT) { $env:VCPKG_ROOT } else { "C:\Users\19612\vcpkg" }
$Vcvars = "G:\BaseWare\VisualStudio\VC\Auxiliary\Build\vcvars64.bat"
$CMake = "G:\BaseWare\VisualStudio\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
$Ninja = "G:\BaseWare\VisualStudio\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe"

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

if (-not (Test-Path $Vcvars)) { throw "找不到 vcvars64.bat" }

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

$zip = Join-Path $DistDir "DirectorDesk-0.1.0-windows-x64.zip"
if (Test-Path $zip) { Remove-Item $zip -Force }
Compress-Archive -Path (Join-Path $StageDir "*") -DestinationPath $zip -Force

$iscc = Find-Iscc
if (-not $iscc) { throw "找不到 Inno Setup ISCC.exe，请先安装 JRSoftware.InnoSetup" }
$iss = Join-Path $PSScriptRoot "DirectorDesk.iss"
& $iscc $iss
if ($LASTEXITCODE -ne 0) { throw "Inno Setup 编译失败" }

Write-Host "Installer and zip written to $DistDir"
Get-ChildItem $DistDir | Format-Table Name, Length
