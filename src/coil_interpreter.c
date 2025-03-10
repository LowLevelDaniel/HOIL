/**
* @file coil_interpreter.c
* @brief Interpreter for COIL (Computer Oriented Intermediate Language)
* 
* This interpreter reads COIL instructions and executes them.
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

/** 
* @brief Maximum memory size in bytes
*/
#define MAX_MEMORY_SIZE 4096

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
  MEM_INT64 = 0x0008   /**< 64-bit integer */
} mem_type_t;

/**
* @brief Structure to hold the interpreter state
*/
typedef struct {
  uint8_t memory[MAX_MEMORY_SIZE];  /**< Memory space */
  size_t memory_used;               /**< Amount of memory used */
} interpreter_state_t;

/**
* @brief Initialize the interpreter state
* 
* @param state Pointer to the interpreter state
* @return 0 on success, non-zero on failure
*/
int initialize_interpreter(interpreter_state_t *state) {
  if (!state) {
    return -1;
  }
  
  memset(state->memory, 0, MAX_MEMORY_SIZE);
  state->memory_used = 0;
  
  return 0;
}

/**
* @brief Get the size of a memory type
* 
* @param type Memory type code
* @return Size in bytes
*/
size_t get_type_size(mem_type_t type) {
  switch (type) {
    case MEM_INT8:  return 1;
    case MEM_INT16: return 2;
    case MEM_INT32: return 4;
    case MEM_INT64: return 8;
    default:        return 0;
  }
}

/**
* @brief Get a pointer to a memory address
* 
* @param state Pointer to the interpreter state
* @param addr Memory address
* @return Pointer to the memory location
*/
void* get_memory_ptr(interpreter_state_t *state, uint16_t addr) {
  if (addr >= MAX_MEMORY_SIZE) {
    fprintf(stderr, "Memory access out of bounds: %u\n", addr);
    return NULL;
  }
  
  return &state->memory[addr];
}

/**
* @brief Execute a COIL instruction
* 
* @param state Pointer to the interpreter state
* @param op_code Operation code
* @param args Array of arguments
* @param num_args Number of arguments
* @return 0 on success, non-zero on failure
*/
int execute_instruction(interpreter_state_t *state, op_code_t op_code, uint16_t *args, int num_args) {
  switch (op_code) {
    case OP_ALLOC_IMM: {
      if (num_args < 3) {
        fprintf(stderr, "ALLOC_IMM requires 3 arguments\n");
        return -1;
      }
      
      mem_type_t type = (mem_type_t)args[0];
      uint16_t dest_addr = args[1];
      int64_t value = args[2];
      
      size_t size = get_type_size(type);
      if (size == 0) {
        fprintf(stderr, "Invalid memory type: %u\n", type);
        return -1;
      }
      
      void *ptr = get_memory_ptr(state, dest_addr);
      if (!ptr) {
        return -1;
      }
      
      memcpy(ptr, &value, size);
      
      if (dest_addr + size > state->memory_used) {
        state->memory_used = dest_addr + size;
      }
      
      break;
    }
    
    case OP_ALLOC_MEM: {
      if (num_args < 3) {
        fprintf(stderr, "ALLOC_MEM requires 3 arguments\n");
        return -1;
      }
      
      mem_type_t type = (mem_type_t)args[0];
      uint16_t dest_addr = args[1];
      uint16_t src_addr = args[2];
      
      size_t size = get_type_size(type);
      if (size == 0) {
        fprintf(stderr, "Invalid memory type: %u\n", type);
        return -1;
      }
      
      void *dest = get_memory_ptr(state, dest_addr);
      void *src = get_memory_ptr(state, src_addr);
      
      if (!dest || !src) {
        return -1;
      }
      
      memcpy(dest, src, size);
      
      if (dest_addr + size > state->memory_used) {
        state->memory_used = dest_addr + size;
      }
      
      break;
    }
    
    case OP_MOVE: {
      if (num_args < 2) {
        fprintf(stderr, "MOVE requires 2 arguments\n");
        return -1;
      }
      
      uint16_t dest_addr = args[0];
      uint16_t src_addr = args[1];
      
      // Assume 8 bytes (int64) for move operations
      size_t size = 8;
      
      void *dest = get_memory_ptr(state, dest_addr);
      void *src = get_memory_ptr(state, src_addr);
      
      if (!dest || !src) {
        return -1;
      }
      
      memcpy(dest, src, size);
      
      break;
    }
    
    case OP_ADD: {
      if (num_args < 3) {
        fprintf(stderr, "ADD requires 3 arguments\n");
        return -1;
      }
      
      uint16_t dest_addr = args[0];
      uint16_t src_addr1 = args[1];
      uint16_t src_addr2 = args[2];
      
      int64_t *dest = (int64_t*)get_memory_ptr(state, dest_addr);
      int64_t *src1 = (int64_t*)get_memory_ptr(state, src_addr1);
      int64_t *src2 = (int64_t*)get_memory_ptr(state, src_addr2);
      
      if (!dest || !src1 || !src2) {
        return -1;
      }
      
      *dest = *src1 + *src2;
      
      if (dest_addr + sizeof(int64_t) > state->memory_used) {
        state->memory_used = dest_addr + sizeof(int64_t);
      }
      
      break;
    }
    
    case OP_SUB: {
      if (num_args < 3) {
        fprintf(stderr, "SUB requires 3 arguments\n");
        return -1;
      }
      
      uint16_t dest_addr = args[0];
      uint16_t src_addr1 = args[1];
      uint16_t src_addr2 = args[2];
      
      int64_t *dest = (int64_t*)get_memory_ptr(state, dest_addr);
      int64_t *src1 = (int64_t*)get_memory_ptr(state, src_addr1);
      int64_t *src2 = (int64_t*)get_memory_ptr(state, src_addr2);
      
      if (!dest || !src1 || !src2) {
        return -1;
      }
      
      *dest = *src1 - *src2;
      
      if (dest_addr + sizeof(int64_t) > state->memory_used) {
        state->memory_used = dest_addr + sizeof(int64_t);
      }
      
      break;
    }
    
    case OP_MUL: {
      if (num_args < 3) {
        fprintf(stderr, "MUL requires 3 arguments\n");
        return -1;
      }
      
      uint16_t dest_addr = args[0];
      uint16_t src_addr1 = args[1];
      uint16_t src_addr2 = args[2];
      
      int64_t *dest = (int64_t*)get_memory_ptr(state, dest_addr);
      int64_t *src1 = (int64_t*)get_memory_ptr(state, src_addr1);
      int64_t *src2 = (int64_t*)get_memory_ptr(state, src_addr2);
      
      if (!dest || !src1 || !src2) {
        return -1;
      }
      
      *dest = *src1 * *src2;
      
      if (dest_addr + sizeof(int64_t) > state->memory_used) {
        state->memory_used = dest_addr + sizeof(int64_t);
      }
      
      break;
    }
    
    case OP_DIV: {
      if (num_args < 3) {
        fprintf(stderr, "DIV requires 3 arguments\n");
        return -1;
      }
      
      uint16_t dest_addr = args[0];
      uint16_t src_addr1 = args[1];
      uint16_t src_addr2 = args[2];
      
      int64_t *dest = (int64_t*)get_memory_ptr(state, dest_addr);
      int64_t *src1 = (int64_t*)get_memory_ptr(state, src_addr1);
      int64_t *src2 = (int64_t*)get_memory_ptr(state, src_addr2);
      
      if (!dest || !src1 || !src2) {
        return -1;
      }
      
      if (*src2 == 0) {
        fprintf(stderr, "Division by zero\n");
        return -1;
      }
      
      *dest = *src1 / *src2;
      
      if (dest_addr + sizeof(int64_t) > state->memory_used) {
        state->memory_used = dest_addr + sizeof(int64_t);
      }
      
      break;
    }
    
    case OP_SYSCALL: {
      if (num_args < 1) {
        fprintf(stderr, "SYSCALL requires at least 1 argument\n");
        return -1;
      }
      
      uint16_t syscall_num = args[0];
      
      switch (syscall_num) {
        case 1: { // write
          if (num_args < 4) {
            fprintf(stderr, "write syscall requires 3 arguments\n");
            return -1;
          }
          
          int fd = args[1];
          uint16_t buf_addr = args[2];
          size_t count = args[3];
          
          void *buf = get_memory_ptr(state, buf_addr);
          if (!buf) {
            return -1;
          }
          
          ssize_t result = write(fd, buf, count);
          if (result < 0) {
            perror("write");
            return -1;
          }
          
          break;
        }
        
        case 60: { // exit
          if (num_args < 2) {
            fprintf(stderr, "exit syscall requires 1 argument\n");
            return -1;
          }
          
          int status = args[1];
          exit(status);
          break;
        }
        
        default:
          fprintf(stderr, "Unsupported syscall: %u\n", syscall_num);
          return -1;
      }
      
      break;
    }
    
    default:
      fprintf(stderr, "Unsupported operation code: %04X\n", op_code);
      return -1;
  }
  
  return 0;
}

/**
* @brief Parse and execute COIL instructions from a file
* 
* @param filename Path to the COIL file
* @return 0 on success, non-zero on failure
*/
int interpret_coil_file(const char *filename) {
  FILE *file = fopen(filename, "r");
  if (!file) {
    perror("fopen");
    return -1;
  }
  
  interpreter_state_t state;
  if (initialize_interpreter(&state) != 0) {
    fprintf(stderr, "Failed to initialize interpreter\n");
    fclose(file);
    return -1;
  }
  
  char line[256];
  while (fgets(line, sizeof(line), file)) {
    // Skip comments and empty lines
    if (line[0] == ';' || line[0] == '\n') {
      continue;
    }
    
    // Parse the operation code and arguments
    op_code_t op_code;
    uint16_t args[16];
    int num_args = 0;
    
    char *token = strtok(line, " \t\n");
    if (!token) {
      continue;
    }
    
    op_code = (op_code_t)strtoul(token, NULL, 16);
    
    token = strtok(NULL, " \t\n");
    while (token && num_args < 16) {
      args[num_args++] = (uint16_t)strtoul(token, NULL, 16);
      token = strtok(NULL, " \t\n");
    }
    
    if (execute_instruction(&state, op_code, args, num_args) != 0) {
      fprintf(stderr, "Failed to execute instruction\n");
      fclose(file);
      return -1;
    }
  }
  
  fclose(file);
  return 0;
}

/**
* @brief Main function
* 
* @param argc Number of command-line arguments
* @param argv Array of command-line arguments
* @return 0 on success, non-zero on failure
*/
int main(int argc, char *argv[]) {
  if (argc < 2) {
    fprintf(stderr, "Usage: %s <coil_file>\n", argv[0]);
    return 1;
  }
  
  return interpret_coil_file(argv[1]);
}