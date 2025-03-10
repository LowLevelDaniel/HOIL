/**
* @file coil_vm.c
* @brief Virtual Machine for COIL
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include "coil_format.h"
#include "coil_vm.h"

/**
* @brief Initialize the VM state
* 
* @param state Pointer to the VM state
* @param input_file Input file for instructions
* @param binary_mode Flag for binary input format
* @return 0 on success, non-zero on failure
*/
int initialize_vm(vm_state_t *state, FILE *input_file, bool binary_mode) {
  if (!state || !input_file) {
    return -1;
  }
  
  // Initialize static memory
  memset(state->memory, 0, STATIC_MEMORY_SIZE);
  state->memory_used = 0;
  
  // Initialize stack
  memset(state->stack, 0, STACK_SIZE);
  state->stack_used = 0;
  
  // Initialize call stack
  memset(state->call_stack, 0, sizeof(long) * CALL_STACK_SIZE);
  state->call_stack_used = 0;
  
  // Initialize labels
  memset(state->labels, 0, sizeof(label_entry_t) * MAX_LABELS);
  state->label_count = 0;
  
  state->input_file = input_file;
  state->binary_mode = binary_mode;
  
  state->instruction_count = 0;
  state->running = true;
  state->exit_code = 0;
  
  return 0;
}

/**
* @brief Get a pointer to a memory address
* 
* @param state Pointer to the VM state
* @param addr Memory address
* @return Pointer to the memory location, or NULL if invalid
*/
void* get_memory_ptr(vm_state_t *state, uint16_t addr) {
  if (addr >= STATIC_MEMORY_SIZE) {
    fprintf(stderr, "Memory access out of bounds: %u\n", addr);
    return NULL;
  }
  
  return &state->memory[addr];
}

/**
* @brief Push a value onto the stack
* 
* @param state Pointer to the VM state
* @param value Value to push
* @param size Size of the value in bytes
* @return 0 on success, non-zero on failure
*/
int stack_push(vm_state_t *state, const void *value, size_t size) {
  if (state->stack_used + size > STACK_SIZE) {
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
* @param state Pointer to the VM state
* @param value Pointer to store the popped value
* @param size Size of the value in bytes
* @return 0 on success, non-zero on failure
*/
int stack_pop(vm_state_t *state, void *value, size_t size) {
  if (state->stack_used < size) {
    fprintf(stderr, "Stack underflow\n");
    return -1;
  }
  
  state->stack_used -= size;
  memcpy(value, &state->stack[state->stack_used], size);
  
  return 0;
}

/**
* @brief Push a return address onto the call stack
* 
* @param state Pointer to the VM state
* @param return_addr Return address
* @return 0 on success, non-zero on failure
*/
int call_stack_push(vm_state_t *state, long return_addr) {
  if (state->call_stack_used >= CALL_STACK_SIZE) {
    fprintf(stderr, "Call stack overflow\n");
    return -1;
  }
  
  state->call_stack[state->call_stack_used++] = return_addr;
  return 0;
}

/**
* @brief Pop a return address from the call stack
* 
* @param state Pointer to the VM state
* @param return_addr Pointer to store the return address
* @return 0 on success, non-zero on failure
*/
int call_stack_pop(vm_state_t *state, long *return_addr) {
  if (state->call_stack_used == 0) {
    fprintf(stderr, "Call stack underflow\n");
    return -1;
  }
  
  *return_addr = state->call_stack[--state->call_stack_used];
  return 0;
}

/**
* @brief Add a label to the label table
* 
* @param state Pointer to the VM state
* @param label_id Label identifier
* @param file_position File position for this label
* @return 0 on success, non-zero on failure
*/
int add_label(vm_state_t *state, uint16_t label_id, long file_position) {
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
* @param state Pointer to the VM state
* @param label_id Label identifier
* @return File position for the label, or -1 if not found
*/
long find_label(vm_state_t *state, uint16_t label_id) {
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
* @param state Pointer to the VM state
* @param instruction Pointer to store the instruction
* @return 0 on success, 1 on EOF, negative on error
*/
int read_binary_instruction(vm_state_t *state, binary_instruction_t *instruction) {
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
* @brief First pass to collect all labels
* 
* @param state Pointer to the VM state
* @return 0 on success, non-zero on failure
*/
int collect_labels(vm_state_t *state) {
  if (!state || !state->input_file) {
    return -1;
  }
  
  long initial_pos = ftell(state->input_file);
  
  if (state->binary_mode) {
    binary_instruction_t instruction;
    int result = 0;
    
    // Scan through the file looking for label definitions
    while ((result = read_binary_instruction(state, &instruction)) == 0) {
      if (instruction.op_code == OP_LABEL_DEF) { // Label definition
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
    // Text mode label collection (not implemented for brevity)
    fprintf(stderr, "Text mode not supported\n");
    return -1;
  }
  
  // Reset file position
  if (fseek(state->input_file, initial_pos, SEEK_SET) != 0) {
    perror("fseek");
    return -1;
  }
  
  return 0;
}

/**
* @brief Execute a binary instruction
* 
* @param state Pointer to the VM state
* @param instruction Binary instruction to execute
* @return 0 on success, non-zero on failure
*/
int execute_instruction(vm_state_t *state, binary_instruction_t *instruction) {
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
    
    case OP_JLT: {
      uint16_t src1_addr = (uint16_t)(instruction->imm_value >> 48);
      uint16_t src2_addr = (uint16_t)(instruction->imm_value >> 32);
      uint16_t label_id = (uint16_t)instruction->imm_value;
      
      int64_t *src1 = (int64_t*)get_memory_ptr(state, src1_addr);
      int64_t *src2 = (int64_t*)get_memory_ptr(state, src2_addr);
      
      if (!src1 || !src2) {
        return -1;
      }
      
      if (*src1 < *src2) {
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
    
    case OP_JLE: {
      uint16_t src1_addr = (uint16_t)(instruction->imm_value >> 48);
      uint16_t src2_addr = (uint16_t)(instruction->imm_value >> 32);
      uint16_t label_id = (uint16_t)instruction->imm_value;
      
      int64_t *src1 = (int64_t*)get_memory_ptr(state, src1_addr);
      int64_t *src2 = (int64_t*)get_memory_ptr(state, src2_addr);
      
      if (!src1 || !src2) {
        return -1;
      }
      
      if (*src1 <= *src2) {
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
    
    case OP_JGT: {
      uint16_t src1_addr = (uint16_t)(instruction->imm_value >> 48);
      uint16_t src2_addr = (uint16_t)(instruction->imm_value >> 32);
      uint16_t label_id = (uint16_t)instruction->imm_value;
      
      int64_t *src1 = (int64_t*)get_memory_ptr(state, src1_addr);
      int64_t *src2 = (int64_t*)get_memory_ptr(state, src2_addr);
      
      if (!src1 || !src2) {
        return -1;
      }
      
      if (*src1 > *src2) {
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
    
    case OP_JGE: {
      uint16_t src1_addr = (uint16_t)(instruction->imm_value >> 48);
      uint16_t src2_addr = (uint16_t)(instruction->imm_value >> 32);
      uint16_t label_id = (uint16_t)instruction->imm_value;
      
      int64_t *src1 = (int64_t*)get_memory_ptr(state, src1_addr);
      int64_t *src2 = (int64_t*)get_memory_ptr(state, src2_addr);
      
      if (!src1 || !src2) {
        return -1;
      }
      
      if (*src1 >= *src2) {
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
    
    case OP_CALL: {
      uint16_t label_id = (uint16_t)instruction->imm_value;
      
      long return_addr = ftell(state->input_file);
      if (return_addr < 0) {
        perror("ftell");
        return -1;
      }
      
      if (call_stack_push(state, return_addr) != 0) {
        return -1;
      }
      
      long file_pos = find_label(state, label_id);
      if (file_pos < 0) {
        return -1;
      }
      
      if (fseek(state->input_file, file_pos, SEEK_SET) != 0) {
        perror("fseek");
        return -1;
      }
      
      break;
    }
    
    case OP_RET: {
      long return_addr;
      
      if (call_stack_pop(state, &return_addr) != 0) {
        return -1;
      }
      
      if (fseek(state->input_file, return_addr, SEEK_SET) != 0) {
        perror("fseek");
        return -1;
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
          arg_instruction.op_code == OP_ARG_DATA) {
        
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
            state->exit_code = status;
            state->running = false;
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
            state->exit_code = 0;
            state->running = false;
            break;
          }
          
          default:
            fprintf(stderr, "Unsupported syscall without arguments: %u\n", syscall_num);
            return -1;
        }
      }
      
      break;
    }
    
    case OP_EXIT: {
      state->exit_code = (int)instruction->imm_value;
      state->running = false;
      break;
    }
    
    case OP_LABEL_DEF: // Label definition - skip
      break;
      
    default:
      fprintf(stderr, "Unsupported operation code: %04X\n", instruction->op_code);
      return -1;
  }
  
  return 0;
}

/**
* @brief Run the virtual machine
* 
* @param state Pointer to the VM state
* @return Exit code from the program
*/
int run_vm(vm_state_t *state) {
  binary_instruction_t instruction;
  int result;
  
  while (state->running) {
    // Read next instruction
    if (state->binary_mode) {
      result = read_binary_instruction(state, &instruction);
    } else {
      // Text mode not implemented for brevity
      fprintf(stderr, "Text mode not supported\n");
      return -1;
    }
    
    if (result != 0) {
      if (result > 0) {
        // End of file reached
        break;
      } else {
        fprintf(stderr, "Error reading instruction\n");
        return -1;
      }
    }
    
    // Execute the instruction
    if (execute_instruction(state, &instruction) != 0) {
      fprintf(stderr, "Error executing instruction\n");
      return -1;
    }
    
    state->instruction_count++;
  }
  
  return state->exit_code;
}

/**
* @brief Print VM statistics
* 
* @param state Pointer to the VM state
*/
void print_statistics(vm_state_t *state) {
  printf("\nVM Statistics:\n");
  printf("  Instructions executed: %lu\n", state->instruction_count);
  printf("  Memory used: %lu bytes\n", state->memory_used);
  printf("  Memory limit: %d bytes\n", STATIC_MEMORY_SIZE);
  printf("  Stack used: %lu bytes\n", state->stack_used);
  printf("  Stack limit: %d bytes\n", STACK_SIZE);
  printf("  Call stack depth: %lu\n", state->call_stack_used);
  printf("  Call stack limit: %d\n", CALL_STACK_SIZE);
  printf("  Exit code: %d\n", state->exit_code);
}

/**
* @brief Main function
* 
* @param argc Number of command-line arguments
* @param argv Array of command-line arguments
* @return Exit code
*/
int main(int argc, char *argv[]) {
  if (argc < 2) {
    fprintf(stderr, "Usage: %s [-b] [-s] <coil_file>\n", argv[0]);
    fprintf(stderr, "  -b: Binary mode (default is text mode)\n");
    fprintf(stderr, "  -s: Show statistics after execution\n");
    return 1;
  }
  
  bool binary_mode = false;
  bool stats_mode = false;
  const char *filename = argv[1];
  int arg_index = 1;
  
  // Parse arguments
  while (arg_index < argc && argv[arg_index][0] == '-') {
    if (strcmp(argv[arg_index], "-b") == 0) {
      binary_mode = true;
    } else if (strcmp(argv[arg_index], "-s") == 0) {
      stats_mode = true;
    } else {
      fprintf(stderr, "Unknown option: %s\n", argv[arg_index]);
      return 1;
    }
    
    arg_index++;
  }
  
  if (arg_index >= argc) {
    fprintf(stderr, "Missing filename\n");
    return 1;
  }
  
  filename = argv[arg_index];
  
  FILE *file = fopen(filename, binary_mode ? "rb" : "r");
  if (!file) {
    perror("Failed to open file");
    return 1;
  }
  
  vm_state_t state;
  if (initialize_vm(&state, file, binary_mode) != 0) {
    fprintf(stderr, "Failed to initialize VM\n");
    fclose(file);
    return 1;
  }
  
  // First pass to collect labels
  if (collect_labels(&state) != 0) {
    fprintf(stderr, "Failed to collect labels\n");
    fclose(file);
    return 1;
  }
  
  // Run the VM
  int exit_code = run_vm(&state);
  
  // Print statistics if requested
  if (stats_mode) {
    print_statistics(&state);
  }
  
  // Clean up
  fclose(file);
  
  return exit_code;
}