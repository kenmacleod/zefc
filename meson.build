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

# Compile-time object dispatch: -DZEFC_OBJECT_DISPATCH=N (see object_dispatch.hpp).
object_dispatch = get_option('object_dispatch')
od_map = {
  'slots': 0,
  'ic': 1,
  'flat': 2,
  'site': 3,
}
zefc_od = od_map[object_dispatch]
zefc_cpp_args = ['-DZEFC_OBJECT_DISPATCH=@0@'.format(zefc_od)]
message('object_dispatch=@0@ (ZEFC_OBJECT_DISPATCH=@1@)'.format(object_dispatch, zefc_od))

subdir('runtime')
subdir('compiler')
subdir('test/smoke')
