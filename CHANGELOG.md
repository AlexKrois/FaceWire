# Changelog

## 0.2.0

- Full face outline, including the forehead.
- Iris rings and optional face mesh with 1,322 unique edges.
- Independent mesh width and opacity controls.
- Existing settings preserved; new regions disabled by default.
- Deduplicated edges shared by the jaw and full face outline.
- Actionable errors for missing runtime/model files.
- Installer with backups and file verification.
- Eight region switches, mesh opacity, face loss, reacquisition and filter
  enable/disable tested with OBS 32.2.2 on Windows.

## 0.1.0

- Initial Windows x64 prototype with eyes, eyebrows, nose, mouth and jaw contours.
- Native MediaPipe inference on a worker thread and configurable smoothing.
- Color, opacity, line width, tracking FPS and resolution settings.
- English and German localization.
