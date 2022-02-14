COMPILER=$(EMSCRIPTEN_PATH)/emcc
BUILD=./build
FINAL=./final

all: landing-page

.PHONY: all

main.bc: main.cpp
	mkdir -p $(BUILD)
	$(COMPILER) -O3 -c main.cpp -o $(BUILD)/main.bc -std=c++2a -s USE_SDL=2 -s ASSERTIONS=1 -s ALLOW_MEMORY_GROWTH=1

landing-page: main.bc index.html
	mkdir -p $(FINAL)
	$(COMPILER) -O3 $(BUILD)/main.bc -o $(FINAL)/index.html -s WASM=1 -s USE_SDL=2 --shell-file index.html -s ASSERTIONS=1 -s ALLOW_MEMORY_GROWTH=1
	cp -rf fonts final/

    
