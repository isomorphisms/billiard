# ICK-only C compilation for every billiard and native-app-glue source.
# NDK is a header/Bionic provider; ld.lld is a link-only tool, not a C compiler.
# No stock compiler and no compile fallback.
NDK_ROOT ?=
ANDROID_API ?= 23
ANDROID_ABI ?= arm64-v8a
ICK_CC ?=
ICK_SOURCE_DIR ?=
ROUGH_SRC ?= ../_rough_framebuffer/src
ICK_SOURCE_SHA = c61e448251744a2f40ad743ebef1a027bdcd2f9d
ICK_REFERENCE_SHA = 6294f1d9e7536e5ffcde09d1528c918d63abfef5

ifeq ($(ANDROID_ABI),armeabi-v7a)
  GNU_TARGET := arm-linux-gnueabi
  NDK_TRIPLE := arm-linux-androideabi
  TARGET_FLAGS := -marm -march=armv7-a -mfpu=vfpv3-d16 -mfloat-abi=softfp
  REQUIRED_CLASS := ELF32
  REQUIRED_MACHINE := ARM
  PAGE_FLAGS :=
else ifeq ($(ANDROID_ABI),arm64-v8a)
  GNU_TARGET := aarch64-linux-gnu
  NDK_TRIPLE := aarch64-linux-android
  TARGET_FLAGS := -ffixed-x18
  REQUIRED_CLASS := ELF64
  REQUIRED_MACHINE := AArch64
  PAGE_FLAGS := -z max-page-size=16384 -z common-page-size=16384
else
  $(error Billiard only accepts A1 armeabi-v7a or C67 arm64-v8a)
endif

NDK_BIN := $(NDK_ROOT)/toolchains/llvm/prebuilt/linux-x86_64/bin
SYSROOT := $(NDK_ROOT)/toolchains/llvm/prebuilt/linux-x86_64/sysroot
LIBDIR := $(SYSROOT)/usr/lib/$(NDK_TRIPLE)/$(ANDROID_API)
BUILTIN := $(shell if test -x "$(ICK_CC)"; then "$(ICK_CC)" -print-file-name=include; fi)
GLUE := $(NDK_ROOT)/sources/android/native_app_glue
OUTDIR := ../build/android/$(ANDROID_ABI)
OBJECTS := $(OUTDIR)/billiard.o $(OUTDIR)/scene.o $(OUTDIR)/framebuffer.o $(OUTDIR)/activity.o $(OUTDIR)/app_glue.o
LIBRARY := $(OUTDIR)/libbilliard.so

CFLAGS_ICK := $(TARGET_FLAGS) --sysroot="$(SYSROOT)" -nostdinc -isystem "$(BUILTIN)" -isystem "$(SYSROOT)/usr/include" -isystem "$(SYSROOT)/usr/include/$(NDK_TRIPLE)" -isystem "$(GLUE)" -I../src -I"$(ROUGH_SRC)" -D__ANDROID__ -D__ANDROID_API__=$(ANDROID_API) -D__ANDROID_MIN_SDK_VERSION__=$(ANDROID_API) -DBIONIC_IOCTL_NO_SIGNEDNESS_OVERLOAD -std=gnu11 -O2 -fPIC -Wall -Wextra -Werror -Wno-unused-parameter

.PHONY: all objects check-ick clean
all: $(LIBRARY)
objects: $(OBJECTS)

check-ick:
	@test "$(ANDROID_API)" = 23 || { echo "BLOCKED: unsupported Android API profile"; exit 3; }
	@test -n "$(ICK_CC)" && test -n "$(ICK_SOURCE_DIR)" || { echo "BLOCKED: pinned Android ICK compiler required" >&2; exit 3; }
	@test -x "$(ICK_CC)" && test -d "$(ICK_SOURCE_DIR)" || { echo "BLOCKED: missing ICK installation/source" >&2; exit 3; }
	@test "$$(git -C "$(ICK_SOURCE_DIR)" rev-parse HEAD)" = "$(ICK_SOURCE_SHA)" || { echo "BLOCKED: ICK source revision mismatch" >&2; exit 3; }
	@test "$$(git -C "$(ICK_SOURCE_DIR)/gcc" rev-parse HEAD)" = "$(ICK_REFERENCE_SHA)" || { echo "BLOCKED: ICK reference revision mismatch" >&2; exit 3; }
	@case "$(ICK_CC)" in */ick-android-$(ANDROID_ABI)-stage/bin/$(GNU_TARGET)-gcc) ;; *) echo "BLOCKED: refuse stock compiler or wrong ABI ICK stage" >&2; exit 3 ;; esac
	@test "$$("$(ICK_CC)" -dumpmachine)" = "$(GNU_TARGET)" || { echo "BLOCKED: ICK target mismatch" >&2; exit 3; }
	@cc1="$$("$(ICK_CC)" -print-prog-name=cc1)"; test -x "$$cc1" || { echo "BLOCKED: ICK cc1 missing" >&2; exit 3; }
	@test -f "$(BUILTIN)/stddef.h" && test -f "$(SYSROOT)/usr/include/android/native_activity.h" || { echo "BLOCKED: missing pinned ICK/Bionic headers" >&2; exit 3; }
	@test -x "$(NDK_BIN)/ld.lld" && test -f "$(LIBDIR)/crtbegin_so.o" && test -f "$(LIBDIR)/crtend_so.o" || { echo "BLOCKED: incomplete link-only Android sysroot" >&2; exit 3; }

$(OUTDIR)/billiard.o: ../src/billiard.c ../src/billiard.h check-ick
	@mkdir -p "$(OUTDIR)"
	"$(ICK_CC)" $(CFLAGS_ICK) -c "$<" -o "$@"

$(OUTDIR)/scene.o: ../src/scene.c ../src/scene.h ../src/billiard.h check-ick
	@mkdir -p "$(OUTDIR)"
	"$(ICK_CC)" $(CFLAGS_ICK) -c "$<" -o "$@"

$(OUTDIR)/framebuffer.o: $(ROUGH_SRC)/framebuffer.c $(ROUGH_SRC)/framebuffer.h check-ick
	@mkdir -p "$(OUTDIR)"
	"$(ICK_CC)" $(CFLAGS_ICK) -c "$<" -o "$@"

$(OUTDIR)/activity.o: ../android/native/billiard_android.c ../src/scene.h ../src/billiard.h ../src/pixel_sample.h $(ROUGH_SRC)/framebuffer.h check-ick
	@mkdir -p "$(OUTDIR)"
	"$(ICK_CC)" $(CFLAGS_ICK) -c "$<" -o "$@"

$(OUTDIR)/app_glue.o: $(GLUE)/android_native_app_glue.c check-ick
	@mkdir -p "$(OUTDIR)"
	"$(ICK_CC)" $(CFLAGS_ICK) -c "$<" -o "$@"

$(LIBRARY): $(OBJECTS)
	"$(NDK_BIN)/ld.lld" -shared --no-undefined --hash-style=gnu -z relro -z now $(PAGE_FLAGS) --sysroot="$(SYSROOT)" -soname libbilliard.so -L"$(LIBDIR)" -o "$@" "$(LIBDIR)/crtbegin_so.o" $(OBJECTS) -landroid -llog -lm -lc -ldl "$(LIBDIR)/crtend_so.o"
	"$(NDK_BIN)/llvm-readelf" -h -d -Ws "$@" > "$(OUTDIR)/linked-elf.txt"
	grep -q ANativeActivity_onCreate "$(OUTDIR)/linked-elf.txt"

clean:
	rm -rf "$(OUTDIR)"
