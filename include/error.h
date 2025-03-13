/**
 * @file error.h
 * @brief Error handling for HOILC.
 * 
 * This header defines the error handling system for HOILC.
 * 
 * @author HOILC Team
 * @date 2025
 */

#ifndef HOILC_ERROR_H
#define HOILC_ERROR_H

#include "ast.h"
#include "hoilc.h"
#include <stdarg.h>
#include <stdbool.h>

/**
 * @brief Maximum error message length.
 */
#define ERROR_MESSAGE_MAX 1024

/**
 * @brief Error context structure.
 */
typedef struct error_context error_context_t;

/**
 * @brief Create a new error context.
 * 
 * @return A new error context or NULL if memory allocation failed.
 */
error_context_t* error_create_context(void);

/**
 * @brief Destroy an error context and free all associated resources.
 * 
 * @param context The error context to destroy.
 */
void error_destroy_context(error_context_t* context);

/**
 * @brief Report an error at a specific location.
 * 
 * @param context The error context.
 * @param result The error result code.
 * @param location The source location.
 * @param format The error message format.
 * @param ... Additional format arguments.
 */
void error_report_at(error_context_t* context, hoilc_result_t result,
                     const source_location_t* location, const char* format, ...);

/**
 * @brief Report an error at a node location.
 * 
 * @param context The error context.
 * @param result The error result code.
 * @param node The AST node with location information.
 * @param format The error message format.
 * @param ... Additional format arguments.
 */
void error_report_at_node(error_context_t* context, hoilc_result_t result,
                          const ast_node_t* node, const char* format, ...);

/**
 * @brief Report an error.
 * 
 * @param context The error context.
 * @param result The error result code.
 * @param format The error message format.
 * @param ... Additional format arguments.
 */
void error_report(error_context_t* context, hoilc_result_t result,
                  const char* format, ...);

/**
 * @brief Check if an error has been reported.
 * 
 * @param context The error context.
 * @return true if an error has been reported, false otherwise.
 */
bool error_occurred(const error_context_t* context);

/**
 * @brief Get the error result code.
 * 
 * @param context The error context.
 * @return The error result code.
 */
hoilc_result_t error_get_result(const error_context_t* context);

/**
 * @brief Get the error message.
 * 
 * @param context The error context.
 * @return The error message or NULL if no error has been reported.
 */
const char* error_get_message(const error_context_t* context);

/**
 * @brief Get the error location.
 * 
 * @param context The error context.
 * @param line Pointer to store the line number (can be NULL).
 * @param column Pointer to store the column number (can be NULL).
 * @param filename Pointer to store the filename (can be NULL).
 * @return true if the location is valid, false otherwise.
 */
bool error_get_location(const error_context_t* context, int* line, int* column,
                        const char** filename);

/**
 * @brief Clear the error state.
 * 
 * @param context The error context.
 */
void error_clear(error_context_t* context);

#endif /* HOILC_ERROR_H */