# Recapture README stills + walkthrough GIF from examples/cafe.ddproj.
# Requires a local Debug build: build/windows-debug/DirectorDesk.exe
$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
$exe = Join-Path $root "build\windows-debug\DirectorDesk.exe"
$project = Join-Path $root "examples\cafe.ddproj"
$img = Join-Path $root "img"
$work = Join-Path $env:TEMP "dd-readme-capture"
New-Item -ItemType Directory -Force -Path $img, $work | Out-Null

Add-Type -AssemblyName System.Drawing
Add-Type @"
using System;
using System.Runtime.InteropServices;
public static class DdCap {
  public static readonly IntPtr TopMost = new IntPtr(-1);
  public static readonly IntPtr NoTopMost = new IntPtr(-2);
  [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out RECT r);
  [DllImport("user32.dll")] public static extern bool ClientToScreen(IntPtr h, ref POINT p);
  [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
  [DllImport("user32.dll")] public static extern bool ShowWindow(IntPtr h, int cmd);
  [DllImport("user32.dll")] public static extern bool SetCursorPos(int x, int y);
  [DllImport("user32.dll")] public static extern void mouse_event(int flags, int x, int y, int data, int extra);
  [DllImport("user32.dll")] public static extern bool SetWindowPos(IntPtr h, IntPtr after, int x, int y, int cx, int cy, uint flags);
  [DllImport("user32.dll")] public static extern bool PrintWindow(IntPtr h, IntPtr hdc, uint flags);
  [StructLayout(LayoutKind.Sequential)]
  public struct RECT { public int L, T, R, B; }
  [StructLayout(LayoutKind.Sequential)]
  public struct POINT { public int X, Y; }
}
"@

function Wait-ProcessWindow([System.Diagnostics.Process]$proc, [int]$seconds) {
    $deadline = [datetime]::UtcNow.AddSeconds($seconds)
    while ([datetime]::UtcNow -lt $deadline) {
        $proc.Refresh()
        if ($proc.HasExited) { throw "DirectorDesk exited early code=$($proc.ExitCode)" }
        if ($proc.MainWindowHandle -ne [IntPtr]::Zero -and $proc.MainWindowTitle -eq "DirectorDesk") {
            return $proc.MainWindowHandle
        }
        Start-Sleep -Milliseconds 200
    }
    throw "DirectorDesk created no window"
}

function Capture-Handle([IntPtr]$hwnd, [string]$path) {
    [DdCap]::ShowWindow($hwnd, 9) | Out-Null
    [DdCap]::SetWindowPos($hwnd, [DdCap]::TopMost, 0, 0, 0, 0, 0x0003) | Out-Null
    [DdCap]::SetForegroundWindow($hwnd) | Out-Null
    Start-Sleep -Milliseconds 500
    $rect = New-Object DdCap+RECT
    [DdCap]::GetWindowRect($hwnd, [ref]$rect) | Out-Null
    $w = $rect.R - $rect.L
    $h = $rect.B - $rect.T
    $bmp = New-Object System.Drawing.Bitmap $w, $h
    $g = [System.Drawing.Graphics]::FromImage($bmp)
    $hdc = $g.GetHdc()
    $ok = [DdCap]::PrintWindow($hwnd, $hdc, 2)
    $g.ReleaseHdc($hdc)
    if (-not $ok) {
        $g.CopyFromScreen($rect.L, $rect.T, 0, 0, $bmp.Size)
    }
    $bmp.Save($path, [System.Drawing.Imaging.ImageFormat]::Png)
    $g.Dispose()
    $bmp.Dispose()
    [DdCap]::SetWindowPos($hwnd, [DdCap]::NoTopMost, 0, 0, 0, 0, 0x0003) | Out-Null
    Write-Host "captured $path ${w}x${h} print=$ok"
}

function Click-Client([IntPtr]$hwnd, [int]$x, [int]$y) {
    $pt = New-Object DdCap+POINT
    $pt.X = $x
    $pt.Y = $y
    [DdCap]::ClientToScreen($hwnd, [ref]$pt) | Out-Null
    [DdCap]::SetForegroundWindow($hwnd) | Out-Null
    Start-Sleep -Milliseconds 120
    [DdCap]::SetCursorPos($pt.X, $pt.Y) | Out-Null
    Start-Sleep -Milliseconds 80
    [DdCap]::mouse_event(0x0002, 0, 0, 0, 0)
    Start-Sleep -Milliseconds 40
    [DdCap]::mouse_event(0x0004, 0, 0, 0, 0)
}

function Start-Desk([string]$mode) {
    Get-Process -Name DirectorDesk -ErrorAction SilentlyContinue | ForEach-Object { $_.CloseMainWindow() | Out-Null }
    Start-Sleep -Seconds 1
    Write-Host "launch mode=$mode"
    $p = Start-Process -FilePath $exe -WorkingDirectory $work -PassThru -ArgumentList @(
        "--project", $project, "--workspace-mode", $mode
    )
    $null = Wait-ProcessWindow $p 90
    Start-Sleep -Seconds 7
    $p.Refresh()
    return $p
}

$modes = @(
    @{ Id = "script"; File = "readme-script.png" }
    @{ Id = "set"; File = "readme-set.png" }
    @{ Id = "shoot"; File = "readme-shoot.png" }
    @{ Id = "review"; File = "readme-review.png" }
)
foreach ($mode in $modes) {
    $proc = Start-Desk $mode.Id
    Capture-Handle $proc.MainWindowHandle (Join-Path $img $mode.File)
    $proc.CloseMainWindow() | Out-Null
    Start-Sleep -Seconds 1
}

$shoot = Start-Desk "shoot"
[DdCap]::SetWindowPos($shoot.MainWindowHandle, [DdCap]::TopMost, 0, 0, 0, 0, 0x0003) | Out-Null
Start-Sleep -Milliseconds 300
foreach ($y in @(8, 14, 20)) {
    Click-Client $shoot.MainWindowHandle 36 $y
    Start-Sleep -Milliseconds 250
}
Capture-Handle $shoot.MainWindowHandle (Join-Path $img "readme-export-menu.png")
$shoot.CloseMainWindow() | Out-Null
Start-Sleep -Seconds 1

$ffmpeg = (Get-Command ffmpeg -ErrorAction Stop).Source
$gifList = Join-Path $work "gif.txt"
$frames = @(
    "readme-script.png",
    "readme-set.png",
    "readme-shoot.png",
    "readme-review.png",
    "readme-export-menu.png"
)
$lines = New-Object System.Collections.Generic.List[string]
foreach ($name in $frames) {
    $path = (Join-Path $img $name).Replace("\", "/")
    $lines.Add("file '$path'")
    $lines.Add("duration 1.6")
}
$last = (Join-Path $img "readme-export-menu.png").Replace("\", "/")
$lines.Add("file '$last'")
$lines | Set-Content -Encoding ASCII $gifList
$palette = Join-Path $work "palette.png"
$gif = Join-Path $img "readme-walkthrough.gif"
& $ffmpeg -y -f concat -safe 0 -i $gifList -vf "scale=960:-1:flags=lanczos,palettegen" $palette
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
& $ffmpeg -y -f concat -safe 0 -i $gifList -i $palette -lavfi "scale=960:-1:flags=lanczos[x];[x][1:v]paletteuse=dither=bayer" $gif
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
Write-Host "gif $gif"
Get-ChildItem $img -Filter "readme-*" | Format-Table Name, Length
