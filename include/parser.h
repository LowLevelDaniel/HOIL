/**
 * @file parser.h
 * @brief Parser for HOIL source code.
 * 
 * This header defines the parser functions for HOIL.
 * 
 * @author HOILC Team
 * @date 2025
 */

#ifndef HOILC_PARSER_H
#define HOILC_PARSER_H

#include "lexer.h"
#include "ast.h"

/**
 * @brief Parser structure.
 */
typedef struct parser parser_t;

/**
 * @brief Parser error structure.
 */
typedef struct {
  char* message;   /**< Error message. */
  source_location_t location; /**< Error location. */
} parser_error_t;

/**
 * @brief Create a new parser.
 * 
 * @param lexer The lexer to use.
 * @param filename The source filename.
 * @return A new parser or NULL if memory allocation failed.
 */
parser_t* parser_create(lexer_t* lexer, const char* filename);

/**
 * @brief Destroy a parser and free all associated resources.
 * 
 * @param parser The parser to destroy.
 */
void parser_destroy(parser_t* parser);

/**
 * @brief Parse a HOIL module.
 * 
 * @param parser The parser.
 * @return The parsed module AST or NULL on error.
 */
ast_node_t* parser_parse_module(parser_t* parser);

/**
 * @brief Get the last parser error.
 * 
 * @param parser The parser.
 * @return The last error.
 */
parser_error_t parser_get_error(const parser_t* parser);

/**
 * @brief Check if the parser encountered an error.
 * 
 * @param parser The parser.
 * @return true if an error occurred, false otherwise.
 */
bool parser_has_error(const parser_t* parser);

#endif /* HOILC_PARSER_H */