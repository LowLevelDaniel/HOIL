/**
* @file hoil_to_coil.c
* @brief Converter from HOIL to COIL
*
* This file contains the implementation of the HOIL to COIL converter,
* which translates HOIL (Human Oriented Intermediate Language) code to
* COIL (Computer Oriented Intermediate Language) format.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <ctype.h>
#include "coil_format.h"
#include "hoil_format.h"

/**
* @brief Converter state structure
*/
typedef struct {
  symbol_table_t symbols;   /**< Symbol table */
  label_table_t labels;     /**< Label table */
  uint16_t next_address;    /**< Next available memory address */
  bool binary_output;       /**< Flag for binary output format */
  FILE *output;             /**< Output file */
} converter_state_t;

/**
* @brief Initialize the converter state
* 
* @param state Pointer to the converter state
* @param output Output file
* @param binary_output Flag for binary output format
* @return 0 on success, non-zero on failure
*/
int initialize_converter(converter_state_t *state, FILE *output, bool binary_output) {
  if (!state || !output) {
    return -1;
  }
  
  memset(&state->symbols, 0, sizeof(symbol_table_t));
  memset(&state->labels, 0, sizeof(label_table_t));
  
  state->next_address = 0;
  state->binary_output = binary_output;
  state->output = output;
  state->labels.next_id = 1; // Start with label ID 1
  
  return 0;
}

/**
* @brief Add a symbol to the symbol table
* 
* @param state Pointer to the converter state
* @param name Symbol name
* @param address Memory address
* @param type Memory type
* @return 0 on success, non-zero on failure
*/
int add_symbol(converter_state_t *state, const char *name, uint16_t address, mem_type_t type) {
  if (state->symbols.count >= MAX_SYMBOLS) {
    fprintf(stderr, "Symbol table full\n");
    return -1;
  }
  
  // Check if symbol already exists
  for (size_t i = 0; i < state->symbols.count; i++) {
    if (strcmp(state->symbols.entries[i].name, name) == 0) {
      fprintf(stderr, "Symbol '%s' already defined\n", name);
      return -1;
    }
  }
  
  strncpy(state->symbols.entries[state->symbols.count].name, name, sizeof(state->symbols.entries[0].name) - 1);
  state->symbols.entries[state->symbols.count].name[sizeof(state->symbols.entries[0].name) - 1] = '\0';
  state->symbols.entries[state->symbols.count].address = address;
  state->symbols.entries[state->symbols.count].type = type;
  
  state->symbols.count++;
  
  // Update next available address based on type size
  state->next_address = address + get_type_size(type);
  
  return 0;
}

/**
* @brief Find a symbol in the symbol table
* 
* @param state Pointer to the converter state
* @param name Symbol name
* @param entry Pointer to store the found entry
* @return 0 on success, non-zero if not found
*/
int find_symbol(converter_state_t *state, const char *name, symbol_entry_t *entry) {
  for (size_t i = 0; i < state->symbols.count; i++) {
    if (strcmp(state->symbols.entries[i].name, name) == 0) {
      if (entry) {
        *entry = state->symbols.entries[i];
      }
      return 0;
    }
  }
  
  return -1;
}

/**
* @brief Add a label to the label table
* 
* @param state Pointer to the converter state
* @param name Label name
* @param defined Flag indicating whether the label is defined
* @return Label ID on success, 0 on failure
*/
uint16_t add_label(converter_state_t *state, const char *name, bool defined) {
  // Check if label already exists
  for (size_t i = 0; i < state->labels.count; i++) {
    if (strcmp(state->labels.entries[i].name, name) == 0) {
      if (defined && state->labels.entries[i].defined) {
        fprintf(stderr, "Label '%s' already defined\n", name);
        return 0;
      }
      
      if (defined) {
        state->labels.entries[i].defined = true;
      }
      
      return state->labels.entries[i].id;
    }
  }
  
  if (state->labels.count >= MAX_SYMBOLS) {
    fprintf(stderr, "Label table full\n");
    return 0;
  }
  
  uint16_t label_id = state->labels.next_id++;
  
  strncpy(state->labels.entries[state->labels.count].name, name, sizeof(state->labels.entries[0].name) - 1);
  state->labels.entries[state->labels.count].name[sizeof(state->labels.entries[0].name) - 1] = '\0';
  state->labels.entries[state->labels.count].id = label_id;
  state->labels.entries[state->labels.count].defined = defined;
  
  state->labels.count++;
  
  return label_id;
}

/**
* @brief Find a label in the label table
* 
* @param state Pointer to the converter state
* @param name Label name
* @return Label ID on success, 0 if not found
*/
uint16_t find_label(converter_state_t *state, const char *name) {
  for (size_t i = 0; i < state->labels.count; i++) {
    if (strcmp(state->labels.entries[i].name, name) == 0) {
      return state->labels.entries[i].id;
    }
  }
  
  return 0;
}

/**
* @brief Check if all labels are defined
* 
* @param state Pointer to the converter state
* @return 0 if all labels are defined, non-zero otherwise
*/
int check_undefined_labels(converter_state_t *state) {
  int undefined_count = 0;
  
  for (size_t i = 0; i < state->labels.count; i++) {
    if (!state->labels.entries[i].defined) {
      fprintf(stderr, "Label '%s' used but not defined\n", state->labels.entries[i].name);
      undefined_count++;
    }
  }
  
  return undefined_count;
}

/**
* @brief Parse a HOIL line into tokens
* 
* @param line Input HOIL line
* @param tokens Array to store tokens
* @param max_tokens Maximum number of tokens to parse
* @return Number of tokens parsed
*/
int tokenize_hoil_line(char *line, char **tokens, int max_tokens) {
  int count = 0;
  char *token = strtok(line, " ,\t\n");
  
  while (token && count < max_tokens) {
    // Skip comments
    if (token[0] == ';') {
      break;
    }
    
    tokens[count++] = token;
    token = strtok(NULL, " ,\t\n");
  }
  
  return count;
}

/**
* @brief Convert a HOIL data type to COIL type code
* 
* @param type_str HOIL type string
* @return COIL type code
*/
mem_type_t hoil_type_to_coil_type(const char *type_str) {
  if (strcmp(type_str, "dint") == 0) {
    return MEM_INT64;
  } else if (strcmp(type_str, "int8") == 0) {
    return MEM_INT8;
  } else if (strcmp(type_str, "int16") == 0) {
    return MEM_INT16;
  } else if (strcmp(type_str, "int32") == 0) {
    return MEM_INT32;
  } else if (strcmp(type_str, "uint8") == 0) {
    return MEM_UINT8;
  } else if (strcmp(type_str, "uint16") == 0) {
    return MEM_UINT16;
  } else if (strcmp(type_str, "uint32") == 0) {
    return MEM_UINT32;
  } else if (strcmp(type_str, "uint64") == 0) {
    return MEM_UINT64;
  } else if (strcmp(type_str, "float32") == 0) {
    return MEM_FLOAT32;
  } else if (strcmp(type_str, "float64") == 0) {
    return MEM_FLOAT64;
  } else if (strcmp(type_str, "bool") == 0) {
    return MEM_BOOL;
  } else if (strcmp(type_str, "ptr") == 0) {
    return MEM_PTR;
  } else {
    fprintf(stderr, "Unknown type: %s\n", type_str);
    return 0;
  }
}

/**
* @brief Convert a HOIL immediate value to a numeric value
* 
* @param value_str HOIL value string
* @return Numeric value
*/
int64_t convert_immediate_value(const char *value_str) {
  if (strncmp(value_str, "id", 2) == 0) {
    // Extract the numeric part of an "idX" format
    return atoll(value_str + 2);
  } else if (strcmp(value_str, "true") == 0) {
    return 1;
  } else if (strcmp(value_str, "false") == 0) {
    return 0;
  } else if (isdigit(value_str[0]) || value_str[0] == '-' || value_str[0] == '+') {
    // Parse a numeric value
    return atoll(value_str);
  } else {
    fprintf(stderr, "Unknown value format: %s\n", value_str);
    return 0;
  }
}

/**
* @brief Resolve a HOIL identifier to a memory address
* 
* @param state Pointer to the converter state
* @param id_str HOIL identifier string
* @param entry Pointer to store the symbol entry (if found)
* @return Memory address
*/
uint16_t resolve_identifier(converter_state_t *state, const char *id_str, symbol_entry_t *entry) {
  if (id_str[0] == '&') {
    // Remove the '&' prefix for address references
    const char *symbol_name = id_str + 1;
    symbol_entry_t symbol;
    
    if (find_symbol(state, symbol_name, &symbol) == 0) {
      if (entry) {
        *entry = symbol;
      }
      return symbol.address;
    } else {
      return 0;
    }
  } else if (isdigit(id_str[0])) {
    // Direct numeric address
    return (uint16_t)atoi(id_str);
  } else {
    // Symbol reference
    symbol_entry_t symbol;
    
    if (find_symbol(state, id_str, &symbol) == 0) {
      if (entry) {
        *entry = symbol;
      }
      return symbol.address;
    } else {
      return 0;
    }
  }
}

/**
* @brief Write a binary instruction to the output file
* 
* @param state Pointer to the converter state
* @param instruction Binary instruction
* @return 0 on success, non-zero on failure
*/
int write_instruction(converter_state_t *state, binary_instruction_t *instruction) {
  if (state->binary_output) {
    // Write binary format
    fwrite(&instruction->start_marker, sizeof(uint8_t), 1, state->output);
    fwrite(&instruction->op_code, sizeof(uint16_t), 1, state->output);
    fwrite(&instruction->type_marker, sizeof(uint8_t), 1, state->output);
    fwrite(&instruction->type, sizeof(uint8_t), 1, state->output);
    fwrite(&instruction->var_marker, sizeof(uint8_t), 1, state->output);
    fwrite(&instruction->var_address, sizeof(uint16_t), 1, state->output);
    fwrite(&instruction->imm_marker, sizeof(uint8_t), 1, state->output);
    fwrite(&instruction->imm_value, sizeof(uint64_t), 1, state->output);
    fwrite(&instruction->end_marker, sizeof(uint8_t), 1, state->output);
  } else {
    // Write text format (hexadecimal representation)
    fprintf(state->output, "%04X %02X %04X %016llX\n", 
            instruction->op_code, instruction->type, 
            instruction->var_address, (unsigned long long)instruction->imm_value);
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
void init_instruction(binary_instruction_t *instruction,
                     uint16_t op_code,
                     uint8_t type,
                     uint16_t var_address,
                     uint64_t imm_value) {
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
* @brief Convert a HOIL VAL instruction to COIL format
* 
* @param state Pointer to the converter state
* @param tokens Array of HOIL tokens
* @param num_tokens Number of tokens
* @return 0 on success, non-zero on failure
*/
int convert_val_instruction(converter_state_t *state, char **tokens, int num_tokens) {
  if (num_tokens < 4) {
    fprintf(stderr, "VAL instruction requires at least 4 tokens\n");
    return -1;
  }
  
  binary_instruction_t instruction;
  
  if (strcmp(tokens[1], "DEFV") == 0) {
    mem_type_t type = hoil_type_to_coil_type(tokens[2]);
    if (type == 0) {
      return -1;
    }
    
    const char *symbol_name = tokens[3];
    int64_t value = convert_immediate_value(tokens[4]);
    
    // Allocate memory for the symbol
    uint16_t address = state->next_address;
    if (add_symbol(state, symbol_name, address, type) != 0) {
      return -1;
    }
    
    // Generate ALLOC_IMM instruction
    init_instruction(&instruction, OP_ALLOC_IMM, type, address, value);
    write_instruction(state, &instruction);
    
  } else if (strcmp(tokens[1], "MOVV") == 0) {
    mem_type_t type = hoil_type_to_coil_type(tokens[2]);
    if (type == 0) {
      return -1;
    }
    
    const char *dest_symbol = tokens[3];
    const char *src_symbol = tokens[4];
    
    // Resolve source symbol
    uint16_t src_addr = resolve_identifier(state, src_symbol, NULL);
    
    // Allocate memory for the destination symbol if it doesn't exist
    uint16_t dest_addr = resolve_identifier(state, dest_symbol, NULL);
    if (dest_addr == 0) {
      dest_addr = state->next_address;
      if (add_symbol(state, dest_symbol, dest_addr, type) != 0) {
        return -1;
      }
    }
    // Generate ALLOC_MEM instruction  
    init_instruction(&instruction, OP_ALLOC_MEM, type, dest_addr, src_addr);
    write_instruction(state, &instruction);
  } else if (strcmp(tokens[1], "LOAD") == 0) {
    mem_type_t type = hoil_type_to_coil_type(tokens[2]);
    if (type == 0) {
      return -1;
    }
    
    const char *dest_symbol = tokens[3];
    const char *addr_symbol = tokens[4];
    
    // Resolve address symbol
    uint16_t addr = resolve_identifier(state, addr_symbol, NULL);
    
    // Allocate memory for the destination symbol
    uint16_t dest_addr = state->next_address;
    if (add_symbol(state, dest_symbol, dest_addr, type) != 0) {
      return -1;
    }
    
    // Generate LOAD instruction
    init_instruction(&instruction, OP_LOAD, type, dest_addr, addr);
    write_instruction(state, &instruction);
    
  } else if (strcmp(tokens[1], "STORE") == 0) {
    mem_type_t type = hoil_type_to_coil_type(tokens[2]);
    if (type == 0) {
      return -1;
    }
    
    const char *addr_symbol = tokens[3];
    const char *src_symbol = tokens[4];
    
    // Resolve address and source symbols
    uint16_t addr = resolve_identifier(state, addr_symbol, NULL);
    uint16_t src_addr = resolve_identifier(state, src_symbol, NULL);
    
    // Generate STORE instruction
    init_instruction(&instruction, OP_STORE, type, addr, src_addr);
    write_instruction(state, &instruction);
    
  } else {
    fprintf(stderr, "Unknown VAL operation: %s\n", tokens[1]);
    return -1;
  }
  
  return 0;
}

/**
* @brief Convert a HOIL MATH instruction to COIL format
* 
* @param state Pointer to the converter state
* @param tokens Array of HOIL tokens
* @param num_tokens Number of tokens
* @return 0 on success, non-zero on failure
*/
int convert_math_instruction(converter_state_t *state, char **tokens, int num_tokens) {
  if (num_tokens < 3) {
    fprintf(stderr, "MATH instruction requires at least 3 tokens\n");
    return -1;
  }
  
  binary_instruction_t instruction;
  uint16_t op_code;
  
  if (strcmp(tokens[1], "ADD") == 0) {
    op_code = OP_ADD;
  } else if (strcmp(tokens[1], "SUB") == 0) {
    op_code = OP_SUB;
  } else if (strcmp(tokens[1], "MUL") == 0) {
    op_code = OP_MUL;
  } else if (strcmp(tokens[1], "DIV") == 0) {
    op_code = OP_DIV;
  } else if (strcmp(tokens[1], "MOD") == 0) {
    op_code = OP_MOD;
  } else if (strcmp(tokens[1], "NEG") == 0) {
    op_code = OP_NEG;
  } else {
    fprintf(stderr, "Unknown MATH operation: %s\n", tokens[1]);
    return -1;
  }
  
  if (op_code == OP_NEG) {
    if (num_tokens < 3) {
      fprintf(stderr, "NEG requires at least 3 tokens\n");
      return -1;
    }
    
    const char *dest_symbol = tokens[2];
    const char *src_symbol = tokens[3];
    
    // Resolve source symbol
    uint16_t src_addr = resolve_identifier(state, src_symbol, NULL);
    
    // Allocate memory for the destination symbol if it doesn't exist
    uint16_t dest_addr;
    if (find_symbol(state, dest_symbol, NULL) == 0) {
      dest_addr = resolve_identifier(state, dest_symbol, NULL);
    } else {
      dest_addr = state->next_address;
      if (add_symbol(state, dest_symbol, dest_addr, MEM_INT64) != 0) {
        return -1;
      }
    }
    
    // Generate NEG instruction
    init_instruction(&instruction, op_code, MEM_INT64, dest_addr, src_addr);
    write_instruction(state, &instruction);
    
  } else {
    if (num_tokens < 5) {
      fprintf(stderr, "%s requires at least 5 tokens\n", tokens[1]);
      return -1;
    }
    
    const char *dest_symbol = tokens[2];
    const char *src_symbol1 = tokens[3];
    const char *src_symbol2 = tokens[4];
    
    // Resolve source symbols
    uint16_t src_addr1 = resolve_identifier(state, src_symbol1, NULL);
    uint16_t src_addr2 = resolve_identifier(state, src_symbol2, NULL);
    
    // Allocate memory for the destination symbol if it doesn't exist
    uint16_t dest_addr;
    if (find_symbol(state, dest_symbol, NULL) == 0) {
      dest_addr = resolve_identifier(state, dest_symbol, NULL);
    } else {
      dest_addr = state->next_address;
      if (add_symbol(state, dest_symbol, dest_addr, MEM_INT64) != 0) {
        return -1;
      }
    }
    
    // Pack source addresses into immediate value
    uint64_t imm_value = ((uint64_t)src_addr1 << 32) | src_addr2;
    
    // Generate arithmetic instruction
    init_instruction(&instruction, op_code, MEM_INT64, dest_addr, imm_value);
    write_instruction(state, &instruction);
  }
  
  return 0;
}

/**
* @brief Convert a HOIL BIT instruction to COIL format
* 
* @param state Pointer to the converter state
* @param tokens Array of HOIL tokens
* @param num_tokens Number of tokens
* @return 0 on success, non-zero on failure
*/
int convert_bit_instruction(converter_state_t *state, char **tokens, int num_tokens) {
  if (num_tokens < 3) {
    fprintf(stderr, "BIT instruction requires at least 3 tokens\n");
    return -1;
  }
  
  binary_instruction_t instruction;
  uint16_t op_code;
  
  if (strcmp(tokens[1], "AND") == 0) {
    op_code = OP_AND;
  } else if (strcmp(tokens[1], "OR") == 0) {
    op_code = OP_OR;
  } else if (strcmp(tokens[1], "XOR") == 0) {
    op_code = OP_XOR;
  } else if (strcmp(tokens[1], "NOT") == 0) {
    op_code = OP_NOT;
  } else if (strcmp(tokens[1], "SHL") == 0) {
    op_code = OP_SHL;
  } else if (strcmp(tokens[1], "SHR") == 0) {
    op_code = OP_SHR;
  } else {
    fprintf(stderr, "Unknown BIT operation: %s\n", tokens[1]);
    return -1;
  }
  
  if (op_code == OP_NOT) {
    if (num_tokens < 3) {
      fprintf(stderr, "NOT requires at least 3 tokens\n");
      return -1;
    }
    
    const char *dest_symbol = tokens[2];
    const char *src_symbol = tokens[3];
    
    // Resolve source symbol
    uint16_t src_addr = resolve_identifier(state, src_symbol, NULL);
    
    // Allocate memory for the destination symbol if it doesn't exist
    uint16_t dest_addr;
    if (find_symbol(state, dest_symbol, NULL) == 0) {
      dest_addr = resolve_identifier(state, dest_symbol, NULL);
    } else {
      dest_addr = state->next_address;
      if (add_symbol(state, dest_symbol, dest_addr, MEM_INT64) != 0) {
        return -1;
      }
    }
    
    // Generate NOT instruction
    init_instruction(&instruction, op_code, MEM_INT64, dest_addr, src_addr);
    write_instruction(state, &instruction);
    
  } else if (op_code == OP_SHL || op_code == OP_SHR) {
    if (num_tokens < 4) {
      fprintf(stderr, "%s requires at least 4 tokens\n", tokens[1]);
      return -1;
    }
    
    const char *dest_symbol = tokens[2];
    const char *src_symbol = tokens[3];
    int shift = atoi(tokens[4]);
    
    // Resolve source symbol
    uint16_t src_addr = resolve_identifier(state, src_symbol, NULL);
    
    // Allocate memory for the destination symbol if it doesn't exist
    uint16_t dest_addr;
    if (find_symbol(state, dest_symbol, NULL) == 0) {
      dest_addr = resolve_identifier(state, dest_symbol, NULL);
    } else {
      dest_addr = state->next_address;
      if (add_symbol(state, dest_symbol, dest_addr, MEM_INT64) != 0) {
        return -1;
      }
    }
    
    // Pack source address and shift amount into immediate value
    uint64_t imm_value = ((uint64_t)src_addr << 32) | (shift & 0xFFFFFFFF);
    
    // Generate shift instruction
    init_instruction(&instruction, op_code, MEM_INT64, dest_addr, imm_value);
    write_instruction(state, &instruction);
    
  } else {
    if (num_tokens < 5) {
      fprintf(stderr, "%s requires at least 5 tokens\n", tokens[1]);
      return -1;
    }
    
    const char *dest_symbol = tokens[2];
    const char *src_symbol1 = tokens[3];
    const char *src_symbol2 = tokens[4];
    
    // Resolve source symbols
    uint16_t src_addr1 = resolve_identifier(state, src_symbol1, NULL);
    uint16_t src_addr2 = resolve_identifier(state, src_symbol2, NULL);
    
    // Allocate memory for the destination symbol if it doesn't exist
    uint16_t dest_addr;
    if (find_symbol(state, dest_symbol, NULL) == 0) {
      dest_addr = resolve_identifier(state, dest_symbol, NULL);
    } else {
      dest_addr = state->next_address;
      if (add_symbol(state, dest_symbol, dest_addr, MEM_INT64) != 0) {
        return -1;
      }
    }
    
    // Pack source addresses into immediate value
    uint64_t imm_value = ((uint64_t)src_addr1 << 32) | src_addr2;
    
    // Generate bitwise instruction
    init_instruction(&instruction, op_code, MEM_INT64, dest_addr, imm_value);
    write_instruction(state, &instruction);
  }
  
  return 0;
}

/**
* @brief Convert a HOIL CF (Control Flow) instruction to COIL format
* 
* @param state Pointer to the converter state
* @param tokens Array of HOIL tokens
* @param num_tokens Number of tokens
* @return 0 on success, non-zero on failure
*/
int convert_cf_instruction(converter_state_t *state, char **tokens, int num_tokens) {
  if (num_tokens < 2) {
    fprintf(stderr, "CF instruction requires at least 2 tokens\n");
    return -1;
  }
  
  binary_instruction_t instruction;
  
  if (strcmp(tokens[1], "JMP") == 0) {
    if (num_tokens < 3) {
      fprintf(stderr, "JMP requires a label\n");
      return -1;
    }
    
    const char *label_name = tokens[2];
    uint16_t label_id = find_label(state, label_name);
    
    if (label_id == 0) {
      // Forward reference, create the label
      label_id = add_label(state, label_name, false);
    }
    
    // Generate JMP instruction
    init_instruction(&instruction, OP_JMP, 0, 0, label_id);
    write_instruction(state, &instruction);
    
  } else if (strcmp(tokens[1], "JCOND") == 0) {
    if (num_tokens < 6) {
      fprintf(stderr, "JCOND requires condition, operands, and label\n");
      return -1;
    }
    
    const char *cond = tokens[2];
    const char *src_symbol1 = tokens[3];
    const char *src_symbol2 = tokens[4];
    const char *label_name = tokens[5];
    
    uint16_t op_code;
    if (strcmp(cond, "EQ") == 0) {
      op_code = OP_JEQ;
    } else if (strcmp(cond, "NE") == 0) {
      op_code = OP_JNE;
    } else if (strcmp(cond, "LT") == 0) {
      op_code = OP_JLT;
    } else if (strcmp(cond, "LE") == 0) {
      op_code = OP_JLE;
    } else if (strcmp(cond, "GT") == 0) {
      op_code = OP_JGT;
    } else if (strcmp(cond, "GE") == 0) {
      op_code = OP_JGE;
    } else {
      fprintf(stderr, "Unknown condition: %s\n", cond);
      return -1;
    }
    
    // Resolve source symbols
    uint16_t src_addr1 = resolve_identifier(state, src_symbol1, NULL);
    uint16_t src_addr2 = resolve_identifier(state, src_symbol2, NULL);
    
    // Find or create the label
    uint16_t label_id = find_label(state, label_name);
    
    if (label_id == 0) {
      // Forward reference, create the label
      label_id = add_label(state, label_name, false);
    }
    
    // Pack source addresses and label ID into immediate value
    uint64_t imm_value = ((uint64_t)src_addr1 << 48) | 
                         ((uint64_t)src_addr2 << 32) | 
                         label_id;
    
    // Generate conditional jump instruction
    init_instruction(&instruction, op_code, 0, 0, imm_value);
    write_instruction(state, &instruction);
    
  } else if (strcmp(tokens[1], "LABEL") == 0) {
    if (num_tokens < 3) {
      fprintf(stderr, "LABEL requires a name\n");
      return -1;
    }
    
    const char *label_name = tokens[2];
    uint16_t label_id = find_label(state, label_name);
    
    if (label_id == 0) {
      // New label
      label_id = add_label(state, label_name, true);
    } else {
      // Update existing label
      for (size_t i = 0; i < state->labels.count; i++) {
        if (strcmp(state->labels.entries[i].name, label_name) == 0) {
          state->labels.entries[i].defined = true;
          break;
        }
      }
    }
    
    // Generate label definition instruction (will be skipped during execution)
    init_instruction(&instruction, OP_LABEL_DEF, 0, label_id, 0);
    write_instruction(state, &instruction);
    
  } else if (strcmp(tokens[1], "CALL") == 0) {
    if (num_tokens < 3) {
      fprintf(stderr, "CALL requires a function name\n");
      return -1;
    }
    
    const char *func_name = tokens[2];
    uint16_t func_id = find_label(state, func_name);
    
    if (func_id == 0) {
      // Forward reference, create the label
      func_id = add_label(state, func_name, false);
    }
    
    // Generate CALL instruction
    init_instruction(&instruction, OP_CALL, 0, 0, func_id);
    write_instruction(state, &instruction);
    
  } else if (strcmp(tokens[1], "RET") == 0) {
    // Generate RET instruction
    init_instruction(&instruction, OP_RET, 0, 0, 0);
    write_instruction(state, &instruction);
    
  } else if (strcmp(tokens[1], "PUSH") == 0) {
    if (num_tokens < 3) {
      fprintf(stderr, "PUSH requires a symbol\n");
      return -1;
    }
    
    const char *symbol_name = tokens[2];
    symbol_entry_t symbol;
    uint16_t addr = resolve_identifier(state, symbol_name, &symbol);
    
    // Generate PUSH instruction
    init_instruction(&instruction, OP_PUSH, symbol.type, addr, 0);
    write_instruction(state, &instruction);
    
  } else if (strcmp(tokens[1], "POP") == 0) {
    if (num_tokens < 3) {
      fprintf(stderr, "POP requires a symbol\n");
      return -1;
    }
    
    const char *symbol_name = tokens[2];
    symbol_entry_t symbol;
    
    if (find_symbol(state, symbol_name, &symbol) == 0) {
      // Symbol exists
      uint16_t addr = symbol.address;
      
      // Generate POP instruction
      init_instruction(&instruction, OP_POP, symbol.type, addr, 0);
      write_instruction(state, &instruction);
    } else {
      // Symbol doesn't exist, create it with default int64 type
      uint16_t addr = state->next_address;
      if (add_symbol(state, symbol_name, addr, MEM_INT64) != 0) {
        return -1;
      }
      
      // Generate POP instruction
      init_instruction(&instruction, OP_POP, MEM_INT64, addr, 0);
      write_instruction(state, &instruction);
    }
    
  } else if (strcmp(tokens[1], "SYSC") == 0) {
    if (num_tokens < 3) {
      fprintf(stderr, "SYSC requires a syscall number\n");
      return -1;
    }
    
    uint16_t syscall_num = (uint16_t)atoi(tokens[2]);
    
    // Generate SYSCALL instruction
    init_instruction(&instruction, OP_SYSCALL, 0, 0, syscall_num);
    write_instruction(state, &instruction);
    
    // Generate arguments instruction if we have any
    if (num_tokens > 3) {
      binary_instruction_t args_instruction;
      init_instruction(&args_instruction, OP_ARG_DATA, 0, 0, 0);
      
      // Pack up to 4 additional arguments
      for (int i = 3; i < num_tokens && i < 7; i++) {
        if (strncmp(tokens[i], "SIZE(", 5) == 0 && tokens[i][strlen(tokens[i])-1] == ')') {
          // Extract the identifier from SIZE(id)
          char id_str[64];
          strncpy(id_str, tokens[i] + 5, strlen(tokens[i]) - 6);
          id_str[strlen(tokens[i]) - 6] = '\0';
          
          symbol_entry_t symbol;
          if (find_symbol(state, id_str, &symbol) == 0) {
            // Use the size of the symbol type
            ((uint16_t*)&args_instruction.imm_value)[i-3] = get_type_size(symbol.type);
          } else {
            fprintf(stderr, "Symbol not found for SIZE: %s\n", id_str);
            ((uint16_t*)&args_instruction.imm_value)[i-3] = 0;
          }
        } else if (strncmp(tokens[i], "SIZEOF(", 7) == 0 && tokens[i][strlen(tokens[i])-1] == ')') {
          // Extract the type from SIZEOF(type)
          char type_str[64];
          strncpy(type_str, tokens[i] + 7, strlen(tokens[i]) - 8);
          type_str[strlen(tokens[i]) - 8] = '\0';
          
          mem_type_t type = hoil_type_to_coil_type(type_str);
          ((uint16_t*)&args_instruction.imm_value)[i-3] = get_type_size(type);
        } else {
          uint16_t arg = resolve_identifier(state, tokens[i], NULL);
          ((uint16_t*)&args_instruction.imm_value)[i-3] = arg;
        }
      }
      
      write_instruction(state, &args_instruction);
    }
    
  } else if (strcmp(tokens[1], "EXIT") == 0) {
    if (num_tokens < 3) {
      fprintf(stderr, "EXIT requires a status code\n");
      return -1;
    }
    
    int status = atoi(tokens[2]);
    
    // Generate EXIT instruction
    init_instruction(&instruction, OP_EXIT, 0, 0, status);
    write_instruction(state, &instruction);
    
  } else {
    fprintf(stderr, "Unknown CF operation: %s\n", tokens[1]);
    return -1;
  }
  
  return 0;
}

/**
* @brief Convert a HOIL instruction to COIL format
* 
* @param state Pointer to the converter state
* @param tokens Array of HOIL tokens
* @param num_tokens Number of tokens
* @return 0 on success, non-zero on failure
*/
int convert_instruction(converter_state_t *state, char **tokens, int num_tokens) {
  if (num_tokens < 1) {
    return 0; // Empty line
  }
  
  if (strcmp(tokens[0], "VAL") == 0) {
    return convert_val_instruction(state, tokens, num_tokens);
  } else if (strcmp(tokens[0], "MATH") == 0) {
    return convert_math_instruction(state, tokens, num_tokens);
  } else if (strcmp(tokens[0], "BIT") == 0) {
    return convert_bit_instruction(state, tokens, num_tokens);
  } else if (strcmp(tokens[0], "CF") == 0) {
    return convert_cf_instruction(state, tokens, num_tokens);
  } else {
    fprintf(stderr, "Unknown instruction: %s\n", tokens[0]);
    return -1;
  }
}

/**
* @brief Convert HOIL code to COIL format
* 
* @param input_filename Path to the HOIL file
* @param output_filename Path to the output COIL file
* @param binary_output Flag for binary output format
* @return 0 on success, non-zero on failure
*/
int convert_hoil_to_coil(const char *input_filename, const char *output_filename, bool binary_output) {
  FILE *input = fopen(input_filename, "r");
  if (!input) {
    perror("Failed to open input file");
    return -1;
  }
  
  FILE *output = fopen(output_filename, binary_output ? "wb" : "w");
  if (!output) {
    perror("Failed to open output file");
    fclose(input);
    return -1;
  }
  
  // Initialize converter state
  converter_state_t state;
  if (initialize_converter(&state, output, binary_output) != 0) {
    fprintf(stderr, "Failed to initialize converter\n");
    fclose(input);
    fclose(output);
    return -1;
  }
  
  // Write header in text mode
  if (!binary_output) {
    fprintf(output, "; COIL code generated from HOIL\n");
    fprintf(output, "; Original HOIL file: %s\n\n", input_filename);
  }
  
  char line[MAX_HOIL_LINE_LENGTH];
  int line_num = 0;
  
  while (fgets(line, sizeof(line), input)) {
    line_num++;
    
    // Skip comments and empty lines
    if (line[0] == ';' || line[0] == '\n') {
      continue;
    }
    
    // Make a copy of the line for tokenization
    char line_copy[MAX_HOIL_LINE_LENGTH];
    strncpy(line_copy, line, sizeof(line_copy));
    
    // Tokenize the line
    char *tokens[MAX_HOIL_TOKENS];
    int num_tokens = tokenize_hoil_line(line_copy, tokens, MAX_HOIL_TOKENS);
    
    // Convert the instruction
    if (convert_instruction(&state, tokens, num_tokens) != 0) {
      fprintf(stderr, "Failed to convert instruction on line %d: %s", line_num, line);
      fclose(input);
      fclose(output);
      return -1;
    }
  }
  
  // Check for undefined labels
  if (check_undefined_labels(&state) != 0) {
    fclose(input);
    fclose(output);
    return -1;
  }
  
  fclose(input);
  fclose(output);
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
  if (argc < 3) {
    fprintf(stderr, "Usage: %s [-b] <hoil_file> <coil_file>\n", argv[0]);
    fprintf(stderr, "  -b: Binary output mode (default is text mode)\n");
    return 1;
  }
  
  bool binary_output = false;
  const char *input_filename;
  const char *output_filename;
  
  if (strcmp(argv[1], "-b") == 0) {
    if (argc < 4) {
      fprintf(stderr, "Missing filenames after -b option\n");
      return 1;
    }
    
    binary_output = true;
    input_filename = argv[2];
    output_filename = argv[3];
  } else {
    input_filename = argv[1];
    output_filename = argv[2];
  }
  
  return convert_hoil_to_coil(input_filename, output_filename, binary_output);
}