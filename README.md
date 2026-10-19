# Billiard — toward Mirzakhani

Start with one rectangle whose right wall can be dragged. A ray follows exact mathematical wall coordinates; visible irregularity is only a drawing treatment.

## Initial contract

- Coordinates: origin at upper-left; x increases rightward, y increases downward.
- First position: **(width / 2, height / 3)**, horizontally centered and one third down from the top.
- First direction: **45 degrees above the rightward horizontal**, i.e. normalized (cos 45°, -sin 45°); start point and angle are adjustable.
- Every flat wall is a specular reflector.
- **An exact corner collision reverses the incoming velocity:** v_out = -v_in. Detect numerical simultaneity only at floating-point roundoff scale; near-corner impacts remain single-wall reflections.
- Recompute paths when the rectangle changes; preserve the starting parameters, do not pretend a wall movement merely stretches a cached path.
- Reveal trajectory by cumulative geometric distance at adjustable speed. Pause preserves exact progress; no deferred time advance on resume.
- Draw rough walls, lightly translucent **dotted** ray paths, and *much fainter* dotted unfolded copies. Keep geometric truth separate from the visual jitter.
- Unfolded rectangle paths are straight; folding them reproduces reflected paths (with the explicit corner convention).
- Track the rectangular flat torus modulus tau = i * height / width; this is one family in the upper half-plane, not a general moduli-space implementation.

## Repository boundaries

- `billiard`: mathematical collisions, trajectories, controls, unfolded scene and rendering parameters.
- `rough.framebuffer`: caller-owned framebuffer, alpha source-over and continuous patterned strokes. Prefer its existing coverage-strokes PR; don't duplicate its alpha work.
- `Cat Food`: build-host A1 (`armeabi-v7a`) + C67 (`arm64-v8a`) planning and verified package delivery. Phones do not compile applications. A1 is the primary package lane; C67 is the current physical acceptance target.
- `ai-ci`: independent acceptance of math, render output and producer receipts.

The initial browser/host demonstration is not evidence of an Android build, APK installation, or physical-device acceptance.

## BIL campaign implementation

The bounded exact polygon kernel, checked translation covers, straight flow,
cylinder certificates, witnessed periods/affine actions and finite orbit corpus
are implemented in separate C modules. Native fixture tabs and truthful
polygon/cover controls use those same modules and the pinned Rough renderer.
See [native explorer controls and acceptance](docs/native-explorer.md),
[bounded experiments](docs/bounded-experiments.md), and the linked mathematical
module notes. All maintained C, including tests and Android glue, is ICK-only.
Physical A1 and C67 acceptance remains separate from successful host/ABI/package
checks; current per-device cases are recorded as NOT RUN.
