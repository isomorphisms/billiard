# Billiard build and mathematical acceptance

The user's build rule is **ICK only** for all maintained billiard consumer C:
math, scene, Android activity, Android native-app glue, and imported
rough.framebuffer geometry. This includes host tests. The ICK compiler is
GCC-derived and its stage executable is historically named `gcc`, but it
must be positively identified as the compiler built from
`dilapidated-shed/ick@c61e448251744a2f40ad743ebef1a027bdcd2f9d`
with GCC reference `6294f1d9e7536e5ffcde09d1528c918d63abfef5`.
A compiler named `gcc` is NOT proof it is ICK.

No stock GCC, Clang, direct Android NDK C compilation, or silent fallback is
permitted for this project. Bootstrap tools used to build the ICK compiler
itself do not qualify to compile Billiard source.

The Android NDK can supply Bionic headers, CRT, binary inspection, and
`ld.lld` **linking already ICK-compiled objects only**. Do not send any
consumer `.c` or `.s` to an NDK C compiler.

Build evidence must identify which exact producer compiled each translation
unit, and keep ICK object compilation, Android linking, APK signing, emulator
execution and real C67/A1 operation distinct. A failed ICK or link stage is
BLOCKED, not authorization for a fallback.

For this initial application:

- start `(width/2, height/3)` from the top-left;
- angle 45 degrees toward the upper-right, adjustable;
- exact corner intersection reverses the full incoming direction;
- near-corner is a single wall reflection;
- visible roughness never changes mathematical collision coordinates;
- the viewport's dotted path phase follows geometric arc length;
- playback speed is distance per second and pause does not accumulate time;
- A1 and C67 need separate target evidence.

Do not merge, package or publish an earlier stock-compiler artifact as an
ICK-built package. Use pinned ICK CI evidence first.
