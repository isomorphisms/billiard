# Billiard Android targets and build obligations

Cat Food plans two build-host artifacts for each qualified source commit:

| Selector | Device | ABI | Priority |
| --- | --- | --- | --- |
| phone | MIRO A1 | armeabi-v7a | primary |
| c67 | MIRO C67 | arm64-v8a | paired/current physical test |

The source contract is in [../ci/android-application.tsv](../ci/android-application.tsv).
With a verified Cat Food host checkout, a release producer starts by running
`./catfood plan-android-build /path/to/billiard/ci/android-application.tsv phone FULL_SOURCE_SHA`.
This returns **obligations**, not actual APKs or an executed build.

## Next implementation stage

Build a DEX-free Android NativeActivity with a thin NativeWindow/touch adapter:
dragging the **right wall** changes width; dragging the start dot changes the
source; angular touch control adjusts direction; play/pause/speed changes the
geometric distance budget. Pausing does not accumulate wall-clock time.

The platform adapter consumes the same billiard C kernel and the caller-owned
rough.framebuffer pixels, including the reviewed alpha and patterned-stroke work.
The wall's mathematical position is never determined by its rough stroke.

The existing exact compiler / ICK-versus-NDK capability check must be recorded
for both target ABIs before any producer stage. NativeActivity packaging must
use the pinned android-NDK packaging/signing lane and independent ai-ci receipt,
not a newly generated signing key.

**Status:** geometry host code and tests only. This repository currently does
not contain or claim a signed APK, qualified crosscompiler stages, C67/A1 device
installation, physical interaction evidence, or a Cat Food publication receipt.
