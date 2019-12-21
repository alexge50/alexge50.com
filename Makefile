COMPILER=$(EMSCRIPTEN_PATH)/emcc
BUILD=./build
FINAL=./final

all: landing-page

.PHONY: all

main.bc: main.cpp
	mkdir -p $(BUILD)
	$(COMPILER) -O2 -c main.cpp -o $(BUILD)/main.bc

landing-page: main.bc index.html
	mkdir -p $(FINAL)
	$(COMPILER) -O2 $(BUILD)/main.bc -o $(FINAL)/index.html -s WASM=1 --shell-file index.html

    
