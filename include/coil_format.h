/**
* @file coil_format.h
* @brief Definition of the COIL instruction format
*
* This file contains definitions specific to the COIL (Computer Oriented Intermediate 
* Language) format, which serves as a target for HOIL compilation and as input for 
* the COIL VM. This format is intended to be stable and usable by other implementations.
*/

#ifndef COIL_FORMAT_H
#define COIL_FORMAT_H

#include <stdint.h>
#include <stddef.h>

/**
* @brief Instruction marker types
*/
typedef enum {
  MARKER_INSTRUCTION = 0xC0,  /**< Start of instruction marker */
  MARKER_VARIABLE    = 0xC1,  /**< Variable reference marker */
  MARKER_IMMEDIATE   = 0xC2,  /**< Immediate value marker */
  MARKER_TYPE        = 0xC3,  /**< Type specifier marker */
  MARKER_END         = 0xCF   /**< End of instruction marker */
} marker_type_t;

/**
* @brief Operation codes for COIL instructions
*/
typedef enum {
  /* Memory operations: 0x00xx */
  OP_ALLOC_IMM = 0x0001,  /**< Allocate memory with immediate value */
  OP_ALLOC_MEM = 0x0002,  /**< Allocate memory with value from another address */
  OP_MOVE      = 0x0003,  /**< Move value from src to dest */
  OP_LOAD      = 0x0004,  /**< Load value from memory address */
  OP_STORE     = 0x0005,  /**< Store value to memory address */
  
  /* Arithmetic operations: 0x01xx */
  OP_ADD       = 0x0101,  /**< Add values */
  OP_SUB       = 0x0102,  /**< Subtract values */
  OP_MUL       = 0x0103,  /**< Multiply values */
  OP_DIV       = 0x0104,  /**< Divide values */
  OP_MOD       = 0x0105,  /**< Modulo operation */
  OP_NEG       = 0x0106,  /**< Negate value */
  
  /* Bitwise operations: 0x02xx */
  OP_AND       = 0x0201,  /**< Bitwise AND */
  OP_OR        = 0x0202,  /**< Bitwise OR */
  OP_XOR       = 0x0203,  /**< Bitwise XOR */
  OP_NOT       = 0x0204,  /**< Bitwise NOT */
  OP_SHL       = 0x0205,  /**< Shift left */
  OP_SHR       = 0x0206,  /**< Shift right */
  
  /* Control flow: 0x03xx */
  OP_JMP       = 0x0301,  /**< Unconditional jump */
  OP_JEQ       = 0x0302,  /**< Jump if equal */
  OP_JNE       = 0x0303,  /**< Jump if not equal */
  OP_JLT       = 0x0304,  /**< Jump if less than */
  OP_JLE       = 0x0305,  /**< Jump if less or equal */
  OP_JGT       = 0x0306,  /**< Jump if greater than */
  OP_JGE       = 0x0307,  /**< Jump if greater or equal */
  
  /* Function operations: 0x04xx */
  OP_CALL      = 0x0401,  /**< Call function */
  OP_RET       = 0x0402,  /**< Return from function */
  OP_PUSH      = 0x0403,  /**< Push value onto stack */
  OP_POP       = 0x0404,  /**< Pop value from stack */
  
  /* System operations: 0x05xx */
  OP_SYSCALL   = 0x0501,  /**< System call */
  OP_EXIT      = 0x0502,  /**< Exit program */

  /* Special codes */
  OP_LABEL_DEF = 0xFFFE,  /**< Label definition (not an actual instruction) */
  OP_ARG_DATA  = 0xFFFF   /**< Argument data (used with other instructions) */
} op_code_t;

/**
* @brief Memory type codes
*/
typedef enum {
  MEM_INT8     = 0x01,  /**< 8-bit integer */
  MEM_INT16    = 0x02,  /**< 16-bit integer */
  MEM_INT32    = 0x04,  /**< 32-bit integer */
  MEM_INT64    = 0x08,  /**< 64-bit integer (default) */
  MEM_UINT8    = 0x11,  /**< 8-bit unsigned integer */
  MEM_UINT16   = 0x12,  /**< 16-bit unsigned integer */
  MEM_UINT32   = 0x14,  /**< 32-bit unsigned integer */
  MEM_UINT64   = 0x18,  /**< 64-bit unsigned integer */
  MEM_FLOAT32  = 0x24,  /**< 32-bit floating point */
  MEM_FLOAT64  = 0x28,  /**< 64-bit floating point */
  MEM_PTR      = 0x40,  /**< Pointer type */
  MEM_BOOL     = 0x81   /**< Boolean type (1 byte) */
} mem_type_t;

/**
* @brief Binary instruction format structure
*/
typedef struct {
  uint8_t start_marker;    /**< Start of instruction marker (MARKER_INSTRUCTION) */
  uint16_t op_code;        /**< Operation code */
  uint8_t type_marker;     /**< Type marker (MARKER_TYPE) */
  uint8_t type;            /**< Memory type */
  uint8_t var_marker;      /**< Variable marker (MARKER_VARIABLE) */
  uint16_t var_address;    /**< Variable address */
  uint8_t imm_marker;      /**< Immediate value marker (MARKER_IMMEDIATE) */
  uint64_t imm_value;      /**< Immediate value */
  uint8_t end_marker;      /**< End of instruction marker (MARKER_END) */
} binary_instruction_t;

/**
* @brief Get the size of a memory type in bytes
* 
* @param type Memory type code
* @return Size in bytes, or 0 if type is invalid
*/
static inline size_t get_type_size(mem_type_t type) {
  switch (type) {
    case MEM_INT8:
    case MEM_UINT8:
    case MEM_BOOL:
      return 1;
    case MEM_INT16:
    case MEM_UINT16:
      return 2;
    case MEM_INT32:
    case MEM_UINT32:
    case MEM_FLOAT32:
      return 4;
    case MEM_INT64:
    case MEM_UINT64:
    case MEM_FLOAT64:
    case MEM_PTR:
      return 8;
    default:
      return 0;
  }
}

/**
* @brief Check if a memory type is signed
* 
* @param type Memory type code
* @return 1 if signed, 0 if unsigned
*/
static inline int is_signed_type(mem_type_t type) {
  return (type == MEM_INT8 || type == MEM_INT16 || 
          type == MEM_INT32 || type == MEM_INT64 ||
          type == MEM_FLOAT32 || type == MEM_FLOAT64);
}

/**
* @brief Check if a memory type is floating point
* 
* @param type Memory type code
* @return 1 if floating point, 0 if integer
*/
static inline int is_float_type(mem_type_t type) {
  return (type == MEM_FLOAT32 || type == MEM_FLOAT64);
}

#endif /* COIL_FORMAT_H */