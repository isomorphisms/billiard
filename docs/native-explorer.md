# Native polygon and cover explorer

The initial rectangle keeps its continuous controls and explicit corner-reversal
rule. The fixture tabs select square, rectangle, right triangle, equilateral
triangle and right-angled L. `VIEW` switches between polygon vertex stops and
translation-cover flow; rectangle also offers the original `LEGACY` scene.
Regular marked cover vertices can be crossed. Positive cone singularities stop
the geodesic. `PERIODIC` requires an exact return witness; caps remain
`UNKNOWN LIMIT`, with no density or orbit-closure inference.

## Controls and geometry

- Drag the blue start on an exact 1/24 bounding-box grid. Points outside the
  polygon, including the L notch, are rejected.
- Drag the direction handle to one of 24 exact directions, 15 degrees apart.
- Drag the rectangle's right-wall handle to change width in 1/16 increments
  between 1/2 and 3/2 of the base width. Legacy controls remain continuous.
- Upper left pauses/resumes. Upper right cycles 0.2, 0.5 and 1.1 source-length
  units per second in mathematical views, or 20, 50 and 110 scene units in legacy.
- `SHEAR` enters cover mode and cycles 0, 0.5, 1 and -0.5. This is explicitly
  labelled `AFFINE COVER / FOLDED PROJECTION`; a sheared folded picture does not
  assert ordinary specular reflection in that polygon.
- `CYL` toggles separately certified cover-cylinder counts. Unsupported searches
  display `CYL UNKNOWN`. Genus and zero data come from the checked cover;
  torus tau is displayed only for genus one.

Geometric distance is measured after applying the displayed affine map to the
developed straight direction. Reflected display chords and Rough jitter never
feed back into the solver. Pause/focus changes update the clock without banking
elapsed time. Geometry edits reset reveal distance. Periodic tips repeat after
one certified period; bounded nonperiodic paths stop at their reported endpoint.

The lower panel shows at most eight genuinely developed chart copies with
paired side identifiers and a faint straight ray. The solver retains at most
160 events; its model is tested below 512 KiB. Exact topology is rebuilt only
after geometry changes, outside the Android window-buffer lock. This is bounded
host memory evidence, not a measured phone performance claim.

## Executed host evidence

`make test explorer-pixels` requires the source-qualified pinned ICK stage.
The shared control tests exercise all five fixtures and 24 headings, polygon
versus cover vertex behavior, cone termination, event caps, inverse-affine
dragging, rectangle width and pause timing. Seven scenes pass through the pinned
ICK-built Rough executable; independent pixel-region checks require visible
header, geometry, unfolding and controls. CI retains the actual operation
streams and PPMs. These are host images, not screenshots from Android.

## Package and physical evidence

The existing Cat Food paired build plan and canonical NativeActivity packager
produce version 2 / `0.2-test` candidates for A1 `armeabi-v7a` and C67
`arm64-v8a`. All application, math, renderer and glue C use pinned ICK. ARM32
division support is assembled from the same verified ICK reference source.
The existing registered test signer is retained. The package is
`org.isomorphisms.billiard`, has no DEX, and embeds only its declared ABI.
Actual source, package and signer digests belong to the workflow artifact's
receipts; a workflow definition alone is not successful package evidence.

Physical acceptance remains **NOT RUN** independently for each device. This
environment has no verified ADB/device connection. No installed, launched,
emulator or human-visible phone PASS is claimed. Cat Food's catalog promotion
requires real published artifacts and per-device acceptance; the build plan is
not that promotion.

For each device, retain the exact model/ABI/Android identity, source and APK
SHA-256, installed version and signer, launch log, screenshots before/after
input, and a verdict for each case below. A C67 receipt cannot substitute for A1.

| Physical case | Required observation | A1 | C67 |
| --- | --- | --- | --- |
| Install and launch | Matching ABI, version 2, visible NativeActivity | NOT RUN | NOT RUN |
| Legacy rectangle | Right wall/start/angle recompute; corner reversal | NOT RUN | NOT RUN |
| Pause/resume | Fixed tip while paused; no accumulated-time jump | NOT RUN | NOT RUN |
| All five tabs | Rough walls, dotted path, faint copies, usable controls | NOT RUN | NOT RUN |
| Square vertex | Polygon vertex stop versus regular cover continuation | NOT RUN | NOT RUN |
| L cone | Start 6/24,6/24; heading 45 degrees stops at the cone | NOT RUN | NOT RUN |
| Limits and cylinders | UNKNOWN stays visible; no finite-run theorem claim | NOT RUN | NOT RUN |
| Shear and drag | Affine label, inverse-map start drag, notch rejection | NOT RUN | NOT RUN |
| Lifecycle | Focus loss/resume, resize and touch cancellation stay usable | NOT RUN | NOT RUN |

`FRAME` logs bind input sequence to pixel hash; mathematical views also emit
`MATH` with fixture, view, heading, grid coordinates, shear, distance, flow stop,
event count and cylinder status. Logs support comparison with screenshots;
neither a log nor a host PPM alone proves physical acceptance.
