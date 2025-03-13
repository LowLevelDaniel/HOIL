/**
 * @file test_lexer.c
 * @brief Tests for the lexical analyzer.
 * 
 * This file contains tests for the lexical analyzer.
 * 
 * @author HOILC Team
 * @date 2025
 */

#include "../include/lexer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

/**
 * @brief Test that expected tokens are produced.
 * 
 * @param source The source code to tokenize.
 * @param expected_types Array of expected token types.
 * @param expected_count Number of expected tokens.
 * @return true if all tokens match expected types, false otherwise.
 */
static bool test_tokens(const char* source, token_type_t* expected_types, size_t expected_count) {
  lexer_t* lexer = lexer_create(source, strlen(source));
  if (lexer == NULL) {
    fprintf(stderr, "Failed to create lexer\n");
    return false;
  }
  
  size_t token_count = 0;
  token_t token;
  
  while (lexer_next_token(lexer, &token)) {
    if (token_count >= expected_count) {
      fprintf(stderr, "Too many tokens, expected %zu\n", expected_count);
      lexer_destroy(lexer);
      return false;
    }
    
    if (token.type != expected_types[token_count]) {
      fprintf(stderr, "Token %zu: expected %s, got %s\n",
              token_count,
              token_type_name(expected_types[token_count]),
              token_type_name(token.type));
      lexer_destroy(lexer);
      return false;
    }
    
    token_count++;
  }
  
  /* Check for EOF token */
  if (token.type != TOKEN_EOF) {
    fprintf(stderr, "Last token should be EOF, got %s\n",
            token_type_name(token.type));
    lexer_destroy(lexer);
    return false;
  }
  
  if (token_count != expected_count) {
    fprintf(stderr, "Expected %zu tokens, got %zu\n",
            expected_count, token_count);
    lexer_destroy(lexer);
    return false;
  }
  
  lexer_destroy(lexer);
  return true;
}

/**
 * @brief Test basic tokens.
 * 
 * @return true if the test passes, false otherwise.
 */
static bool test_basic_tokens(void) {
  const char* source = "MODULE \"test\";\nFUNCTION main() -> i32 {\n}";
  token_type_t expected[] = {
    TOKEN_MODULE,
    TOKEN_STRING,
    TOKEN_SEMICOLON,
    TOKEN_FUNCTION,
    TOKEN_IDENTIFIER,
    TOKEN_LPAREN,
    TOKEN_RPAREN,
    TOKEN_ARROW,
    TOKEN_I32,
    TOKEN_LBRACE,
    TOKEN_RBRACE
  };
  
  return test_tokens(source, expected, sizeof(expected) / sizeof(expected[0]));
}

/**
 * @brief Test numeric tokens.
 * 
 * @return true if the test passes, false otherwise.
 */
static bool test_numeric_tokens(void) {
  const char* source = "123 45.67 1e3 1.2e-4";
  token_type_t expected[] = {
    TOKEN_INTEGER,
    TOKEN_FLOAT,
    TOKEN_FLOAT,
    TOKEN_FLOAT
  };
  
  return test_tokens(source, expected, sizeof(expected) / sizeof(expected[0]));
}

/**
 * @brief Test instruction tokens.
 * 
 * @return true if the test passes, false otherwise.
 */
static bool test_instruction_tokens(void) {
  const char* source = "ADD SUB MUL DIV REM CMP_EQ CMP_LT BR RET";
  token_type_t expected[] = {
    TOKEN_ADD,
    TOKEN_SUB,
    TOKEN_MUL,
    TOKEN_DIV,
    TOKEN_REM,
    TOKEN_CMP_EQ,
    TOKEN_CMP_LT,
    TOKEN_BR,
    TOKEN_RET
  };
  
  return test_tokens(source, expected, sizeof(expected) / sizeof(expected[0]));
}

/**
 * @brief Test tokens with comments.
 * 
 * @return true if the test passes, false otherwise.
 */
static bool test_comment_tokens(void) {
  const char* source = "// Line comment\nADD /* Block comment */ SUB";
  token_type_t expected[] = {
    TOKEN_ADD,
    TOKEN_SUB
  };
  
  return test_tokens(source, expected, sizeof(expected) / sizeof(expected[0]));
}

/**
 * @brief Run all lexer tests.
 * 
 * @return 0 if all tests pass, non-zero otherwise.
 */
int test_lexer(void) {
  bool result = true;
  
  printf("Testing basic tokens...\n");
  result = result && test_basic_tokens();
  
  printf("Testing numeric tokens...\n");
  result = result && test_numeric_tokens();
  
  printf("Testing instruction tokens...\n");
  result = result && test_instruction_tokens();
  
  printf("Testing tokens with comments...\n");
  result = result && test_comment_tokens();
  
  if (result) {
    printf("All lexer tests passed!\n");
    return 0;
  } else {
    printf("Some lexer tests failed!\n");
    return 1;
  }
}