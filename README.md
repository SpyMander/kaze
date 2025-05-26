
# REQUIREMENTS

pkgconf or pkg-config <br />
vulkan (dynamically linked) <br />
vulkan-devel <br />
SDL3 will be compiled inside. <br />

# BUILD

```
mkdir subprojects
meson wrap install sdl3
meson setup build
meson compile -C build
and ./build/main to run
```

## shader compile:

now compile the shaders for the binary.
the binary grabs the shaders in the 
directory it's in.

```
glslc basic.vert -o build/basic.vert.spv
glslc basic.frag -o build/basic.frag.spv
```
