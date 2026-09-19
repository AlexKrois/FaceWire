param([Parameter(Mandatory = $true)][string]$Package)
$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.IO.Compression.FileSystem
$expected = @(
    'obs-plugins/64bit/camoutlines.dll',
    'data/obs-plugins/camoutlines/libmediapipe.dll',
    'data/obs-plugins/camoutlines/face_landmarker.task',
    'data/obs-plugins/camoutlines/locale/en-US.ini',
    'data/obs-plugins/camoutlines/locale/de-DE.ini',
    'data/obs-plugins/camoutlines/licenses/MediaPipe-LICENSE.txt',
    'data/obs-plugins/camoutlines/LICENSE',
    'data/obs-plugins/camoutlines/THIRD_PARTY.md'
)
$zip = [IO.Compression.ZipFile]::OpenRead((Resolve-Path -LiteralPath $Package).Path)
try {
    $files = @($zip.Entries | Where-Object { $_.Name })
    $actual = @($files | ForEach-Object { $_.FullName.Replace('\','/') })
    $difference = Compare-Object ($expected | Sort-Object) ($actual | Sort-Object)
    if ($difference -or $files.Count -ne $expected.Count) { throw "Unexpected release ZIP contents: $($difference | Out-String)" }
    if ($files | Where-Object Length -EQ 0) { throw 'Release ZIP contains an empty file.' }
    Write-Host "Release ZIP verified: $($files.Count) runtime and license files; no source or build files."
} finally { $zip.Dispose() }
