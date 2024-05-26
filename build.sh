#!/bin/bash

CC=clang
CFLAGS="-Isrc -Wall -Wextra -Werror -std=c11 -g"
RAYFLAGS="-Ithirdparty/raylib -framework CoreVideo -framework IOKit -framework Cocoa -framework GLUT -framework OpenGL"
# -Lthirdparty/raylib

$CC $CFLAGS $RAYFLAGS -o casc ./thirdparty/raylib/libraylib.a ./src/*.c