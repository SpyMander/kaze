project('kaze', 'cpp',
  default_options: ['default_library=static', 'cpp_std=c++20']
)

src = [
  'main.cpp',
  'vulkan_init.cpp',
  'vulkan_validation_layers.cpp',
  'shaders.cpp',
  'error_handling.cpp',
]

vulkan_dep = dependency('vulkan', required: true)
sdl_dep = dependency('sdl3', required: true)

add_project_link_arguments([
  '-lvulkan',
  '-lSDL3'
], language: 'cpp')

add_project_arguments('-g', language: 'cpp')
executable('main', src,
           dependencies: [ sdl_dep, vulkan_dep ])

