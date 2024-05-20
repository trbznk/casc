#!/bin/bash

rm -rf ./build
mkdir -p ./build

CFLAGS="-Isrc -Wall -Wextra -Werror -std=c11"

cc $CFLAGS -c -o ./build/casc.o ./src/casc.c
ar rcs ./build/libcasc.a ./build/*.o

cc $CFLAGS -Lbuild -lcasc -o main main.c
