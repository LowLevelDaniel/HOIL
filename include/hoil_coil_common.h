/**
* @file hoil_coil_common.h
* @brief Common definitions for HOIL/COIL tools
*/

#ifndef HOIL_COIL_COMMON_H
#define HOIL_COIL_COMMON_H

#include <stdint.h>

/**
* @brief Operation codes for COIL instructions
*/
typedef enum {
  OP_ALLOC_IMM = 0x0001,  /**< Allocate memory with immediate value */
  OP_ALLOC_MEM = 0x0002,  /**< Allocate memory with value from another address */
  OP_MOVE      = 0x0003,  /**< Move value from src to dest */
  
  OP_ADD       = 0x0101,  /**< Add values */
  OP_SUB       = 0x0102,  /**< Subtract values */
  OP_MUL       = 0x0103,  /**< Multiply values */
  OP_DIV       = 0x0104,  /**< Divide values */
  
  OP_SYSCALL   = 0x0201   /**< System call */
} op_code_t;

/**
* @brief Memory type codes
*/
typedef enum {
  MEM_INT8  = 0x0001,  /**< 8-bit integer */
  MEM_INT16 = 0x0002,  /**< 16-bit integer */
  MEM_INT32 = 0x0004,  /**< 32-bit integer */
  MEM_INT64 = 0x0008   /**< 64-bit integer (default) */
} mem_type_t;

/**
* @brief Get the size of a memory type
* 
* @param type Memory type code
* @return Size in bytes
*/
static inline size_t get_type_size(mem_type_t type) {
  switch (type) {
    case MEM_INT8:  return 1;
    case MEM_INT16: return 2;
    case MEM_INT32: return 4;
    case MEM_INT64: return 8;
    default:        return 0;
  }
}

#endif /* HOIL_COIL_COMMON_H */