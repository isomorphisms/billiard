NDK_ROOT ?= $(ANDROID_NDK_HOME)
ANDROID_API ?= 23
ANDROID_ABI ?= arm64-v8a
ROUGH_SRC ?= ../_rough_framebuffer/src
ifeq ($(ANDROID_ABI),armeabi-v7a)
  TARGET := armv7a-linux-androideabi
  ALIGNMENT :=
else ifeq ($(ANDROID_ABI),arm64-v8a)
  TARGET := aarch64-linux-android
  ALIGNMENT := -Wl,-z,max-page-size=16384 -Wl,-z,common-page-size=16384
else ifeq ($(ANDROID_ABI),x86_64)
  TARGET := x86_64-linux-android
  ALIGNMENT := -Wl,-z,max-page-size=16384 -Wl,-z,common-page-size=16384
else
  $(error Unsupported ABI: $(ANDROID_ABI))
endif
BIN := $(NDK_ROOT)/toolchains/llvm/prebuilt/linux-x86_64/bin
CC := $(BIN)/$(TARGET)$(ANDROID_API)-clang
GLUE := $(NDK_ROOT)/sources/android/native_app_glue
OUT := ../build/android/$(ANDROID_ABI)/libbilliard.so

.PHONY: all clean
all: $(OUT)

$(OUT): ../android/native/billiard_android.c ../src/billiard.c ../src/scene.c ../src/billiard.h ../src/scene.h $(ROUGH_SRC)/framebuffer.c $(ROUGH_SRC)/framebuffer.h
	@test -x "$(CC)" || { echo "Missing pinned NDK compiler $(CC)" >&2; exit 1; }
	@test -f "$(GLUE)/android_native_app_glue.c" || { echo "Missing NDK native app glue" >&2; exit 1; }
	@mkdir -p ../build/android/$(ANDROID_ABI)
	$(CC) -std=c11 -O2 -fPIC -shared -Wall -Wextra -Werror -Wno-unused-parameter \
	    -isystem $(GLUE) -I../src -I$(ROUGH_SRC) \
	    ../android/native/billiard_android.c ../src/billiard.c ../src/scene.c \
	    $(ROUGH_SRC)/framebuffer.c $(GLUE)/android_native_app_glue.c \
	    -Wl,--no-undefined -Wl,-soname,libbilliard.so $(ALIGNMENT) \
	    -landroid -llog -lm -o $(OUT)

clean:
	rm -rf ../build/android/$(ANDROID_ABI)
