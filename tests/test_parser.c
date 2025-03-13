/**
 * @file test_parser.c
 * @brief Tests for the parser.
 * 
 * This file contains tests for the parser.
 * 
 * @author HOILC Team
 * @date 2025
 */

#include "../include/lexer.h"
#include "../include/parser.h"
#include "../include/ast.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

/**
 * @brief Test that a source string can be parsed correctly.
 * 
 * @param source The source code to parse.
 * @param expected_success Whether parsing is expected to succeed.
 * @return true if the parsing result matches expectation, false otherwise.
 */
static bool test_parse(const char* source, bool expected_success) {
  lexer_t* lexer = lexer_create(source, strlen(source));
  if (lexer == NULL) {
    fprintf(stderr, "Failed to create lexer\n");
    return false;
  }
  
  parser_t* parser = parser_create(lexer, "test.hoil");
  if (parser == NULL) {
    fprintf(stderr, "Failed to create parser\n");
    lexer_destroy(lexer);
    return false;
  }
  
  ast_node_t* module = parser_parse_module(parser);
  bool success = module != NULL && !parser_has_error(parser);
  
  if (success != expected_success) {
    fprintf(stderr, "Expected parsing to %s, but it %s\n",
            expected_success ? "succeed" : "fail",
            success ? "succeeded" : "failed");
    
    if (parser_has_error(parser)) {
      parser_error_t error = parser_get_error(parser);
      fprintf(stderr, "Error: %s at line %d column %d\n",
              error.message, error.location.line, error.location.column);
    }
    
    ast_destroy_node(module);
    parser_destroy(parser);
    lexer_destroy(lexer);
    return false;
  }
  
  ast_destroy_node(module);
  parser_destroy(parser);
  lexer_destroy(lexer);
  return true;
}

/**
 * @brief Test parsing a valid module.
 * 
 * @return true if the test passes, false otherwise.
 */
static bool test_valid_module(void) {
  const char* source = "MODULE \"test\";\n";
  return test_parse(source, true);
}

/**
 * @brief Test parsing a valid function.
 * 
 * @return true if the test passes, false otherwise.
 */
static bool test_valid_function(void) {
  const char* source = 
    "MODULE \"test\";\n"
    "FUNCTION add(a: i32, b: i32) -> i32 {\n"
    "    ENTRY:\n"
    "        result = ADD a, b;\n"
    "        RET result;\n"
    "}\n"
    "FUNCTION sub(a: i32, b: i32) -> i32 {\n"
    "    ENTRY:\n"
    "        result = SUB a, b;\n"
    "        RET result;\n"
    "}\n";
  
  return test_parse(source, true);
}

/**
 * @brief Test parsing a valid type definition.
 * 
 * @return true if the test passes, false otherwise.
 */
static bool test_valid_type_def(void) {
  const char* source = 
    "MODULE \"test\";\n"
    "TYPE point {\n"
    "    x: f32,\n"
    "    y: f32,\n"
    "    z: f32\n"
    "}\n";
  
  return test_parse(source, true);
}

/**
 * @brief Test parsing an invalid module.
 * 
 * @return true if the test passes, false otherwise.
 */
static bool test_invalid_module(void) {
  const char* source = "MODULE test;\n";  /* Missing quotes */
  return test_parse(source, false);
}

/**
 * @brief Test parsing an invalid function.
 * 
 * @return true if the test passes, false otherwise.
 */
static bool test_invalid_function(void) {
  const char* source = 
    "MODULE \"test\";\n"
    "FUNCTION add(a: i32, b: i32) i32 {\n"  /* Missing arrow */
    "    ENTRY:\n"
    "        result = ADD a, b;\n"
    "        RET result;\n"
    "}\n";
  
  return test_parse(source, false);
}

/**
 * @brief Run all parser tests.
 * 
 * @return 0 if all tests pass, non-zero otherwise.
 */
int test_parser(void) {
  bool result = true;
  
  printf("Testing valid module...\n");
  result = result && test_valid_module();
  
  printf("Testing valid function...\n");
  result = result && test_valid_function();
  
  printf("Testing valid type definition...\n");
  result = result && test_valid_type_def();
  
  printf("Testing invalid module...\n");
  result = result && test_invalid_module();
  
  printf("Testing invalid function...\n");
  result = result && test_invalid_function();
  
  if (result) {
    printf("All parser tests passed!\n");
    return 0;
  } else {
    printf("Some parser tests failed!\n");
    return 1;
  }
}