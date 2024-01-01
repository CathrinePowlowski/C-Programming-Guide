#!/bin/bash

SourceFiles="../../main.c ../../glfw/src/context.c ../../glfw/src/egl_context.c ../../glfw/src/glx_context.c ../../glfw/src/init.c ../../glfw/src/input.c ../../glfw/src/linux_joystick.c ../../glfw/src/monitor.c ../../glfw/src/osmesa_context.c ../../glfw/src/posix_thread.c ../../glfw/src/posix_time.c ../../glfw/src/vulkan.c ../../glfw/src/window.c ../../glfw/src/x11_init.c ../../glfw/src/x11_monitor.c ../../glfw/src/x11_window.c ../../glfw/src/xkb_unicode.c ../../glad/src/glad.c"
IncludeDirs="-I../../glfw/include/ -I../../glad/include/"

CompilerFlags=-O
if [ "$1" == "optimize" ]; then
	CompilerFlags=-O2
	echo --------------------------------------------------
	echo Compiling with Optimizations
fi

mkdir -p bin

echo --------------------------------------------------
which clang &> /dev/null
if [ $? == 0 ]; then
	mkdir -p bin/CLANG
	cp Logo.bmp bin/CLANG/ &> /dev/null
	pushd bin/CLANG &> /dev/null
	echo Compiling with CLANG...
	clang $CompilerFlags $IncludeDirs -Wno-switch -Wno-pointer-sign -D_GLFW_X11 $SourceFiles -o Mandelbrot-GPU.out -ldl -lGL -lpthread -lm
	popd &> /dev/null
	echo Compiling with CLANG finished.
  cp -t ./bin/CLANG/ *.vert *.frag
else
	echo Clang compiler not detected. Skipping compiling with Clang.
fi
echo --------------------------------------------------

which gcc &> /dev/null
if [ $? == 0 ]; then
	mkdir -p bin/GCC
	cp Logo.bmp bin/GCC/ &> /dev/null
	pushd bin/GCC &> /dev/null
	echo Compiling with GCC...
	gcc $CompilerFlags $IncludeDirs -g -Wno-switch -Wno-pointer-sign -Wno-unused-result -D_GLFW_X11 $SourceFiles -o Mandelbrot-GPU.out -ldl -lGL -lpthread -lm
	popd &> /dev/null
	echo Compiling with GCC finished.
  cp -t ./bin/GCC/ *.vert *.frag
else
	echo Gcc compiler not detected. Skipping compiling with Gcc.
fi
echo --------------------------------------------------
