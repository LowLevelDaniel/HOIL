/**
 * @file ast.h
 * @brief Abstract Syntax Tree definitions for HOIL
 * 
 * This file defines the structures that make up the Abstract Syntax Tree (AST)
 * for representing HOIL source code after parsing.
 *
 * @author Generated by Claude
 * @date 2025-03-13
 */

#ifndef AST_H
#define AST_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Forward declarations for AST types
 */
typedef struct ast_module_t ast_module_t;
typedef struct ast_type_def_t ast_type_def_t;
typedef struct ast_function_t ast_function_t;
typedef struct ast_global_t ast_global_t;
typedef struct ast_constant_t ast_constant_t;
typedef struct ast_basic_block_t ast_basic_block_t;
typedef struct ast_instruction_t ast_instruction_t;
typedef struct ast_operand_t ast_operand_t;
typedef struct ast_type_t ast_type_t;
typedef struct ast_target_t ast_target_t;

/**
 * @brief Type category in COIL's type system
 */
typedef enum {
  TYPE_VOID = 0,
  TYPE_BOOLEAN,
  TYPE_INTEGER,
  TYPE_FLOAT,
  TYPE_POINTER,
  TYPE_VECTOR,
  TYPE_ARRAY,
  TYPE_STRUCTURE,
  TYPE_FUNCTION
} type_category_t;

/**
 * @brief Type qualifiers
 */
typedef enum {
  QUALIFIER_NONE = 0,
  QUALIFIER_UNSIGNED = 1 << 0,
  QUALIFIER_CONST = 1 << 1,
  QUALIFIER_VOLATILE = 1 << 2,
  QUALIFIER_RESTRICT = 1 << 3,
  QUALIFIER_ATOMIC = 1 << 4
} type_qualifier_t;

/**
 * @brief Memory spaces for pointers
 */
typedef enum {
  MEMORY_DEFAULT = 0,
  MEMORY_GLOBAL,
  MEMORY_LOCAL,
  MEMORY_SHARED,
  MEMORY_CONSTANT,
  MEMORY_PRIVATE
} memory_space_t;

/**
 * @brief Type definition structure
 */
struct ast_type_t {
  type_category_t category;     /**< Type category */
  uint32_t qualifier_flags;     /**< Type qualifiers */
  
  union {
    // Integer type info
    struct {
      uint32_t bit_width;       /**< Bit width (8, 16, 32, 64) */
      bool is_unsigned;         /**< Whether type is unsigned */
    } integer;
    
    // Float type info
    struct {
      uint32_t bit_width;       /**< Bit width (16, 32, 64) */
    } float_type;
    
    // Pointer type info
    struct {
      ast_type_t* element_type; /**< Type being pointed to */
      memory_space_t space;     /**< Memory space */
    } pointer;
    
    // Vector type info
    struct {
      ast_type_t* element_type; /**< Element type */
      uint32_t element_count;   /**< Number of elements */
    } vector;
    
    // Array type info
    struct {
      ast_type_t* element_type; /**< Element type */
      uint32_t element_count;   /**< Number of elements (0 for unknown/runtime) */
    } array;
    
    // Structure type info
    struct {
      const char* name;         /**< Structure name */
      ast_type_def_t* def;      /**< Structure definition (NULL if forward declaration) */
    } structure;
    
    // Function type info
    struct {
      ast_type_t* return_type;  /**< Return type */
      ast_type_t** param_types; /**< Parameter types */
      uint32_t param_count;     /**< Number of parameters */
      bool is_vararg;           /**< Whether function takes variable arguments */
    } function;
  };
};

/**
 * @brief Structure field definition
 */
typedef struct {
  const char* name;             /**< Field name */
  ast_type_t* type;             /**< Field type */
  uint32_t offset;              /**< Byte offset within structure */
} ast_struct_field_t;

/**
 * @brief Type definition structure
 */
struct ast_type_def_t {
  const char* name;               /**< Type name */
  ast_type_t type;                /**< Base type */
  
  // For structure types
  ast_struct_field_t* fields;     /**< Structure fields */
  uint32_t field_count;           /**< Number of fields */
  uint32_t size;                  /**< Total size in bytes */
  uint32_t alignment;             /**< Alignment requirement in bytes */
};

/**
 * @brief Constant value types
 */
typedef enum {
  CONSTANT_INTEGER,
  CONSTANT_FLOAT,
  CONSTANT_BOOLEAN,
  CONSTANT_STRING,
  CONSTANT_ARRAY,
  CONSTANT_STRUCT
} constant_type_t;

/**
 * @brief Constant value
 */
struct ast_constant_t {
  const char* name;             /**< Constant name */
  ast_type_t* type;             /**< Constant type */
  constant_type_t value_type;   /**< Type of constant value */
  
  union {
    int64_t int_value;          /**< Integer value */
    double float_value;         /**< Float value */
    bool bool_value;            /**< Boolean value */
    const char* string_value;   /**< String value */
    struct {
      ast_constant_t** elements; /**< Array elements */
      uint32_t element_count;    /**< Number of elements */
    } array;
    struct {
      ast_constant_t** fields;   /**< Structure fields */
      uint32_t field_count;      /**< Number of fields */
    } structure;
  };
};

/**
 * @brief Global variable
 */
struct ast_global_t {
  const char* name;             /**< Global name */
  ast_type_t* type;             /**< Global type */
  bool has_initializer;         /**< Whether global has initializer */
  ast_constant_t* initializer;  /**< Initial value (NULL if none) */
};

/**
 * @brief Parameter definition
 */
typedef struct {
  const char* name;             /**< Parameter name */
  ast_type_t* type;             /**< Parameter type */
} ast_parameter_t;

/**
 * @brief Function definition or declaration
 */
struct ast_function_t {
  const char* name;               /**< Function name */
  ast_type_t* return_type;        /**< Return type */
  ast_parameter_t* parameters;    /**< Parameters */
  uint32_t parameter_count;       /**< Number of parameters */
  bool is_vararg;                 /**< Whether function has variable arguments */
  bool is_extern;                 /**< Whether function is external */
  ast_basic_block_t** blocks;     /**< Basic blocks (NULL if extern) */
  uint32_t block_count;           /**< Number of blocks */
  ast_target_t* target;           /**< Target-specific version (NULL if generic) */
};

/**
 * @brief Target requirements
 */
struct ast_target_t {
  const char* device_class;       /**< Target device class */
  const char** required_features; /**< Required features */
  uint32_t required_count;        /**< Number of required features */
  const char** preferred_features;/**< Preferred features */
  uint32_t preferred_count;       /**< Number of preferred features */
};

/**
 * @brief Instruction opcodes
 */
typedef enum {
  // Arithmetic
  OPCODE_ADD,              /**< Addition */
  OPCODE_SUB,              /**< Subtraction */
  OPCODE_MUL,              /**< Multiplication */
  OPCODE_DIV,              /**< Division */
  OPCODE_REM,              /**< Remainder */
  OPCODE_NEG,              /**< Negation */
  OPCODE_ABS,              /**< Absolute value */
  OPCODE_MIN,              /**< Minimum */
  OPCODE_MAX,              /**< Maximum */
  OPCODE_FMA,              /**< Fused multiply-add */
  
  // Logical
  OPCODE_AND,              /**< Bitwise AND */
  OPCODE_OR,               /**< Bitwise OR */
  OPCODE_XOR,              /**< Bitwise XOR */
  OPCODE_NOT,              /**< Bitwise NOT */
  OPCODE_SHL,              /**< Shift left */
  OPCODE_SHR,              /**< Shift right */
  
  // Comparison
  OPCODE_CMP_EQ,           /**< Equal */
  OPCODE_CMP_NE,           /**< Not equal */
  OPCODE_CMP_LT,           /**< Less than */
  OPCODE_CMP_LE,           /**< Less than or equal */
  OPCODE_CMP_GT,           /**< Greater than */
  OPCODE_CMP_GE,           /**< Greater than or equal */
  
  // Memory
  OPCODE_LOAD,             /**< Load from memory */
  OPCODE_STORE,            /**< Store to memory */
  OPCODE_LEA,              /**< Load effective address */
  OPCODE_FENCE,            /**< Memory fence */
  
  // Control Flow
  OPCODE_BR,               /**< Branch */
  OPCODE_CALL,             /**< Function call */
  OPCODE_RET,              /**< Return */
  OPCODE_SWITCH,           /**< Multi-way branch */
  
  // Vector
  OPCODE_VADD,             /**< Vector addition */
  OPCODE_VDOT,             /**< Vector dot product */
  OPCODE_VSPLAT,           /**< Splat scalar to vector */
  OPCODE_VLOAD,            /**< Load vector from memory */
  
  // Atomic
  OPCODE_ATOMIC_ADD,       /**< Atomic add */
  OPCODE_ATOMIC_CAS,       /**< Compare and swap */
  
  // Conversion
  OPCODE_CONVERT,          /**< Type conversion */
  OPCODE_TRUNC,            /**< Truncate */
  OPCODE_EXTEND            /**< Extend */
} opcode_t;

/**
 * @brief Operand types
 */
typedef enum {
  OPERAND_NONE,            /**< No operand */
  OPERAND_LOCAL,           /**< Local variable */
  OPERAND_GLOBAL,          /**< Global variable */
  OPERAND_CONSTANT,        /**< Constant value */
  OPERAND_FUNCTION,        /**< Function reference */
  OPERAND_BLOCK,           /**< Basic block reference */
  OPERAND_REGISTER         /**< Virtual register */
} operand_type_t;

/**
 * @brief Instruction operand
 */
struct ast_operand_t {
  operand_type_t type;     /**< Operand type */
  ast_type_t* value_type;  /**< Type of the value */
  
  union {
    // For local variables and virtual registers
    struct {
      const char* name;    /**< Variable or register name */
    } local;
    
    // For global variables
    struct {
      const char* name;    /**< Global variable name */
    } global;
    
    // For constants
    struct {
      ast_constant_t* constant; /**< Constant value */
    } constant;
    
    // For function references
    struct {
      const char* name;    /**< Function name */
    } function;
    
    // For basic block references
    struct {
      const char* name;    /**< Block name */
    } block;
  };
};

/**
 * @brief Instruction in the AST
 */
struct ast_instruction_t {
  opcode_t opcode;                 /**< Instruction opcode */
  ast_operand_t* result;           /**< Result operand (NULL if no result) */
  ast_operand_t** operands;        /**< Source operands */
  uint32_t operand_count;          /**< Number of operands */
  
  // For CALL instructions
  struct {
    bool is_vararg;                /**< Whether call is to vararg function */
    ast_type_t* return_type;       /**< Return type */
  } call_info;
  
  // For BRANCH instructions
  struct {
    bool is_conditional;           /**< Whether branch is conditional */
    const char* true_label;        /**< True branch target */
    const char* false_label;       /**< False branch target (NULL if unconditional) */
  } branch_info;
  
  // For RET instructions
  struct {
    bool has_value;                /**< Whether return has value */
  } return_info;
};

/**
 * @brief Basic block in a function
 */
struct ast_basic_block_t {
  const char* name;                /**< Block name */
  ast_instruction_t** instructions;/**< Instructions in the block */
  uint32_t instruction_count;      /**< Number of instructions */
  bool is_entry;                   /**< Whether this is an entry block */
};

/**
 * @brief Module structure (top-level container)
 */
struct ast_module_t {
  const char* name;                /**< Module name */
  ast_type_def_t** types;          /**< Type definitions */
  uint32_t type_count;             /**< Number of type definitions */
  ast_global_t** globals;          /**< Global variables */
  uint32_t global_count;           /**< Number of global variables */
  ast_constant_t** constants;      /**< Constants */
  uint32_t constant_count;         /**< Number of constants */
  ast_function_t** functions;      /**< Functions */
  uint32_t function_count;         /**< Number of functions */
  ast_target_t* target;            /**< Target requirements */
};

/**
 * @brief Create a new empty module
 * 
 * @param name Module name
 * @return New module or NULL on error
 */
ast_module_t* ast_module_create(const char* name);

/**
 * @brief Destroy a module and all its contents
 * 
 * @param module Module to destroy
 */
void ast_module_destroy(ast_module_t* module);

/**
 * @brief Create a new type definition
 * 
 * @param name Type name
 * @param category Type category
 * @return New type definition or NULL on error
 */
ast_type_def_t* ast_type_def_create(const char* name, type_category_t category);

/**
 * @brief Create a new basic type
 * 
 * @param category Type category
 * @return New type or NULL on error
 */
ast_type_t* ast_type_create(type_category_t category);

/**
 * @brief Create a new integer type
 * 
 * @param bit_width Bit width (8, 16, 32, 64)
 * @param is_unsigned Whether type is unsigned
 * @return New type or NULL on error
 */
ast_type_t* ast_type_create_integer(uint32_t bit_width, bool is_unsigned);

/**
 * @brief Create a new float type
 * 
 * @param bit_width Bit width (16, 32, 64)
 * @return New type or NULL on error
 */
ast_type_t* ast_type_create_float(uint32_t bit_width);

/**
 * @brief Create a new pointer type
 * 
 * @param element_type Type being pointed to
 * @param space Memory space
 * @return New type or NULL on error
 */
ast_type_t* ast_type_create_pointer(ast_type_t* element_type, memory_space_t space);

/**
 * @brief Create a new vector type
 * 
 * @param element_type Element type
 * @param element_count Number of elements
 * @return New type or NULL on error
 */
ast_type_t* ast_type_create_vector(ast_type_t* element_type, uint32_t element_count);

/**
 * @brief Create a new array type
 * 
 * @param element_type Element type
 * @param element_count Number of elements (0 for unknown/runtime)
 * @return New type or NULL on error
 */
ast_type_t* ast_type_create_array(ast_type_t* element_type, uint32_t element_count);

/**
 * @brief Create a new structure type
 * 
 * @param name Structure name
 * @param def Structure definition (NULL if forward declaration)
 * @return New type or NULL on error
 */
ast_type_t* ast_type_create_structure(const char* name, ast_type_def_t* def);

/**
 * @brief Create a new function type
 * 
 * @param return_type Return type
 * @param param_types Parameter types
 * @param param_count Number of parameters
 * @param is_vararg Whether function takes variable arguments
 * @return New type or NULL on error
 */
ast_type_t* ast_type_create_function(ast_type_t* return_type, ast_type_t** param_types, 
                                    uint32_t param_count, bool is_vararg);

/**
 * @brief Destroy a type
 * 
 * @param type Type to destroy
 */
void ast_type_destroy(ast_type_t* type);

/**
 * @brief Create a new constant
 * 
 * @param name Constant name
 * @param type Constant type
 * @return New constant or NULL on error
 */
ast_constant_t* ast_constant_create(const char* name, ast_type_t* type);

/**
 * @brief Destroy a constant
 * 
 * @param constant Constant to destroy
 */
void ast_constant_destroy(ast_constant_t* constant);

/**
 * @brief Create a new global variable
 * 
 * @param name Global name
 * @param type Global type
 * @return New global or NULL on error
 */
ast_global_t* ast_global_create(const char* name, ast_type_t* type);

/**
 * @brief Destroy a global variable
 * 
 * @param global Global to destroy
 */
void ast_global_destroy(ast_global_t* global);

/**
 * @brief Create a new function
 * 
 * @param name Function name
 * @param return_type Return type
 * @param is_extern Whether function is external
 * @return New function or NULL on error
 */
ast_function_t* ast_function_create(const char* name, ast_type_t* return_type, bool is_extern);

/**
 * @brief Destroy a function
 * 
 * @param function Function to destroy
 */
void ast_function_destroy(ast_function_t* function);

/**
 * @brief Create a new basic block
 * 
 * @param name Block name
 * @param is_entry Whether this is an entry block
 * @return New basic block or NULL on error
 */
ast_basic_block_t* ast_basic_block_create(const char* name, bool is_entry);

/**
 * @brief Destroy a basic block
 * 
 * @param block Basic block to destroy
 */
void ast_basic_block_destroy(ast_basic_block_t* block);

/**
 * @brief Create a new instruction
 * 
 * @param opcode Instruction opcode
 * @return New instruction or NULL on error
 */
ast_instruction_t* ast_instruction_create(opcode_t opcode);

/**
 * @brief Set instruction result
 * 
 * @param instruction Instruction to modify
 * @param result Result operand
 * @return true if successful, false otherwise
 */
bool ast_instruction_set_result(ast_instruction_t* instruction, ast_operand_t* result);

/**
 * @brief Add operand to instruction
 * 
 * @param instruction Instruction to modify
 * @param operand Operand to add
 * @return true if successful, false otherwise
 */
bool ast_instruction_add_operand(ast_instruction_t* instruction, ast_operand_t* operand);

/**
 * @brief Destroy an instruction
 * 
 * @param instruction Instruction to destroy
 */
void ast_instruction_destroy(ast_instruction_t* instruction);

/**
 * @brief Create a new operand
 * 
 * @param type Operand type
 * @param value_type Type of the value
 * @return New operand or NULL on error
 */
ast_operand_t* ast_operand_create(operand_type_t type, ast_type_t* value_type);

/**
 * @brief Create a local variable operand
 * 
 * @param name Variable name
 * @param type Variable type
 * @return New operand or NULL on error
 */
ast_operand_t* ast_operand_create_local(const char* name, ast_type_t* type);

/**
 * @brief Create a global variable operand
 * 
 * @param name Global variable name
 * @param type Global variable type
 * @return New operand or NULL on error
 */
ast_operand_t* ast_operand_create_global(const char* name, ast_type_t* type);

/**
 * @brief Create a constant operand
 * 
 * @param constant Constant value
 * @return New operand or NULL on error
 */
ast_operand_t* ast_operand_create_constant(ast_constant_t* constant);

/**
 * @brief Create a function reference operand
 * 
 * @param name Function name
 * @param type Function type
 * @return New operand or NULL on error
 */
ast_operand_t* ast_operand_create_function(const char* name, ast_type_t* type);

/**
 * @brief Create a basic block reference operand
 * 
 * @param name Block name
 * @return New operand or NULL on error
 */
ast_operand_t* ast_operand_create_block(const char* name);

/**
 * @brief Destroy an operand
 * 
 * @param operand Operand to destroy
 */
void ast_operand_destroy(ast_operand_t* operand);

/**
 * @brief Create a new target specification
 * 
 * @param device_class Target device class
 * @return New target or NULL on error
 */
ast_target_t* ast_target_create(const char* device_class);

/**
 * @brief Add required feature to target
 * 
 * @param target Target to modify
 * @param feature Feature name
 * @return true if successful, false otherwise
 */
bool ast_target_add_required_feature(ast_target_t* target, const char* feature);

/**
 * @brief Add preferred feature to target
 * 
 * @param target Target to modify
 * @param feature Feature name
 * @return true if successful, false otherwise
 */
bool ast_target_add_preferred_feature(ast_target_t* target, const char* feature);

/**
 * @brief Destroy a target
 * 
 * @param target Target to destroy
 */
void ast_target_destroy(ast_target_t* target);

#endif /* AST_H */