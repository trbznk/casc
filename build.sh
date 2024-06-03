#!/bin/bash

BUILD_DIR="./build"
CC=clang
CFLAGS="-Isrc -Wall -Wextra -Werror -std=c11 -g"
RAYFLAGS="-Ithirdparty/raylib -framework CoreVideo -framework IOKit -framework Cocoa -framework GLUT -framework OpenGL"

rm -rf $BUILD_DIR
mkdir -p $BUILD_DIR

set -xe

$CC $CFLAGS $RAYFLAGS -o $BUILD_DIR/casc ./thirdparty/raylib/libraylib.a ./src/*.c
