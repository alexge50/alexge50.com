COMPILER=emcc
BUILD=./build
FINAL=./final

all: landing-page

.PHONY: all

main.bc: main.cpp
	mkdir -p $(BUILD)
	$(COMPILER) -O2 -c main.cpp -o $(BUILD)/main.bc -std=c++2a -s USE_SDL=2

landing-page: main.bc index.html
	mkdir -p $(FINAL)
	$(COMPILER) -O2 $(BUILD)/main.bc -o $(FINAL)/index.html -s WASM=1 -s USE_SDL=2 --shell-file index.html
	cp -rf fonts final/

    
