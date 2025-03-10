project('hoil-coil', 'c',
  version : '0.1.0',
  default_options : [
    'warning_level=3',
    'werror=true',
    'c_std=c11',
  ]
)

cc = meson.get_compiler('c')

# Compiler flags
add_project_arguments(
  cc.get_supported_arguments([
    '-Wformat=2',
    '-Wstrict-prototypes',
    '-Wpointer-arith',
    '-Wcast-qual',
    '-Wcast-align',
    '-Wnested-externs',
    '-Wshadow',
    '-Winline',
    '-Wpacked',
    '-Wmissing-format-attribute',
    '-Wredundant-decls',
    '-Wvla'
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

# Test
test('converter_test', hoil_to_coil, args : ['test/example.hoil', 'test/example.coil'])
test('interpreter_test', coil_interpreter, args : ['test/example.coil'])