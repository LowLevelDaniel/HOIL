/**
* @file hoil_format.h
* @brief Definition of the HOIL language format
*
* This file contains definitions specific to the HOIL (Human Oriented Intermediate 
* Language) format. HOIL is a high-level assembly-like language that compiles to COIL.
*/

#ifndef HOIL_FORMAT_H
#define HOIL_FORMAT_H

#include "coil_format.h"

/**
* @brief Maximum line length for HOIL instructions
*/
#define MAX_HOIL_LINE_LENGTH 256

/**
* @brief Maximum number of tokens in a HOIL instruction
*/
#define MAX_HOIL_TOKENS 16

/**
* @brief Maximum number of symbols in symbol table
*/
#define MAX_SYMBOLS 256

/**
* @brief Symbol table entry structure
*/
typedef struct {
  char name[64];            /**< Symbol name */
  uint16_t address;         /**< Memory address */
  mem_type_t type;          /**< Memory type */
} symbol_entry_t;

/**
* @brief Symbol table structure
*/
typedef struct {
  symbol_entry_t entries[MAX_SYMBOLS];  /**< Symbol entries */
  size_t count;                         /**< Number of symbols defined */
} symbol_table_t;

/**
* @brief Label table entry structure
*/
typedef struct {
  char name[64];            /**< Label name */
  uint16_t id;              /**< Label identifier */
  bool defined;             /**< Flag indicating whether the label is defined */
} label_entry_t;

/**
* @brief Label table structure
*/
typedef struct {
  label_entry_t entries[MAX_SYMBOLS];   /**< Label entries */
  size_t count;                         /**< Number of labels defined */
  uint16_t next_id;                     /**< Next label identifier */
} label_table_t;

/**
* @brief HOIL instruction categories
*/
typedef enum {
  HOIL_CAT_VAL,   /**< Value operations */
  HOIL_CAT_MATH,  /**< Mathematical operations */
  HOIL_CAT_BIT,   /**< Bitwise operations */
  HOIL_CAT_CF,    /**< Control flow operations */
  HOIL_CAT_MEM    /**< Memory operations */
} hoil_category_t;

/**
* @brief HOIL value operations
*/
typedef enum {
  HOIL_VAL_DEFV,  /**< Define a value with immediate value */
  HOIL_VAL_MOVV,  /**< Move value from source to destination */
  HOIL_VAL_LOAD,  /**< Load from memory address */
  HOIL_VAL_STORE  /**< Store to memory address */
} hoil_val_op_t;

/**
* @brief HOIL mathematical operations
*/
typedef enum {
  HOIL_MATH_ADD,  /**< Add values */
  HOIL_MATH_SUB,  /**< Subtract values */
  HOIL_MATH_MUL,  /**< Multiply values */
  HOIL_MATH_DIV,  /**< Divide values */
  HOIL_MATH_MOD,  /**< Modulo operation */
  HOIL_MATH_NEG   /**< Negate value */
} hoil_math_op_t;

/**
* @brief HOIL bitwise operations
*/
typedef enum {
  HOIL_BIT_AND,  /**< Bitwise AND */
  HOIL_BIT_OR,   /**< Bitwise OR */
  HOIL_BIT_XOR,  /**< Bitwise XOR */
  HOIL_BIT_NOT,  /**< Bitwise NOT */
  HOIL_BIT_SHL,  /**< Shift left */
  HOIL_BIT_SHR   /**< Shift right */
} hoil_bit_op_t;

/**
* @brief HOIL control flow operations
*/
typedef enum {
  HOIL_CF_JMP,    /**< Unconditional jump */
  HOIL_CF_JCOND,  /**< Conditional jump */
  HOIL_CF_LABEL,  /**< Define a label */
  HOIL_CF_CALL,   /**< Call function */
  HOIL_CF_RET,    /**< Return from function */
  HOIL_CF_PUSH,   /**< Push value onto stack */
  HOIL_CF_POP,    /**< Pop value from stack */
  HOIL_CF_SYSC,   /**< System call */
  HOIL_CF_EXIT    /**< Exit program */
} hoil_cf_op_t;

/**
* @brief HOIL conditional jump types
*/
typedef enum {
  HOIL_COND_EQ,  /**< Equal */
  HOIL_COND_NE,  /**< Not equal */
  HOIL_COND_LT,  /**< Less than */
  HOIL_COND_LE,  /**< Less or equal */
  HOIL_COND_GT,  /**< Greater than */
  HOIL_COND_GE   /**< Greater or equal */
} hoil_cond_t;

/**
* @brief Convert a HOIL data type to COIL type code
* 
* @param type_str HOIL type string
* @return COIL type code
*/
mem_type_t hoil_type_to_coil_type(const char *type_str);

/**
* @brief Parse a HOIL line into tokens
* 
* @param line Input HOIL line
* @param tokens Array to store tokens
* @param max_tokens Maximum number of tokens to parse
* @return Number of tokens parsed
*/
int tokenize_hoil_line(char *line, char **tokens, int max_tokens);

/**
* @brief Convert a HOIL immediate value to a numeric value
* 
* @param value_str HOIL value string
* @return Numeric value
*/
int64_t convert_immediate_value(const char *value_str);

#endif /* HOIL_FORMAT_H */