CPP = g++

all: prep rv32ima

prep:
	mkdir -p build

rv32ima: src/*.cpp
	$(CPP) $^ -Ofast -o build/$@