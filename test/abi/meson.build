# Runtime / dispatch ABI checks that are not Zef source programs.

python3 = find_program('python3')

patch1_exe = executable(
  'zefc-abi-patch1',
  files('patch1.cpp') + [
    meson.project_source_root() / 'test/smoke/runtime_init.cpp',
    meson.project_source_root() / 'test/smoke/generated/cstr.cpp',
    meson.project_source_root() / 'test/smoke/generated/int.cpp',
    meson.project_source_root() / 'test/smoke/generated/double.cpp',
    meson.project_source_root() / 'test/smoke/generated/array.cpp',
    meson.project_source_root() / 'test/smoke/generated/loadable_modules.cpp',
  ],
  include_directories : [runtime_inc, include_directories('../smoke')],
  dependencies : [runtime_dep],
  cpp_args : zefc_cpp_args,
  install : false,
)

test(
  'abi patch1 stdout',
  python3,
  args : [
    meson.project_source_root() / 'test/compiler/check_stdout.py',
    patch1_exe,
    meson.current_source_dir() / 'expected' / 'patch1.stdout',
  ],
  suite : 'abi',
)
