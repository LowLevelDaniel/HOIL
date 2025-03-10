/**
* @file coil_debugger.c
* @brief Interactive debugger for COIL programs
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>
#include <ctype.h>
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
* @brief Maximum number of breakpoints
*/
#define MAX_BREAKPOINTS 16

/**
* @brief Maximum number of labels
*/
#define MAX_LABELS 256

/**
* @brief Maximum input line length
*/
#define MAX_LINE_LENGTH 256

/**
* @brief Label entry structure
*/
typedef struct {
  uint16_t label_id;         /**< Label identifier */
  long file_position;        /**< File position for this label */
  char name[64];             /**< Label name (if available) */
} label_entry_t;

/**
* @brief Structure to hold the debugger state
*/
typedef struct {
  uint8_t memory[MAX_MEMORY_SIZE];     /**< Memory space */
  size_t memory_used;                  /**< Amount of memory used */
  
  uint8_t stack[MAX_STACK_SIZE];       /**< Stack space */
  size_t stack_used;                   /**< Amount of stack used */
  
  label_entry_t labels[MAX_LABELS];    /**< Label table */
  size_t label_count;                  /**< Number of labels defined */
  
  long breakpoints[MAX_BREAKPOINTS];   /**< Breakpoint positions */
  size_t breakpoint_count;             /**< Number of breakpoints defined */
  
  FILE* input_file;                    /**< Input file for instructions */
  bool binary_mode;                    /**< Flag for binary input format */
  long current_position;               /**< Current file position */
  
  bool step_mode;                      /**< Single-step execution mode */
  bool running;                        /**< Execution running flag */
  
  uint64_t instruction_count;          /**< Number of instructions executed */
} debugger_state_t;

/**
* @brief Initialize the debugger state
* 
* @param state Pointer to the debugger state
* @param input_file Input file for instructions
* @param binary_mode Flag indicating binary input format
* @return 0 on success, non-zero on failure
*/
int initialize_debugger(debugger_state_t *state, FILE *input_file, bool binary_mode) {
  if (!state || !input_file) {
    return -1;
  }
  
  memset(state->memory, 0, MAX_MEMORY_SIZE);
  state->memory_used = 0;
  
  memset(state->stack, 0, MAX_STACK_SIZE);
  state->stack_used = 0;
  
  memset(state->labels, 0, sizeof(label_entry_t) * MAX_LABELS);
  state->label_count = 0;
  
  memset(state->breakpoints, 0, sizeof(long) * MAX_BREAKPOINTS);
  state->breakpoint_count = 0;
  
  state->input_file = input_file;
  state->binary_mode = binary_mode;
  state->current_position = 0;
  
  state->step_mode = true;  // Start in step mode
  state->running = false;
  
  state->instruction_count = 0;
  
  return 0;
}

/**
* @brief Get a pointer to a memory address
* 
* @param state Pointer to the debugger state
* @param addr Memory address
* @return Pointer to the memory location
*/
void* get_memory_ptr(debugger_state_t *state, uint16_t addr) {
  if (addr >= MAX_MEMORY_SIZE) {
    fprintf(stderr, "Memory access out of bounds: %u\n", addr);
    return NULL;
  }
  
  return &state->memory[addr];
}

/**
* @brief Push a value onto the stack
* 
* @param state Pointer to the debugger state
* @param value Value to push
* @param size Size of the value in bytes
* @return 0 on success, non-zero on failure
*/
int stack_push(debugger_state_t *state, const void *value, size_t size) {
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
* @param state Pointer to the debugger state
* @param value Pointer to store the popped value
* @param size Size of the value in bytes
* @return 0 on success, non-zero on failure
*/
int stack_pop(debugger_state_t *state, void *value, size_t size) {
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
* @param state Pointer to the debugger state
* @param label_id Label identifier
* @param file_position File position for this label
* @param name Label name (if available)
* @return 0 on success, non-zero on failure
*/
int add_label(debugger_state_t *state, uint16_t label_id, long file_position, const char *name) {
  if (state->label_count >= MAX_LABELS) {
    fprintf(stderr, "Too many labels defined\n");
    return -1;
  }
  
  // Check if label already exists
  for (size_t i = 0; i < state->label_count; i++) {
    if (state->labels[i].label_id == label_id) {
      // Update existing label
      state->labels[i].file_position = file_position;
      if (name) {
        strncpy(state->labels[i].name, name, sizeof(state->labels[i].name) - 1);
        state->labels[i].name[sizeof(state->labels[i].name) - 1] = '\0';
      }
      return 0;
    }
  }
  
  // Add new label
  state->labels[state->label_count].label_id = label_id;
  state->labels[state->label_count].file_position = file_position;
  
  if (name) {
    strncpy(state->labels[state->label_count].name, name, sizeof(state->labels[0].name) - 1);
    state->labels[state->label_count].name[sizeof(state->labels[0].name) - 1] = '\0';
  } else {
    snprintf(state->labels[state->label_count].name, sizeof(state->labels[0].name), "L%u", label_id);
  }
  
  state->label_count++;
  
  return 0;
}

/**
* @brief Find a label in the label table
* 
* @param state Pointer to the debugger state
* @param label_id Label identifier
* @return File position for the label, or -1 if not found
*/
long find_label(debugger_state_t *state, uint16_t label_id) {
  for (size_t i = 0; i < state->label_count; i++) {
    if (state->labels[i].label_id == label_id) {
      return state->labels[i].file_position;
    }
  }
  
  fprintf(stderr, "Label %u not found\n", label_id);
  return -1;
}

/**
* @brief Find a label name by file position
* 
* @param state Pointer to the debugger state
* @param file_position File position to look up
* @return Label name or NULL if not found
*/
const char* find_label_by_position(debugger_state_t *state, long file_position) {
  for (size_t i = 0; i < state->label_count; i++) {
    if (state->labels[i].file_position == file_position) {
      return state->labels[i].name;
    }
  }
  
  return NULL;
}

/**
* @brief Add a breakpoint
* 
* @param state Pointer to the debugger state
* @param file_position File position for the breakpoint
* @return 0 on success, non-zero on failure
*/
int add_breakpoint(debugger_state_t *state, long file_position) {
  if (state->breakpoint_count >= MAX_BREAKPOINTS) {
    fprintf(stderr, "Too many breakpoints defined\n");
    return -1;
  }
  
  // Check if breakpoint already exists
  for (size_t i = 0; i < state->breakpoint_count; i++) {
    if (state->breakpoints[i] == file_position) {
      fprintf(stderr, "Breakpoint already exists at this position\n");
      return -1;
    }
  }
  
  state->breakpoints[state->breakpoint_count++] = file_position;
  printf("Breakpoint added at position %ld\n", file_position);
  return 0;
}

/**
* @brief Remove a breakpoint
* 
* @param state Pointer to the debugger state
* @param index Breakpoint index to remove
* @return 0 on success, non-zero on failure
*/
int remove_breakpoint(debugger_state_t *state, size_t index) {
  if (index >= state->breakpoint_count) {
    fprintf(stderr, "Invalid breakpoint index\n");
    return -1;
  }
  
  printf("Removed breakpoint at position %ld\n", state->breakpoints[index]);
  
  // Shift remaining breakpoints
  for (size_t i = index; i < state->breakpoint_count - 1; i++) {
    state->breakpoints[i] = state->breakpoints[i + 1];
  }
  
  state->breakpoint_count--;
  return 0;
}

/**
* @brief Check if current position matches a breakpoint
* 
* @param state Pointer to the debugger state
* @return true if at breakpoint, false otherwise
*/
bool is_at_breakpoint(debugger_state_t *state) {
  for (size_t i = 0; i < state->breakpoint_count; i++) {
    if (state->breakpoints[i] == state->current_position) {
      return true;
    }
  }
  
  return false;
}

/**
* @brief Read a binary instruction from the input file
* 
* @param state Pointer to the debugger state
* @param instruction Pointer to store the instruction
* @return 0 on success, 1 on EOF, negative on error
*/
int read_binary_instruction(debugger_state_t *state, binary_instruction_t *instruction) {
  if (!state || !instruction || !state->input_file) {
    return -1;
  }
  
  // Store current position
  state->current_position = ftell(state->input_file);
  
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
* @param state Pointer to the debugger state
* @param op_code Pointer to store the operation code
* @param args Array to store arguments
* @param max_args Maximum number of arguments
* @param num_args Pointer to store the number of arguments read
* @return 0 on success, 1 on EOF, negative on error
*/
int read_text_instruction(debugger_state_t *state, uint16_t *op_code, 
                         uint16_t *args, int max_args, int *num_args) {
  if (!state || !op_code || !args || !num_args || !state->input_file) {
    return -1;
  }
  
  char line[256];
  
  // Store current position
  state->current_position = ftell(state->input_file);
  
  while (fgets(line, sizeof(line), state->input_file)) {
    // Skip comments and empty lines
    if (line[0] == ';' || line[0] == '\n') {
      state->current_position = ftell(state->input_file);
      continue;
    }
    
    // Parse the operation code and arguments
    *num_args = 0;
    
    char *token = strtok(line, " \t\n");
    if (!token) {
      state->current_position = ftell(state->input_file);
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
* @brief Format a value as string based on type
*
* @param value Value to format
* @param type Memory type
* @param buffer Buffer to store the formatted string
* @param buffer_size Size of the buffer
*/
void format_value_by_type(uint64_t value, mem_type_t type, char *buffer, size_t buffer_size) {
  switch (type) {
    case MEM_INT8:
      snprintf(buffer, buffer_size, "%d", (int8_t)value);
      break;
    case MEM_INT16:
      snprintf(buffer, buffer_size, "%d", (int16_t)value);
      break;
    case MEM_INT32:
      snprintf(buffer, buffer_size, "%d", (int32_t)value);
      break;
    case MEM_INT64:
      snprintf(buffer, buffer_size, "%ld", (int64_t)value);
      break;
    case MEM_UINT8:
      snprintf(buffer, buffer_size, "%u", (uint8_t)value);
      break;
    case MEM_UINT16:
      snprintf(buffer, buffer_size, "%u", (uint16_t)value);
      break;
    case MEM_UINT32:
      snprintf(buffer, buffer_size, "%u", (uint32_t)value);
      break;
    case MEM_UINT64:
      snprintf(buffer, buffer_size, "%lu", (uint64_t)value);
      break;
    case MEM_FLOAT32: {
      float f;
      memcpy(&f, &value, sizeof(float));
      snprintf(buffer, buffer_size, "%f", f);
      break;
    }
    case MEM_FLOAT64: {
      double d;
      memcpy(&d, &value, sizeof(double));
      snprintf(buffer, buffer_size, "%f", d);
      break;
    }
    case MEM_BOOL:
      snprintf(buffer, buffer_size, "%s", (value ? "true" : "false"));
      break;
    case MEM_PTR:
      snprintf(buffer, buffer_size, "0x%lx", (uint64_t)value);
      break;
    default:
      snprintf(buffer, buffer_size, "0x%lx", (uint64_t)value);
  }
}

/**
* @brief Get operation name from opcode
*
* @param op_code Operation code
* @return String representation of the operation
*/
const char* get_op_name(uint16_t op_code) {
  switch (op_code) {
    case OP_ALLOC_IMM: return "ALLOC_IMM";
    case OP_ALLOC_MEM: return "ALLOC_MEM";
    case OP_MOVE: return "MOVE";
    case OP_LOAD: return "LOAD";
    case OP_STORE: return "STORE";
    case OP_ADD: return "ADD";
    case OP_SUB: return "SUB";
    case OP_MUL: return "MUL";
    case OP_DIV: return "DIV";
    case OP_MOD: return "MOD";
    case OP_NEG: return "NEG";
    case OP_AND: return "AND";
    case OP_OR: return "OR";
    case OP_XOR: return "XOR";
    case OP_NOT: return "NOT";
    case OP_SHL: return "SHL";
    case OP_SHR: return "SHR";
    case OP_JMP: return "JMP";
    case OP_JEQ: return "JEQ";
    case OP_JNE: return "JNE";
    case OP_JLT: return "JLT";
    case OP_JLE: return "JLE";
    case OP_JGT: return "JGT";
    case OP_JGE: return "JGE";
    case OP_CALL: return "CALL";
    case OP_RET: return "RET";
    case OP_PUSH: return "PUSH";
    case OP_POP: return "POP";
    case OP_SYSCALL: return "SYSCALL";
    case OP_EXIT: return "EXIT";
    case 0xFFFE: return "LABEL";
    case 0xFFFF: return "ARG";
    default: return "UNKNOWN";
  }
}

/**
* @brief Get type name from memory type
*
* @param type Memory type
* @return String representation of the type
*/
const char* get_type_name(mem_type_t type) {
  switch (type) {
    case MEM_INT8: return "int8";
    case MEM_INT16: return "int16";
    case MEM_INT32: return "int32";
    case MEM_INT64: return "int64";
    case MEM_UINT8: return "uint8";
    case MEM_UINT16: return "uint16";
    case MEM_UINT32: return "uint32";
    case MEM_UINT64: return "uint64";
    case MEM_FLOAT32: return "float32";
    case MEM_FLOAT64: return "float64";
    case MEM_BOOL: return "bool";
    case MEM_PTR: return "ptr";
    default: return "unknown";
  }
}

/**
* @brief Print the current instruction in human-readable format
*
* @param state Pointer to the debugger state
* @param instruction Current instruction
*/
void print_instruction(debugger_state_t *state, binary_instruction_t *instruction) {
  const char *label_name = find_label_by_position(state, state->current_position);
  if (label_name) {
    printf("[%s] ", label_name);
  } else {
    printf("[%08lx] ", state->current_position);
  }
  
  printf("%-10s ", get_op_name(instruction->op_code));
  
  // Format based on operation
  switch (instruction->op_code) {
    case OP_ALLOC_IMM: {
      char value_str[32];
      format_value_by_type(instruction->imm_value, instruction->type, value_str, sizeof(value_str));
      printf("%s @%u = %s", get_type_name(instruction->type), instruction->var_address, value_str);
      break;
    }
    
    case OP_ALLOC_MEM:
      printf("%s @%u = @%lu", 
             get_type_name(instruction->type), 
             instruction->var_address, 
             instruction->imm_value);
      break;
    
    case OP_MOVE:
      printf("%s @%u <- @%lu", 
             get_type_name(instruction->type), 
             instruction->var_address, 
             instruction->imm_value);
      break;
      
    case OP_ADD:
    case OP_SUB:
    case OP_MUL:
    case OP_DIV:
    case OP_MOD: {
      uint16_t src1_addr = (uint16_t)(instruction->imm_value >> 32);
      uint16_t src2_addr = (uint16_t)instruction->imm_value;
      printf("@%u = @%u %s @%u", instruction->var_address, 
             src1_addr, 
             (instruction->op_code == OP_ADD) ? "+" :
             (instruction->op_code == OP_SUB) ? "-" :
             (instruction->op_code == OP_MUL) ? "*" :
             (instruction->op_code == OP_DIV) ? "/" : "%",
             src2_addr);
      break;
    }
    
    case OP_JMP: {
      uint16_t label_id = (uint16_t)instruction->imm_value;
      const char *target_name = NULL;
      for (size_t i = 0; i < state->label_count; i++) {
        if (state->labels[i].label_id == label_id) {
          target_name = state->labels[i].name;
          break;
        }
      }
      if (target_name) {
        printf("-> %s", target_name);
      } else {
        printf("-> L%u", label_id);
      }
      break;
    }
    
    case OP_JEQ:
    case OP_JNE:
    case OP_JLT:
    case OP_JLE:
    case OP_JGT:
    case OP_JGE: {
      uint16_t src1_addr = (uint16_t)(instruction->imm_value >> 48);
      uint16_t src2_addr = (uint16_t)(instruction->imm_value >> 32);
      uint16_t label_id = (uint16_t)instruction->imm_value;
      const char *target_name = NULL;
      for (size_t i = 0; i < state->label_count; i++) {
        if (state->labels[i].label_id == label_id) {
          target_name = state->labels[i].name;
          break;
        }
      }
      
      const char *cond = 
        (instruction->op_code == OP_JEQ) ? "==" :
        (instruction->op_code == OP_JNE) ? "!=" :
        (instruction->op_code == OP_JLT) ? "<" :
        (instruction->op_code == OP_JLE) ? "<=" :
        (instruction->op_code == OP_JGT) ? ">" : ">=";
      
      if (target_name) {
        printf("if @%u %s @%u -> %s", src1_addr, cond, src2_addr, target_name);
      } else {
        printf("if @%u %s @%u -> L%u", src1_addr, cond, src2_addr, label_id);
      }
      break;
    }
    
    case OP_SYSCALL:
      printf("%lu", instruction->imm_value);
      break;
      
    default:
      printf("%s @%u imm:%lx", 
             get_type_name(instruction->type), 
             instruction->var_address, 
             instruction->imm_value);
  }
  
  printf("\n");
}

/**
* @brief Execute a binary instruction (same as in interpreter but for debugger)
* 
* @param state Pointer to the debugger state
* @param instruction Binary instruction to execute
* @return 0 on success, non-zero on failure
*/
int execute_instruction(debugger_state_t *state, binary_instruction_t *instruction) {
  // Same execution code as in interpreter
  // This would be a copy of the execute_binary_instruction function
  // from the enhanced_coil_interpreter.c file, adapted for the debugger state
  
  // For brevity, I'll just implement a few operations to demonstrate
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
    
    case 0xFFFE: // Label definition - skip
      break;
      
    // ... other operations would be implemented here
    
    default:
      fprintf(stderr, "Unsupported operation code: %04X\n", instruction->op_code);
      return -1;
  }
  
  return 0;
}

/**
* @brief Print memory dump in a formatted way
*
* @param state Pointer to the debugger state
* @param start_addr Start address to dump
* @param count Number of bytes to dump
*/
void dump_memory(debugger_state_t *state, uint16_t start_addr, uint16_t count) {
  if (start_addr >= MAX_MEMORY_SIZE) {
    fprintf(stderr, "Start address out of bounds\n");
    return;
  }
  
  if (start_addr + count > MAX_MEMORY_SIZE) {
    count = MAX_MEMORY_SIZE - start_addr;
  }
  
  printf("Memory dump from %u to %u:\n", start_addr, start_addr + count - 1);
  
  for (uint16_t addr = start_addr; addr < start_addr + count; addr += 16) {
    printf("%04x: ", addr);
    
    // Hex values
    for (int i = 0; i < 16; i++) {
      if (addr + i < start_addr + count) {
        printf("%02x ", state->memory[addr + i]);
      } else {
        printf("   ");
      }
      
      if (i == 7) {
        printf(" ");
      }
    }
    
    printf(" |");
    
    // ASCII representation
    for (int i = 0; i < 16; i++) {
      if (addr + i < start_addr + count) {
        char c = state->memory[addr + i];
        printf("%c", isprint(c) ? c : '.');
      } else {
        printf(" ");
      }
    }
    
    printf("|\n");
  }
}

/**
* @brief Print stack dump in a formatted way
*
* @param state Pointer to the debugger state
*/
void dump_stack(debugger_state_t *state) {
  if (state->stack_used == 0) {
    printf("Stack is empty\n");
    return;
  }
  
  printf("Stack dump (%lu bytes used):\n", state->stack_used);
  
  for (size_t i = 0; i < state->stack_used; i += 16) {
    printf("%04lx: ", i);
    
    // Hex values
    for (int j = 0; j < 16; j++) {
      if (i + j < state->stack_used) {
        printf("%02x ", state->stack[i + j]);
      } else {
        printf("   ");
      }
      
      if (j == 7) {
        printf(" ");
      }
    }
    
    printf(" |");
    
    // ASCII representation
    for (int j = 0; j < 16; j++) {
      if (i + j < state->stack_used) {
        char c = state->stack[i + j];
        printf("%c", isprint(c) ? c : '.');
      } else {
        printf(" ");
      }
    }
    
    printf("|\n");
  }
}

/**
* @brief Print a list of all labels
*
* @param state Pointer to the debugger state
*/
void list_labels(debugger_state_t *state) {
  if (state->label_count == 0) {
    printf("No labels defined\n");
    return;
  }
  
  printf("Labels:\n");
  for (size_t i = 0; i < state->label_count; i++) {
    printf("  %2u: %-20s [ID:%u] @ position %ld\n", 
           (unsigned int)i, 
           state->labels[i].name, 
           state->labels[i].label_id, 
           state->labels[i].file_position);
  }
}

/**
* @brief Print a list of all breakpoints
*
* @param state Pointer to the debugger state
*/
void list_breakpoints(debugger_state_t *state) {
  if (state->breakpoint_count == 0) {
    printf("No breakpoints defined\n");
    return;
  }
  
  printf("Breakpoints:\n");
  for (size_t i = 0; i < state->breakpoint_count; i++) {
    const char *label = find_label_by_position(state, state->breakpoints[i]);
    if (label) {
      printf("  %2u: Position %ld [%s]\n", (unsigned int)i, state->breakpoints[i], label);
    } else {
      printf("  %2u: Position %ld\n", (unsigned int)i, state->breakpoints[i]);
    }
  }
}

/**
* @brief Process a debugger command
*
* @param state Pointer to the debugger state
* @param cmd Command string
* @return 0 to continue, 1 to exit debugger
*/
int process_command(debugger_state_t *state, char *cmd) {
  // Remove newline
  char *nl = strchr(cmd, '\n');
  if (nl) *nl = '\0';
  
  // Skip leading whitespace
  while (isspace(*cmd)) cmd++;
  
  // Empty command = repeat last command (step)
  if (*cmd == '\0') {
    return 0; // Continue with step
  }
  
  // Parse command
  char *arg = cmd;
  while (*arg && !isspace(*arg)) arg++;
  if (*arg) {
    *arg = '\0';
    arg++;
    while (isspace(*arg)) arg++;
  }
  
  // Process command
  if (strcmp(cmd, "help") == 0 || strcmp(cmd, "h") == 0) {
    printf("Debugger commands:\n");
    printf("  help, h                - Show this help\n");
    printf("  step, s                - Execute current instruction and step to next\n");
    printf("  continue, c            - Continue execution until breakpoint or end\n");
    printf("  run, r                 - Same as continue\n");
    printf("  break, b <pos>         - Set breakpoint at file position\n");
    printf("  break, b <label>       - Set breakpoint at label\n");
    printf("  delete, d <index>      - Delete breakpoint by index\n");
    printf("  list, l                - List all labels\n");
    printf("  breakpoints, bp        - List all breakpoints\n");
    printf("  memory, m <addr> <len> - Dump memory\n");
    printf("  stack, st              - Dump stack\n");
    printf("  goto, g <label>        - Go to label\n");
    printf("  goto, g <pos>          - Go to file position\n");
    printf("  info, i                - Show execution statistics\n");
    printf("  quit, q                - Exit debugger\n");
    printf("  <enter>                - Repeat last command (step)\n");
    
  } else if (strcmp(cmd, "step") == 0 || strcmp(cmd, "s") == 0) {
    state->step_mode = true;
    return 0; // Continue with step
    
  } else if (strcmp(cmd, "continue") == 0 || strcmp(cmd, "c") == 0 ||
             strcmp(cmd, "run") == 0 || strcmp(cmd, "r") == 0) {
    state->step_mode = false;
    return 0; // Continue with run
    
  } else if (strcmp(cmd, "break") == 0 || strcmp(cmd, "b") == 0) {
    if (*arg == '\0') {
      printf("Missing argument for break command\n");
    } else if (isdigit(*arg)) {
      // Position
      long pos = atol(arg);
      add_breakpoint(state, pos);
    } else {
      // Label
      bool found = false;
      for (size_t i = 0; i < state->label_count; i++) {
        if (strcmp(state->labels[i].name, arg) == 0) {
          add_breakpoint(state, state->labels[i].file_position);
          found = true;
          break;
        }
      }
      
      if (!found) {
        printf("Label not found: %s\n", arg);
      }
    }
    
  } else if (strcmp(cmd, "delete") == 0 || strcmp(cmd, "d") == 0) {
    if (*arg == '\0') {
      printf("Missing argument for delete command\n");
    } else {
      int index = atoi(arg);
      remove_breakpoint(state, index);
    }
    
  } else if (strcmp(cmd, "list") == 0 || strcmp(cmd, "l") == 0) {
    list_labels(state);
    
  } else if (strcmp(cmd, "breakpoints") == 0 || strcmp(cmd, "bp") == 0) {
    list_breakpoints(state);
    
  } else if (strcmp(cmd, "memory") == 0 || strcmp(cmd, "m") == 0) {
    if (*arg == '\0') {
      printf("Missing arguments for memory command\n");
    } else {
      char *endptr;
      uint16_t addr = (uint16_t)strtoul(arg, &endptr, 0);
      
      while (isspace(*endptr)) endptr++;
      if (*endptr == '\0') {
        // Default length
        dump_memory(state, addr, 64);
      } else {
        uint16_t len = (uint16_t)strtoul(endptr, NULL, 0);
        dump_memory(state, addr, len);
      }
    }
    
  } else if (strcmp(cmd, "stack") == 0 || strcmp(cmd, "st") == 0) {
    dump_stack(state);
    
  } else if (strcmp(cmd, "goto") == 0 || strcmp(cmd, "g") == 0) {
    if (*arg == '\0') {
      printf("Missing argument for goto command\n");
    } else if (isdigit(*arg)) {
      // Position
      long pos = atol(arg);
      if (fseek(state->input_file, pos, SEEK_SET) != 0) {
        perror("fseek");
      } else {
        printf("Moved to position %ld\n", pos);
      }
    } else {
      // Label
      bool found = false;
      for (size_t i = 0; i < state->label_count; i++) {
        if (strcmp(state->labels[i].name, arg) == 0) {
          if (fseek(state->input_file, state->labels[i].file_position, SEEK_SET) != 0) {
            perror("fseek");
          } else {
            printf("Moved to label %s at position %ld\n", 
                   state->labels[i].name, 
                   state->labels[i].file_position);
          }
          found = true;
          break;
        }
      }
      
      if (!found) {
        printf("Label not found: %s\n", arg);
      }
    }
    
  } else if (strcmp(cmd, "info") == 0 || strcmp(cmd, "i") == 0) {
    printf("Execution statistics:\n");
    printf("  Instructions executed: %lu\n", state->instruction_count);
    printf("  Memory used: %lu bytes\n", state->memory_used);
    printf("  Stack used: %lu bytes\n", state->stack_used);
    printf("  Current position: %ld\n", state->current_position);
    
  } else if (strcmp(cmd, "quit") == 0 || strcmp(cmd, "q") == 0) {
    return 1; // Exit
    
  } else {
    printf("Unknown command: %s\n", cmd);
  }
  
  return 2; // Stay in command mode
}

/**
* @brief First pass to collect all labels
* 
* @param state Pointer to the debugger state
* @return 0 on success, non-zero on failure
*/
int collect_labels(debugger_state_t *state) {
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
        
        if (add_label(state, label_id, label_pos, NULL) != 0) {
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
        
        if (add_label(state, label_id, label_pos, NULL) != 0) {
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
* @brief Debugger main loop
* 
* @param state Pointer to the debugger state
* @return 0 on success, non-zero on failure
*/
int debugger_loop(debugger_state_t *state) {
  char cmd_buf[MAX_LINE_LENGTH];
  binary_instruction_t instruction;
  int result;
  
  state->running = true;
  
  while (state->running) {
    // Read next instruction
    if (state->binary_mode) {
      result = read_binary_instruction(state, &instruction);
    } else {
      // For a text-mode debugger, we'd actually need to parse and convert the text instruction
      // For simplicity, I'll just simulate this with a fake read
      result = -1;
      printf("Text mode debugging not fully implemented\n");
      break;
    }
    
    if (result != 0) {
      if (result > 0) {
        printf("End of file reached\n");
      } else {
        printf("Error reading instruction\n");
      }
      break;
    }
    
    // Check for breakpoint
    bool hit_breakpoint = is_at_breakpoint(state);
    
    // Stop if in step mode or hit a breakpoint
    if (state->step_mode || hit_breakpoint) {
      if (hit_breakpoint) {
        printf("Breakpoint hit at position %ld\n", state->current_position);
      }
      
      // Print current instruction
      print_instruction(state, &instruction);
      
      // Command loop
      int cmd_result;
      do {
        printf("(COIL-dbg) ");
        fflush(stdout);
        
        if (fgets(cmd_buf, sizeof(cmd_buf), stdin) == NULL) {
          return 1; // Exit on EOF
        }
        
        cmd_result = process_command(state, cmd_buf);
        if (cmd_result == 1) {
          return 0; // Exit
        }
      } while (cmd_result != 0);
    }
    
    // Execute the instruction
    if (execute_instruction(state, &instruction) != 0) {
      printf("Error executing instruction\n");
      return -1;
    }
    
    state->instruction_count++;
  }
  
  return 0;
}

/**
* @brief Debug a COIL file
* 
* @param filename Path to the COIL file
* @param binary_mode Flag indicating binary input format
* @return 0 on success, non-zero on failure
*/
int debug_coil_file(const char *filename, bool binary_mode) {
  FILE *file = fopen(filename, binary_mode ? "rb" : "r");
  if (!file) {
    perror("Failed to open file");
    return -1;
  }
  
  debugger_state_t state;
  if (initialize_debugger(&state, file, binary_mode) != 0) {
    fprintf(stderr, "Failed to initialize debugger\n");
    fclose(file);
    return -1;
  }
  
  // First pass to collect labels
  if (collect_labels(&state) != 0) {
    fprintf(stderr, "Failed to collect labels\n");
    fclose(file);
    return -1;
  }
  
  printf("COIL Debugger\n");
  printf("Type 'help' for a list of commands\n");
  
  // Start the debugger loop
  int result = debugger_loop(&state);
  
  fclose(file);
  return result;
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
  
  return debug_coil_file(filename, binary_mode);
}