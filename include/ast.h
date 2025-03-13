/**
 * @file ast.h
 * @brief Abstract Syntax Tree for HOIL.
 * 
 * This header defines the AST node types and structures for HOIL.
 * 
 * @author HOILC Team
 * @date 2025
 */

#ifndef HOILC_AST_H
#define HOILC_AST_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/**
 * @brief AST node types.
 */
typedef enum {
  /* Top-level nodes */
  AST_MODULE,         /**< Module declaration. */
  AST_TARGET,         /**< Target specification. */
  AST_TYPE_DEF,       /**< Type definition. */
  AST_CONSTANT,       /**< Constant declaration. */
  AST_GLOBAL,         /**< Global variable declaration. */
  AST_FUNCTION,       /**< Function declaration. */
  AST_EXTERN_FUNCTION, /**< External function declaration. */
  
  /* Type nodes */
  AST_TYPE_VOID,      /**< Void type. */
  AST_TYPE_BOOL,      /**< Boolean type. */
  AST_TYPE_INT,       /**< Integer type. */
  AST_TYPE_FLOAT,     /**< Floating point type. */
  AST_TYPE_PTR,       /**< Pointer type. */
  AST_TYPE_VEC,       /**< Vector type. */
  AST_TYPE_ARRAY,     /**< Array type. */
  AST_TYPE_STRUCT,    /**< Structure type. */
  AST_TYPE_FUNCTION,  /**< Function type. */
  AST_TYPE_NAME,      /**< Named type. */
  
  /* Expression nodes */
  AST_EXPR_INTEGER,   /**< Integer literal. */
  AST_EXPR_FLOAT,     /**< Floating point literal. */
  AST_EXPR_STRING,    /**< String literal. */
  AST_EXPR_IDENTIFIER, /**< Identifier. */
  AST_EXPR_FIELD,     /**< Field access. */
  AST_EXPR_INDEX,     /**< Array indexing. */
  AST_EXPR_CALL,      /**< Function call. */
  
  /* Statement nodes */
  AST_STMT_BLOCK,     /**< Basic block. */
  AST_STMT_ASSIGN,    /**< Assignment statement. */
  AST_STMT_INSTRUCTION, /**< Instruction statement. */
  AST_STMT_BRANCH,    /**< Branch statement. */
  AST_STMT_RETURN,    /**< Return statement. */
  
  /* Other node types */
  AST_PARAMETER,      /**< Function parameter. */
  AST_FIELD,          /**< Structure field. */
} ast_node_type_t;

/* Forward declaration of AST node structure */
typedef struct ast_node ast_node_t;

/**
 * @brief AST node list structure.
 */
typedef struct ast_node_list {
  ast_node_t** nodes;    /**< Array of node pointers. */
  size_t count;          /**< Number of nodes. */
  size_t capacity;       /**< Capacity of the array. */
} ast_node_list_t;

/**
 * @brief Source location structure.
 */
typedef struct {
  int line;              /**< Line number (1-based). */
  int column;            /**< Column number (1-based). */
  const char* filename;  /**< Source filename. */
} source_location_t;

/**
 * @brief Module node structure.
 */
typedef struct {
  char* name;            /**< Module name. */
  ast_node_list_t declarations; /**< Module declarations. */
} ast_module_t;

/**
 * @brief Target node structure.
 */
typedef struct {
  char* device_class;    /**< Device class. */
  ast_node_list_t required_features; /**< Required features. */
  ast_node_list_t preferred_features; /**< Preferred features. */
} ast_target_t;

/**
 * @brief Type definition node structure.
 */
typedef struct {
  char* name;            /**< Type name. */
  ast_node_list_t fields; /**< Type fields. */
} ast_type_def_t;

/**
 * @brief Constant declaration node structure.
 */
typedef struct {
  char* name;            /**< Constant name. */
  ast_node_t* type;      /**< Constant type. */
  ast_node_t* value;     /**< Constant value. */
} ast_constant_t;

/**
 * @brief Global variable declaration node structure.
 */
typedef struct {
  char* name;            /**< Global variable name. */
  ast_node_t* type;      /**< Global variable type. */
  ast_node_t* initializer; /**< Global variable initializer (can be NULL). */
} ast_global_t;

/**
 * @brief Function declaration node structure.
 */
typedef struct {
  char* name;            /**< Function name. */
  ast_node_list_t parameters; /**< Function parameters. */
  ast_node_t* return_type; /**< Function return type. */
  ast_node_list_t blocks; /**< Function basic blocks. */
  ast_node_t* target;    /**< Function target (can be NULL). */
} ast_function_t;

/**
 * @brief External function declaration node structure.
 */
typedef struct {
  char* name;            /**< Function name. */
  ast_node_list_t parameters; /**< Function parameters. */
  ast_node_t* return_type; /**< Function return type. */
  bool is_variadic;      /**< Whether the function is variadic. */
} ast_extern_function_t;

/**
 * @brief Integer type node structure.
 */
typedef struct {
  uint8_t bits;          /**< Number of bits. */
  bool is_signed;        /**< Whether the integer is signed. */
} ast_type_int_t;

/**
 * @brief Floating point type node structure.
 */
typedef struct {
  uint8_t bits;          /**< Number of bits. */
} ast_type_float_t;

/**
 * @brief Pointer type node structure.
 */
typedef struct {
  ast_node_t* element_type; /**< Element type. */
  char* memory_space;    /**< Memory space (can be NULL). */
} ast_type_ptr_t;

/**
 * @brief Vector type node structure.
 */
typedef struct {
  ast_node_t* element_type; /**< Element type. */
  uint32_t size;          /**< Number of elements. */
} ast_type_vec_t;

/**
 * @brief Array type node structure.
 */
typedef struct {
  ast_node_t* element_type; /**< Element type. */
  uint32_t size;          /**< Number of elements (0 for unsized arrays). */
} ast_type_array_t;

/**
 * @brief Structure type node structure.
 */
typedef struct {
  ast_node_list_t fields; /**< Structure fields. */
} ast_type_struct_t;

/**
 * @brief Function type node structure.
 */
typedef struct {
  ast_node_list_t parameter_types; /**< Parameter types. */
  ast_node_t* return_type; /**< Return type. */
} ast_type_function_t;

/**
 * @brief Named type node structure.
 */
typedef struct {
  char* name;            /**< Type name. */
} ast_type_name_t;

/**
 * @brief Integer literal node structure.
 */
typedef struct {
  int64_t value;         /**< Integer value. */
} ast_expr_integer_t;

/**
 * @brief Floating point literal node structure.
 */
typedef struct {
  double value;          /**< Floating point value. */
} ast_expr_float_t;

/**
 * @brief String literal node structure.
 */
typedef struct {
  char* value;           /**< String value. */
} ast_expr_string_t;

/**
 * @brief Identifier node structure.
 */
typedef struct {
  char* name;            /**< Identifier name. */
} ast_expr_identifier_t;

/**
 * @brief Field access node structure.
 */
typedef struct {
  ast_node_t* object;    /**< Object expression. */
  char* field;           /**< Field name. */
} ast_expr_field_t;

/**
 * @brief Array indexing node structure.
 */
typedef struct {
  ast_node_t* array;     /**< Array expression. */
  ast_node_t* index;     /**< Index expression. */
} ast_expr_index_t;

/**
 * @brief Function call node structure.
 */
typedef struct {
  ast_node_t* function;  /**< Function expression. */
  ast_node_list_t arguments; /**< Call arguments. */
} ast_expr_call_t;

/**
 * @brief Basic block node structure.
 */
typedef struct {
  char* label;           /**< Block label. */
  ast_node_list_t statements; /**< Block statements. */
} ast_stmt_block_t;

/**
 * @brief Assignment statement node structure.
 */
typedef struct {
  char* target;          /**< Assignment target. */
  ast_node_t* value;     /**< Assignment value. */
} ast_stmt_assign_t;

/**
 * @brief Instruction statement node structure.
 */
typedef struct {
  char* opcode;          /**< Instruction opcode. */
  ast_node_list_t operands; /**< Instruction operands. */
} ast_stmt_instruction_t;

/**
 * @brief Branch statement node structure.
 */
typedef struct {
  ast_node_t* condition; /**< Branch condition (can be NULL for unconditional branches). */
  char* true_target;     /**< Target block for true condition. */
  char* false_target;    /**< Target block for false condition (can be NULL). */
} ast_stmt_branch_t;

/**
 * @brief Return statement node structure.
 */
typedef struct {
  ast_node_t* value;     /**< Return value (can be NULL for void returns). */
} ast_stmt_return_t;

/**
 * @brief Function parameter node structure.
 */
typedef struct {
  char* name;            /**< Parameter name. */
  ast_node_t* type;      /**< Parameter type. */
} ast_parameter_t;

/**
 * @brief Structure field node structure.
 */
typedef struct {
  char* name;            /**< Field name. */
  ast_node_t* type;      /**< Field type. */
} ast_field_t;

/**
 * @brief AST node structure.
 */
struct ast_node {
  ast_node_type_t type;   /**< Node type. */
  source_location_t location; /**< Source location. */
  
  /* Node-specific data */
  union {
    ast_module_t module;
    ast_target_t target;
    ast_type_def_t type_def;
    ast_constant_t constant;
    ast_global_t global;
    ast_function_t function;
    ast_extern_function_t extern_function;
    
    /* Type nodes */
    ast_type_int_t type_int;
    ast_type_float_t type_float;
    ast_type_ptr_t type_ptr;
    ast_type_vec_t type_vec;
    ast_type_array_t type_array;
    ast_type_struct_t type_struct;
    ast_type_function_t type_function;
    ast_type_name_t type_name;
    
    /* Expression nodes */
    ast_expr_integer_t expr_integer;
    ast_expr_float_t expr_float;
    ast_expr_string_t expr_string;
    ast_expr_identifier_t expr_identifier;
    ast_expr_field_t expr_field;
    ast_expr_index_t expr_index;
    ast_expr_call_t expr_call;
    
    /* Statement nodes */
    ast_stmt_block_t stmt_block;
    ast_stmt_assign_t stmt_assign;
    ast_stmt_instruction_t stmt_instruction;
    ast_stmt_branch_t stmt_branch;
    ast_stmt_return_t stmt_return;
    
    /* Other nodes */
    ast_parameter_t parameter;
    ast_field_t field;
  } data;
};

/**
 * @brief Create a new AST node.
 * 
 * @param type The node type.
 * @return A new AST node or NULL if memory allocation failed.
 */
ast_node_t* ast_create_node(ast_node_type_t type);

/**
 * @brief Destroy an AST node and all its children.
 * 
 * @param node The node to destroy.
 */
void ast_destroy_node(ast_node_t* node);

/**
 * @brief Create a new empty node list.
 * 
 * @return A new node list or NULL if memory allocation failed.
 */
ast_node_list_t* ast_create_node_list(void);

/**
 * @brief Add a node to a node list.
 * 
 * @param list The node list.
 * @param node The node to add.
 * @return true on success, false on memory allocation failure.
 */
bool ast_add_node(ast_node_list_t* list, ast_node_t* node);

/**
 * @brief Destroy a node list and all its nodes.
 * 
 * @param list The list to destroy.
 */
void ast_destroy_node_list(ast_node_list_t* list);

/**
 * @brief Create a module node.
 * 
 * @param name The module name.
 * @return A new module node or NULL if memory allocation failed.
 */
ast_node_t* ast_create_module(const char* name);

/**
 * @brief Create a function node.
 * 
 * @param name The function name.
 * @param return_type The function return type.
 * @return A new function node or NULL if memory allocation failed.
 */
ast_node_t* ast_create_function(const char* name, ast_node_t* return_type);

/**
 * @brief Create a basic block node.
 * 
 * @param label The block label.
 * @return A new basic block node or NULL if memory allocation failed.
 */
ast_node_t* ast_create_block(const char* label);

/**
 * @brief Create an assignment statement node.
 * 
 * @param target The assignment target.
 * @param value The assignment value.
 * @return A new assignment node or NULL if memory allocation failed.
 */
ast_node_t* ast_create_assignment(const char* target, ast_node_t* value);

/**
 * @brief Create an instruction statement node.
 * 
 * @param opcode The instruction opcode.
 * @return A new instruction node or NULL if memory allocation failed.
 */
ast_node_t* ast_create_instruction(const char* opcode);

/**
 * @brief Create an identifier expression node.
 * 
 * @param name The identifier name.
 * @return A new identifier node or NULL if memory allocation failed.
 */
ast_node_t* ast_create_identifier(const char* name);

/**
 * @brief Create an integer expression node.
 * 
 * @param value The integer value.
 * @return A new integer node or NULL if memory allocation failed.
 */
ast_node_t* ast_create_integer(int64_t value);

/**
 * @brief Create a floating point expression node.
 * 
 * @param value The floating point value.
 * @return A new float node or NULL if memory allocation failed.
 */
ast_node_t* ast_create_float(double value);

/**
 * @brief Create a string expression node.
 * 
 * @param value The string value.
 * @return A new string node or NULL if memory allocation failed.
 */
ast_node_t* ast_create_string(const char* value);

/**
 * @brief Copy a source location to a node.
 * 
 * @param node The node to update.
 * @param line The source line.
 * @param column The source column.
 * @param filename The source filename.
 */
void ast_set_location(ast_node_t* node, int line, int column, const char* filename);

/**
 * @brief Check if a node is a type node.
 * 
 * @param node The node to check.
 * @return true if the node is a type node, false otherwise.
 */
bool ast_is_type_node(const ast_node_t* node);

/**
 * @brief Check if a node is an expression node.
 * 
 * @param node The node to check.
 * @return true if the node is an expression node, false otherwise.
 */
bool ast_is_expression_node(const ast_node_t* node);

/**
 * @brief Check if a node is a statement node.
 * 
 * @param node The node to check.
 * @return true if the node is a statement node, false otherwise.
 */
bool ast_is_statement_node(const ast_node_t* node);

#endif /* HOILC_AST_H */