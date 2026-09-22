# Cam Outlines for OBS Studio

A native Windows x64 OBS filter that draws facial contours, iris rings and an
optional face mesh over an existing video source. Powered by MediaPipe Face
Landmarker; no Python process or separate camera connection is needed at runtime.

[Deutsch](docs/README.de.md) · [Releases](https://github.com/AlexKrois/obs-camoutlines/releases) · [Changelog](CHANGELOG.md)



https://github.com/user-attachments/assets/13bfda0f-3258-4234-87d2-945895f82d33



## Features

- Independently selectable eyes, eyebrows, nose, mouth and jaw.
- Full face outline including the forehead, plus iris rings.
- Optional wireframe with 1,322 unique edges and separate width/opacity controls.
- Adjustable line color, opacity, width, smoothing, tracking resolution and FPS.
- Background CPU inference for one face; expired results disappear automatically.
- English and German settings.

**Status:** early Windows release. Built against OBS 30.2.3 and integration-tested
with OBS 30.2.3 and 32.2.2 (Steam). Other versions/platforms are not yet validated.

## Install

1. Close OBS and download **camoutlines-0.2.1-windows-x64.zip** from
   [Releases](https://github.com/AlexKrois/obs-camoutlines/releases/tag/v0.2.1).
   Choose this asset, not GitHub's automatically generated source-code ZIP.
2. Open your OBS installation folder. For Steam the default is:
   `C:\Program Files (x86)\Steam\steamapps\common\OBS Studio`.
3. Extract **both** folders from the ZIP (`obs-plugins` and `data`) directly into
   that OBS folder. Merge folders and replace the existing Cam Outlines files when updating.
   The resulting layout is:

```text
C:\Program Files (x86)\Steam\steamapps\common\OBS Studio\
  obs-plugins\64bit\camoutlines.dll
  data\obs-plugins\camoutlines\libmediapipe.dll
  data\obs-plugins\camoutlines\face_landmarker.task
  data\obs-plugins\camoutlines\locale\en-US.ini
  data\obs-plugins\camoutlines\locale\de-DE.ini
  data\obs-plugins\camoutlines\LICENSE
  data\obs-plugins\camoutlines\THIRD_PARTY.md
  data\obs-plugins\camoutlines\licenses\MediaPipe-LICENSE.txt
```

4. Start OBS. Open your video source's **Filters → Effect Filters → + →
   Cam Outlines — Face landmarks**.
5. Choose the regions to display. Start with 640 tracking resolution, 30 tracking
   FPS and 2.5-pixel lines. For the mesh, try 1 pixel and 35% opacity.

The full outline, iris and mesh are opt-in. Upgrade the existing filter rather
than adding another instance; two active instances draw two overlays.
The Microsoft Visual C++ x64 runtime must be installed (normally present with OBS).
For standalone OBS, use its installation folder (usually `C:\Program Files\obs-studio`)
instead. Do not extract the whole ZIP into `obs-plugins\64bit`: only the plugin DLL
belongs there. Its supporting files must retain their separate `data` paths.

### Direct installation, including Steam

From a built checkout, while OBS is closed:

```powershell
./scripts/install.ps1 -ObsRoot 'C:\Program Files (x86)\Steam\steamapps\common\OBS Studio'
```

The installer backs up replaced files and checks their hashes. Administrator
rights may be needed for the destination. It installs the same two-folder layout
as the release ZIP. No additional `camoutlines` or `data` wrapper folder is needed.

If settings show `FilterName` or `Help`, or the runtime/model cannot be found,
check the data layout. Consult the OBS log for `[camoutlines]` errors. After
correcting files, restart OBS.

## Build

Requirements: Windows x64, PowerShell 7, Git, and Visual Studio 2022 or newer with
Desktop development with C++ and the CMake tools component.

```powershell
git clone https://github.com/AlexKrois/obs-camoutlines.git
cd obs-camoutlines
./scripts/prepare.ps1 -IncludeObsRuntime
./scripts/build.ps1 -ObsRoot "$PWD/.deps/obs-runtime"
```

Alternatively omit `-IncludeObsRuntime` and pass your installed OBS folder to
`build.ps1`. The default is `C:\Program Files\obs-studio`.

The source archives, MediaPipe wheel, model and optional build-only OBS runtime
are checked against committed SHA-256 hashes in `dependencies.json`.
No Python packages are installed. Python is only needed to regenerate the mesh
topology or prepare an optional test image.

The build runs geometry and native inference tests and produces a Windows ZIP,
SHA-256 checksum and `build-info.json` in `dist/`. The ZIP includes the plugin,
model, native runtime, translations and license notices only. Source code and
developer files remain in this repository and GitHub's separate source-code archive.
The ZIP's exact eight-file layout is checked by `scripts/test-package.ps1`.

See [tests/README.md](tests/README.md) for the additional D3D11 integration test
and [CONTRIBUTING.md](CONTRIBUTING.md) for development guidance.

## Limits

- One face and SDR sources; no HDR color-fidelity guarantee.
- Fast movement, profile views or occlusion may cause lag or missing contours.
- Iris rings are estimated landmarks, not calibrated gaze or pupil-size measurements.
- CPU inference runs independently, but GPU readback and mesh drawing still cost
  rendering time. Performance depends on hardware and settings.
- No Linux/macOS builds, multiple-face selection or editable connections yet.
- Live camera motion, transforms, source-resolution changes and long sessions
  require further testing across systems.

