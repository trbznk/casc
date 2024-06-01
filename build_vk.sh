#!/bin/bash

BUILD_DIR="./build"
CC=clang
CFLAGS="-Isrc_vk -Wall -Wextra -Werror -std=c11 -g"
OUTPUT="$BUILD_DIR/vk"
VULKAN_SDK="/Users/alex/thirdparty/VulkanSDK/1.3.283.0/macOS"
VKFLAGS="-I$VULKAN_SDK/include -L$VULKAN_SDK/lib -lvulkan"
GLFWFLAGS="-lglfw"

rm -rf $BUILD_DIR
mkdir -p $BUILD_DIR
set -xe

# compile shaders

$VULKAN_SDK/bin/glslc shaders/shader.vert -o ./shaders/vert.spv
$VULKAN_SDK/bin/glslc shaders/shader.frag -o ./shaders/frag.spv

# compile program

$CC $CFLAGS $GLFWFLAGS $VKFLAGS -o $OUTPUT ./src_vk/vk.c
