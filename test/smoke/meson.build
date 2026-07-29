smoke_cases = [
  'hello',
  'test',
  'precedence',
  'test5',
  'test4',
  'test3',
  'test2',
  'super',
  'staticcall',
  'test6',
  'test8',
  'test9',
  'staticcall2',
]

smoke_error_cases = [
  'nocons',
  'test7b',
  'test10',
]

smoke_sources = files(
  'main.cpp',
  'runtime_init.cpp',
  'generated/cstr.cpp',
  'generated/int.cpp',
  'generated/array.cpp',
  'generated/case_hello.cpp',
  'generated/case_test.cpp',
  'generated/case_precedence.cpp',
  'generated/case_test5.cpp',
  'generated/case_test4.cpp',
  'generated/case_test3.cpp',
  'generated/case_test2.cpp',
  'generated/case_super.cpp',
  'generated/case_staticcall.cpp',
  'generated/case_test6.cpp',
  'generated/case_test8.cpp',
  'generated/case_test9.cpp',
  'generated/case_staticcall2.cpp',
  'generated/case_nocons.cpp',
  'generated/case_test7b.cpp',
  'generated/case_test10.cpp',
)

smoke_inc = include_directories('.')

smoke_exe = executable(
  'zefc-smoke',
  smoke_sources,
  include_directories : [runtime_inc, smoke_inc],
  dependencies : [runtime_dep],
  install : false,
)

python3 = find_program('python3')

foreach case : smoke_cases
  expected = meson.current_source_dir() / 'expected' / (case + '.stdout')
  test(
    'smoke @0@'.format(case),
    python3,
    args : [
      meson.current_source_dir() / 'check_stdout.py',
      smoke_exe,
      case,
      expected,
    ],
    suite : 'smoke',
  )
endforeach

foreach case : smoke_error_cases
  expected = meson.current_source_dir() / 'expected' / (case + '.stderr')
  test(
    'smoke error @0@'.format(case),
    python3,
    args : [
      meson.current_source_dir() / 'check_error.py',
      smoke_exe,
      case,
      expected,
    ],
    suite : 'smoke',
  )
endforeach
