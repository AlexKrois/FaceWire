# Publishing a release

Repository: https://github.com/AlexKrois/obs-camoutlines

1. Update the version in `CMakeLists.txt` and write the changelog entry.
2. Run `scripts/prepare.ps1`, `scripts/build.ps1` and the optional OBS render test.
3. Check the packaged installation on Windows and review the license notices.
4. Commit the release changes and push the default branch.
5. Tag the exact commit with a version matching CMake:

```sh
git tag -a v0.2.0 -m "Cam Outlines 0.2.0"
git push origin v0.2.0
```

The Windows workflow builds from the tagged source, checks that the tag matches
the CMake version, and creates a draft prerelease. The release job has write access;
normal builds and pull requests have read-only repository permissions.

Review the draft's notes and assets, then publish it from GitHub Releases.
No tag or public binary release is created by a local build. Do not move an
existing published tag: fix issues in a new version.

For a build without creating a release, use **Actions → Windows build and release
→ Run workflow**. CI runs the geometry and native inference smoke tests. The OBS
D3D11 render test is a separate local check because it requires a graphics adapter.

Downloads and build outputs are excluded from Git. The workflow downloads pinned
dependencies; the first online build must pass before publishing a release. Byte-
identical artifacts across compilers are not guaranteed. `build-info.json` records
the commit and runtime/model/package hashes used for the build.

GitHub documentation: [manual workflow runs](https://docs.github.com/en/actions/how-tos/manage-workflow-runs/manually-run-a-workflow).
