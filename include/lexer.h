/**
 * @file lexer.h
 * @brief Lexical analyzer for HOIL source code.
 * 
 * This header defines the token types and lexer functions for HOIL.
 * 
 * @author HOILC Team
 * @date 2025
 */

#ifndef HOILC_LEXER_H
#define HOILC_LEXER_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/**
 * @brief Token types in HOIL.
 */
typedef enum {
  /* Special tokens */
  TOKEN_EOF = 0,      /**< End of file. */
  TOKEN_ERROR,        /**< Lexical error. */
  
  /* Punctuation */
  TOKEN_LPAREN,       /**< Left parenthesis '('. */
  TOKEN_RPAREN,       /**< Right parenthesis ')'. */
  TOKEN_LBRACE,       /**< Left brace '{'. */
  TOKEN_RBRACE,       /**< Right brace '}'. */
  TOKEN_LBRACKET,     /**< Left bracket '['. */
  TOKEN_RBRACKET,     /**< Right bracket ']'. */
  TOKEN_COMMA,        /**< Comma ','. */
  TOKEN_DOT,          /**< Dot '.'. */
  TOKEN_SEMICOLON,    /**< Semicolon ';'. */
  TOKEN_COLON,        /**< Colon ':'. */
  TOKEN_ARROW,        /**< Arrow '->'. */
  TOKEN_EQUAL,        /**< Equal '='. */
  TOKEN_LESS,         /**< Less than '<'. */
  TOKEN_GREATER,      /**< Greater than '>'. */
  
  /* Keywords */
  TOKEN_MODULE,       /**< 'MODULE' keyword. */
  TOKEN_TARGET,       /**< 'TARGET' keyword. */
  TOKEN_TYPE,         /**< 'TYPE' keyword. */
  TOKEN_CONSTANT,     /**< 'CONSTANT' keyword. */
  TOKEN_GLOBAL,       /**< 'GLOBAL' keyword. */
  TOKEN_EXTERN,       /**< 'EXTERN' keyword. */
  TOKEN_FUNCTION,     /**< 'FUNCTION' keyword. */
  TOKEN_ENTRY,        /**< 'ENTRY' keyword. */
  
  /* Type keywords */
  TOKEN_VOID,         /**< 'void' type. */
  TOKEN_BOOL,         /**< 'bool' type. */
  TOKEN_I8,           /**< 'i8' type. */
  TOKEN_I16,          /**< 'i16' type. */
  TOKEN_I32,          /**< 'i32' type. */
  TOKEN_I64,          /**< 'i64' type. */
  TOKEN_U8,           /**< 'u8' type. */
  TOKEN_U16,          /**< 'u16' type. */
  TOKEN_U32,          /**< 'u32' type. */
  TOKEN_U64,          /**< 'u64' type. */
  TOKEN_F16,          /**< 'f16' type. */
  TOKEN_F32,          /**< 'f32' type. */
  TOKEN_F64,          /**< 'f64' type. */
  TOKEN_PTR,          /**< 'ptr' type. */
  TOKEN_VEC,          /**< 'vec' type. */
  TOKEN_ARRAY,        /**< 'array' type. */
  
  /* Literals */
  TOKEN_IDENTIFIER,   /**< Identifier. */
  TOKEN_INTEGER,      /**< Integer literal. */
  TOKEN_FLOAT,        /**< Floating point literal. */
  TOKEN_STRING,       /**< String literal. */
  
  /* Instructions */
  TOKEN_ADD,          /**< 'ADD' instruction. */
  TOKEN_SUB,          /**< 'SUB' instruction. */
  TOKEN_MUL,          /**< 'MUL' instruction. */
  TOKEN_DIV,          /**< 'DIV' instruction. */
  TOKEN_REM,          /**< 'REM' instruction. */
  TOKEN_NEG,          /**< 'NEG' instruction. */
  TOKEN_AND,          /**< 'AND' instruction. */
  TOKEN_OR,           /**< 'OR' instruction. */
  TOKEN_XOR,          /**< 'XOR' instruction. */
  TOKEN_NOT,          /**< 'NOT' instruction. */
  TOKEN_SHL,          /**< 'SHL' instruction. */
  TOKEN_SHR,          /**< 'SHR' instruction. */
  TOKEN_CMP_EQ,       /**< 'CMP_EQ' instruction. */
  TOKEN_CMP_NE,       /**< 'CMP_NE' instruction. */
  TOKEN_CMP_LT,       /**< 'CMP_LT' instruction. */
  TOKEN_CMP_LE,       /**< 'CMP_LE' instruction. */
  TOKEN_CMP_GT,       /**< 'CMP_GT' instruction. */
  TOKEN_CMP_GE,       /**< 'CMP_GE' instruction. */
  TOKEN_LOAD,         /**< 'LOAD' instruction. */
  TOKEN_STORE,        /**< 'STORE' instruction. */
  TOKEN_LEA,          /**< 'LEA' instruction. */
  TOKEN_BR,           /**< 'BR' instruction. */
  TOKEN_CALL,         /**< 'CALL' instruction. */
  TOKEN_RET,          /**< 'RET' instruction. */
  
  /* Special */
  TOKEN_COUNT         /**< Total number of token types. */
} token_type_t;

/**
 * @brief Token structure.
 */
typedef struct {
  token_type_t type;  /**< Type of the token. */
  const char* start;  /**< Start of the token in the source. */
  size_t length;      /**< Length of the token in bytes. */
  int line;           /**< Line number (1-based). */
  int column;         /**< Column number (1-based). */
  
  /* Token value for literals */
  union {
    int64_t int_value;    /**< Integer value. */
    double float_value;   /**< Floating point value. */
    const char* str_value; /**< String value. */
  } value;
} token_t;

/**
 * @brief Lexer structure.
 */
typedef struct lexer lexer_t;

/**
 * @brief Create a new lexer.
 * 
 * @param source The source code string.
 * @param length The length of the source code.
 * @return A new lexer or NULL if memory allocation failed.
 */
lexer_t* lexer_create(const char* source, size_t length);

/**
 * @brief Destroy a lexer and free all associated resources.
 * 
 * @param lexer The lexer to destroy.
 */
void lexer_destroy(lexer_t* lexer);

/**
 * @brief Get the next token from the source.
 * 
 * @param lexer The lexer.
 * @param token Pointer to store the token.
 * @return true if a token was scanned, false on error or end of file.
 */
bool lexer_next_token(lexer_t* lexer, token_t* token);

/**
 * @brief Peek at the next token without consuming it.
 * 
 * @param lexer The lexer.
 * @param token Pointer to store the token.
 * @return true if a token was peeked, false on error or end of file.
 */
bool lexer_peek_token(lexer_t* lexer, token_t* token);

/**
 * @brief Get token type name as string.
 * 
 * @param type The token type.
 * @return The name of the token type.
 */
const char* token_type_name(token_type_t type);

/**
 * @brief Get token content as a null-terminated string.
 * 
 * @param token The token.
 * @param buffer Buffer to store the token content.
 * @param size Size of the buffer.
 * @return The buffer containing the token content.
 */
char* token_to_string(const token_t* token, char* buffer, size_t size);

/**
 * @brief Check if a token is a type token.
 * 
 * @param type The token type.
 * @return true if the token is a type, false otherwise.
 */
bool token_is_type(token_type_t type);

/**
 * @brief Check if a token is an instruction token.
 * 
 * @param type The token type.
 * @return true if the token is an instruction, false otherwise.
 */
bool token_is_instruction(token_type_t type);

#endif /* HOILC_LEXER_H */