project('hoilc', 'c',
  version : '0.1.0',
  default_options : [
    'c_std=c11',
    'warning_level=3',
    'werror=true',
  ]
)

# Compiler setup
cc = meson.get_compiler('c')
add_project_arguments(
  cc.get_supported_arguments([
    '-Wall',
    '-Wextra',
    '-Wpedantic',
    '-Wshadow',
    '-Wpointer-arith',
    '-Wcast-qual',
    '-Wstrict-prototypes',
    #'-Wmissing-prototypes',
    '-Wno-unused-parameter',
    '-D_DEFAULT_SOURCE',
  ]),
  language : 'c'
)

# Include directories
inc_dirs = include_directories('include')

# Source files
src_files = [
  'src/main.c',
  'src/lexer.c',
  'src/parser.c',
  'src/ast.c',
  'src/typecheck.c',
  'src/codegen.c',
  'src/binary.c',
  'src/error.c',
  'src/symtable.c',
  'src/util.c',
]

# Main executable
hoilc = executable('hoilc',
  src_files,
  include_directories : inc_dirs,
  install : true,
)

# Tests
test_files = [
  'tests/test_lexer.c',
  'tests/test_parser.c',
  'tests/test_main.c',
]

# Unit tests executable
hoilc_tests = executable('hoilc_tests',
  test_files + [
    'src/lexer.c',
    'src/parser.c',
    'src/ast.c',
    'src/typecheck.c',
    'src/codegen.c',
    'src/binary.c',
    'src/error.c',
    'src/symtable.c',
    'src/util.c',
  ],
  include_directories : inc_dirs,
  install : false,
)

# Documentation
# doxygen = find_program('doxygen', required : false)
# if doxygen.found()
#   doxygen_input = files('Doxyfile')
#   custom_target('docs',
#     input : doxygen_input,
#     output : 'html',
#     command : [doxygen, '@INPUT@'],
#     build_by_default : false,
#   )
# endif

# Summary
message('HOILC configuration:')
message('  Version: @0@'.format(meson.project_version()))
message('  C compiler: @0@'.format(cc.get_id()))
# message('  Documentation: @0@'.format(doxygen.found() ? 'enabled' : 'disabled'))