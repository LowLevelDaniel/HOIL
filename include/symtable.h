/**
 * @file symtable.h
 * @brief Symbol table for HOILC.
 * 
 * This header defines the symbol table data structures and functions.
 * 
 * @author HOILC Team
 * @date 2025
 */

#ifndef HOILC_SYMTABLE_H
#define HOILC_SYMTABLE_H

#include "ast.h"
#include <stdbool.h>

/**
 * @brief Symbol kind enumeration.
 */
typedef enum {
  SYMBOL_TYPE,         /**< Type definition. */
  SYMBOL_CONSTANT,     /**< Constant value. */
  SYMBOL_GLOBAL,       /**< Global variable. */
  SYMBOL_FUNCTION,     /**< Function. */
  SYMBOL_PARAMETER,    /**< Function parameter. */
  SYMBOL_LOCAL,        /**< Local variable. */
  SYMBOL_BLOCK,        /**< Basic block label. */
} symbol_kind_t;

/**
 * @brief Symbol entry structure.
 */
typedef struct symbol_entry symbol_entry_t;

/**
 * @brief Symbol table structure.
 */
typedef struct symbol_table symbol_table_t;

/**
 * @brief Create a new symbol table.
 * 
 * @param parent The parent symbol table (can be NULL for global scope).
 * @return A new symbol table or NULL if memory allocation failed.
 */
symbol_table_t* symtable_create(symbol_table_t* parent);

/**
 * @brief Destroy a symbol table and free all associated resources.
 * 
 * @param table The symbol table to destroy.
 */
void symtable_destroy(symbol_table_t* table);

/**
 * @brief Add a symbol to the table.
 * 
 * @param table The symbol table.
 * @param name The symbol name.
 * @param kind The symbol kind.
 * @param node The AST node associated with the symbol.
 * @return The symbol entry or NULL if memory allocation failed.
 */
symbol_entry_t* symtable_add(symbol_table_t* table, const char* name, 
                             symbol_kind_t kind, ast_node_t* node);

/**
 * @brief Look up a symbol in the table.
 * 
 * @param table The symbol table.
 * @param name The symbol name.
 * @param search_parent Whether to search parent scopes.
 * @return The symbol entry or NULL if not found.
 */
symbol_entry_t* symtable_lookup(symbol_table_t* table, const char* name, 
                                bool search_parent);

/**
 * @brief Get the symbol name.
 * 
 * @param entry The symbol entry.
 * @return The symbol name.
 */
const char* symtable_get_name(const symbol_entry_t* entry);

/**
 * @brief Get the symbol kind.
 * 
 * @param entry The symbol entry.
 * @return The symbol kind.
 */
symbol_kind_t symtable_get_kind(const symbol_entry_t* entry);

/**
 * @brief Get the AST node associated with the symbol.
 * 
 * @param entry The symbol entry.
 * @return The AST node.
 */
ast_node_t* symtable_get_node(const symbol_entry_t* entry);

/**
 * @brief Get the source location of the symbol.
 * 
 * @param entry The symbol entry.
 * @return The source location.
 */
source_location_t symtable_get_location(const symbol_entry_t* entry);

/**
 * @brief Set type information for a symbol.
 * 
 * @param entry The symbol entry.
 * @param type_node The type AST node.
 * @return true on success, false on failure.
 */
bool symtable_set_type(symbol_entry_t* entry, ast_node_t* type_node);

/**
 * @brief Get the type node for a symbol.
 * 
 * @param entry The symbol entry.
 * @return The type AST node or NULL if no type is set.
 */
ast_node_t* symtable_get_type(const symbol_entry_t* entry);

/**
 * @brief Check if a symbol is defined.
 * 
 * @param entry The symbol entry.
 * @return true if the symbol is defined, false otherwise.
 */
bool symtable_is_defined(const symbol_entry_t* entry);

/**
 * @brief Mark a symbol as defined.
 * 
 * @param entry The symbol entry.
 */
void symtable_mark_defined(symbol_entry_t* entry);

/**
 * @brief Get the parent symbol table.
 * 
 * @param table The symbol table.
 * @return The parent symbol table or NULL if there is no parent.
 */
symbol_table_t* symtable_get_parent(const symbol_table_t* table);

/**
 * @brief Create a child symbol table.
 * 
 * @param parent The parent symbol table.
 * @return A new child symbol table or NULL if memory allocation failed.
 */
symbol_table_t* symtable_create_child(symbol_table_t* parent);

#endif /* HOILC_SYMTABLE_H */