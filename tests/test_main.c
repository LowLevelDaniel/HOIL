/**
 * @file tests_main.c
 * @brief Main test suite for HOILC.
 * 
 * This file contains the main function to run all tests.
 * 
 * @author HOILC Team
 * @date 2025
 */

#include <stdio.h>
#include <stdlib.h>

/**
 * @brief Run all lexer tests.
 * 
 * @return 0 if all tests pass, non-zero otherwise.
 */
extern int test_lexer(void);

/**
 * @brief Run all parser tests.
 * 
 * @return 0 if all tests pass, non-zero otherwise.
 */
extern int test_parser(void);

/**
 * @brief Run all tests.
 * 
 * @return 0 if all tests pass, non-zero otherwise.
 */
int main(void) {
  int result = 0;
  
  printf("===== Running Lexer Tests =====\n");
  result |= test_lexer();
  
  printf("\n===== Running Parser Tests =====\n");
  result |= test_parser();
  
  if (result == 0) {
    printf("\n===== All Tests Passed =====\n");
  } else {
    printf("\n===== Some Tests Failed =====\n");
  }
  
  return result;
}