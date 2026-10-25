# VulnVM Makefile
CC      ?= gcc
CFLAGS  ?= -std=c11 -Wall -Wextra -Wshadow -O2 -Iinclude
LDFLAGS ?= -lm -Wl,-z,lazy

# UBSan build — the reliable sanitizer gate. UBSan uses no shadow memory, so it
# runs correctly here (this host's max ASLR entropy, vm.mmap_rnd_bits=32, makes
# ASan's shadow mapping crash even a trivial program, and we lack privileges to
# lower it). UBSan validates exactly the risky parts of this code: the packed
# heap-header/payload unaligned accesses and the allocator's integer arithmetic.
UBSAN_CFLAGS := -std=c11 -Wall -Wextra -Wshadow -g -O0 -Iinclude \
                -fsanitize=undefined -fno-sanitize-recover=all -fno-omit-frame-pointer

# ASan build — kept for hosts with lower ASLR entropy (vm.mmap_rnd_bits <= 28).
# NOTE: unreliable on high-ASLR kernels; not the CI gate here. Since the VM uses a
# custom arena (not libc malloc), intra-arena overflow detection comes from
# alloc_check_heap + canary stress tests, not ASan redzones.
ASAN_CFLAGS := -std=c11 -Wall -Wextra -Wshadow -g -O0 -Iinclude \
               -fsanitize=address,undefined -fno-omit-frame-pointer

SRC_DIR   := src
BUILD_DIR := build
TEST_DIR  := tests

# --- source groups ---------------------------------------------------------
# Shared by everything (no compiler, no VM): allocator, values, objects, chunk,
# disassembler and the .vmb (de)serializer.
CORE_SRCS := $(SRC_DIR)/alloc.c $(SRC_DIR)/value.c $(SRC_DIR)/object.c \
             $(SRC_DIR)/chunk.c $(SRC_DIR)/disasm.c $(SRC_DIR)/serialize.c \
             $(SRC_DIR)/debug.c

# The compiler front-end (lexer/parser/AST -> bytecode).
COMPILER_SRCS := $(SRC_DIR)/lexer.c $(SRC_DIR)/ast.c $(SRC_DIR)/parser.c \
                 $(SRC_DIR)/compiler.c

# The execution engine. vm_source.c (vm_run_source) is the only VM unit that
# needs the compiler, so it is kept separate: the interpreter binary omits it.
VM_SRCS       := $(SRC_DIR)/vm.c $(SRC_DIR)/builtins.c
VMSOURCE_SRCS := $(SRC_DIR)/vm_source.c

# Full library (used by tests): everything except the precompiled prelude blob.
LIB_SRCS := $(CORE_SRCS) $(COMPILER_SRCS) $(VM_SRCS) $(VMSOURCE_SRCS)

# The two shipped binaries: `vulnvmc` (compiler, no VM) and `vulnvm`
# (interpreter, no compiler).
COMPILER_BIN_SRCS := $(SRC_DIR)/main_compile.c $(CORE_SRCS) $(COMPILER_SRCS)
RUNTIME_BIN_SRCS  := $(SRC_DIR)/main_run.c $(CORE_SRCS) $(VM_SRCS)

# Precompiled runtime prelude (built by vulnvmc, embedded into the interpreter).
PRELUDE_MC   := runtime/prelude.mc
PRELUDE_VMB  := $(BUILD_DIR)/prelude.vmb
PRELUDE_BLOB := $(BUILD_DIR)/prelude_blob.c

# Test programs: tests/test_*.c -> build/test_*
TEST_SRCS := $(wildcard $(TEST_DIR)/test_*.c)
TEST_BINS := $(patsubst $(TEST_DIR)/%.c,$(BUILD_DIR)/%,$(TEST_SRCS))

.PHONY: all test test-ubsan test-asan examples clean

all: $(BUILD_DIR)/vulnvmc $(BUILD_DIR)/vulnvm

# --- compiler binary (no VM linked) ----------------------------------------
$(BUILD_DIR)/vulnvmc: $(COMPILER_BIN_SRCS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(COMPILER_BIN_SRCS) -o $@ $(LDFLAGS)

# --- precompiled prelude blob (bootstrap: needs vulnvmc first) -------------
$(PRELUDE_VMB): $(BUILD_DIR)/vulnvmc $(PRELUDE_MC) | $(BUILD_DIR)
	$(BUILD_DIR)/vulnvmc $(PRELUDE_MC) -o $@

$(PRELUDE_BLOB): $(PRELUDE_VMB) tools/bin2c.sh
	sh tools/bin2c.sh vulnvm_prelude_blob < $(PRELUDE_VMB) > $@

# --- interpreter binary (no compiler linked) -------------------------------
$(BUILD_DIR)/vulnvm: $(RUNTIME_BIN_SRCS) $(PRELUDE_BLOB) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(RUNTIME_BIN_SRCS) $(PRELUDE_BLOB) -o $@ $(LDFLAGS)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# End-to-end: compile every examples/*.mc with vulnvmc, run the .vmb, and diff.
examples: $(BUILD_DIR)/vulnvmc $(BUILD_DIR)/vulnvm
	@sh $(TEST_DIR)/run_examples.sh $(BUILD_DIR)/vulnvmc $(BUILD_DIR)/vulnvm

# Each test binary links the full library plus the precompiled prelude blob.
$(BUILD_DIR)/test_%: $(TEST_DIR)/test_%.c $(LIB_SRCS) $(PRELUDE_BLOB) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $< $(LIB_SRCS) $(PRELUDE_BLOB) -o $@ $(LDFLAGS)

# Build + run every test binary.
test: $(TEST_BINS)
	@fail=0; for t in $(TEST_BINS); do \
		echo "== $$t =="; \
		$$t || fail=1; \
	done; \
	if [ $$fail -eq 0 ]; then echo "ALL TESTS PASSED"; else echo "TESTS FAILED"; exit 1; fi

# Reliable sanitizer gate: all tests under UBSan.
test-ubsan: $(PRELUDE_BLOB) | $(BUILD_DIR)
	@fail=0; for t in $(TEST_SRCS); do \
		bin=$(BUILD_DIR)/ubsan_$$(basename $$t .c); \
		$(CC) $(UBSAN_CFLAGS) $$t $(LIB_SRCS) $(PRELUDE_BLOB) -o $$bin $(LDFLAGS) || exit 1; \
		echo "== $$bin =="; \
		$$bin || fail=1; \
	done; \
	if [ $$fail -eq 0 ]; then echo "ALL UBSAN TESTS PASSED"; else echo "UBSAN TESTS FAILED"; exit 1; fi

# Optional ASan gate (needs a low-ASLR host; see ASAN_CFLAGS note above).
test-asan: $(PRELUDE_BLOB) | $(BUILD_DIR)
	@fail=0; for t in $(TEST_SRCS); do \
		bin=$(BUILD_DIR)/asan_$$(basename $$t .c); \
		$(CC) $(ASAN_CFLAGS) $$t $(LIB_SRCS) $(PRELUDE_BLOB) -o $$bin $(LDFLAGS) || exit 1; \
		echo "== $$bin =="; \
		$$bin || fail=1; \
	done; \
	if [ $$fail -eq 0 ]; then echo "ALL ASAN TESTS PASSED"; else echo "ASAN TESTS FAILED"; exit 1; fi

clean:
	rm -rf $(BUILD_DIR)
