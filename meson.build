project('kaze', 'cpp',
  default_options: ['default_library=static', 'cpp_std=c++20']
)

src = [
  #spirv reflection
  'gitsubmodules.d/SPIRV-Reflect/spirv_reflect.cpp',
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
#it's a git submodule.
#spirv_reflect_dep = dependency('spirv-reflect', required: true)

executable('main', src,
           dependencies: [fmt_dep,
	   sdl_dep, vulkan_dep, vma_dep, glm_dep ])
