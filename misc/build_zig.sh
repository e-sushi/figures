#!/bin/bash
#_____________________________________________________________________________________________________
#                                      Includes/Sources/Libs
#_____________________________________________________________________________________________________
misc_folder="$(dirname $(realpath "$0"))"
root_folder="$misc_folder/.."
glfw_folder="C:/src/glfw-3.3.2.bin.WIN64"
vulkan_folder="$VULKAN_SDK"
INCLUDES="-I$root_folder/src -I$root_folder/deshi/src -I$root_folder/deshi/src/external -I$glfw_folder/include -I$vulkan_folder/include"
SOURCES="$root_folder/deshi/src/deshi.cpp $root_folder/src/suugu.cpp"
LIBS="-L$glfw_folder/lib-vc2019 -L$vulkan_folder/lib -lc++ -lgdi32 -lshell32 -ldwmapi -lglfw3 -lopengl32 -lvulkan-1 -lshaderc"
#_____________________________________________________________________________________________________
#                                     Compiler and Linker Flags
#_____________________________________________________________________________________________________
WARNINGS="-Wno-unused-value -Wno-implicitly-unsigned-literal -Wno-nonportable-include-path -Wno-writable-strings -Wno-unused-function -Wno-unused-variable"
COMPILE_FLAGS="-std=c++17 -fexceptions -fcxx-exceptions -finline-functions -pipe $WARNINGS"
LINK_FLAGS="-mno-incremental-linker-compatible"
OUT_EXE="suugu.exe"
#_____________________________________________________________________________________________________
#                                           Platform
#_____________________________________________________________________________________________________
FLAGS_DEBUG="-DBUILD_INTERNAL=1 -DBUILD_SLOW=1 -DBUILD_RELEASE=0"
FLAGS_RELEASE="-DBUILD_INTERNAL=0 -DBUILD_SLOW=0 -DBUILD_RELEASE=1"
FLAGS_OS="-DDESHI_WINDOWS=1 -DDESHI_MAC=0 -DDESHI_LINUX=0"
FLAGS_RENDERER="-DDESHI_VULKAN=0 -DDESHI_OPENGL=1 -DDESHI_DIRECTX12=0"
#_____________________________________________________________________________________________________
#                             DEBUG (compiles without optimization)
#_____________________________________________________________________________________________________
if [ "$1" == "" ] || [ "$1" == "-d" ] || [ "$1" == "/d" ]; then
  OUT_DIR="$root_folder/build/debug"
  [ ! -e $OUT_DIR ] && mkdir $OUT_DIR
  zig.exe c++ -g -gcodeview -O0 $COMPILE_FLAGS $FLAGS_DEBUG $FLAGS_OS $FLAGS_RENDERER $INCLUDES $SOURCES -o$OUT_DIR/$OUT_EXE $LINK_FLAGS $LIBS
#_____________________________________________________________________________________________________
#                                RELEASE (compiles with optimization)
#_____________________________________________________________________________________________________
elif [ "$1" == "-r" ] || [ "$1" == "/r" ]; then
  OUT_DIR="$root_folder/build/release"
  [ ! -e $OUT_DIR ] && mkdir $OUT_DIR
  zig.exe c++ -O2 -Wall $COMPILE_FLAGS $FLAGS_RELEASE $FLAGS_OS $FLAGS_RENDERER $INCLUDES $SOURCES -o$OUT_DIR/$OUT_EXE $LINK_FLAGS $LIBS
#_____________________________________________________________________________________________________
else
  ECHO "Unknown argument $1"
fi
if [ -e "$OUT_DIR/$OUT_EXE" ]; then
  readlink -f "$OUT_DIR/$OUT_EXE"
fi