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
  'package1',
  'package2',
  'package3',
  'package4',
  'package5',
  'load1',
  'load4',
  'load5',
  'load6',
  'load7',
  'load8',
  'load9',
  'load10',
]

smoke_error_cases = [
  'nocons',
  'test7b',
  'test10',
  'load2',
  'load3',
]

smoke_sources = files(
  'main.cpp',
  'runtime_init.cpp',
  'generated/cstr.cpp',
  'generated/int.cpp',
  'generated/array.cpp',
  'generated/loadable_modules.cpp',
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
  'generated/case_package1.cpp',
  'generated/case_package2.cpp',
  'generated/case_package3.cpp',
  'generated/case_package4.cpp',
  'generated/case_package5.cpp',
  'generated/case_load1.cpp',
  'generated/case_load2.cpp',
  'generated/case_load3.cpp',
  'generated/case_load4.cpp',
  'generated/case_load5.cpp',
  'generated/case_load6.cpp',
  'generated/case_load7.cpp',
  'generated/case_load8.cpp',
  'generated/case_load9.cpp',
  'generated/case_load10.cpp',
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
