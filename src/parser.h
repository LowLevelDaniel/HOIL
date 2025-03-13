/**
 * @file parser.h
 * @brief Parser for HOIL language
 * 
 * This file defines the interface for the parser that processes tokens
 * from the lexer and builds an abstract syntax tree (AST).
 *
 * @author Generated by Claude
 * @date 2025-03-13
 */

#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"
#include "ast.h"
#include "symbol_table.h"
#include <stdbool.h>

/**
 * @brief Parser structure for parsing HOIL
 */
typedef struct parser_t parser_t;

/**
 * @brief Create a new parser instance
 * 
 * @param lexer Lexer instance to read tokens from
 * @return Pointer to created parser or NULL on failure
 */
parser_t* parser_create(lexer_t* lexer);

/**
 * @brief Destroy a parser instance and free resources
 * 
 * @param parser Parser to destroy
 */
void parser_destroy(parser_t* parser);

/**
 * @brief Parse a complete module from HOIL source
 * 
 * @param parser The parser instance
 * @return Parsed module or NULL on error
 */
ast_module_t* parser_parse_module(parser_t* parser);

/**
 * @brief Parse a type definition
 * 
 * @param parser The parser instance
 * @return Parsed type definition or NULL on error
 */
ast_type_def_t* parser_parse_type_def(parser_t* parser);

/**
 * @brief Parse a global variable declaration
 * 
 * @param parser The parser instance
 * @return Parsed global variable or NULL on error
 */
ast_global_t* parser_parse_global(parser_t* parser);

/**
 * @brief Parse a constant declaration
 * 
 * @param parser The parser instance
 * @return Parsed constant or NULL on error
 */
ast_constant_t* parser_parse_constant(parser_t* parser);

/**
 * @brief Parse a function definition or declaration
 * 
 * @param parser The parser instance
 * @return Parsed function or NULL on error
 */
ast_function_t* parser_parse_function(parser_t* parser);

/**
 * @brief Parse a target specification
 * 
 * @param parser The parser instance
 * @return Parsed target specification or NULL on error
 */
ast_target_t* parser_parse_target(parser_t* parser);

/**
 * @brief Parse a type reference
 * 
 * @param parser The parser instance
 * @return Parsed type or NULL on error
 */
ast_type_t* parser_parse_type(parser_t* parser);

/**
 * @brief Get the current line number
 * 
 * @param parser The parser instance
 * @return Current line number
 */
size_t parser_line(const parser_t* parser);

/**
 * @brief Get the current column number
 * 
 * @param parser The parser instance
 * @return Current column number
 */
size_t parser_column(const parser_t* parser);

/**
 * @brief Get the source filename
 * 
 * @param parser The parser instance
 * @return Source filename
 */
const char* parser_filename(const parser_t* parser);

#endif /* PARSER_H */