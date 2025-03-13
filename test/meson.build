# Test sources that need to be compiled separately
test_sources = [
  '../src/lexer.c',
  '../src/parser.c',
  '../src/ast.c',
  '../src/symbol_table.c',
  '../src/type_system.c',
  '../src/coil_binary.c',
  '../src/error_handling.c',
]

# Add include directories
test_inc_dirs = include_directories('../src')

# Create test for lexer
test_lexer = executable('test_lexer',
  ['test_lexer.c'] + test_sources,
  include_directories : test_inc_dirs,
  c_args : c_args,
)

# Create test for parser
test_parser = executable('test_parser',
  ['test_parser.c'] + test_sources,
  include_directories : test_inc_dirs,
  c_args : c_args,
)

# Register tests
test('Lexer Test', test_lexer)
test('Parser Test', test_parser)