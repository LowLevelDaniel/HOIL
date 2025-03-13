/**
 * @file binary.h
 * @brief COIL binary format handling.
 * 
 * This header defines the structures and functions for handling COIL binary format.
 * 
 * @author HOILC Team
 * @date 2025
 */

#ifndef HOILC_BINARY_H
#define HOILC_BINARY_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/**
 * @brief Magic identifier for COIL binary format.
 */
#define COIL_MAGIC 0x434F494C  /* "COIL" in ASCII */

/**
 * @brief Section type definitions.
 */
typedef enum {
  SECTION_TYPE,      /**< Type section. */
  SECTION_FUNCTION,  /**< Function section. */
  SECTION_GLOBAL,    /**< Global section. */
  SECTION_CONSTANT,  /**< Constant section. */
  SECTION_CODE,      /**< Code section. */
  SECTION_RELOCATION, /**< Relocation section. */
  SECTION_METADATA,  /**< Metadata section. */
  
  SECTION_COUNT      /**< Number of section types. */
} section_type_t;

/**
 * @brief Type category definitions.
 */
typedef enum {
  TYPE_VOID = 0x00,   /**< Void type. */
  TYPE_BOOLEAN = 0x01, /**< Boolean type. */
  TYPE_INTEGER = 0x02, /**< Integer type. */
  TYPE_FLOAT = 0x03,   /**< Floating point type. */
  TYPE_POINTER = 0x04, /**< Pointer type. */
  TYPE_VECTOR = 0x05,  /**< Vector type. */
  TYPE_ARRAY = 0x06,   /**< Array type. */
  TYPE_STRUCTURE = 0x07, /**< Structure type. */
  TYPE_FUNCTION = 0x08, /**< Function type. */
} type_category_t;

/**
 * @brief Type qualifier flags.
 */
typedef enum {
  QUALIFIER_UNSIGNED = 0x01, /**< Unsigned integer. */
  QUALIFIER_CONST = 0x02,    /**< Constant value. */
  QUALIFIER_VOLATILE = 0x04, /**< Volatile memory access. */
  QUALIFIER_RESTRICT = 0x08, /**< Non-aliased pointer. */
  QUALIFIER_ATOMIC = 0x10,   /**< Atomic access. */
} type_qualifier_t;

/**
 * @brief Memory space definitions.
 */
typedef enum {
  MEMORY_GLOBAL,     /**< Global memory. */
  MEMORY_LOCAL,      /**< Thread-local memory. */
  MEMORY_SHARED,     /**< Shared memory. */
  MEMORY_CONSTANT,   /**< Constant memory. */
  MEMORY_PRIVATE,    /**< Private memory. */
} memory_space_t;

/**
 * @brief Memory ordering definitions.
 */
typedef enum {
  ORDER_RELAXED,     /**< Relaxed ordering. */
  ORDER_ACQUIRE,     /**< Acquire ordering. */
  ORDER_RELEASE,     /**< Release ordering. */
  ORDER_ACQ_REL,     /**< Acquire-release ordering. */
  ORDER_SEQ_CST,     /**< Sequentially consistent ordering. */
} memory_order_t;

/**
 * @brief COIL file header.
 */
typedef struct {
  uint32_t magic;          /**< Magic identifier. */
  uint32_t version;        /**< Version information. */
  uint32_t section_count;  /**< Number of sections. */
  uint32_t flags;          /**< Module flags. */
} coil_header_t;

/**
 * @brief Section header.
 */
typedef struct {
  uint32_t section_type;   /**< Type of section. */
  uint32_t offset;         /**< Offset from start of file. */
  uint32_t size;           /**< Size of section in bytes. */
} section_header_t;

/**
 * @brief Type encoding.
 * 
 * Format: [category:4][width:8][qualifiers:8][attributes:12]
 */
typedef uint32_t type_encoding_t;

/**
 * @brief Instruction format.
 */
typedef struct {
  uint8_t opcode;          /**< Instruction opcode. */
  uint8_t flags;           /**< Instruction flags. */
  uint8_t operand_count;   /**< Number of source operands. */
  uint8_t destination;     /**< Destination register/value. */
  /* Followed by operands */
} instruction_t;

/**
 * @brief COIL binary builder.
 */
typedef struct coil_builder coil_builder_t;

/**
 * @brief Create a new COIL binary builder.
 * 
 * @return A new builder or NULL if memory allocation failed.
 */
coil_builder_t* coil_builder_create(void);

/**
 * @brief Destroy a COIL binary builder.
 * 
 * @param builder The builder to destroy.
 */
void coil_builder_destroy(coil_builder_t* builder);

/**
 * @brief Set the module name.
 * 
 * @param builder The builder.
 * @param name The module name.
 * @return true on success, false on failure.
 */
bool coil_builder_set_module_name(coil_builder_t* builder, const char* name);

/**
 * @brief Add a type definition.
 * 
 * @param builder The builder.
 * @param encoding The type encoding.
 * @param name The type name (can be NULL).
 * @return The type index or -1 on failure.
 */
int32_t coil_builder_add_type(coil_builder_t* builder, type_encoding_t encoding, const char* name);

/**
 * @brief Add a structure type.
 * 
 * @param builder The builder.
 * @param field_types Array of field type indices.
 * @param field_count Number of fields.
 * @param name The structure name (can be NULL).
 * @return The structure type index or -1 on failure.
 */
int32_t coil_builder_add_struct_type(coil_builder_t* builder, int32_t* field_types, 
                                     uint32_t field_count, const char* name);

/**
 * @brief Add a function declaration.
 * 
 * @param builder The builder.
 * @param name The function name.
 * @param return_type The return type index.
 * @param param_types Array of parameter type indices.
 * @param param_count Number of parameters.
 * @param is_external Whether the function is external.
 * @return The function index or -1 on failure.
 */
int32_t coil_builder_add_function(coil_builder_t* builder, const char* name, 
                                 int32_t return_type, int32_t* param_types, 
                                 uint32_t param_count, bool is_external);

/**
 * @brief Add a global variable.
 * 
 * @param builder The builder.
 * @param name The global variable name.
 * @param type The variable type index.
 * @param initializer The constant initializer data (can be NULL).
 * @param initializer_size The size of the initializer in bytes.
 * @return The global variable index or -1 on failure.
 */
int32_t coil_builder_add_global(coil_builder_t* builder, const char* name, 
                               int32_t type, const void* initializer, 
                               size_t initializer_size);

/**
 * @brief Begin adding code to a function.
 * 
 * @param builder The builder.
 * @param function The function index.
 * @return true on success, false on failure.
 */
bool coil_builder_begin_function_code(coil_builder_t* builder, int32_t function);

/**
 * @brief Add a basic block to the current function.
 * 
 * @param builder The builder.
 * @param name The block name.
 * @return The block index or -1 on failure.
 */
int32_t coil_builder_add_block(coil_builder_t* builder, const char* name);

/**
 * @brief Add an instruction to the current block.
 * 
 * @param builder The builder.
 * @param opcode The instruction opcode.
 * @param flags The instruction flags.
 * @param destination The destination register.
 * @param operands Array of operand values.
 * @param operand_count Number of operands.
 * @return true on success, false on failure.
 */
bool coil_builder_add_instruction(coil_builder_t* builder, uint8_t opcode, 
                                  uint8_t flags, uint8_t destination, 
                                  uint8_t* operands, uint8_t operand_count);

/**
 * @brief End adding code to the current function.
 * 
 * @param builder The builder.
 * @return true on success, false on failure.
 */
bool coil_builder_end_function_code(coil_builder_t* builder);

/**
 * @brief Build the COIL binary.
 * 
 * @param builder The builder.
 * @param output Pointer to store the output binary.
 * @param size Pointer to store the output size.
 * @return true on success, false on failure.
 */
bool coil_builder_build(coil_builder_t* builder, uint8_t** output, size_t* size);

/**
 * @brief Create a predefined type encoding.
 * 
 * @param category The type category.
 * @param width The type width in bits.
 * @param qualifiers The type qualifiers.
 * @param attributes The type attributes.
 * @return The type encoding.
 */
type_encoding_t coil_create_type_encoding(type_category_t category, uint8_t width, 
                                          uint8_t qualifiers, uint16_t attributes);

/**
 * @brief Get a predefined type encoding.
 * 
 * @param type The predefined type.
 * @return The type encoding.
 */
type_encoding_t coil_get_predefined_type(int type);

/**
 * @brief Predefined type constants.
 */
enum {
  PREDEFINED_VOID,    /**< Void type. */
  PREDEFINED_BOOL,    /**< Boolean type. */
  PREDEFINED_INT8,    /**< 8-bit signed integer. */
  PREDEFINED_UINT8,   /**< 8-bit unsigned integer. */
  PREDEFINED_INT16,   /**< 16-bit signed integer. */
  PREDEFINED_UINT16,  /**< 16-bit unsigned integer. */
  PREDEFINED_INT32,   /**< 32-bit signed integer. */
  PREDEFINED_UINT32,  /**< 32-bit unsigned integer. */
  PREDEFINED_INT64,   /**< 64-bit signed integer. */
  PREDEFINED_UINT64,  /**< 64-bit unsigned integer. */
  PREDEFINED_FLOAT16, /**< 16-bit floating point. */
  PREDEFINED_FLOAT32, /**< 32-bit floating point. */
  PREDEFINED_FLOAT64, /**< 64-bit floating point. */
  PREDEFINED_PTR,     /**< Generic pointer. */
  
  PREDEFINED_COUNT    /**< Number of predefined types. */
};

/**
 * @brief Instruction opcode constants.
 */
enum {
  /* Arithmetic instructions */
  OPCODE_ADD = 0x01,  /**< Addition. */
  OPCODE_SUB = 0x02,  /**< Subtraction. */
  OPCODE_MUL = 0x03,  /**< Multiplication. */
  OPCODE_DIV = 0x04,  /**< Division. */
  OPCODE_REM = 0x05,  /**< Remainder. */
  OPCODE_NEG = 0x06,  /**< Negation. */
  OPCODE_ABS = 0x07,  /**< Absolute value. */
  OPCODE_MIN = 0x08,  /**< Minimum. */
  OPCODE_MAX = 0x09,  /**< Maximum. */
  OPCODE_FMA = 0x0A,  /**< Fused multiply-add. */
  
  /* Logical instructions */
  OPCODE_AND = 0x10,  /**< Bitwise AND. */
  OPCODE_OR  = 0x11,  /**< Bitwise OR. */
  OPCODE_XOR = 0x12,  /**< Bitwise XOR. */
  OPCODE_NOT = 0x13,  /**< Bitwise NOT. */
  OPCODE_SHL = 0x14,  /**< Shift left. */
  OPCODE_SHR = 0x15,  /**< Shift right. */
  
  /* Comparison instructions */
  OPCODE_CMP_EQ = 0x20, /**< Equal. */
  OPCODE_CMP_NE = 0x21, /**< Not equal. */
  OPCODE_CMP_LT = 0x22, /**< Less than. */
  OPCODE_CMP_LE = 0x23, /**< Less than or equal. */
  OPCODE_CMP_GT = 0x24, /**< Greater than. */
  OPCODE_CMP_GE = 0x25, /**< Greater than or equal. */
  
  /* Memory instructions */
  OPCODE_LOAD = 0x30,  /**< Load from memory. */
  OPCODE_STORE = 0x31, /**< Store to memory. */
  OPCODE_LEA = 0x32,   /**< Load effective address. */
  OPCODE_FENCE = 0x33, /**< Memory fence. */
  
  /* Control flow instructions */
  OPCODE_BR = 0x40,     /**< Branch. */
  OPCODE_BR_COND = 0x41, /**< Conditional branch. */
  OPCODE_SWITCH = 0x42, /**< Switch. */
  OPCODE_CALL = 0x43,   /**< Function call. */
  OPCODE_RET = 0x44,    /**< Function return. */
};

#endif /* HOILC_BINARY_H */