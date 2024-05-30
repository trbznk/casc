#!/bin/bash

CC=clang
CFLAGS="-Isrc_vk -Wall -Wextra -Werror -std=c11 -g"
OUTPUT="./vk"

VULKAN_SDK="/Users/alex/thirdparty/VulkanSDK/1.3.283.0/macOS"
VKFLAGS="-I$VULKAN_SDK/include -L$VULKAN_SDK/lib -lvulkan"

GLFWFLAGS="-lglfw"

$CC $CFLAGS $GLFWFLAGS $VKFLAGS -o $OUTPUT ./src_vk/vk.c

# install_name_tool -add_rpath ${VULKAN_SDK}/lib $OUTPUT
