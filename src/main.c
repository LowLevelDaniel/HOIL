/**
 * @file hoilc.c
 * @brief Main implementation of the HOIL to COIL compiler.
 * 
 * This file contains the implementation of the main compiler functions.
 * 
 * @author HOILC Team
 * @date 2025
 */

#include "../include/hoilc.h"
#include "../include/lexer.h"
#include "../include/parser.h"
#include "../include/ast.h"
#include "../include/typecheck.h"
#include "../include/codegen.h"
#include "../include/error.h"
#include "../include/util.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

/**
 * @brief Compiler version string.
 */
static const char* VERSION = "0.1.0";

/**
 * @brief Compiler context structure.
 */
struct hoilc_context {
  char* source;                /**< Source code buffer. */
  size_t source_length;        /**< Source code length. */
  char* output_file;           /**< Output file path. */
  error_context_t* error_ctx;  /**< Error context. */
  bool verbose;                /**< Whether to enable verbose output. */
};

hoilc_context_t* hoilc_create_context(void) {
  hoilc_context_t* context = (hoilc_context_t*)malloc(sizeof(hoilc_context_t));
  if (context == NULL) {
    return NULL;
  }
  
  context->source = NULL;
  context->source_length = 0;
  context->output_file = NULL;
  
  context->error_ctx = error_create_context();
  if (context->error_ctx == NULL) {
    free(context);
    return NULL;
  }
  
  context->verbose = false;
  
  return context;
}

void hoilc_destroy_context(hoilc_context_t* context) {
  if (context == NULL) {
    return;
  }
  
  free(context->source);
  free(context->output_file);
  error_destroy_context(context->error_ctx);
  
  free(context);
}

hoilc_result_t hoilc_set_source_file(hoilc_context_t* context, const char* filename) {
  assert(context != NULL);
  assert(filename != NULL);
  
  /* Clean up previous source */
  free(context->source);
  context->source = NULL;
  context->source_length = 0;
  
  /* Read the source file */
  if (!util_read_file(filename, &context->source, &context->source_length)) {
    error_report(context->error_ctx, HOILC_ERROR_IO,
                 "Failed to read source file: %s", filename);
    return HOILC_ERROR_IO;
  }
  
  return HOILC_SUCCESS;
}

hoilc_result_t hoilc_set_source_string(hoilc_context_t* context, 
                                      const char* source, size_t length) {
  assert(context != NULL);
  assert(source != NULL || length == 0);
  
  /* Clean up previous source */
  free(context->source);
  context->source = NULL;
  context->source_length = 0;
  
  /* Copy the source string */
  if (length > 0) {
    context->source = (char*)malloc(length + 1);  /* +1 for null terminator */
    if (context->source == NULL) {
      error_report(context->error_ctx, HOILC_ERROR_MEMORY,
                   "Memory allocation failed");
      return HOILC_ERROR_MEMORY;
    }
    
    memcpy(context->source, source, length);
    context->source[length] = '\0';  /* Ensure null termination */
    context->source_length = length;
  }
  
  return HOILC_SUCCESS;
}

hoilc_result_t hoilc_set_output_file(hoilc_context_t* context, const char* filename) {
  assert(context != NULL);
  assert(filename != NULL);
  
  /* Clean up previous output file */
  free(context->output_file);
  
  /* Copy the output file path */
  context->output_file = util_strdup(filename);
  if (context->output_file == NULL) {
    error_report(context->error_ctx, HOILC_ERROR_MEMORY,
                 "Memory allocation failed");
    return HOILC_ERROR_MEMORY;
  }
  
  return HOILC_SUCCESS;
}

hoilc_result_t hoilc_compile(hoilc_context_t* context) {
  assert(context != NULL);
  
  /* Check source code */
  if (context->source == NULL || context->source_length == 0) {
    error_report(context->error_ctx, HOILC_ERROR_IO,
                 "No source code provided");
    return HOILC_ERROR_IO;
  }
  
  /* Check output file */
  if (context->output_file == NULL) {
    error_report(context->error_ctx, HOILC_ERROR_IO,
                 "No output file specified");
    return HOILC_ERROR_IO;
  }
  
  /* Create lexer */
  lexer_t* lexer = lexer_create(context->source, context->source_length);
  if (lexer == NULL) {
    error_report(context->error_ctx, HOILC_ERROR_MEMORY,
                 "Failed to create lexer");
    return HOILC_ERROR_MEMORY;
  }
  
  /* Create parser */
  parser_t* parser = parser_create(lexer, context->output_file);
  if (parser == NULL) {
    lexer_destroy(lexer);
    error_report(context->error_ctx, HOILC_ERROR_MEMORY,
                 "Failed to create parser");
    return HOILC_ERROR_MEMORY;
  }
  
  /* Parse the source code */
  if (context->verbose) {
    printf("Parsing source code...\n");
  }
  
  ast_node_t* module = parser_parse_module(parser);
  
  /* Check for parser errors */
  if (parser_has_error(parser)) {
    parser_error_t error = parser_get_error(parser);
    error_report_at(context->error_ctx, HOILC_ERROR_SYNTAX, 
                   &error.location, "%s", error.message);
    
    ast_destroy_node(module);
    parser_destroy(parser);
    lexer_destroy(lexer);
    return HOILC_ERROR_SYNTAX;
  }
  
  /* Destroy the parser and lexer */
  parser_destroy(parser);
  lexer_destroy(lexer);
  
  /* Check if module was successfully parsed */
  if (module == NULL) {
    error_report(context->error_ctx, HOILC_ERROR_SYNTAX,
                 "Failed to parse module");
    return HOILC_ERROR_SYNTAX;
  }
  
  /* Type check the module */
  if (context->verbose) {
    printf("Type checking module...\n");
  }
  
  typecheck_context_t* typecheck_ctx = typecheck_create_context(context->error_ctx);
  if (typecheck_ctx == NULL) {
    ast_destroy_node(module);
    error_report(context->error_ctx, HOILC_ERROR_MEMORY,
                 "Failed to create type checker");
    return HOILC_ERROR_MEMORY;
  }
  
  if (!typecheck_module(typecheck_ctx, module)) {
    symbol_table_t* symbol_table = typecheck_get_symbol_table(typecheck_ctx);
    typecheck_destroy_context(typecheck_ctx);
    ast_destroy_node(module);
    
    (void)symbol_table;

    /* Error already reported by type checker */
    return HOILC_ERROR_TYPE;
  }
  
  /* Get the symbol table */
  symbol_table_t* symbol_table = typecheck_get_symbol_table(typecheck_ctx);
  
  /* Generate code */
  if (context->verbose) {
    printf("Generating COIL code...\n");
  }
  
  codegen_context_t* codegen_ctx = codegen_create_context(context->error_ctx, symbol_table);
  if (codegen_ctx == NULL) {
    typecheck_destroy_context(typecheck_ctx);
    ast_destroy_node(module);
    error_report(context->error_ctx, HOILC_ERROR_MEMORY,
                 "Failed to create code generator");
    return HOILC_ERROR_MEMORY;
  }
  
  /* Generate COIL binary */
  uint8_t* binary = NULL;
  size_t binary_size = 0;
  
  if (!codegen_generate(codegen_ctx, module, &binary, &binary_size)) {
    codegen_destroy_context(codegen_ctx);
    typecheck_destroy_context(typecheck_ctx);
    ast_destroy_node(module);
    
    /* Error already reported by code generator */
    return HOILC_ERROR_INTERNAL;
  }
  
  /* Destroy code generator and type checker */
  codegen_destroy_context(codegen_ctx);
  typecheck_destroy_context(typecheck_ctx);
  
  /* Destroy the AST */
  ast_destroy_node(module);
  
  /* Write output file */
  if (context->verbose) {
    printf("Writing output file: %s\n", context->output_file);
  }
  
  if (!util_write_file(context->output_file, binary, binary_size)) {
    free(binary);
    error_report(context->error_ctx, HOILC_ERROR_IO,
                 "Failed to write output file: %s", context->output_file);
    return HOILC_ERROR_IO;
  }
  
  /* Clean up */
  free(binary);
  
  if (context->verbose) {
    printf("Compilation successful.\n");
  }
  
  return HOILC_SUCCESS;
}

const char* hoilc_get_error_message(hoilc_context_t* context) {
  assert(context != NULL);
  
  return error_get_message(context->error_ctx);
}

hoilc_result_t hoilc_get_error_location(hoilc_context_t* context, int* line, int* column) {
  assert(context != NULL);
  
  if (!error_occurred(context->error_ctx)) {
    return HOILC_ERROR_INTERNAL;
  }
  
  const char* filename;
  if (!error_get_location(context->error_ctx, line, column, &filename)) {
    return HOILC_ERROR_INTERNAL;
  }
  
  return HOILC_SUCCESS;
}

void hoilc_set_verbose(hoilc_context_t* context, bool verbose) {
  assert(context != NULL);
  
  context->verbose = verbose;
}

const char* hoilc_get_version(void) {
  return VERSION;
}


/**
 * @brief Print usage information.
 * 
 * @param program_name The name of the program.
 */
static void print_usage(const char* program_name) {
  fprintf(stderr, "Usage: %s [options] input_file\n", program_name);
  fprintf(stderr, "Options:\n");
  fprintf(stderr, "  -o <file>     Output file (default: input.coil)\n");
  fprintf(stderr, "  -v            Enable verbose output\n");
  fprintf(stderr, "  -h, --help    Show this help message\n");
  fprintf(stderr, "  --version     Show version information\n");
}

/**
 * @brief Print version information.
 */
static void print_version(void) {
  fprintf(stdout, "HOILC (HOIL to COIL Compiler) version %s\n", hoilc_get_version());
  fprintf(stdout, "Copyright (c) 2025 HOILC Team\n");
}

/**
 * @brief Main function.
 * 
 * @param argc Number of command-line arguments.
 * @param argv Array of command-line arguments.
 * @return 0 on success, non-zero on failure.
 */
int main(int argc, char** argv) {
  const char* input_file = NULL;
  const char* output_file = NULL;
  bool verbose = false;
  
  /* Parse command-line arguments */
  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-o") == 0) {
      if (i + 1 < argc) {
        output_file = argv[++i];
      } else {
        fprintf(stderr, "Error: -o option requires an argument\n");
        print_usage(argv[0]);
        return 1;
      }
    } else if (strcmp(argv[i], "-v") == 0) {
      verbose = true;
    } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
      print_usage(argv[0]);
      return 0;
    } else if (strcmp(argv[i], "--version") == 0) {
      print_version();
      return 0;
    } else if (argv[i][0] == '-') {
      fprintf(stderr, "Error: Unknown option: %s\n", argv[i]);
      print_usage(argv[0]);
      return 1;
    } else {
      if (input_file == NULL) {
        input_file = argv[i];
      } else {
        fprintf(stderr, "Error: Multiple input files specified\n");
        print_usage(argv[0]);
        return 1;
      }
    }
  }
  
  /* Check if input file is specified */
  if (input_file == NULL) {
    fprintf(stderr, "Error: No input file specified\n");
    print_usage(argv[0]);
    return 1;
  }
  
  /* Generate output file name if not specified */
  char default_output[FILENAME_MAX];
  if (output_file == NULL) {
    const char* ext = strrchr(input_file, '.');
    if (ext != NULL) {
      size_t base_len = ext - input_file;
      if (base_len < FILENAME_MAX - 6) {  /* 6 = length of ".coil" + 1 */
        strncpy(default_output, input_file, base_len);
        default_output[base_len] = '\0';
        strcat(default_output, ".coil");
        output_file = default_output;
      } else {
        fprintf(stderr, "Error: Generated output filename is too long\n");
        return 1;
      }
    } else {
      if (strlen(input_file) < FILENAME_MAX - 6) {
        strcpy(default_output, input_file);
        strcat(default_output, ".coil");
        output_file = default_output;
      } else {
        fprintf(stderr, "Error: Generated output filename is too long\n");
        return 1;
      }
    }
  }
  
  /* Create compiler context */
  hoilc_context_t* context = hoilc_create_context();
  if (context == NULL) {
    fprintf(stderr, "Error: Failed to create compiler context\n");
    return 1;
  }
  
  /* Set verbose flag */
  hoilc_set_verbose(context, verbose);
  
  /* Set input and output files */
  hoilc_result_t result = hoilc_set_source_file(context, input_file);
  if (result != HOILC_SUCCESS) {
    fprintf(stderr, "Error: Failed to open input file: %s\n", input_file);
    hoilc_destroy_context(context);
    return 1;
  }
  
  result = hoilc_set_output_file(context, output_file);
  if (result != HOILC_SUCCESS) {
    fprintf(stderr, "Error: Failed to open output file: %s\n", output_file);
    hoilc_destroy_context(context);
    return 1;
  }
  
  /* Compile */
  if (verbose) {
    printf("Compiling %s to %s...\n", input_file, output_file);
  }
  
  result = hoilc_compile(context);
  if (result != HOILC_SUCCESS) {
    /* Get error information */
    const char* error_message = hoilc_get_error_message(context);
    int line, column;
    hoilc_get_error_location(context, &line, &column);
    
    /* Print error message */
    if (line > 0 && column > 0) {
      fprintf(stderr, "%s:%d:%d: error: %s\n", input_file, line, column, error_message);
    } else {
      fprintf(stderr, "%s: error: %s\n", input_file, error_message);
    }
    
    hoilc_destroy_context(context);
    return 1;
  }
  
  if (verbose) {
    printf("Compilation successful.\n");
  }
  
  /* Clean up */
  hoilc_destroy_context(context);
  
  return 0;
}
