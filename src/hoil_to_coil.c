/**
* @file hoil_to_coil.c
* @brief Converter from HOIL (Human Oriented Intermediate Language) to 
*        COIL (Computer Oriented Intermediate Language)
* 
* This program reads HOIL instructions and converts them to COIL format.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>

/**
* @brief Maximum line length for HOIL instructions
*/
#define MAX_LINE_LENGTH 256

/**
* @brief Maximum number of tokens in a HOIL instruction
*/
#define MAX_TOKENS 16

/**
* @brief Parses a HOIL line into tokens
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
* @brief Converts a HOIL data type to COIL type code
* 
* @param type_str HOIL type string
* @return COIL type code
*/
uint16_t convert_type(const char *type_str) {
  if (strcmp(type_str, "dint") == 0) {
    return 0x0008; // 64-bit integer
  } else if (strcmp(type_str, "int8") == 0) {
    return 0x0001;
  } else if (strcmp(type_str, "int16") == 0) {
    return 0x0002;
  } else if (strcmp(type_str, "int32") == 0) {
    return 0x0004;
  } else {
    fprintf(stderr, "Unknown type: %s\n", type_str);
    return 0;
  }
}

/**
* @brief Converts a HOIL value to COIL immediate value
* 
* @param value_str HOIL value string
* @return COIL immediate value
*/
uint16_t convert_value(const char *value_str) {
  if (strncmp(value_str, "id", 2) == 0) {
    // Extract the numeric part of an "idX" format
    return (uint16_t)atoi(value_str + 2);
  } else if (isdigit(value_str[0]) || value_str[0] == '-') {
    // Parse a numeric value
    return (uint16_t)atoi(value_str);
  } else {
    fprintf(stderr, "Unknown value format: %s\n", value_str);
    return 0;
  }
}

/**
* @brief Converts a HOIL identifier to COIL address
* 
* @param id_str HOIL identifier string
* @return COIL address
*/
uint16_t convert_identifier(const char *id_str) {
  if (id_str[0] == '&') {
    // Remove the '&' prefix for address references
    return (uint16_t)atoi(id_str + 1);
  } else {
    return (uint16_t)atoi(id_str);
  }
}

/**
* @brief Converts a HOIL instruction to COIL format
* 
* @param tokens Array of HOIL tokens
* @param num_tokens Number of tokens
* @param output Output file for COIL instructions
* @return 0 on success, non-zero on failure
*/
int convert_instruction(char **tokens, int num_tokens, FILE *output) {
  if (num_tokens < 1) {
    return 0; // Empty line
  }
  
  if (strcmp(tokens[0], "VAL") == 0) {
    if (num_tokens < 4) {
      fprintf(stderr, "VAL instruction requires at least 4 tokens\n");
      return -1;
    }
    
    if (strcmp(tokens[1], "DEFV") == 0) {
      uint16_t type_code = convert_type(tokens[2]);
      uint16_t dest_addr = convert_identifier(tokens[3]);
      uint16_t value = convert_value(tokens[4]);
      
      fprintf(output, "0001 %04X %04X %04X  ; ALLOC_IMM: Allocate memory at address %u with value %u\n",
              type_code, dest_addr, value, dest_addr, value);
    } else if (strcmp(tokens[1], "MOVV") == 0) {
      uint16_t type_code = convert_type(tokens[2]);
      uint16_t dest_addr = convert_identifier(tokens[3]);
      uint16_t src_addr = convert_identifier(tokens[4]);
      
      fprintf(output, "0002 %04X %04X %04X  ; ALLOC_MEM: Allocate memory at address %u with value from address %u\n",
              type_code, dest_addr, src_addr, dest_addr, src_addr);
    } else {
      fprintf(stderr, "Unknown VAL operation: %s\n", tokens[1]);
      return -1;
    }
  } else if (strcmp(tokens[0], "MATH") == 0) {
    if (num_tokens < 4) {
      fprintf(stderr, "MATH instruction requires at least 4 tokens\n");
      return -1;
    }
    
    if (strcmp(tokens[1], "ADD") == 0) {
      uint16_t dest_addr = convert_identifier(tokens[2]);
      uint16_t src_addr1 = convert_identifier(tokens[3]);
      uint16_t src_addr2 = convert_identifier(tokens[4]);
      
      fprintf(output, "0101 %04X %04X %04X  ; ADD: Add values from addresses %u and %u, store in address %u\n",
              dest_addr, src_addr1, src_addr2, src_addr1, src_addr2, dest_addr);
    } else if (strcmp(tokens[1], "SUB") == 0) {
      uint16_t dest_addr = convert_identifier(tokens[2]);
      uint16_t src_addr1 = convert_identifier(tokens[3]);
      uint16_t src_addr2 = convert_identifier(tokens[4]);
      
      fprintf(output, "0102 %04X %04X %04X  ; SUB: Subtract value at address %u from value at address %u, store in address %u\n",
              dest_addr, src_addr1, src_addr2, src_addr2, src_addr1, dest_addr);
    } else if (strcmp(tokens[1], "MUL") == 0) {
      uint16_t dest_addr = convert_identifier(tokens[2]);
      uint16_t src_addr1 = convert_identifier(tokens[3]);
      uint16_t src_addr2 = convert_identifier(tokens[4]);
      
      fprintf(output, "0103 %04X %04X %04X  ; MUL: Multiply values from addresses %u and %u, store in address %u\n",
              dest_addr, src_addr1, src_addr2, src_addr1, src_addr2, dest_addr);
    } else if (strcmp(tokens[1], "DIV") == 0) {
      uint16_t dest_addr = convert_identifier(tokens[2]);
      uint16_t src_addr1 = convert_identifier(tokens[3]);
      uint16_t src_addr2 = convert_identifier(tokens[4]);
      
      fprintf(output, "0104 %04X %04X %04X  ; DIV: Divide value at address %u by value at address %u, store in address %u\n",
              dest_addr, src_addr1, src_addr2, src_addr1, src_addr2, dest_addr);
    } else {
      fprintf(stderr, "Unknown MATH operation: %s\n", tokens[1]);
      return -1;
    }
  } else if (strcmp(tokens[0], "CF") == 0) {
    if (num_tokens < 3) {
      fprintf(stderr, "CF instruction requires at least 3 tokens\n");
      return -1;
    }
    
    if (strcmp(tokens[1], "SYSC") == 0) {
      uint16_t syscall_num = (uint16_t)atoi(tokens[2]);
      
      fprintf(output, "0201 %04X", syscall_num);
      
      for (int i = 3; i < num_tokens; i++) {
        if (strncmp(tokens[i], "SIZE(", 5) == 0 && tokens[i][strlen(tokens[i])-1] == ')') {
          // Extract the identifier from SIZE(id)
          char id_str[16];
          strncpy(id_str, tokens[i] + 5, strlen(tokens[i]) - 6);
          id_str[strlen(tokens[i]) - 6] = '\0';
          
          // Assume dint (8 bytes) for SIZE
          fprintf(output, " %04X", 0x0008);
        } else {
          uint16_t arg = convert_identifier(tokens[i]);
          fprintf(output, " %04X", arg);
        }
      }
      
      fprintf(output, "  ; SYSCALL: System call %u\n", syscall_num);
    } else {
      fprintf(stderr, "Unknown CF operation: %s\n", tokens[1]);
      return -1;
    }
  } else {
    fprintf(stderr, "Unknown instruction: %s\n", tokens[0]);
    return -1;
  }
  
  return 0;
}

/**
* @brief Converts HOIL code to COIL format
* 
* @param input_filename Path to the HOIL file
* @param output_filename Path to the output COIL file
* @return 0 on success, non-zero on failure
*/
int convert_hoil_to_coil(const char *input_filename, const char *output_filename) {
  FILE *input = fopen(input_filename, "r");
  if (!input) {
    perror("fopen");
    return -1;
  }
  
  FILE *output = fopen(output_filename, "w");
  if (!output) {
    perror("fopen");
    fclose(input);
    return -1;
  }
  
  fprintf(output, "; COIL code generated from HOIL\n");
  fprintf(output, "; Original HOIL file: %s\n\n", input_filename);
  
  char line[MAX_LINE_LENGTH];
  while (fgets(line, sizeof(line), input)) {
    // Skip comments and empty lines
    if (line[0] == ';' || line[0] == '\n') {
      continue;
    }
    
    // Make a copy of the line for tokenization
    char line_copy[MAX_LINE_LENGTH];
    strncpy(line_copy, line, sizeof(line_copy));
    
    // Tokenize the line
    char *tokens[MAX_TOKENS];
    int num_tokens = tokenize_hoil_line(line_copy, tokens, MAX_TOKENS);
    
    // Convert the instruction
    if (convert_instruction(tokens, num_tokens, output) != 0) {
      fprintf(stderr, "Failed to convert instruction: %s", line);
      fclose(input);
      fclose(output);
      return -1;
    }
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
    fprintf(stderr, "Usage: %s <hoil_file> <coil_file>\n", argv[0]);
    return 1;
  }
  
  return convert_hoil_to_coil(argv[1], argv[2]);
}