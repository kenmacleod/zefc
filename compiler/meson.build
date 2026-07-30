# ZefC transpiler: .zef → C++ (uses ZEFC_* macros; object_dispatch is a C++ compile flag).

compiler_inc = include_directories('.')

zefc_exe = executable(
  'zefc',
  files(
    'main.cpp',
    'lexer.cpp',
    'parse.cpp',
    'codegen.cpp',
  ),
  include_directories : compiler_inc,
  install : false,
)

python3 = find_program('python3')

hello_zef = meson.project_source_root() / 'test/smoke/zef/hello.zef'
hello_gen = custom_target(
  'hello_from_zef',
  input : hello_zef,
  output : 'hello.gen.cpp',
  command : [zefc_exe, '@INPUT@', '-o', '@OUTPUT@'],
)

# Stdlib bootstrap currently under smoke (shared until folded into runtime/).
stdlib_sources = files(
  '../test/smoke/runtime_init.cpp',
  '../test/smoke/generated/cstr.cpp',
  '../test/smoke/generated/int.cpp',
  '../test/smoke/generated/double.cpp',
  '../test/smoke/generated/array.cpp',
)

hello_exe = executable(
  'zefc-hello',
  [hello_gen] + stdlib_sources,
  dependencies : [runtime_dep],
  include_directories : runtime_inc,
  cpp_args : zefc_cpp_args,
  install : false,
)

test(
  'compiler hello stdout',
  python3,
  args : [
    meson.project_source_root() / 'test/compiler/check_stdout.py',
    hello_exe,
    meson.project_source_root() / 'test/smoke/expected/hello.stdout',
  ],
  suite : 'compiler',
)
