/**
 * @file symtable.c
 * @brief Implementation of the symbol table for HOILC.
 * 
 * This file contains the implementation of the symbol table data structures
 * and functions.
 * 
 * @author HOILC Team
 * @date 2025
 */

#include "../include/symtable.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/**
 * @brief Hash table entry structure.
 */
struct hash_entry {
  struct hash_entry* next;  /**< Next entry in the hash chain. */
  symbol_entry_t* symbol;   /**< Symbol entry. */
};

/**
 * @brief Symbol entry structure implementation.
 */
struct symbol_entry {
  char* name;              /**< Symbol name. */
  symbol_kind_t kind;      /**< Symbol kind. */
  ast_node_t* node;        /**< AST node. */
  ast_node_t* type_node;   /**< Type AST node. */
  bool is_defined;         /**< Whether the symbol is defined. */
};

/**
 * @brief Symbol table structure implementation.
 */
struct symbol_table {
  struct hash_entry** entries;  /**< Hash table entries. */
  size_t capacity;              /**< Hash table capacity. */
  size_t count;                 /**< Number of symbols in the table. */
  symbol_table_t* parent;       /**< Parent symbol table. */
};

/**
 * @brief Initial hash table capacity.
 */
#define INITIAL_CAPACITY 64

/**
 * @brief Maximum load factor before resizing.
 */
#define MAX_LOAD_FACTOR 0.75

/**
 * @brief Compute a hash value for a string.
 * 
 * @param str The string to hash.
 * @return The hash value.
 */
static size_t hash_string(const char* str) {
  size_t hash = 5381;
  int c;
  
  while ((c = *str++)) {
    hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
  }
  
  return hash;
}

/**
 * @brief Create a new symbol entry.
 * 
 * @param name The symbol name.
 * @param kind The symbol kind.
 * @param node The AST node.
 * @return A new symbol entry or NULL if memory allocation failed.
 */
static symbol_entry_t* create_symbol(const char* name, symbol_kind_t kind, 
                                     ast_node_t* node) {
  symbol_entry_t* symbol = (symbol_entry_t*)malloc(sizeof(symbol_entry_t));
  if (symbol == NULL) {
    return NULL;
  }
  
  symbol->name = strdup(name);
  if (symbol->name == NULL) {
    free(symbol);
    return NULL;
  }
  
  symbol->kind = kind;
  symbol->node = node;
  symbol->type_node = NULL;
  symbol->is_defined = false;
  
  return symbol;
}

/**
 * @brief Free a symbol entry.
 * 
 * @param symbol The symbol entry to free.
 */
static void free_symbol(symbol_entry_t* symbol) {
  if (symbol == NULL) {
    return;
  }
  
  free(symbol->name);
  free(symbol);
}

/**
 * @brief Resize the hash table.
 * 
 * @param table The symbol table.
 * @param new_capacity The new capacity.
 * @return true on success, false on memory allocation failure.
 */
static bool resize_table(symbol_table_t* table, size_t new_capacity) {
  struct hash_entry** new_entries = (struct hash_entry**)calloc(
    new_capacity, sizeof(struct hash_entry*)
  );
  
  if (new_entries == NULL) {
    return false;
  }
  
  /* Rehash all entries */
  for (size_t i = 0; i < table->capacity; i++) {
    struct hash_entry* entry = table->entries[i];
    
    while (entry != NULL) {
      struct hash_entry* next = entry->next;
      
      /* Insert into new table */
      size_t index = hash_string(entry->symbol->name) % new_capacity;
      entry->next = new_entries[index];
      new_entries[index] = entry;
      
      entry = next;
    }
  }
  
  /* Free old table and update */
  free(table->entries);
  table->entries = new_entries;
  table->capacity = new_capacity;
  
  return true;
}

symbol_table_t* symtable_create(symbol_table_t* parent) {
  symbol_table_t* table = (symbol_table_t*)malloc(sizeof(symbol_table_t));
  if (table == NULL) {
    return NULL;
  }
  
  table->entries = (struct hash_entry**)calloc(
    INITIAL_CAPACITY, sizeof(struct hash_entry*)
  );
  
  if (table->entries == NULL) {
    free(table);
    return NULL;
  }
  
  table->capacity = INITIAL_CAPACITY;
  table->count = 0;
  table->parent = parent;
  
  return table;
}

void symtable_destroy(symbol_table_t* table) {
  if (table == NULL) {
    return;
  }
  
  /* Free all entries and symbols */
  for (size_t i = 0; i < table->capacity; i++) {
    struct hash_entry* entry = table->entries[i];
    
    while (entry != NULL) {
      struct hash_entry* next = entry->next;
      
      free_symbol(entry->symbol);
      free(entry);
      
      entry = next;
    }
  }
  
  free(table->entries);
  free(table);
}

symbol_entry_t* symtable_add(symbol_table_t* table, const char* name, 
                             symbol_kind_t kind, ast_node_t* node) {
  assert(table != NULL);
  assert(name != NULL);
  
  /* Check if we need to resize */
  if ((float)table->count / table->capacity > MAX_LOAD_FACTOR) {
    if (!resize_table(table, table->capacity * 2)) {
      return NULL;
    }
  }
  
  /* Check if the symbol already exists in this scope */
  symbol_entry_t* existing = symtable_lookup(table, name, false);
  if (existing != NULL) {
    return NULL;  /* Symbol already exists */
  }
  
  /* Create the new symbol */
  symbol_entry_t* symbol = create_symbol(name, kind, node);
  if (symbol == NULL) {
    return NULL;
  }
  
  /* Create a hash entry */
  struct hash_entry* entry = (struct hash_entry*)malloc(sizeof(struct hash_entry));
  if (entry == NULL) {
    free_symbol(symbol);
    return NULL;
  }
  
  /* Insert into hash table */
  size_t index = hash_string(name) % table->capacity;
  entry->symbol = symbol;
  entry->next = table->entries[index];
  table->entries[index] = entry;
  
  table->count++;
  
  return symbol;
}

symbol_entry_t* symtable_lookup(symbol_table_t* table, const char* name, 
                                bool search_parent) {
  assert(table != NULL);
  assert(name != NULL);
  
  /* Calculate hash and bucket index */
  size_t index = hash_string(name) % table->capacity;
  
  /* Search in the current table */
  for (struct hash_entry* entry = table->entries[index]; 
       entry != NULL; 
       entry = entry->next) {
    if (strcmp(entry->symbol->name, name) == 0) {
      return entry->symbol;
    }
  }
  
  /* If not found and search_parent is true, search in parent */
  if (search_parent && table->parent != NULL) {
    return symtable_lookup(table->parent, name, true);
  }
  
  return NULL;  /* Not found */
}

const char* symtable_get_name(const symbol_entry_t* entry) {
  assert(entry != NULL);
  
  return entry->name;
}

symbol_kind_t symtable_get_kind(const symbol_entry_t* entry) {
  assert(entry != NULL);
  
  return entry->kind;
}

ast_node_t* symtable_get_node(const symbol_entry_t* entry) {
  assert(entry != NULL);
  
  return entry->node;
}

source_location_t symtable_get_location(const symbol_entry_t* entry) {
  assert(entry != NULL);
  
  if (entry->node != NULL) {
    return entry->node->location;
  }
  
  /* Return empty location if no node is available */
  source_location_t location = {0};
  return location;
}

bool symtable_set_type(symbol_entry_t* entry, ast_node_t* type_node) {
  assert(entry != NULL);
  assert(type_node != NULL);
  assert(ast_is_type_node(type_node));
  
  entry->type_node = type_node;
  
  return true;
}

ast_node_t* symtable_get_type(const symbol_entry_t* entry) {
  assert(entry != NULL);
  
  return entry->type_node;
}

bool symtable_is_defined(const symbol_entry_t* entry) {
  assert(entry != NULL);
  
  return entry->is_defined;
}

void symtable_mark_defined(symbol_entry_t* entry) {
  assert(entry != NULL);
  
  entry->is_defined = true;
}

symbol_table_t* symtable_get_parent(const symbol_table_t* table) {
  assert(table != NULL);
  
  return table->parent;
}

symbol_table_t* symtable_create_child(symbol_table_t* parent) {
  assert(parent != NULL);
  
  return symtable_create(parent);
}