# ZefC transpiler: .zef → C++ (uses ZEFC_* macros; object_dispatch is a C++ compile flag).

compiler_inc = include_directories('.')

git = find_program('git', required : false)
if git.found()
  git_describe_run = run_command(
    git, '-C', meson.project_source_root(),
    'describe', '--tags', '--always', '--dirty',
    check : false,
  )
  if git_describe_run.returncode() == 0
    zefc_git_describe = git_describe_run.stdout().strip()
  else
    zefc_git_describe = 'unknown'
  endif
else
  zefc_git_describe = 'unknown'
endif

zefc_version_conf = configuration_data()
zefc_version_conf.set('ZEFC_VERSION', meson.project_version())
zefc_version_conf.set('ZEFC_GIT_DESCRIBE', zefc_git_describe)
zefc_version_h = configure_file(
  input : 'version.hpp.in',
  output : 'version.hpp',
  configuration : zefc_version_conf,
)

zefc_exe = executable(
  'zefc',
  files(
    'main.cpp',
    'lexer.cpp',
    'parse.cpp',
    'codegen.cpp',
  ) + [zefc_version_h],
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
  '../test/smoke/generated/loadable_modules.cpp',
)
smoke_inc = include_directories('../test/smoke')
compiler_exe_inc = [runtime_inc, smoke_inc]

hello_exe = executable(
  'zefc-hello',
  [hello_gen] + stdlib_sources,
  dependencies : [runtime_dep],
  include_directories : compiler_exe_inc,
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
  include_directories : compiler_exe_inc,
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
  include_directories : compiler_exe_inc,
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
                     'accessors2b', 'private1', 'duplicateparam',
                     'classinfunction', 'classinfunction2', 'test9',
                     'private2', 'private3',
                     'staticcall2', 'staticcall3', 'staticcall6', 'staticcall7',
                     'staticcall4', 'staticcall5',
                     'package1', 'package2', 'package3', 'package4', 'package5',
                     'package2b', 'package2c', 'package2d', 'package2e', 'package2f',
                     'package6', 'package7', 'package8', 'package9', 'package10',
                     'package12', 'package12b', 'package12c',
                     'test26b', 'test26c', 'test37', 'test38', 'test43', 'test',
                     'test42', 'testc', 'nbody', 'richards', 'splay', 'load1',
                     'load4', 'load5', 'load6', 'load7', 'load8', 'load9',
                     'load10']
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
    include_directories : compiler_exe_inc,
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

foreach err_name : ['test10', 'test11', 'test16', 'test17', 'test18', 'test21',
                    'load2', 'load3', 'package11', 'accessors2', 'test28']
  test(
    'compiler ' + err_name + ' stderr',
    python3,
    args : [
      meson.project_source_root() / 'test/compiler/check_compile_error.py',
      zefc_exe,
      meson.project_source_root() / 'test/smoke/zef' / (err_name + '.zef'),
      meson.project_source_root() / 'test/smoke/expected' / (err_name + '.stderr'),
    ],
    suite : 'compiler',
  )
endforeach

test39_zef = meson.project_source_root() / 'test/smoke/zef/test39.zef'
test39_gen = custom_target(
  'test39_from_zef',
  input : test39_zef,
  output : 'test39.gen.cpp',
  command : [zefc_exe, '@INPUT@', '-o', '@OUTPUT@'],
)
test39_exe = executable(
  'zefc-test39',
  [test39_gen] + stdlib_sources,
  dependencies : [runtime_dep],
  include_directories : compiler_exe_inc,
  cpp_args : zefc_cpp_args,
  install : false,
)
test(
  'compiler test39 stderr',
  python3,
  args : [
    meson.project_source_root() / 'test/compiler/check_runtime_error.py',
    test39_exe,
    meson.project_source_root() / 'test/smoke/expected/test39.stderr',
  ],
  suite : 'compiler',
)

test7b_zef = meson.project_source_root() / 'test/smoke/zef/test7b.zef'
test7b_gen = custom_target(
  'test7b_from_zef',
  input : test7b_zef,
  output : 'test7b.gen.cpp',
  command : [zefc_exe, '@INPUT@', '-o', '@OUTPUT@'],
)
test7b_exe = executable(
  'zefc-test7b',
  [test7b_gen] + stdlib_sources,
  dependencies : [runtime_dep],
  include_directories : compiler_exe_inc,
  cpp_args : zefc_cpp_args,
  install : false,
)
test(
  'compiler test7b stderr',
  python3,
  args : [
    meson.project_source_root() / 'test/compiler/check_runtime_error.py',
    test7b_exe,
    meson.project_source_root() / 'test/smoke/expected/test7b.stderr',
  ],
  suite : 'compiler',
)
