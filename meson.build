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

#add_project_link_arguments([
#  '-lvulkan',
#  '-lSDL3'
#], language: 'cpp')

#add_project_arguments('-g', language: 'cpp')
executable('main', src,
           dependencies: [ fmt_dep, sdl_dep, vulkan_dep, vma_dep, glm_dep ])
