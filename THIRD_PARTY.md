# Dependencies and references

- OBS Studio 30.2.3: https://github.com/obsproject/obs-studio/tree/30.2.3
  GPL-2.0-or-later; headers used at build time, OBS runtime supplied by the user.
- MediaPipe 0.10.32: https://github.com/google-ai-edge/mediapipe/tree/v0.10.32
  Apache-2.0. Native Windows runtime extracted from the official PyPI wheel:
  https://pypi.org/project/mediapipe/0.10.32/
  The wheel's license file is included in the generated package under data/licenses.
- MediaPipe Face Landmarker model v1:
  https://storage.googleapis.com/mediapipe-models/face_landmarker/face_landmarker/float16/1/face_landmarker.task
  Documentation/model information:
  https://ai.google.dev/edge/mediapipe/solutions/vision/face_landmarker
- Landmark topology reference:
  https://github.com/google-ai-edge/mediapipe/blob/v0.10.32/mediapipe/tasks/python/vision/face_landmarker.py
  `src/mesh-topology.hpp` is derived from its Apache-2.0-licensed tessellation list,
  converted to unique undirected edges by `scripts/generate-topology.py`.
- Conceptual inspiration: https://github.com/norihiro/obs-face-tracker
  No source files from that project are incorporated.

Cam Outlines source is provided under GPL-3.0-or-later (compatible with the
GPL-2.0-or-later OBS interface and Apache-2.0 dependency). See LICENSE.
This prototype does not use dlib's iBUG 68-point model.
