
# REQUIREMENTS

vulkan (dynamically linked)
SDL3 (dynamically linked)

# BUILD

```
meson configure build
meson compile -C build
and ./build/main to run
```

you also need the shaders to be in the 
build directory.

## shader compile:

now compile the shaders for the binary.
the binary grabs the shaders in the 
directory it's in.

```
glslc basic.vert -o build/basic.vert.spv
glslc basic.frag -o build/basic.frag.spv
```
