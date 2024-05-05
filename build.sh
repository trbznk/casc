#!/bin/bash

find . -name '*.c' | xargs wc -l
cc -Wall -Wextra -std=c11 -o maco maco.c
