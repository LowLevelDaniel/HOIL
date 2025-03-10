project('hoil-coil', 'c',
  version : '0.1.0',
  default_options : [
    'c_std=c11',
    'warning_level=3',
    'optimization=2'
  ]
)

cc = meson.get_compiler('c')

# Common compiler flags for safety and correctness
common_flags = [
  '-Wextra',
  '-Wformat=2',
  '-Wstrict-prototypes',
  '-Wpointer-arith',
  '-Wcast-qual',
  '-Wcast-align',
  '-Wshadow',
  '-Winline',
  '-Wpacked',
  '-Wmissing-format-attribute',
  '-Wredundant-decls',
  '-Wdouble-promotion',
  '-Wfloat-equal',
  '-Wwrite-strings',
  '-fstack-protector-strong'
]

# Development-specific flags
if get_option('buildtype').startswith('debug')
  common_flags += [
    # '-fno-omit-frame-pointer',
    # '-fsanitize=address,undefined',
    # '-D_FORTIFY_SOURCE=2',
    # '-DDEBUG'
  ]
endif

# Add all supported flags
add_project_arguments(
  cc.get_supported_arguments(common_flags),
  language : 'c'
)

# Include directories
inc = include_directories('include')

# Library containing common COIL/HOIL functionality
coil_lib = static_library('coil',
  sources : [],
  include_directories : inc,
  install : false
)

# HOIL to COIL converter
hoil_to_coil = executable('hoil_to_coil',
  sources : [
    'src/hoil_to_coil.c',
  ],
  include_directories : inc,
  link_with : coil_lib,
  install : true,
)

# COIL VM
coil_vm = executable('coil_vm',
  sources : [
    'src/coil_vm.c',
  ],
  include_directories : inc,
  link_with : coil_lib,
  install : true,
)

# Documentation target
if find_program('doxygen', required : false).found()
  custom_target('documentation',
    output : 'docs',
    command : ['doxygen', join_paths(meson.source_root(), 'Doxyfile')],
    build_by_default : false
  )
endif

# Tests
test_dir = join_paths(meson.source_root(), 'test')

if find_program('valgrind', required : false).found()
  add_test_setup('valgrind',
    exe_wrapper : ['valgrind', '--leak-check=full', '--error-exitcode=1'],
    timeout_multiplier : 2
  )
endif

# Factorial test
test('factorial', 
  hoil_to_coil,
  args : [
    '-b',
    join_paths(test_dir, 'factorial.hoil'),
    join_paths(meson.build_root(), 'factorial.coil')
  ],
  workdir : meson.build_root()
)

# Run the factorial test with COIL VM
test('run_factorial',
  coil_vm,
  args : [
    '-b',
    join_paths(meson.build_root(), 'factorial.coil')
  ],
  depends : [hoil_to_coil],
  workdir : meson.build_root()
)

# Fibonacci test
test('fibonacci', 
  hoil_to_coil,
  args : [
    '-b',
    join_paths(test_dir, 'fibonacci.hoil'),
    join_paths(meson.build_root(), 'fibonacci.coil')
  ],
  workdir : meson.build_root()
)

# Run the fibonacci test with COIL VM
test('run_fibonacci',
  coil_vm,
  args : [
    '-b',
    join_paths(meson.build_root(), 'fibonacci.coil')
  ],
  depends : [hoil_to_coil],
  workdir : meson.build_root()
)