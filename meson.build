project('hoil-coil', 'c',
  version : '0.1.0',
  default_options : [
    'c_std=c11',
  ]
)

cc = meson.get_compiler('c')

# Compiler flags
add_project_arguments(
  cc.get_supported_arguments([
    '-Wno-unused-parameter',
    # '-Wformat=2',
    # '-Wstrict-prototypes',
    # '-Wpointer-arith',
    # '-Wcast-qual',
    # '-Wcast-align',
    # '-Wnested-externs',
    # '-Wshadow',
    # '-Winline',
    # '-Wpacked',
    # '-Wmissing-format-attribute',
    # '-Wredundant-decls',
    # '-Wvla'
  ]),
  language : 'c'
)

# Includes
inc = include_directories('include')

# HOIL to COIL converter
hoil_to_coil = executable('hoil_to_coil',
  sources : [
    'src/hoil_to_coil.c',
  ],
  include_directories : inc,
  install : true,
)

# COIL interpreter
coil_interpreter = executable('coil_interpreter',
  sources : [
    'src/coil_interpreter.c',
  ],
  include_directories : inc,
  install : true,
)

coil_vm = executable('coil_vm',
  sources : [
    'src/coil_vm.c',
  ],
  include_directories : inc,
  install : true,
)
