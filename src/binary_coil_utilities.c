/**
* @file binary_coil_utils.c
* @brief Utilities for encoding and decoding binary COIL instructions
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <endian.h>
#include "coil_format.h"

/**
* @brief Write a binary instruction to a file
* 
* @param file Output file
* @param instruction Binary instruction
* @return 0 on success, non-zero on failure
*/
int write_binary_instruction(FILE *file, const binary_instruction_t *instruction) {
  if (!file || !instruction) {
    return -1;
  }
  
  // Write the instruction components
  fwrite(&instruction->start_marker, sizeof(uint8_t), 1, file);
  
  // Convert to little-endian before writing
  uint16_t op_code_le = htole16(instruction->op_code);
  fwrite(&op_code_le, sizeof(uint16_t), 1, file);
  
  fwrite(&instruction->type_marker, sizeof(uint8_t), 1, file);
  fwrite(&instruction->type, sizeof(uint8_t), 1, file);
  
  fwrite(&instruction->var_marker, sizeof(uint8_t), 1, file);
  uint16_t var_address_le = htole16(instruction->var_address);
  fwrite(&var_address_le, sizeof(uint16_t), 1, file);
  
  fwrite(&instruction->imm_marker, sizeof(uint8_t), 1, file);
  uint64_t imm_value_le = htole64(instruction->imm_value);
  fwrite(&imm_value_le, sizeof(uint64_t), 1, file);
  
  fwrite(&instruction->end_marker, sizeof(uint8_t), 1, file);
  
  return 0;
}

/**
* @brief Read a binary instruction from a file
* 
* @param file Input file
* @param instruction Pointer to store the read instruction
* @return 0 on success, non-zero on failure or EOF
*/
int read_binary_instruction(FILE *file, binary_instruction_t *instruction) {
  if (!file || !instruction) {
    return -1;
  }
  
  // Read the start marker
  if (fread(&instruction->start_marker, sizeof(uint8_t), 1, file) != 1) {
    if (feof(file)) {
      return 1; // EOF
    }
    return -1;
  }
  
  // Verify start marker
  if (instruction->start_marker != MARKER_INSTRUCTION) {
    fprintf(stderr, "Invalid instruction marker: %02X\n", instruction->start_marker);
    return -1;
  }
  
  // Read operation code (little-endian)
  uint16_t op_code_le;
  if (fread(&op_code_le, sizeof(uint16_t), 1, file) != 1) {
    return -1;
  }
  instruction->op_code = le16toh(op_code_le);
  
  // Read type marker and type
  if (fread(&instruction->type_marker, sizeof(uint8_t), 1, file) != 1 ||
      instruction->type_marker != MARKER_TYPE) {
    return -1;
  }
  
  if (fread(&instruction->type, sizeof(uint8_t), 1, file) != 1) {
    return -1;
  }
  
  // Read variable marker and address
  if (fread(&instruction->var_marker, sizeof(uint8_t), 1, file) != 1 ||
      instruction->var_marker != MARKER_VARIABLE) {
    return -1;
  }
  
  uint16_t var_address_le;
  if (fread(&var_address_le, sizeof(uint16_t), 1, file) != 1) {
    return -1;
  }
  instruction->var_address = le16toh(var_address_le);
  
  // Read immediate marker and value
  if (fread(&instruction->imm_marker, sizeof(uint8_t), 1, file) != 1 ||
      instruction->imm_marker != MARKER_IMMEDIATE) {
    return -1;
  }
  
  uint64_t imm_value_le;
  if (fread(&imm_value_le, sizeof(uint64_t), 1, file) != 1) {
    return -1;
  }
  instruction->imm_value = le64toh(imm_value_le);
  
  // Read end marker
  if (fread(&instruction->end_marker, sizeof(uint8_t), 1, file) != 1 ||
      instruction->end_marker != MARKER_END) {
    return -1;
  }
  
  return 0;
}

/**
* @brief Initialize a binary instruction with default values
* 
* @param instruction Pointer to the instruction
* @param op_code Operation code
* @param type Memory type
* @param var_address Variable address
* @param imm_value Immediate value
*/
void init_binary_instruction(binary_instruction_t *instruction,
                            uint16_t op_code,
                            uint8_t type,
                            uint16_t var_address,
                            uint64_t imm_value) {
  if (!instruction) {
    return;
  }
  
  instruction->start_marker = MARKER_INSTRUCTION;
  instruction->op_code = op_code;
  instruction->type_marker = MARKER_TYPE;
  instruction->type = type;
  instruction->var_marker = MARKER_VARIABLE;
  instruction->var_address = var_address;
  instruction->imm_marker = MARKER_IMMEDIATE;
  instruction->imm_value = imm_value;
  instruction->end_marker = MARKER_END;
}

/**
* @brief Print a binary instruction in human-readable format
* 
* @param instruction Pointer to the instruction
*/
void print_binary_instruction(const binary_instruction_t *instruction) {
  if (!instruction) {
    return;
  }
  
  printf("[%02X] Op: %04X, Type: %02X, Var: %04X, Imm: %016lX [%02X]\n",
         instruction->start_marker,
         instruction->op_code,
         instruction->type,
         instruction->var_address,
         instruction->imm_value,
         instruction->end_marker);
}

/**
* @brief Convert a COIL text file to binary format
* 
* @param input_filename Path to the text COIL file
* @param output_filename Path to the output binary COIL file
* @return 0 on success, non-zero on failure
*/
int convert_text_to_binary(const char *input_filename, const char *output_filename) {
  FILE *input = fopen(input_filename, "r");
  if (!input) {
    perror("Failed to open input file");
    return -1;
  }
  
  FILE *output = fopen(output_filename, "wb");
  if (!output) {
    perror("Failed to open output file");
    fclose(input);
    return -1;
  }
  
  char line[256];
  while (fgets(line, sizeof(line), input)) {
    // Skip comments and empty lines
    if (line[0] == ';' || line[0] == '\n') {
      continue;
    }
    
    // Parse the text instruction
    uint16_t op_code;
    uint16_t args[16];
    int num_args = 0;
    
    char *token = strtok(line, " \t\n");
    if (!token) {
      continue;
    }
    
    op_code = (uint16_t)strtoul(token, NULL, 16);
    
    token = strtok(NULL, " \t\n");
    while (token && num_args < 16) {
      args[num_args++] = (uint16_t)strtoul(token, NULL, 16);
      token = strtok(NULL, " \t\n");
    }
    
    // Create a binary instruction
    binary_instruction_t instruction;
    
    // Extract information based on opcode
    uint8_t type = 0;
    uint16_t var_address = 0;
    uint64_t imm_value = 0;
    
    switch (op_code) {
      case OP_ALLOC_IMM:
        if (num_args >= 3) {
          type = (uint8_t)args[0];
          var_address = args[1];
          imm_value = args[2];
        }
        break;
        
      case OP_ALLOC_MEM:
        if (num_args >= 3) {
          type = (uint8_t)args[0];
          var_address = args[1];
          // Source address stored in immediate value
          imm_value = args[2];
        }
        break;
        
      case OP_ADD:
      case OP_SUB:
      case OP_MUL:
      case OP_DIV:
        if (num_args >= 3) {
          var_address = args[0];
          // Source addresses stored as 32-bit values in immediate
          imm_value = ((uint64_t)args[1] << 32) | args[2];
          type = MEM_INT64; // Default to 64-bit
        }
        break;
        
      case OP_SYSCALL:
        if (num_args >= 1) {
          var_address = 0;
          // Syscall number stored in immediate
          imm_value = args[0];
          
          // Pack additional arguments into a separate instruction
          if (num_args > 1) {
            binary_instruction_t arg_instruction;
            init_binary_instruction(&arg_instruction, 0xFFFF, 0, 0, 0);
            
            // Pack up to 4 additional arguments
            for (int i = 1; i < num_args && i < 5; i++) {
              ((uint16_t*)&arg_instruction.imm_value)[i-1] = args[i];
            }
            
            write_binary_instruction(output, &arg_instruction);
          }
        }
        break;
        
      default:
        fprintf(stderr, "Unknown opcode: %04X\n", op_code);
        break;
    }
    
    init_binary_instruction(&instruction, op_code, type, var_address, imm_value);
    write_binary_instruction(output, &instruction);
  }
  
  fclose(input);
  fclose(output);
  return 0;
}

/**
* @brief Convert a binary COIL file to text format
* 
* @param input_filename Path to the binary COIL file
* @param output_filename Path to the output text COIL file
* @return 0 on success, non-zero on failure
*/
int convert_binary_to_text(const char *input_filename, const char *output_filename) {
  FILE *input = fopen(input_filename, "rb");
  if (!input) {
    perror("Failed to open input file");
    return -1;
  }
  
  FILE *output = fopen(output_filename, "w");
  if (!output) {
    perror("Failed to open output file");
    fclose(input);
    return -1;
  }
  
  fprintf(output, "; COIL text representation generated from binary\n");
  fprintf(output, "; Original binary file: %s\n\n", input_filename);
  
  binary_instruction_t instruction;
  int result;
  
  while ((result = read_binary_instruction(input, &instruction)) == 0) {
    // Convert back to text format based on opcode
    switch (instruction.op_code) {
      case OP_ALLOC_IMM:
        fprintf(output, "%04X %04X %04X %04lX  ; ALLOC_IMM: Allocate memory at address %u with value %lu\n",
                instruction.op_code, instruction.type, instruction.var_address, 
                instruction.imm_value, instruction.var_address, instruction.imm_value);
        break;
        
      case OP_ALLOC_MEM:
        fprintf(output, "%04X %04X %04X %04lX  ; ALLOC_MEM: Allocate memory at address %u with value from address %lu\n",
                instruction.op_code, instruction.type, instruction.var_address, 
                instruction.imm_value, instruction.var_address, instruction.imm_value);
        break;
        
      case OP_ADD:
        fprintf(output, "%04X %04X %04X %04X  ; ADD: Add values from addresses %u and %u, store in address %u\n",
                instruction.op_code, instruction.var_address, 
                (uint16_t)(instruction.imm_value >> 32), (uint16_t)instruction.imm_value,
                (uint16_t)(instruction.imm_value >> 32), (uint16_t)instruction.imm_value, 
                instruction.var_address);
        break;
        
      case OP_SUB:
        fprintf(output, "%04X %04X %04X %04X  ; SUB: Subtract value at address %u from value at address %u, store in address %u\n",
                instruction.op_code, instruction.var_address, 
                (uint16_t)(instruction.imm_value >> 32), (uint16_t)instruction.imm_value,
                (uint16_t)instruction.imm_value, (uint16_t)(instruction.imm_value >> 32), 
                instruction.var_address);
        break;
        
      case OP_MUL:
        fprintf(output, "%04X %04X %04X %04X  ; MUL: Multiply values from addresses %u and %u, store in address %u\n",
                instruction.op_code, instruction.var_address, 
                (uint16_t)(instruction.imm_value >> 32), (uint16_t)instruction.imm_value,
                (uint16_t)(instruction.imm_value >> 32), (uint16_t)instruction.imm_value, 
                instruction.var_address);
        break;
        
      case OP_DIV:
        fprintf(output, "%04X %04X %04X %04X  ; DIV: Divide value at address %u by value at address %u, store in address %u\n",
                instruction.op_code, instruction.var_address, 
                (uint16_t)(instruction.imm_value >> 32), (uint16_t)instruction.imm_value,
                (uint16_t)(instruction.imm_value >> 32), (uint16_t)instruction.imm_value, 
                instruction.var_address);
        break;
        
      case OP_SYSCALL:
        fprintf(output, "%04X %04lX  ; SYSCALL: System call %lu\n",
                instruction.op_code, instruction.imm_value, instruction.imm_value);
        
        // Check for argument instruction
        binary_instruction_t arg_instruction;
        if (read_binary_instruction(input, &arg_instruction) == 0 && 
            arg_instruction.op_code == 0xFFFF) {
          
          fprintf(output, "; SYSCALL ARGS:");
          for (int i = 0; i < 4; i++) {
            uint16_t arg = ((uint16_t*)&arg_instruction.imm_value)[i];
            if (arg != 0) {
              fprintf(output, " %04X", arg);
            }
          }
          fprintf(output, "\n");
        } else {
          // Go back to previous position if not an argument instruction
          fseek(input, -sizeof(binary_instruction_t), SEEK_CUR);
        }
        break;
        
      default:
        fprintf(output, "%04X %02X %04X %016lX  ; Unknown instruction\n",
                instruction.op_code, instruction.type, 
                instruction.var_address, instruction.imm_value);
    }
  }
  
  if (result < 0) {
    fprintf(stderr, "Error reading binary instruction\n");
  }
  
  fclose(input);
  fclose(output);
  return 0;
}