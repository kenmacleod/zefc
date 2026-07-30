project('zefc', 'cpp',
  version : '0.0.0',
  default_options : [
    'cpp_std=c++17',
    'warning_level=2',
  ],
)

cpp = meson.get_compiler('cpp')
filc_cpp = get_option('filc_cpp')

# Fil-C++ reports version 0.x; system GCC/Clang is typically >= 1.0 or 10+.
if get_option('use_filc') and cpp.version().version_compare('>=1.0')
  warning('use_filc=true but C++ compiler version is @0@ (expected Fil-C 0.x at @1@). Run: ./tools/setup-build.sh'.format(
    cpp.version(), filc_cpp))
elif get_option('use_filc')
  message('Using Fil-C++ (compiler version @0@)'.format(cpp.version()))
endif

runtime_inc = include_directories('runtime/include')

# Compile-time method dispatch: -DZEFC_METHOD_IC=1 (ic) or 0 (pure vtable).
method_dispatch = get_option('method_dispatch')
if method_dispatch == 'ic'
  zefc_method_ic = 1
else
  zefc_method_ic = 0
endif
zefc_cpp_args = ['-DZEFC_METHOD_IC=@0@'.format(zefc_method_ic)]
message('method_dispatch=@0@ (ZEFC_METHOD_IC=@1@)'.format(method_dispatch, zefc_method_ic))

subdir('runtime')
subdir('test/smoke')
