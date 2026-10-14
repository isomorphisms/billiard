CC ?= cc
CFLAGS ?= -O2 -std=c11 -Wall -Wextra -Werror -pedantic
LDLIBS = -lm

.PHONY: test clean
build/test_billiard: src/billiard.c src/billiard.h tests/test_billiard.c
	@mkdir -p build
	$(CC) $(CFLAGS) -Isrc src/billiard.c tests/test_billiard.c -o $@ $(LDLIBS)

test: build/test_billiard
	./build/test_billiard

clean:
	rm -rf build

build/scene_ops: src/billiard.c src/scene.c src/scene_ops.c src/billiard.h src/scene.h
	@mkdir -p build
	$(CC) $(CFLAGS) -Isrc src/billiard.c src/scene.c src/scene_ops.c -o $@ $(LDLIBS)

.PHONY: scene
scene: build/scene_ops
	./build/scene_ops > build/billiard.ops
