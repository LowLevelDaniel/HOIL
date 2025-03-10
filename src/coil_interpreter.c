/**
* @file enhanced_coil_interpreter.c
* @brief Enhanced interpreter for COIL supporting binary format and control flow
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include "coil_format.h"

/** 
* @brief Maximum memory size in bytes
*/
#define MAX_MEMORY_SIZE 8192

/**
* @brief Maximum stack size in bytes
*/
#define MAX_STACK_SIZE 1024

/**
* @brief Maximum number of labels for control flow
*/
#define MAX_LABELS 256

/**
* @brief Label entry structure
*/
typedef struct {
  uint16_t label_id;         /**< Label identifier */
  long file_position;        /**< File position for this label */
} label_entry_t;

/**
* @brief Structure to hold the interpreter state
*/
typedef struct {
  uint8_t memory[MAX_MEMORY_SIZE];  /**< Memory space */
  size_t memory_used;               /**< Amount of memory used */
  
  uint8_t stack[MAX_STACK_SIZE];    /**< Stack space */
  size_t stack_used;                /**< Amount of stack used */
  
  label_entry_t labels[MAX_LABELS]; /**< Label table */
  size_t label_count;               /**< Number of labels defined */
  
  FILE *input_file;                 /**< Input file for reading instructions */
  bool binary_mode;                 /**< Flag for binary input format */
} interpreter_state_t;

/**
* @brief Initialize the interpreter state
* 
* @param state Pointer to the interpreter state
* @param input_file Input file for instructions
* @param binary_mode Flag indicating binary input format
* @return 0 on success, non-zero on failure
*/
int initialize_interpreter(interpreter_state_t *state, FILE *input_file, bool binary_mode) {
  if (!state || !input_file) {
    return -1;
  }
  
  memset(state->memory, 0, MAX_MEMORY_SIZE);
  state->memory_used = 0;
  
  memset(state->stack, 0, MAX_STACK_SIZE);
  state->stack_used = 0;
  
  memset(state->labels, 0, sizeof(label_entry_t) * MAX_LABELS);
  state->label_count = 0;
  
  state->input_file = input_file;
  state->binary_mode = binary_mode;
  
  return 0;
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
* @brief Push a value onto the stack
* 
* @param state Pointer to the interpreter state
* @param value Value to push
* @param size Size of the value in bytes
* @return 0 on success, non-zero on failure
*/
int stack_push(interpreter_state_t *state, const void *value, size_t size) {
  if (state->stack_used + size > MAX_STACK_SIZE) {
    fprintf(stderr, "Stack overflow\n");
    return -1;
  }
  
  memcpy(&state->stack[state->stack_used], value, size);
  state->stack_used += size;
  
  return 0;
}

/**
* @brief Pop a value from the stack
* 
* @param state Pointer to the interpreter state
* @param value Pointer to store the popped value
* @param size Size of the value in bytes
* @return 0 on success, non-zero on failure
*/
int stack_pop(interpreter_state_t *state, void *value, size_t size) {
  if (state->stack_used < size) {
    fprintf(stderr, "Stack underflow\n");
    return -1;
  }
  
  state->stack_used -= size;
  memcpy(value, &state->stack[state->stack_used], size);
  
  return 0;
}

/**
* @brief Add a label to the label table
* 
* @param state Pointer to the interpreter state
* @param label_id Label identifier
* @param file_position File position for this label
* @return 0 on success, non-zero on failure
*/
int add_label(interpreter_state_t *state, uint16_t label_id, long file_position) {
  if (state->label_count >= MAX_LABELS) {
    fprintf(stderr, "Too many labels defined\n");
    return -1;
  }
  
  // Check if label already exists
  for (size_t i = 0; i < state->label_count; i++) {
    if (state->labels[i].label_id == label_id) {
      fprintf(stderr, "Label %u already defined\n", label_id);
      return -1;
    }
  }
  
  state->labels[state->label_count].label_id = label_id;
  state->labels[state->label_count].file_position = file_position;
  state->label_count++;
  
  return 0;
}

/**
* @brief Find a label in the label table
* 
* @param state Pointer to the interpreter state
* @param label_id Label identifier
* @return File position for the label, or -1 if not found
*/
long find_label(interpreter_state_t *state, uint16_t label_id) {
  for (size_t i = 0; i < state->label_count; i++) {
    if (state->labels[i].label_id == label_id) {
      return state->labels[i].file_position;
    }
  }
  
  fprintf(stderr, "Label %u not found\n", label_id);
  return -1;
}

/**
* @brief Read a binary instruction from the input file
* 
* @param state Pointer to the interpreter state
* @param instruction Pointer to store the instruction
* @return 0 on success, 1 on EOF, negative on error
*/
int read_binary_instruction(interpreter_state_t *state, binary_instruction_t *instruction) {
  if (!state || !instruction || !state->input_file) {
    return -1;
  }
  
  // Read the start marker
  if (fread(&instruction->start_marker, sizeof(uint8_t), 1, state->input_file) != 1) {
    if (feof(state->input_file)) {
      return 1; // EOF
    }
    return -1;
  }
  
  // Verify start marker
  if (instruction->start_marker != MARKER_INSTRUCTION) {
    fprintf(stderr, "Invalid instruction marker: %02X\n", instruction->start_marker);
    return -1;
  }
  
  // Read operation code
  if (fread(&instruction->op_code, sizeof(uint16_t), 1, state->input_file) != 1) {
    return -1;
  }
  
  // Read type marker and type
  if (fread(&instruction->type_marker, sizeof(uint8_t), 1, state->input_file) != 1 ||
      instruction->type_marker != MARKER_TYPE) {
    return -1;
  }
  
  if (fread(&instruction->type, sizeof(uint8_t), 1, state->input_file) != 1) {
    return -1;
  }
  
  // Read variable marker and address
  if (fread(&instruction->var_marker, sizeof(uint8_t), 1, state->input_file) != 1 ||
      instruction->var_marker != MARKER_VARIABLE) {
    return -1;
  }
  
  if (fread(&instruction->var_address, sizeof(uint16_t), 1, state->input_file) != 1) {
    return -1;
  }
  
  // Read immediate marker and value
  if (fread(&instruction->imm_marker, sizeof(uint8_t), 1, state->input_file) != 1 ||
      instruction->imm_marker != MARKER_IMMEDIATE) {
    return -1;
  }
  
  if (fread(&instruction->imm_value, sizeof(uint64_t), 1, state->input_file) != 1) {
    return -1;
  }
  
  // Read end marker
  if (fread(&instruction->end_marker, sizeof(uint8_t), 1, state->input_file) != 1 ||
      instruction->end_marker != MARKER_END) {
    return -1;
  }
  
  return 0;
}

/**
* @brief Read a text instruction from the input file
* 
* @param state Pointer to the interpreter state
* @param op_code Pointer to store the operation code
* @param args Array to store arguments
* @param max_args Maximum number of arguments
* @param num_args Pointer to store the number of arguments read
* @return 0 on success, 1 on EOF, negative on error
*/
int read_text_instruction(interpreter_state_t *state, uint16_t *op_code, 
                         uint16_t *args, int max_args, int *num_args) {
  if (!state || !op_code || !args || !num_args || !state->input_file) {
    return -1;
  }
  
  char line[256];
  while (fgets(line, sizeof(line), state->input_file)) {
    // Skip comments and empty lines
    if (line[0] == ';' || line[0] == '\n') {
      continue;
    }
    
    // Parse the operation code and arguments
    *num_args = 0;
    
    char *token = strtok(line, " \t\n");
    if (!token) {
      continue;
    }
    
    *op_code = (uint16_t)strtoul(token, NULL, 16);
    
    token = strtok(NULL, " \t\n");
    while (token && *num_args < max_args) {
      args[(*num_args)++] = (uint16_t)strtoul(token, NULL, 16);
      token = strtok(NULL, " \t\n");
    }
    
    return 0;
  }
  
  if (feof(state->input_file)) {
    return 1; // EOF
  }
  
  return -1;
}

/**
* @brief Execute a binary instruction
* 
* @param state Pointer to the interpreter state
* @param instruction Binary instruction to execute
* @return 0 on success, non-zero on failure
*/
int execute_binary_instruction(interpreter_state_t *state, binary_instruction_t *instruction) {
  switch (instruction->op_code) {
    case OP_ALLOC_IMM: {
      size_t size = get_type_size((mem_type_t)instruction->type);
      if (size == 0) {
        fprintf(stderr, "Invalid memory type: %u\n", instruction->type);
        return -1;
      }
      
      void *ptr = get_memory_ptr(state, instruction->var_address);
      if (!ptr) {
        return -1;
      }
      
      memcpy(ptr, &instruction->imm_value, size);
      
      if (instruction->var_address + size > state->memory_used) {
        state->memory_used = instruction->var_address + size;
      }
      
      break;
    }
    
    case OP_ALLOC_MEM: {
      size_t size = get_type_size((mem_type_t)instruction->type);
      if (size == 0) {
        fprintf(stderr, "Invalid memory type: %u\n", instruction->type);
        return -1;
      }
      
      void *dest = get_memory_ptr(state, instruction->var_address);
      void *src = get_memory_ptr(state, (uint16_t)instruction->imm_value);
      
      if (!dest || !src) {
        return -1;
      }
      
      memcpy(dest, src, size);
      
      if (instruction->var_address + size > state->memory_used) {
        state->memory_used = instruction->var_address + size;
      }
      
      break;
    }
    
    case OP_MOVE: {
      size_t size = get_type_size((mem_type_t)instruction->type);
      if (size == 0) {
        fprintf(stderr, "Invalid memory type: %u\n", instruction->type);
        return -1;
      }
      
      void *dest = get_memory_ptr(state, instruction->var_address);
      void *src = get_memory_ptr(state, (uint16_t)instruction->imm_value);
      
      if (!dest || !src) {
        return -1;
      }
      
      memcpy(dest, src, size);
      
      break;
    }
    
    case OP_ADD: {
      uint16_t src1_addr = (uint16_t)(instruction->imm_value >> 32);
      uint16_t src2_addr = (uint16_t)instruction->imm_value;
      
      int64_t *dest = (int64_t*)get_memory_ptr(state, instruction->var_address);
      int64_t *src1 = (int64_t*)get_memory_ptr(state, src1_addr);
      int64_t *src2 = (int64_t*)get_memory_ptr(state, src2_addr);
      
      if (!dest || !src1 || !src2) {
        return -1;
      }
      
      *dest = *src1 + *src2;
      
      break;
    }
    
    case OP_SUB: {
      uint16_t src1_addr = (uint16_t)(instruction->imm_value >> 32);
      uint16_t src2_addr = (uint16_t)instruction->imm_value;
      
      int64_t *dest = (int64_t*)get_memory_ptr(state, instruction->var_address);
      int64_t *src1 = (int64_t*)get_memory_ptr(state, src1_addr);
      int64_t *src2 = (int64_t*)get_memory_ptr(state, src2_addr);
      
      if (!dest || !src1 || !src2) {
        return -1;
      }
      
      *dest = *src1 - *src2;
      
      break;
    }
    
    case OP_MUL: {
      uint16_t src1_addr = (uint16_t)(instruction->imm_value >> 32);
      uint16_t src2_addr = (uint16_t)instruction->imm_value;
      
      int64_t *dest = (int64_t*)get_memory_ptr(state, instruction->var_address);
      int64_t *src1 = (int64_t*)get_memory_ptr(state, src1_addr);
      int64_t *src2 = (int64_t*)get_memory_ptr(state, src2_addr);
      
      if (!dest || !src1 || !src2) {
        return -1;
      }
      
      *dest = *src1 * *src2;
      
      break;
    }
    
    case OP_DIV: {
      uint16_t src1_addr = (uint16_t)(instruction->imm_value >> 32);
      uint16_t src2_addr = (uint16_t)instruction->imm_value;
      
      int64_t *dest = (int64_t*)get_memory_ptr(state, instruction->var_address);
      int64_t *src1 = (int64_t*)get_memory_ptr(state, src1_addr);
      int64_t *src2 = (int64_t*)get_memory_ptr(state, src2_addr);
      
      if (!dest || !src1 || !src2) {
        return -1;
      }
      
      if (*src2 == 0) {
        fprintf(stderr, "Division by zero\n");
        return -1;
      }
      
      *dest = *src1 / *src2;
      
      break;
    }
    
    case OP_MOD: {
      uint16_t src1_addr = (uint16_t)(instruction->imm_value >> 32);
      uint16_t src2_addr = (uint16_t)instruction->imm_value;
      
      int64_t *dest = (int64_t*)get_memory_ptr(state, instruction->var_address);
      int64_t *src1 = (int64_t*)get_memory_ptr(state, src1_addr);
      int64_t *src2 = (int64_t*)get_memory_ptr(state, src2_addr);
      
      if (!dest || !src1 || !src2) {
        return -1;
      }
      
      if (*src2 == 0) {
        fprintf(stderr, "Modulo by zero\n");
        return -1;
      }
      
      *dest = *src1 % *src2;
      
      break;
    }
    
    case OP_JMP: {
      long file_pos = find_label(state, (uint16_t)instruction->imm_value);
      if (file_pos < 0) {
        return -1;
      }
      
      if (fseek(state->input_file, file_pos, SEEK_SET) != 0) {
        perror("fseek");
        return -1;
      }
      
      break;
    }
    
    case OP_JEQ: {
      uint16_t src1_addr = (uint16_t)(instruction->imm_value >> 48);
      uint16_t src2_addr = (uint16_t)(instruction->imm_value >> 32);
      uint16_t label_id = (uint16_t)instruction->imm_value;
      
      int64_t *src1 = (int64_t*)get_memory_ptr(state, src1_addr);
      int64_t *src2 = (int64_t*)get_memory_ptr(state, src2_addr);
      
      if (!src1 || !src2) {
        return -1;
      }
      
      if (*src1 == *src2) {
        long file_pos = find_label(state, label_id);
        if (file_pos < 0) {
          return -1;
        }
        
        if (fseek(state->input_file, file_pos, SEEK_SET) != 0) {
          perror("fseek");
          return -1;
        }
      }
      
      break;
    }
    
    case OP_JNE: {
      uint16_t src1_addr = (uint16_t)(instruction->imm_value >> 48);
      uint16_t src2_addr = (uint16_t)(instruction->imm_value >> 32);
      uint16_t label_id = (uint16_t)instruction->imm_value;
      
      int64_t *src1 = (int64_t*)get_memory_ptr(state, src1_addr);
      int64_t *src2 = (int64_t*)get_memory_ptr(state, src2_addr);
      
      if (!src1 || !src2) {
        return -1;
      }
      
      if (*src1 != *src2) {
        long file_pos = find_label(state, label_id);
        if (file_pos < 0) {
          return -1;
        }
        
        if (fseek(state->input_file, file_pos, SEEK_SET) != 0) {
          perror("fseek");
          return -1;
        }
      }
      
      break;
    }
    
    case OP_PUSH: {
      size_t size = get_type_size((mem_type_t)instruction->type);
      if (size == 0) {
        fprintf(stderr, "Invalid memory type: %u\n", instruction->type);
        return -1;
      }
      
      void *src = get_memory_ptr(state, instruction->var_address);
      if (!src) {
        return -1;
      }
      
      if (stack_push(state, src, size) != 0) {
        return -1;
      }
      
      break;
    }
    
    case OP_POP: {
      size_t size = get_type_size((mem_type_t)instruction->type);
      if (size == 0) {
        fprintf(stderr, "Invalid memory type: %u\n", instruction->type);
        return -1;
      }
      
      void *dest = get_memory_ptr(state, instruction->var_address);
      if (!dest) {
        return -1;
      }
      
      if (stack_pop(state, dest, size) != 0) {
        return -1;
      }
      
      break;
    }
    
    case OP_SYSCALL: {
      // Read syscall arguments from next instruction
      binary_instruction_t arg_instruction;
      long current_pos = ftell(state->input_file);
      
      if (read_binary_instruction(state, &arg_instruction) == 0 && 
          arg_instruction.op_code == 0xFFFF) {
        
        // Arguments are available in the arg_instruction
        uint16_t syscall_num = (uint16_t)instruction->imm_value;
        uint16_t *args = (uint16_t*)&arg_instruction.imm_value;
        
        switch (syscall_num) {
          case 1: { // write
            int fd = args[0];
            uint16_t buf_addr = args[1];
            size_t count = args[2];
            
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
            int status = args[0];
            exit(status);
            break;
          }
          
          default:
            fprintf(stderr, "Unsupported syscall: %u\n", syscall_num);
            return -1;
        }
      } else {
        // No arguments, reset file position
        fseek(state->input_file, current_pos, SEEK_SET);
        
        uint16_t syscall_num = (uint16_t)instruction->imm_value;
        
        switch (syscall_num) {
          case 60: { // exit with default status 0
            exit(0);
            break;
          }
          
          default:
            fprintf(stderr, "Unsupported syscall without arguments: %u\n", syscall_num);
            return -1;
        }
      }
      
      break;
    }
    
    default:
      fprintf(stderr, "Unsupported operation code: %04X\n", instruction->op_code);
      return -1;
  }
  
  return 0;
}

/**
* @brief Execute a text instruction
* 
* @param state Pointer to the interpreter state
* @param op_code Operation code
* @param args Array of arguments
* @param num_args Number of arguments
* @return 0 on success, non-zero on failure
*/
int execute_text_instruction(interpreter_state_t *state, uint16_t op_code, 
                            uint16_t *args, int num_args) {
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
    
    // ... Other operations similar to the original coil_interpreter.c ...
    
    default:
      fprintf(stderr, "Unsupported operation code: %04X\n", op_code);
      return -1;
  }
  
  return 0;
}

/**
* @brief First pass to collect all labels
* 
* @param state Pointer to the interpreter state
* @return 0 on success, non-zero on failure
*/
int collect_labels(interpreter_state_t *state) {
  if (!state || !state->input_file) {
    return -1;
  }
  
  long initial_pos = ftell(state->input_file);
  
  if (state->binary_mode) {
    binary_instruction_t instruction;
    int result = 0;
    
    // Scan through the file looking for label definitions
    while ((result = read_binary_instruction(state, &instruction)) == 0) {
      if (instruction.op_code == 0xFFFE) { // Label definition
        uint16_t label_id = instruction.var_address;
        long label_pos = ftell(state->input_file);
        
        if (add_label(state, label_id, label_pos) != 0) {
          return -1;
        }
      }
    }
    
    if (result < 0) {
      fprintf(stderr, "Error reading binary instruction during label collection\n");
      return -1;
    }
  } else {
    uint16_t op_code;
    uint16_t args[16];
    int num_args;
    
    // Scan through the file looking for label definitions
    while (read_text_instruction(state, &op_code, args, 16, &num_args) == 0) {
      if (op_code == 0xFFFE && num_args >= 1) { // Label definition
        uint16_t label_id = args[0];
        long label_pos = ftell(state->input_file);
        
        if (add_label(state, label_id, label_pos) != 0) {
          return -1;
        }
      }
    }
  }
  
  // Reset file position
  if (fseek(state->input_file, initial_pos, SEEK_SET) != 0) {
    perror("fseek");
    return -1;
  }
  
  return 0;
}

/**
* @brief Interpret COIL instructions from a file
* 
* @param filename Path to the COIL file
* @param binary_mode Flag indicating binary input format
* @return 0 on success, non-zero on failure
*/
int interpret_coil_file(const char *filename, bool binary_mode) {
  FILE *file = fopen(filename, binary_mode ? "rb" : "r");
  if (!file) {
    perror("fopen");
    return -1;
  }
  
  interpreter_state_t state;
  if (initialize_interpreter(&state, file, binary_mode) != 0) {
    fprintf(stderr, "Failed to initialize interpreter\n");
    fclose(file);
    return -1;
  }
  
  // First pass to collect labels
  if (collect_labels(&state) != 0) {
    fprintf(stderr, "Failed to collect labels\n");
    fclose(file);
    return -1;
  }
  
  // Second pass to execute instructions
  if (binary_mode) {
    binary_instruction_t instruction;
    int result;
    
    while ((result = read_binary_instruction(&state, &instruction)) == 0) {
      if (instruction.op_code != 0xFFFE) { // Skip label definitions
        if (execute_binary_instruction(&state, &instruction) != 0) {
          fprintf(stderr, "Failed to execute binary instruction\n");
          fclose(file);
          return -1;
        }
      }
    }
    
    if (result < 0) {
      fprintf(stderr, "Error reading binary instruction\n");
      fclose(file);
      return -1;
    }
  } else {
    uint16_t op_code;
    uint16_t args[16];
    int num_args;
    
    while (read_text_instruction(&state, &op_code, args, 16, &num_args) == 0) {
      if (op_code != 0xFFFE) { // Skip label definitions
        if (execute_text_instruction(&state, op_code, args, num_args) != 0) {
          fprintf(stderr, "Failed to execute text instruction\n");
          fclose(file);
          return -1;
        }
      }
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
    fprintf(stderr, "Usage: %s [-b] <coil_file>\n", argv[0]);
    fprintf(stderr, "  -b: Binary mode (default is text mode)\n");
    return 1;
  }
  
  bool binary_mode = false;
  const char *filename = argv[1];
  
  if (strcmp(argv[1], "-b") == 0) {
    if (argc < 3) {
      fprintf(stderr, "Missing filename after -b option\n");
      return 1;
    }
    
    binary_mode = true;
    filename = argv[2];
  }
  
  return interpret_coil_file(filename, binary_mode);
}