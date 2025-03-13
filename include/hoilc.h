/**
 * @file hoilc.h
 * @brief Main header file for the HOIL to COIL compiler.
 * 
 * This header provides the main interface for the HOIL to COIL compiler.
 * 
 * @author HOILC Team
 * @date 2025
 */

#ifndef HOILC_H
#define HOILC_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/**
 * @brief Result code for compiler operations.
 */
typedef enum {
  HOILC_SUCCESS = 0,     /**< Operation succeeded. */
  HOILC_ERROR_IO,        /**< I/O error occurred. */
  HOILC_ERROR_SYNTAX,    /**< Syntax error in the source. */
  HOILC_ERROR_SEMANTIC,  /**< Semantic error in the source. */
  HOILC_ERROR_TYPE,      /**< Type error in the source. */
  HOILC_ERROR_INTERNAL,  /**< Internal compiler error. */
  HOILC_ERROR_MEMORY     /**< Memory allocation error. */
} hoilc_result_t;

/**
 * @brief Compiler context structure.
 */
typedef struct hoilc_context hoilc_context_t;

/**
 * @brief Create a new compiler context.
 * 
 * @return A new compiler context or NULL if memory allocation failed.
 */
hoilc_context_t* hoilc_create_context(void);

/**
 * @brief Destroy a compiler context and free all associated resources.
 * 
 * @param context The context to destroy.
 */
void hoilc_destroy_context(hoilc_context_t* context);

/**
 * @brief Set a source file for compilation.
 * 
 * @param context The compiler context.
 * @param filename The source file path.
 * @return HOILC_SUCCESS on success, error code otherwise.
 */
hoilc_result_t hoilc_set_source_file(hoilc_context_t* context, const char* filename);

/**
 * @brief Set source code for compilation from a string.
 * 
 * @param context The compiler context.
 * @param source The source code string.
 * @param length The length of the source code string.
 * @return HOILC_SUCCESS on success, error code otherwise.
 */
hoilc_result_t hoilc_set_source_string(hoilc_context_t* context, const char* source, size_t length);

/**
 * @brief Set the output file for the COIL binary.
 * 
 * @param context The compiler context.
 * @param filename The output file path.
 * @return HOILC_SUCCESS on success, error code otherwise.
 */
hoilc_result_t hoilc_set_output_file(hoilc_context_t* context, const char* filename);

/**
 * @brief Compile the HOIL source to COIL binary.
 * 
 * @param context The compiler context.
 * @return HOILC_SUCCESS on success, error code otherwise.
 */
hoilc_result_t hoilc_compile(hoilc_context_t* context);

/**
 * @brief Get the last error message.
 * 
 * @param context The compiler context.
 * @return The error message or NULL if no error occurred.
 */
const char* hoilc_get_error_message(hoilc_context_t* context);

/**
 * @brief Get the last error location.
 * 
 * @param context The compiler context.
 * @param line Pointer to store the line number (can be NULL).
 * @param column Pointer to store the column number (can be NULL).
 * @return HOILC_SUCCESS on success, error code otherwise.
 */
hoilc_result_t hoilc_get_error_location(hoilc_context_t* context, int* line, int* column);

/**
 * @brief Enable or disable verbose output.
 * 
 * @param context The compiler context.
 * @param verbose Whether to enable verbose output.
 */
void hoilc_set_verbose(hoilc_context_t* context, bool verbose);

/**
 * @brief Get the HOILC library version.
 * 
 * @return Version string.
 */
const char* hoilc_get_version(void);

#endif /* HOILC_H */