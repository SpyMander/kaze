
files := main.cpp vulkan_init.cpp vulkan_validation_layers.cpp shaders.cpp \
error_handling.cpp

libs := -lvulkan -lSDL3

releaseFlags := -O3 -funroll-loops -ftree-vectorize -flto -fno-exceptions
# -march=native

debugFlags := -gfull -O0 -fno-omit-frame-pointer -fsanitize=undefined
#-fsanitize=address sdl has memory leaks lol.

ALL:
	glslc basic.frag -o basic.frag.spv
	glslc basic.vert -o basic.vert.spv
	clang++ ${files} -o bin/bin ${libs} -std=c++20 ${debugFlags}

