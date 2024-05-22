#!/bin/bash

CC=clang
CFLAGS="-Isrc -Wall -Wextra -Werror -std=c11"

$CC $CFLAGS -o casc ./src/casc.c
