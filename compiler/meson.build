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

test3_zef = meson.project_source_root() / 'test/smoke/zef/test3.zef'
test3_gen = custom_target(
  'test3_from_zef',
  input : test3_zef,
  output : 'test3.gen.cpp',
  command : [zefc_exe, '@INPUT@', '-o', '@OUTPUT@'],
)

test3_exe = executable(
  'zefc-test3',
  [test3_gen] + stdlib_sources,
  dependencies : [runtime_dep],
  include_directories : runtime_inc,
  cpp_args : zefc_cpp_args,
  install : false,
)

test(
  'compiler test3 stdout',
  python3,
  args : [
    meson.project_source_root() / 'test/compiler/check_stdout.py',
    test3_exe,
    meson.project_source_root() / 'test/smoke/expected/test3.stdout',
  ],
  suite : 'compiler',
)

accessors_zef = meson.project_source_root() / 'test/smoke/zef/accessors.zef'
accessors_gen = custom_target(
  'accessors_from_zef',
  input : accessors_zef,
  output : 'accessors.gen.cpp',
  command : [zefc_exe, '@INPUT@', '-o', '@OUTPUT@'],
)

accessors_exe = executable(
  'zefc-accessors',
  [accessors_gen] + stdlib_sources,
  dependencies : [runtime_dep],
  include_directories : runtime_inc,
  cpp_args : zefc_cpp_args,
  install : false,
)

test(
  'compiler accessors stdout',
  python3,
  args : [
    meson.project_source_root() / 'test/compiler/check_stdout.py',
    accessors_exe,
    meson.project_source_root() / 'test/smoke/expected/accessors.stdout',
  ],
  suite : 'compiler',
)

foreach case_name : ['test30', 'super', 'test4', 'test6', 'test24', 'test25',
                     'test4b', 'test25b', 'hex', 'precedence', 'super2',
                     'test20', 'test5', 'test23', 'test29', 'test27', 'int64',
                     'test14', 'test15', 'test26', 'test2',
                     'test33', 'test19', 'test22', 'test13', 'testb', 'testb2',
                     'test34', 'test36', 'nocons', 'test8',
                     'test35', 'testb3', 'testb4', 'teste', 'staticcall',
                     'accessors2b', 'private1', 'duplicateparam']
  case_zef = meson.project_source_root() / 'test/smoke/zef' / (case_name + '.zef')
  case_gen = custom_target(
    case_name + '_from_zef',
    input : case_zef,
    output : case_name + '.gen.cpp',
    command : [zefc_exe, '@INPUT@', '-o', '@OUTPUT@'],
  )
  case_exe = executable(
    'zefc-' + case_name,
    [case_gen] + stdlib_sources,
    dependencies : [runtime_dep],
    include_directories : runtime_inc,
    cpp_args : zefc_cpp_args,
    install : false,
  )
  test(
    'compiler ' + case_name + ' stdout',
    python3,
    args : [
      meson.project_source_root() / 'test/compiler/check_stdout.py',
      case_exe,
      meson.project_source_root() / 'test/smoke/expected' / (case_name + '.stdout'),
    ],
    suite : 'compiler',
  )
endforeach
