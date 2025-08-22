
# REQUIREMENTS

pkgconf or pkg-config <br />
vulkan (dynamically linked) <br />
vulkan-devel <br />
SDL3 will be compiled inside. <br />
GLM <br />

# BUILD

```
git submodule init
git submodule update
meson setup build
meson compile -C build
// ! COMPILE THE SHADERS ! (that is the next step!)
// ./build/main to run
```

## shader compile:

now compile the shaders for the binary.
the binary grabs the shaders in the 
directory it's in.

```
glslc ./shaders.d/basic_input.vert -o build/basic.vert.spv
glslc ./shaders.d/basic.frag -o build/basic.frag.spv
```
