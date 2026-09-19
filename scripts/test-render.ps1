param([string]$ObsRoot = 'C:\Program Files\obs-studio')
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$fixture = Join-Path $root '.deps/portrait.rgba'
if (!(Test-Path $fixture)) {
    throw 'Prepare .deps/portrait.rgba as described in tests/README.md.'
}
$env:PATH = "$ObsRoot/bin/64bit;" + $env:PATH
& "$root/build/native/detector-test.exe" "$root/.deps/mediapipe/mediapipe/tasks/c/libmediapipe.dll" "$root/.deps/face_landmarker.task" $fixture
if ($LASTEXITCODE) { throw 'Portrait inference test failed.' }
& "$root/build/native/obs-render-test.exe" $ObsRoot "$root/dist/camoutlines/bin/64bit/camoutlines.dll" "$root/dist/camoutlines/data" $fixture "$root/build/obs-preview.ppm"
if ($LASTEXITCODE) { throw 'OBS render integration test failed.' }
