param([switch]$IncludeObsRuntime)
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$deps = Join-Path $root '.deps'
New-Item -ItemType Directory -Force $deps | Out-Null
$lock = Get-Content "$root/dependencies.json" -Raw | ConvertFrom-Json
foreach ($artifact in $lock.artifacts) {
    $path = Join-Path $deps $artifact.file
    if (!(Test-Path -LiteralPath $path)) {
        $url = $artifact.url
        if (!$url) {
            $release = Invoke-RestMethod $artifact.metadataUrl
            $wheel = $release.urls | Where-Object filename -EQ $artifact.wheel | Select-Object -First 1
            if (!$wheel -or $wheel.digests.sha256 -ne $artifact.sha256) { throw 'Unexpected MediaPipe wheel metadata.' }
            $url = $wheel.url
        }
        Invoke-WebRequest $url -OutFile "$path.partial"
        if ((Get-FileHash "$path.partial" -Algorithm SHA256).Hash.ToLower() -ne $artifact.sha256) {
            throw "Download checksum mismatch: $($artifact.file)"
        }
        Move-Item -LiteralPath "$path.partial" -Destination $path
    }
    if ((Get-FileHash $path -Algorithm SHA256).Hash.ToLower() -ne $artifact.sha256) {
        throw "Cached dependency checksum mismatch: $($artifact.file)"
    }
}
if (!(Test-Path "$deps/obs-studio-30.2.3/libobs/obs.h")) {
    Expand-Archive "$deps/obs.zip" $deps -Force
}
if (!(Test-Path "$deps/mediapipe-0.10.32/mediapipe/tasks/c/vision/face_landmarker/face_landmarker.h")) {
    Expand-Archive "$deps/mediapipe-source.zip" $deps -Force
}
if (!(Test-Path "$deps/mediapipe/mediapipe/tasks/c/libmediapipe.dll")) {
    Expand-Archive "$deps/mediapipe.whl" "$deps/mediapipe" -Force
}
if ($IncludeObsRuntime) {
    # Build-only OBS runtime; versioned upstream asset, not shipped in the plugin.
    $archive = "$deps/obs-runtime-$($lock.obsVersion).zip"
    if (!(Test-Path $archive)) {
        Invoke-WebRequest "https://github.com/obsproject/obs-studio/releases/download/$($lock.obsVersion)/OBS-Studio-$($lock.obsVersion)-Windows.zip" -OutFile "$archive.partial"
        if ((Get-FileHash "$archive.partial" -Algorithm SHA256).Hash.ToLower() -ne $lock.obsRuntimeSha256) {
            throw 'OBS runtime download checksum mismatch.'
        }
        Move-Item -LiteralPath "$archive.partial" -Destination $archive
    }
    if ((Get-FileHash $archive -Algorithm SHA256).Hash.ToLower() -ne $lock.obsRuntimeSha256) {
        throw 'Cached OBS runtime checksum mismatch.'
    }
    if (!(Test-Path "$deps/obs-runtime/bin/64bit/obs.dll")) {
        Expand-Archive $archive "$deps/obs-runtime" -Force
    }
    (Get-FileHash $archive -Algorithm SHA256).Hash.ToLower() | Set-Content "$deps/obs-runtime.sha256" -Encoding ascii
}
Write-Host 'Pinned OBS headers, MediaPipe runtime and face model verified and ready.'
