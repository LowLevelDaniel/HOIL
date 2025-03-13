/**
 * @file codegen.h
 * @brief Code generation for HOIL to COIL.
 * 
 * This header defines the code generation system for HOIL to COIL conversion.
 * 
 * @author HOILC Team
 * @date 2025
 */

#ifndef HOILC_CODEGEN_H
#define HOILC_CODEGEN_H

#include "ast.h"
#include "binary.h"
#include "symtable.h"
#include "error.h"
#include <stdbool.h>

/**
 * @brief Code generator context.
 */
typedef struct codegen_context codegen_context_t;

/**
 * @brief Create a new code generator context.
 * 
 * @param error_ctx The error context to use.
 * @param symbol_table The global symbol table.
 * @return A new code generator context or NULL if memory allocation failed.
 */
codegen_context_t* codegen_create_context(error_context_t* error_ctx,
                                          symbol_table_t* symbol_table);

/**
 * @brief Destroy a code generator context and free all associated resources.
 * 
 * @param context The code generator context to destroy.
 */
void codegen_destroy_context(codegen_context_t* context);

/**
 * @brief Generate COIL code from an AST module.
 * 
 * @param context The code generator context.
 * @param module The AST module.
 * @param output Pointer to store the output binary.
 * @param size Pointer to store the output size.
 * @return true on success, false on failure.
 */
bool codegen_generate(codegen_context_t* context, ast_node_t* module,
                      uint8_t** output, size_t* size);

/**
 * @brief Get the COIL builder from the code generator context.
 * 
 * @param context The code generator context.
 * @return The COIL builder.
 */
coil_builder_t* codegen_get_builder(codegen_context_t* context);

/**
 * @brief Map a HOIL type node to a COIL type encoding.
 * 
 * @param context The code generator context.
 * @param type_node The AST type node.
 * @return The type index in the COIL binary, or -1 on error.
 */
int32_t codegen_map_type(codegen_context_t* context, ast_node_t* type_node);

/**
 * @brief Map a HOIL instruction to a COIL opcode.
 * 
 * @param context The code generator context.
 * @param instruction The instruction name.
 * @return The COIL opcode or 0 if the instruction is not recognized.
 */
uint8_t codegen_map_instruction(codegen_context_t* context, const char* instruction);

/**
 * @brief Generate code for a constant value.
 * 
 * @param context The code generator context.
 * @param value The AST value node.
 * @param output Pointer to store the output data.
 * @param size Pointer to store the output size.
 * @return true on success, false on failure.
 */
bool codegen_generate_constant(codegen_context_t* context, ast_node_t* value,
                              void** output, size_t* size);

#endif /* HOILC_CODEGEN_H */