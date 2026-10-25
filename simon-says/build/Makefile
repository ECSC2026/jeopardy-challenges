
FLAGS = -O2

LDFLAGS = $(FLAGS) -lSDL2 -lSDL2_ttf -lSDL2_mixer -lbearssl
SRCFLAGS = $(FLAGS) -Isrc -MMD -std=c++20

SRC = $(shell find src/ -type f -name "*.cpp")
OBJS = $(patsubst src/%,build/%.o,$(SRC))
OBJS_DEBUG = $(patsubst src/%,build_debug/%.o,$(SRC))

export DECLANG_HOME = $(CURDIR)

main: $(OBJS)
	$(DECLANG_HOME)/.DeClang/compiler/bin/clang++ -o $@ $(OBJS) $(LDFLAGS) -s

main_debug: $(OBJS_DEBUG)
	g++ -o $@ $(OBJS_DEBUG) $(LDFLAGS)

build/%.o: src/%
	mkdir -p $(dir $@)
	$(DECLANG_HOME)/.DeClang/compiler/bin/clang++ $(SRCFLAGS) -c -o $@ $< -fno-rtti

build_debug/%.o: src/%
	mkdir -p $(dir $@)
	g++ $(SRCFLAGS) -c -o $@ $< -DDEBUG

clean:
	rm -rf build
	rm -f main

clean_debug:
	rm -rf build_debug
	rm -f main_debug

%.h:

-include $(OBJS:.o=.d)
-include $(OBJS_DEBUG:.o=.d)
