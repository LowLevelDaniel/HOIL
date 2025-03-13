/**
 * @file typecheck.h
 * @brief Type checking for HOIL.
 * 
 * This header defines the type checking system for HOIL.
 * 
 * @author HOILC Team
 * @date 2025
 */

#ifndef HOILC_TYPECHECK_H
#define HOILC_TYPECHECK_H

#include "ast.h"
#include "symtable.h"
#include "error.h"
#include <stdbool.h>

/**
 * @brief Type checker context.
 */
typedef struct typecheck_context typecheck_context_t;

/**
 * @brief Create a new type checker context.
 * 
 * @param error_ctx The error context to use.
 * @return A new type checker context or NULL if memory allocation failed.
 */
typecheck_context_t* typecheck_create_context(error_context_t* error_ctx);

/**
 * @brief Destroy a type checker context and free all associated resources.
 * 
 * @param context The type checker context to destroy.
 */
void typecheck_destroy_context(typecheck_context_t* context);

/**
 * @brief Check an AST module for type correctness.
 * 
 * @param context The type checker context.
 * @param module The AST module to check.
 * @return true if the module is type-correct, false otherwise.
 */
bool typecheck_module(typecheck_context_t* context, ast_node_t* module);

/**
 * @brief Check if two types are compatible.
 * 
 * @param context The type checker context.
 * @param type1 The first type.
 * @param type2 The second type.
 * @return true if the types are compatible, false otherwise.
 */
bool typecheck_are_types_compatible(typecheck_context_t* context, 
                                   ast_node_t* type1, ast_node_t* type2);

/**
 * @brief Check an expression and determine its type.
 * 
 * @param context The type checker context.
 * @param expr The expression to check.
 * @param symtable The symbol table to use.
 * @return The expression type or NULL on error.
 */
ast_node_t* typecheck_expression(typecheck_context_t* context, 
                                ast_node_t* expr, 
                                symbol_table_t* symtable);

/**
 * @brief Check if an operation is valid for the given operand types.
 * 
 * @param context The type checker context.
 * @param opcode The operation code.
 * @param operand_types Array of operand types.
 * @param operand_count Number of operands.
 * @return The result type or NULL if the operation is invalid.
 */
ast_node_t* typecheck_operation(typecheck_context_t* context, 
                               const char* opcode,
                               ast_node_t** operand_types, 
                               size_t operand_count);

/**
 * @brief Get the symbol table from the type checker context.
 * 
 * @param context The type checker context.
 * @return The global symbol table.
 */
symbol_table_t* typecheck_get_symbol_table(typecheck_context_t* context);

#endif /* HOILC_TYPECHECK_H */