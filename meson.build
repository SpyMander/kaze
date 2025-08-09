project('kaze', 'cpp',
  default_options: ['default_library=static', 'cpp_std=c++20']
)

src = [
  'main.cpp',
  'vulkan_init.cpp',
  'vulkan_validation_layers.cpp',
  'shaders.cpp',
  'error_handling.cpp',
  'logging.cpp',
  'vulkan_memory.cpp',
]

fmt_dep = dependency('fmt', required: true)
glm_dep = dependency('glm', required: true, fallback: ['glm'])
vulkan_dep = dependency('vulkan', required: true)
vma_dep = dependency('vulkan-memory-allocator', required: true)
sdl_dep = dependency('sdl3', required: true)

# shitty ass spirv-cross, old ass cmake
cmake = import('cmake')

# Configure CMake subproject
spirv_cross_proj = cmake.subproject('spirv-cross', options: cmake.subproject_options())

# Get the dependency
spirv_cross_dep = spirv_cross_proj.dependency('spirv-cross-core')

executable('main', src,
           dependencies: [ spirv_cross_dep, fmt_dep,
	   sdl_dep, vulkan_dep, vma_dep, glm_dep ])
