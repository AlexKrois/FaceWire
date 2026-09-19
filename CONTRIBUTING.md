# Contributing

Build requirements and commands are in [README.md](README.md). Keep changes
focused, describe the user-visible behavior and report which OBS version you tested.

1. Run `scripts/prepare.ps1` and `scripts/build.ps1` on Windows x64.
2. For tracking/rendering changes, run `scripts/test-render.ps1` and visually inspect
   the output as described in [tests/README.md](tests/README.md).
3. Keep saved filter keys compatible. New visual options should be opt-in.
4. Update English/German strings and the changelog as needed.

Do not commit downloaded dependencies, model binaries, build outputs, OBS logs or
personal camera images. Regenerate topology with `python scripts/generate-topology.py`
after changing the pinned MediaPipe source, and preserve its attribution.

Contributions are licensed under GPL-3.0-or-later unless a file explicitly states
another compatible license (for example the generated Apache-2.0 topology).
