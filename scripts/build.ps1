param([string]$ObsRoot = 'C:\Program Files\obs-studio')
$ErrorActionPreference = 'Stop'
# Ninja avoids launcher-specific MSBuild environment collisions.
$root = Split-Path $PSScriptRoot -Parent
$deps = Join-Path $root '.deps'
$vswhere = "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe"
$vs = & $vswhere -latest -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (!$vs) { throw 'Install Visual Studio with the Desktop development with C++ workload.' }
$versionMatch = [regex]::Match((Get-Content "$root/CMakeLists.txt" -Raw), 'project\(camoutlines VERSION ([0-9]+\.[0-9]+\.[0-9]+)')
if (!$versionMatch.Success) { throw 'Cannot read project version.' }
$version = $versionMatch.Groups[1].Value
$tools = Get-ChildItem "$vs/VC/Tools/MSVC" -Directory | Sort-Object Name -Descending | Select-Object -First 1
$bin = Join-Path $tools.FullName 'bin/Hostx64/x64'
$cmake = Join-Path $vs 'Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe'
$ctest = Join-Path (Split-Path $cmake) 'ctest.exe'
$ninja = Join-Path $vs 'Common7/IDE/CommonExtensions/Microsoft/CMake/Ninja/ninja.exe'
Import-Module "$vs/Common7/Tools/Microsoft.VisualStudio.DevShell.dll"
Enter-VsDevShell -VsInstallPath $vs -SkipAutomaticLocation -DevCmdArguments '-arch=x64 -host_arch=x64' | Out-Null
if (!(Test-Path "$deps/obs-studio-30.2.3/libobs/obs.h")) { throw 'Run scripts/prepare.ps1 first.' }
# The installed OBS runtime exposes the official ABI; generate a matching import library.
$exports = & "$bin/dumpbin.exe" /nologo /exports "$ObsRoot/bin/64bit/obs.dll"
if ($LASTEXITCODE) { throw 'Cannot inspect obs.dll.' }
$names = $exports | ForEach-Object { if ($_ -match '^\s+\d+\s+[0-9A-F]+\s+[0-9A-F]+\s+(\S+)') { $Matches[1] } }
if (!$names) { throw 'No OBS exports found.' }
@('LIBRARY obs.dll', 'EXPORTS') + $names | Set-Content "$deps/obs.def" -Encoding ascii
& "$bin/lib.exe" /nologo /machine:x64 "/def:$deps/obs.def" "/out:$deps/obs.lib"
if ($LASTEXITCODE) { throw 'Cannot build OBS import library.' }
& $cmake -S $root -B "$root/build/native" -G Ninja "-DCMAKE_MAKE_PROGRAM=$ninja" -DCMAKE_BUILD_TYPE=Release
if ($LASTEXITCODE) { throw 'CMake configuration failed.' }
& $cmake --build "$root/build/native" --config Release --parallel
if ($LASTEXITCODE) { throw 'Build failed.' }
& $ctest --test-dir "$root/build/native" -C Release --output-on-failure
if ($LASTEXITCODE) { throw 'Tests failed.' }
$stage = [IO.Path]::GetFullPath((Join-Path $root 'dist/runtime'))
# Delete only this dedicated generated staging directory, never a caller-provided path.
if ($stage -ne [IO.Path]::GetFullPath("$root/dist/runtime")) { throw 'Unexpected staging path.' }
if (Test-Path -LiteralPath $stage) { Remove-Item -LiteralPath $stage -Recurse -Force }
& $cmake --install "$root/build/native" --config Release --prefix $stage
if ($LASTEXITCODE) { throw 'Packaging failed.' }
$package = "$root/dist/camoutlines-$version-windows-x64.zip"
Compress-Archive -Path "$stage/obs-plugins", "$stage/data" -DestinationPath $package -Force
& "$PSScriptRoot/test-package.ps1" -Package $package
$checksum = (Get-FileHash $package -Algorithm SHA256).Hash.ToLower()
"$checksum  $(Split-Path $package -Leaf)" | Set-Content "$package.sha256" -Encoding ascii
$commit = & git -C $root rev-parse --verify HEAD 2>$null
if ($LASTEXITCODE) { $commit = 'uncommitted' }
@{
    version = $version
    commit = $commit
    platform = 'windows-x64'
    obsRuntime = (Get-Item "$ObsRoot/bin/64bit/obs.dll").VersionInfo.ProductVersion
    obsImportSourceSha256 = (Get-FileHash "$ObsRoot/bin/64bit/obs.dll").Hash.ToLower()
    mediapipeRuntimeSha256 = (Get-FileHash "$deps/mediapipe/mediapipe/tasks/c/libmediapipe.dll").Hash.ToLower()
    modelSha256 = (Get-FileHash "$deps/face_landmarker.task").Hash.ToLower()
    packageSha256 = $checksum
} | ConvertTo-Json | Set-Content "$root/dist/build-info.json" -Encoding utf8
Write-Host "Plugin package: $package"
exit 0
