# Tests

`scripts/build.ps1` runs the dependency-free geometry tests and a native MediaPipe
smoke test (model initialization, video inference on blank frames, cleanup).

The optional integration test uses the installed OBS 30.2.3 runtime and D3D11
in a separate console process. It does not open the OBS application, access a
camera, modify scenes, stream, or record the desktop.

Prepare a portrait fixture with Python and Pillow (test-only dependencies):

```powershell
Invoke-WebRequest 'https://storage.googleapis.com/mediapipe-assets/portrait.jpg' -OutFile '.deps/portrait.jpg'
python -c "from PIL import Image; import struct; im=Image.open('.deps/portrait.jpg').convert('RGBA'); im.thumbnail((640,640)); open('.deps/portrait.rgba','wb').write(struct.pack('<II',*im.size)+im.tobytes())"
./scripts/test-render.ps1
python -c "from PIL import Image; Image.open('build/obs-preview.ppm').save('build/obs-preview.png')"
```

The fixture is an official MediaPipe test asset, downloaded separately and not
included in the plugin distribution. Its origin is the URL above.

The detector test checks 478 finite, in-frame landmarks and subsequent face loss.
The render test loads the built plugin through `obs_open_module`, captures OBS
video output, checks each of the eight region toggles, all regions disabled,
mesh opacity (zero/full), the German locale,
face loss/reacquisition and filter disable/enable. An actual rendered frame is
saved for visual inspection. Separate full-outline, iris and mesh frames are saved
as `build/obs-preview.ppm.oval.ppm`, `.iris.ppm` and `.mesh.ppm`.
Camera motion, source transforms, changing source
resolution, alpha compositing, long sessions and different hardware still need
interactive testing.
