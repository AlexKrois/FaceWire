param([Parameter(Mandatory = $true)][string]$ObsRoot)
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$package = Join-Path $root 'dist/camoutlines'
$target = (Resolve-Path -LiteralPath $ObsRoot).Path
if (!(Test-Path -LiteralPath "$target/bin/64bit/obs64.exe")) { throw 'ObsRoot must be an OBS installation directory.' }
if (Get-Process obs64 -ErrorAction SilentlyContinue) { throw 'Close OBS before installing Cam Outlines.' }
$files = @(
    @{ Source = 'bin/64bit/camoutlines.dll'; Target = 'obs-plugins/64bit/camoutlines.dll' },
    @{ Source = 'data/libmediapipe.dll'; Target = 'data/obs-plugins/camoutlines/libmediapipe.dll' },
    @{ Source = 'data/face_landmarker.task'; Target = 'data/obs-plugins/camoutlines/face_landmarker.task' },
    @{ Source = 'data/locale/de-DE.ini'; Target = 'data/obs-plugins/camoutlines/locale/de-DE.ini' },
    @{ Source = 'data/locale/en-US.ini'; Target = 'data/obs-plugins/camoutlines/locale/en-US.ini' },
    @{ Source = 'data/licenses/MediaPipe-LICENSE.txt'; Target = 'data/obs-plugins/camoutlines/licenses/MediaPipe-LICENSE.txt' },
    @{ Source = 'LICENSE'; Target = 'data/obs-plugins/camoutlines/LICENSE' },
    @{ Source = 'THIRD_PARTY.md'; Target = 'data/obs-plugins/camoutlines/THIRD_PARTY.md' }
)
foreach ($file in $files) {
    if (!(Test-Path -LiteralPath (Join-Path $package $file.Source))) { throw "Package file missing: $($file.Source)" }
}
$backup = Join-Path $root ('.deps/install-backups/' + (Get-Date -Format 'yyyyMMdd-HHmmss'))
foreach ($file in $files) {
    $destination = Join-Path $target $file.Target
    if (Test-Path -LiteralPath $destination) {
        $saved = Join-Path $backup $file.Target
        New-Item -ItemType Directory -Force (Split-Path $saved) | Out-Null
        Copy-Item -LiteralPath $destination -Destination $saved
    }
}
foreach ($file in $files) {
    $source = Join-Path $package $file.Source
    $destination = Join-Path $target $file.Target
    New-Item -ItemType Directory -Force (Split-Path $destination) | Out-Null
    Copy-Item -LiteralPath $source -Destination $destination -Force
    if ((Get-FileHash -LiteralPath $source).Hash -ne (Get-FileHash -LiteralPath $destination).Hash) {
        throw "Installed file verification failed: $destination"
    }
}
Write-Host "Cam Outlines installed in $target. Backup of replaced files: $backup"
