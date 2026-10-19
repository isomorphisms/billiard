# Billiard: paired Android NativeActivity

The same C billiard kernel and rough.framebuffer dotted/alpha drawing stream
power a DEX-free native app, without Java or Kotlin application logic.

| Cat Food selector | Phone | ABI | Role |
| --- | --- | --- | --- |
| phone | MIRO A1 | armeabi-v7a | primary producer |
| c67 | MIRO C67 | arm64-v8a | paired producer, current physical user test |

See the [Cat Food application contract](../ci/android-application.tsv)
(`min_sdk=23` for the native in-memory FILE adapters).

## User interaction

The rectangle below remains the initial legacy scene. The new fixture tabs,
exact polygon/cover views, cylinder labels and affine controls are documented
with their limits in [native explorer](../docs/native-explorer.md).

Initial point is one-third from the top and centered horizontally. Initial
direction is 45° up-right. A corner reverses the entering velocity exactly.
The physics is independent of the rough display.

- Drag the right wall horizontally: lengthen/shorten the rectangle.
- Drag the starting point: change the relative origin.
- Drag the launch direction handle: change the starting angle.
- Tap upper left: play/pause. Tap upper right: cycle travel speeds
  (20, 50, 110 mathematical pixels/second, then MAX at 2000).
- A paused app does not accumulate elapsed time; the progressive trajectory
  is measured by geometric distance rather than counting rebounds.
- Main box is rough but mathematically sharp. The ray is translucent, dotted.
  Reflected copies and unfolded ray are much fainter.

The app produces a bounded software ARGB framebuffer and copies the result
into a native RGBA window. The source imports the pinned native
`rough.framebuffer` implementation rather than bundling an altered copy.

## ICK-only host and Android compilation

**No GCC, Clang or NDK C compiler may compile Billiard or its imported
rough.framebuffer code.** Host and A1/C67 C source compilation must run
through positively verified ICK. ICK's executable historically ends in
\`gcc\` because of its ancestry; the binary must be the source-built ICK
revision, not the stock compiler.

The CI uses \`isomorphisms/ai-ci/ick-host\` for mathematical tests and
\`isomorphisms/ai-ci/ick-android\` for both mobile ABIs. The pinned ICK
source is \`c61e448251744a2f40ad743ebef1a027bdcd2f9d\`.
Consumer source is compiled into ICK objects, including Android native app
glue. Only NDK Bionic headers, CRT and the direct \`ld.lld\` linker are used
after ICK source compilation. This stage must fail closed if ICK cannot
compile the required headers or objects.

## Legacy build notes (superseded)

A qualified Linux host with Android NDK `29.0.14206865` and a verified
checkout of the exact rough.framebuffer dotted/alpha branch at
`d8ede905ac83c27370a14b92f81b77d6d189d989` can build:

Use the pinned ICK Android action output as `ICK_CC` and its verified source as `ICK_SOURCE_DIR` before invoking the Android Makefile. No default compiler exists.

The paths are illustrative and require verified host inputs, not Termux
on a phone. `.github/workflows/billiard-native.yml` runs both real builds
and records ABI ELF headers, source identity, native symbol/size/hash and
unstripped vs stripped differences. Cat Food's read-only
`./catfood plan-android-build` produces A1+C67 obligations on the host,
but that plan is not a build or a published APK.

## Remaining acceptance

Only an **approved existing** signer, registered package identity and the
canonical `android-NDK/apk/build-nativeactivity-apk.sh` producer may produce
installable APK candidates. After that: independent AICI test, emulator
screenshot movement verification, physical C67 drag/direction/path acceptance,
and separate A1 physical acceptance.

A native `.so` alone is not an APK and cannot be installed. Exact-head CI
produces paired signed test-only APK candidates and package receipts after
ICK-only compilation. Verify the run and receipt for the actual source version;
an earlier rectangle APK does not contain the explorer. Installation and
physical C67/A1 results remain NOT RUN. Device builds must not fall back to
compiling on the handset.


## Excluded historical evidence

Earlier candidate Android native-library builds and numerical tests invoked
stock GCC/Clang or an NDK C compiler. They remain visible in historical CI
but **do not qualify ICK-only deliverables**. The current accepted input is
an exact-head run of the new ICK workflows, not an older green check.

## ARM32 integer division support

The direct linker must also receive arithmetic support normally added by a
compiler driver. The pinned ARMv7-A target cannot assume hardware integer
division. `android/Makefile` assembles `L_udivsi3`, `L_divsi3` and `L_dvmd_lnx` from the
verified ICK reference's `libgcc/config/arm/lib1funcs.S` through the same ICK
driver. This supplies signed/unsigned division/remainder and the divide-by-zero
handler (which calls Bionic `raise`). Its source carries the GCC Runtime
Library Exception. No prebuilt stock compiler runtime is linked.

The renderer's checked 8192 × 1000 bounds allow exact 32-bit pixel products.
That removes its 64-bit division requirement but still needs the ARM EABI
32-bit divide helper. Host tests compare the bounded mapping with the old
64-bit formula. Native link success and physical runtime acceptance remain
separate evidence.
