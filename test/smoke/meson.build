smoke_sources = files(
  'main.cpp',
  'generated/cstr.cpp',
  'generated/example.cpp',
)

smoke_exe = executable(
  'zefc-smoke',
  smoke_sources,
  include_directories : runtime_inc,
  dependencies : [runtime_dep],
  install : false,
)

expected_stdout_file = meson.current_source_dir() / 'expected' / 'example.stdout'

python3 = find_program('python3')

test(
  'smoke example stdout',
  python3,
  args : [
    meson.current_source_dir() / 'check_stdout.py',
    smoke_exe,
    expected_stdout_file,
  ],
  suite : 'smoke',
)
