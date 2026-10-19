# Product C compilation belongs exclusively to pinned, source-built ICK.
# The ICK driver is GCC-derived and named gcc; it is NOT system GCC.
ICK_CC ?=
ICK_SOURCE_DIR ?=
ICK_LINK_FLAGS ?= -fno-link-libatomic
ICK_CFLAGS ?= -O2 -std=c11 -Wall -Wextra -Werror -pedantic -ffp-contract=off -fno-fast-math
LDLIBS = -lm
EXPERIMENT_SOURCES = src/experiments.c src/cylinders.c src/flow.c src/periods.c src/affine.c src/surface.c src/polygon.c src/billiard.c
EXPERIMENT_HEADERS = $(wildcard src/*.h)
SOURCE_REVISION := $(shell git rev-parse HEAD)
SOURCE_STATE := $(if $(shell git status --porcelain),working_tree,commit)
ICK_SOURCE_SHA = c61e448251744a2f40ad743ebef1a027bdcd2f9d
ICK_REFERENCE_SHA = 6294f1d9e7536e5ffcde09d1528c918d63abfef5

.PHONY: test scene clean check-ick
check-ick:
	@test -n "$(ICK_CC)" && test -n "$(ICK_SOURCE_DIR)" || { echo "BLOCKED: pinned ICK_CC and ICK_SOURCE_DIR required" >&2; exit 3; }
	@test -x "$(ICK_CC)" && test -d "$(ICK_SOURCE_DIR)" || { echo "BLOCKED: owned ICK executable/source missing" >&2; exit 3; }
	@test "$$(git -C "$(ICK_SOURCE_DIR)" rev-parse HEAD)" = "$(ICK_SOURCE_SHA)" || { echo "BLOCKED: ICK revision mismatch" >&2; exit 3; }
	@test "$$(git -C "$(ICK_SOURCE_DIR)/gcc" rev-parse HEAD)" = "$(ICK_REFERENCE_SHA)" || { echo "BLOCKED: ICK reference revision mismatch" >&2; exit 3; }
	@case "$(ICK_CC)" in */ick-host-stage/bin/gcc) ;; *) echo "BLOCKED: compiler must be source-qualified ICK stage, never stock gcc/clang" >&2; exit 3 ;; esac
	@test "$$("$(ICK_CC)" -dumpmachine)" = x86_64-pc-linux-gnu || { echo "BLOCKED: wrong ICK host target" >&2; exit 3; }
	@cc1="$$("$(ICK_CC)" -print-prog-name=cc1)"; test -x "$$cc1" || { echo "BLOCKED: ICK cc1 missing" >&2; exit 3; }

build/test_billiard: src/billiard.c src/billiard.h src/pixel_sample.h tests/test_billiard.c check-ick
	@mkdir -p build
	"$(ICK_CC)" $(ICK_LINK_FLAGS) $(ICK_CFLAGS) -Isrc src/billiard.c tests/test_billiard.c -o $@ $(LDLIBS)

build/test_polygon: src/polygon.c src/polygon.h src/billiard.c src/billiard.h tests/test_polygon.c check-ick
	@mkdir -p build
	"$(ICK_CC)" $(ICK_LINK_FLAGS) $(ICK_CFLAGS) -Isrc src/polygon.c src/billiard.c tests/test_polygon.c -o $@ $(LDLIBS)

build/test_surface: src/surface.c src/surface.h src/exact_geometry.h src/polygon.c src/polygon.h src/billiard.c tests/test_surface.c check-ick
	@mkdir -p build
	"$(ICK_CC)" $(ICK_LINK_FLAGS) $(ICK_CFLAGS) -Isrc src/surface.c src/polygon.c src/billiard.c tests/test_surface.c -o $@ $(LDLIBS)

build/test_flow: src/flow.c src/flow.h src/surface.c src/surface.h src/exact_geometry.h src/polygon.c src/polygon.h src/billiard.c tests/test_flow.c check-ick
	@mkdir -p build
	"$(ICK_CC)" $(ICK_LINK_FLAGS) $(ICK_CFLAGS) -Isrc src/flow.c src/surface.c src/polygon.c src/billiard.c tests/test_flow.c -o $@ $(LDLIBS)

build/test_cylinders: src/cylinders.c src/cylinders.h src/flow.c src/flow.h src/surface.c src/surface.h src/exact_geometry.h src/polygon.c src/polygon.h src/billiard.c tests/test_cylinders.c check-ick
	@mkdir -p build
	"$(ICK_CC)" $(ICK_LINK_FLAGS) $(ICK_CFLAGS) -Isrc src/cylinders.c src/flow.c src/surface.c src/polygon.c src/billiard.c tests/test_cylinders.c -o $@ $(LDLIBS)

build/test_periods: src/periods.c src/periods.h src/affine.c src/affine.h src/surface.c src/surface.h src/exact_geometry.h src/polygon.c src/polygon.h src/billiard.c tests/test_periods.c check-ick
	@mkdir -p build
	"$(ICK_CC)" $(ICK_LINK_FLAGS) $(ICK_CFLAGS) -Isrc src/periods.c src/affine.c src/surface.c src/polygon.c src/billiard.c tests/test_periods.c -o $@ $(LDLIBS)

build/test_experiments: $(EXPERIMENT_SOURCES) $(EXPERIMENT_HEADERS) tests/test_experiments.c check-ick
	@mkdir -p build
	"$(ICK_CC)" $(ICK_LINK_FLAGS) $(ICK_CFLAGS) -Isrc $(EXPERIMENT_SOURCES) tests/test_experiments.c -o $@ $(LDLIBS)

build/billiard-experiments: $(EXPERIMENT_SOURCES) $(EXPERIMENT_HEADERS) src/experiments_cli.c check-ick
	@mkdir -p build
	"$(ICK_CC)" $(ICK_LINK_FLAGS) $(ICK_CFLAGS) -DBILLIARD_SOURCE_REVISION=\"$(SOURCE_REVISION)\" -DBILLIARD_SOURCE_STATE=\"$(SOURCE_STATE)\" -Isrc $(EXPERIMENT_SOURCES) src/experiments_cli.c -o $@ $(LDLIBS)

.PHONY: experiments
experiments: build/billiard-experiments
	./build/billiard-experiments > build/bil-orbit.tsv
	sha256sum "$(ICK_CC)" "$$("$(ICK_CC)" -print-prog-name=cc1)" build/billiard-experiments build/bil-orbit.tsv > build/experiment-producer.sha256
	printf 'source_revision\t%s\nsource_state\t%s\ncompiler_source\t%s\ncompiler_reference\t%s\n' "$(SOURCE_REVISION)" "$(SOURCE_STATE)" "$(ICK_SOURCE_SHA)" "$(ICK_REFERENCE_SHA)" > build/experiment-producer.tsv

test: build/test_billiard build/test_polygon build/test_surface build/test_flow build/test_cylinders build/test_periods build/test_experiments
	./build/test_billiard
	./build/test_polygon
	./build/test_surface
	./build/test_flow
	./build/test_cylinders
	./build/test_periods
	./build/test_experiments

build/scene_ops: src/billiard.c src/scene.c src/scene_ops.c src/billiard.h src/scene.h check-ick
	@mkdir -p build
	"$(ICK_CC)" $(ICK_LINK_FLAGS) $(ICK_CFLAGS) -Isrc src/billiard.c src/scene.c src/scene_ops.c -o $@ $(LDLIBS)

scene: build/scene_ops
	./build/scene_ops > build/billiard.ops

clean:
	rm -rf build

# This fixed Make interface packages the already qualified compiler for local
# campaign verification. Stripping removes debug sections, not code.
.PHONY: export-ick-stage
export-ick-stage: check-ick
	mkdir -p build/ick-host-stage
	cp -a "$(ICK_EXPORT_STAGE)/." build/ick-host-stage/
	sha256sum build/ick-host-stage/bin/gcc build/ick-host-stage/libexec/gcc/x86_64-pc-linux-gnu/17.0.0/cc1 > build/ick-host-stage/export-original.sha256
	strip --strip-debug build/ick-host-stage/libexec/gcc/x86_64-pc-linux-gnu/17.0.0/cc1
	if test ! -f build/ick-host-stage/libexec/gcc/x86_64-pc-linux-gnu/17.0.0/lto1; then :; else strip --strip-debug build/ick-host-stage/libexec/gcc/x86_64-pc-linux-gnu/17.0.0/lto1; fi
	sha256sum build/ick-host-stage/bin/gcc build/ick-host-stage/libexec/gcc/x86_64-pc-linux-gnu/17.0.0/cc1 > build/ick-host-stage/export-stripped.sha256
	$(MAKE) test scene ICK_CC="$(abspath build/ick-host-stage/bin/gcc)"
	tar -C build -czf "$(abspath build/ick-host-stage.tar.gz)" ick-host-stage
