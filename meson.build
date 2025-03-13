project('hoil2coil', 'c',
  version : '1.0.0',
  default_options : ['warning_level=3', 'c_std=c11']
)

# Add compiler flags
c_args = [
  '-Wextra',
  '-Wpedantic',
  '-Wstrict-prototypes',
  '-Wmissing-prototypes',
  '-Wno-unused-parameter',
]

# Add include directories
inc_dirs = include_directories('src')

# Define source files
sources = [
  'src/main.c',
  'src/lexer.c',
  'src/parser.c',
  'src/ast.c',
  'src/symbol_table.c',
  'src/type_system.c',
  'src/coil_binary.c',
  'src/error_handling.c',
]

# Build executable
hoil2coil = executable('hoil2coil',
  sources,
  include_directories : inc_dirs,
  c_args : c_args,
  install : true,
)

# Add tests
subdir('test')