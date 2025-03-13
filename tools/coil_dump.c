/**
 * @file coil_dump.c
 * @brief Utility to dump the contents of a COIL binary file.
 * 
 * This file contains a tool to display the contents of a COIL binary.
 * 
 * @author HOILC Team
 * @date 2025
 */

#include "../include/binary.h"
#include "../include/util.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * @brief Display usage information.
 * 
 * @param program_name The name of the program.
 */
static void print_usage(const char* program_name) {
  fprintf(stderr, "Usage: %s <coil_file>\n", program_name);
}

/**
 * @brief Display the contents of the COIL header.
 * 
 * @param header The COIL header.
 */
static void print_header(const coil_header_t* header) {
  printf("=== COIL Header ===\n");
  printf("Magic: 0x%08X (", header->magic);
  
  /* Print magic as ASCII */
  for (int i = 0; i < 4; i++) {
    char c = (header->magic >> (i * 8)) & 0xFF;
    printf("%c", c);
  }
  printf(")\n");
  
  /* Extract version components */
  uint8_t major = (header->version >> 24) & 0xFF;
  uint8_t minor = (header->version >> 16) & 0xFF;
  uint8_t patch = (header->version >> 8) & 0xFF;
  
  printf("Version: %u.%u.%u\n", major, minor, patch);
  printf("Section Count: %u\n", header->section_count);
  printf("Flags: 0x%08X\n", header->flags);
}

/**
 * @brief Display the contents of the COIL section table.
 * 
 * @param sections The section headers.
 * @param count The number of sections.
 */
static void print_section_table(const section_header_t* sections, uint32_t count) {
  printf("\n=== Section Table ===\n");
  printf("%-15s %-10s %-10s\n", "Type", "Offset", "Size");
  printf("----------------------------------------\n");
  
  const char* section_names[SECTION_COUNT] = {
    "Type",
    "Function",
    "Global",
    "Constant",
    "Code",
    "Relocation",
    "Metadata"
  };
  
  for (uint32_t i = 0; i < count; i++) {
    const char* type_name = "Unknown";
    if (sections[i].section_type < SECTION_COUNT) {
      type_name = section_names[sections[i].section_type];
    }
    
    printf("%-15s 0x%08X 0x%08X\n", 
           type_name, sections[i].offset, sections[i].size);
  }
}

/**
 * @brief Display the contents of the type section.
 * 
 * @param data The section data.
 * @param size The section size.
 */
static void print_type_section(const uint8_t* data, uint32_t size) {
  printf("\n=== Type Section ===\n");
  
  /* This is a simplified implementation; a full implementation would parse the type entries */
  printf("Raw data (%u bytes):\n", size);
  
  /* Print at most the first 64 bytes */
  uint32_t display_size = size > 64 ? 64 : size;
  for (uint32_t i = 0; i < display_size; i++) {
    printf("%02X ", data[i]);
    if ((i + 1) % 16 == 0) {
      printf("\n");
    }
  }
  
  if (display_size % 16 != 0) {
    printf("\n");
  }
  
  if (display_size < size) {
    printf("... (%u more bytes)\n", size - display_size);
  }
}

/**
 * @brief Display the contents of the function section.
 * 
 * @param data The section data.
 * @param size The section size.
 */
static void print_function_section(const uint8_t* data, uint32_t size) {
  printf("\n=== Function Section ===\n");
  
  /* This is a simplified implementation; a full implementation would parse the function entries */
  printf("Raw data (%u bytes):\n", size);
  
  /* Print at most the first 64 bytes */
  uint32_t display_size = size > 64 ? 64 : size;
  for (uint32_t i = 0; i < display_size; i++) {
    printf("%02X ", data[i]);
    if ((i + 1) % 16 == 0) {
      printf("\n");
    }
  }
  
  if (display_size % 16 != 0) {
    printf("\n");
  }
  
  if (display_size < size) {
    printf("... (%u more bytes)\n", size - display_size);
  }
}

/**
 * @brief Main function.
 * 
 * @param argc Number of command-line arguments.
 * @param argv Array of command-line arguments.
 * @return 0 on success, non-zero on failure.
 */
int main(int argc, char** argv) {
  /* Check arguments */
  if (argc < 2) {
    print_usage(argv[0]);
    return 1;
  }
  
  const char* filename = argv[1];
  
  /* Read the COIL binary file */
  uint8_t* data = NULL;
  size_t size = 0;
  
  if (!util_read_file(filename, (char**)&data, &size)) {
    fprintf(stderr, "Error: Failed to read file %s\n", filename);
    return 1;
  }
  
  /* Check file size */
  if (size < sizeof(coil_header_t)) {
    fprintf(stderr, "Error: File is too small to be a valid COIL binary\n");
    free(data);
    return 1;
  }
  
  /* Parse the header */
  coil_header_t header;
  memcpy(&header, data, sizeof(coil_header_t));
  
  /* Check magic */
  if (header.magic != COIL_MAGIC) {
    fprintf(stderr, "Error: Invalid COIL binary (magic mismatch)\n");
    free(data);
    return 1;
  }
  
  /* Display the header */
  print_header(&header);
  
  /* Check if we have enough data for the section table */
  size_t section_table_size = header.section_count * sizeof(section_header_t);
  if (size < sizeof(coil_header_t) + section_table_size) {
    fprintf(stderr, "Error: File is too small to contain the section table\n");
    free(data);
    return 1;
  }
  
  /* Parse the section table */
  section_header_t* sections = (section_header_t*)(data + sizeof(coil_header_t));
  
  /* Display the section table */
  print_section_table(sections, header.section_count);
  
  /* Display individual sections */
  for (uint32_t i = 0; i < header.section_count; i++) {
    if (sections[i].offset + sections[i].size > size) {
      fprintf(stderr, "Error: Section %u extends beyond file size\n", i);
      continue;
    }
    
    const uint8_t* section_data = data + sections[i].offset;
    
    switch (sections[i].section_type) {
      case SECTION_TYPE:
        print_type_section(section_data, sections[i].size);
        break;
        
      case SECTION_FUNCTION:
        print_function_section(section_data, sections[i].size);
        break;
        
      /* Additional section types can be handled here */
      
      default:
        /* Skip unknown or unhandled sections */
        break;
    }
  }
  
  /* Clean up */
  free(data);
  
  return 0;
}