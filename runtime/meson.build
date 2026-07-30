runtime_sources = files(
  'src/runtime.cpp',
  'src/error.cpp',
  'src/module.cpp',
  'src/known_selectors.cpp',
  'src/field_ic.cpp',
  'src/call_ic.cpp',
)

runtime_lib = static_library(
  'zefc_runtime',
  runtime_sources,
  include_directories : runtime_inc,
  install : false,
)

runtime_dep = declare_dependency(
  link_with : runtime_lib,
  include_directories : runtime_inc,
)
