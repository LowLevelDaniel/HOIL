/**
 * @file binary.c
 * @brief Implementation of COIL binary format handling.
 * 
 * This file contains the implementation of the structures and functions for
 * handling the COIL binary format.
 * 
 * @author HOILC Team
 * @date 2025
 */

#include "../include/binary.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/**
 * @brief Section structure.
 */
typedef struct {
  section_type_t type;    /**< Section type. */
  uint8_t* data;          /**< Section data. */
  size_t size;            /**< Section size in bytes. */
  size_t capacity;        /**< Section capacity in bytes. */
} section_t;

/**
 * @brief Type entry structure.
 */
typedef struct {
  type_encoding_t encoding;  /**< Type encoding. */
  char* name;                /**< Type name (can be NULL). */
} type_entry_t;

/**
 * @brief Function entry structure.
 */
typedef struct {
  char* name;              /**< Function name. */
  int32_t return_type;     /**< Return type index. */
  int32_t* param_types;    /**< Parameter type indices. */
  uint32_t param_count;    /**< Number of parameters. */
  bool is_external;        /**< Whether the function is external. */
} function_entry_t;

/**
 * @brief Global variable entry structure.
 */
typedef struct {
  char* name;              /**< Global variable name. */
  int32_t type;            /**< Variable type index. */
  uint8_t* initializer;    /**< Constant initializer data (can be NULL). */
  size_t initializer_size; /**< Size of the initializer in bytes. */
} global_entry_t;

/**
 * @brief Basic block structure.
 */
typedef struct {
  char* name;              /**< Block name. */
  uint8_t* code;           /**< Block code. */
  size_t code_size;        /**< Block code size in bytes. */
  size_t code_capacity;    /**< Block code capacity in bytes. */
} basic_block_t;

/**
 * @brief Function code structure.
 */
typedef struct {
  int32_t function;        /**< Function index. */
  basic_block_t** blocks;  /**< Basic blocks. */
  size_t block_count;      /**< Number of blocks. */
  size_t block_capacity;   /**< Capacity of blocks array. */
  int32_t current_block;   /**< Current block index. */
} function_code_t;

/**
 * @brief COIL binary builder structure.
 */
struct coil_builder {
  section_t sections[SECTION_COUNT];   /**< Sections. */
  type_entry_t* types;                 /**< Type entries. */
  size_t type_count;                   /**< Number of types. */
  size_t type_capacity;                /**< Capacity of types array. */
  function_entry_t* functions;         /**< Function entries. */
  size_t function_count;               /**< Number of functions. */
  size_t function_capacity;            /**< Capacity of functions array. */
  global_entry_t* globals;             /**< Global variable entries. */
  size_t global_count;                 /**< Number of global variables. */
  size_t global_capacity;              /**< Capacity of globals array. */
  function_code_t* current_function;   /**< Current function code. */
  char* module_name;                   /**< Module name. */
};

/**
 * @brief Predefined type encodings.
 */
static const type_encoding_t predefined_types[PREDEFINED_COUNT] = {
  /* PREDEFINED_VOID */
  0x00000000,
  
  /* PREDEFINED_BOOL */
  0x01000001,
  
  /* PREDEFINED_INT8 */
  0x02080000,
  
  /* PREDEFINED_UINT8 */
  0x02080100,
  
  /* PREDEFINED_INT16 */
  0x02100000,
  
  /* PREDEFINED_UINT16 */
  0x02100100,
  
  /* PREDEFINED_INT32 */
  0x02200000,
  
  /* PREDEFINED_UINT32 */
  0x02200100,
  
  /* PREDEFINED_INT64 */
  0x02400000,
  
  /* PREDEFINED_UINT64 */
  0x02400100,
  
  /* PREDEFINED_FLOAT16 */
  0x03100000,
  
  /* PREDEFINED_FLOAT32 */
  0x03200000,
  
  /* PREDEFINED_FLOAT64 */
  0x03400000,
  
  /* PREDEFINED_PTR */
  0x04400000,
};

/**
 * @brief Initialize a section.
 * 
 * @param section The section to initialize.
 * @param type The section type.
 * @return true on success, false on memory allocation failure.
 */
static bool init_section(section_t* section, section_type_t type) {
  assert(section != NULL);
  
  section->type = type;
  section->size = 0;
  section->capacity = 1024;  /* Initial capacity */
  
  section->data = (uint8_t*)malloc(section->capacity);
  if (section->data == NULL) {
    return false;
  }
  
  return true;
}

/**
 * @brief Free a section.
 * 
 * @param section The section to free.
 */
static void free_section(section_t* section) {
  assert(section != NULL);
  
  free(section->data);
  section->data = NULL;
  section->size = 0;
  section->capacity = 0;
}

/**
 * @brief Ensure that a section has enough capacity for additional data.
 * 
 * @param section The section.
 * @param additional The number of additional bytes needed.
 * @return true on success, false on memory allocation failure.
 */
static bool ensure_section_capacity(section_t* section, size_t additional) {
  assert(section != NULL);
  
  if (section->size + additional <= section->capacity) {
    return true;
  }
  
  size_t new_capacity = section->capacity;
  while (new_capacity < section->size + additional) {
    new_capacity *= 2;
  }
  
  uint8_t* new_data = (uint8_t*)realloc(section->data, new_capacity);
  if (new_data == NULL) {
    return false;
  }
  
  section->data = new_data;
  section->capacity = new_capacity;
  
  return true;
}

/**
 * @brief Append data to a section.
 * 
 * @param section The section.
 * @param data The data to append.
 * @param size The size of the data.
 * @return true on success, false on memory allocation failure.
 */
static bool append_to_section(section_t* section, const void* data, size_t size) {
  assert(section != NULL);
  assert(data != NULL || size == 0);
  
  if (!ensure_section_capacity(section, size)) {
    return false;
  }
  
  memcpy(section->data + section->size, data, size);
  section->size += size;
  
  return true;
}

/**
 * @brief Append a 32-bit integer to a section.
 * 
 * @param section The section.
 * @param value The value to append.
 * @return true on success, false on memory allocation failure.
 */
static bool append_uint32(section_t* section, uint32_t value) {
  return append_to_section(section, &value, sizeof(value));
}

/**
 * @brief Append a string to a section.
 * 
 * @param section The section.
 * @param str The string to append.
 * @return true on success, false on memory allocation failure.
 */
static bool append_string(section_t* section, const char* str) {
  assert(section != NULL);
  assert(str != NULL);
  
  size_t len = strlen(str);
  if (!append_uint32(section, (uint32_t)len)) {
    return false;
  }
  
  return append_to_section(section, str, len);
}

/**
 * @brief Free a basic block.
 * 
 * @param block The basic block to free.
 */
static void free_basic_block(basic_block_t* block) {
  assert(block != NULL);
  
  free(block->name);
  free(block->code);
  free(block);
}

/**
 * @brief Free a function code structure.
 * 
 * @param function_code The function code structure to free.
 */
static void free_function_code(function_code_t* function_code) {
  assert(function_code != NULL);
  
  for (size_t i = 0; i < function_code->block_count; i++) {
    free_basic_block(function_code->blocks[i]);
  }
  
  free(function_code->blocks);
  free(function_code);
}

/**
 * @brief Find a basic block by name.
 * 
 * @param function_code The function code.
 * @param name The block name.
 * @return The block index or -1 if not found.
 */
static int32_t find_block_by_name(function_code_t* function_code, const char* name) {
  assert(function_code != NULL);
  assert(name != NULL);
  
  for (size_t i = 0; i < function_code->block_count; i++) {
    if (strcmp(function_code->blocks[i]->name, name) == 0) {
      return (int32_t)i;
    }
  }
  
  return -1;
}

coil_builder_t* coil_builder_create(void) {
  coil_builder_t* builder = (coil_builder_t*)malloc(sizeof(coil_builder_t));
  if (builder == NULL) {
    return NULL;
  }
  
  /* Initialize sections */
  for (int i = 0; i < SECTION_COUNT; i++) {
    if (!init_section(&builder->sections[i], (section_type_t)i)) {
      /* Free previously allocated sections */
      for (int j = 0; j < i; j++) {
        free_section(&builder->sections[j]);
      }
      
      free(builder);
      return NULL;
    }
  }
  
  /* Initialize arrays */
  builder->types = NULL;
  builder->type_count = 0;
  builder->type_capacity = 0;
  
  builder->functions = NULL;
  builder->function_count = 0;
  builder->function_capacity = 0;
  
  builder->globals = NULL;
  builder->global_count = 0;
  builder->global_capacity = 0;
  
  builder->current_function = NULL;
  builder->module_name = NULL;
  
  /* Add predefined types */
  for (int i = 0; i < PREDEFINED_COUNT; i++) {
    if (coil_builder_add_type(builder, predefined_types[i], NULL) < 0) {
      coil_builder_destroy(builder);
      return NULL;
    }
  }
  
  return builder;
}

void coil_builder_destroy(coil_builder_t* builder) {
  if (builder == NULL) {
    return;
  }
  
  /* Free sections */
  for (int i = 0; i < SECTION_COUNT; i++) {
    free_section(&builder->sections[i]);
  }
  
  /* Free types */
  for (size_t i = 0; i < builder->type_count; i++) {
    free(builder->types[i].name);
  }
  free(builder->types);
  
  /* Free functions */
  for (size_t i = 0; i < builder->function_count; i++) {
    free(builder->functions[i].name);
    free(builder->functions[i].param_types);
  }
  free(builder->functions);
  
  /* Free globals */
  for (size_t i = 0; i < builder->global_count; i++) {
    free(builder->globals[i].name);
    free(builder->globals[i].initializer);
  }
  free(builder->globals);
  
  /* Free current function */
  if (builder->current_function != NULL) {
    free_function_code(builder->current_function);
  }
  
  /* Free module name */
  free(builder->module_name);
  
  free(builder);
}

bool coil_builder_set_module_name(coil_builder_t* builder, const char* name) {
  assert(builder != NULL);
  assert(name != NULL);
  
  free(builder->module_name);
  builder->module_name = strdup(name);
  
  return builder->module_name != NULL;
}

int32_t coil_builder_add_type(coil_builder_t* builder, type_encoding_t encoding, 
                              const char* name) {
  assert(builder != NULL);
  
  /* Check if we need to resize the types array */
  if (builder->type_count >= builder->type_capacity) {
    size_t new_capacity = builder->type_capacity == 0 ? 16 : builder->type_capacity * 2;
    type_entry_t* new_types = (type_entry_t*)realloc(
      builder->types, new_capacity * sizeof(type_entry_t)
    );
    
    if (new_types == NULL) {
      return -1;
    }
    
    builder->types = new_types;
    builder->type_capacity = new_capacity;
  }
  
  /* Add the type */
  int32_t type_index = (int32_t)builder->type_count;
  builder->types[type_index].encoding = encoding;
  
  if (name != NULL) {
    builder->types[type_index].name = strdup(name);
    if (builder->types[type_index].name == NULL) {
      return -1;
    }
  } else {
    builder->types[type_index].name = NULL;
  }
  
  builder->type_count++;
  
  return type_index;
}

int32_t coil_builder_add_struct_type(coil_builder_t* builder, int32_t* field_types, 
                                    uint32_t field_count, const char* name) {
  assert(builder != NULL);
  assert(field_types != NULL || field_count == 0);
  
  /* Create the structure type encoding */
  /* Format: [category:4][width:8][qualifiers:8][attributes:12] */
  type_encoding_t encoding = (TYPE_STRUCTURE << 28) | (field_count & 0xFFF);
  
  /* Add the type */
  int32_t type_index = coil_builder_add_type(builder, encoding, name);
  if (type_index < 0) {
    return -1;
  }
  
  /* Add the field types to the type section */
  section_t* type_section = &builder->sections[SECTION_TYPE];
  
  /* Append the type index */
  if (!append_uint32(type_section, (uint32_t)type_index)) {
    return -1;
  }
  
  /* Append the field count */
  if (!append_uint32(type_section, field_count)) {
    return -1;
  }
  
  /* Append the field types */
  for (uint32_t i = 0; i < field_count; i++) {
    if (!append_uint32(type_section, (uint32_t)field_types[i])) {
      return -1;
    }
  }
  
  return type_index;
}

int32_t coil_builder_add_function(coil_builder_t* builder, const char* name, 
                                 int32_t return_type, int32_t* param_types, 
                                 uint32_t param_count, bool is_external) {
  assert(builder != NULL);
  assert(name != NULL);
  assert(param_types != NULL || param_count == 0);
  
  /* Check if we need to resize the functions array */
  if (builder->function_count >= builder->function_capacity) {
    size_t new_capacity = builder->function_capacity == 0 ? 16 : builder->function_capacity * 2;
    function_entry_t* new_functions = (function_entry_t*)realloc(
      builder->functions, new_capacity * sizeof(function_entry_t)
    );
    
    if (new_functions == NULL) {
      return -1;
    }
    
    builder->functions = new_functions;
    builder->function_capacity = new_capacity;
  }
  
  /* Add the function */
  int32_t function_index = (int32_t)builder->function_count;
  builder->functions[function_index].name = strdup(name);
  if (builder->functions[function_index].name == NULL) {
    return -1;
  }
  
  builder->functions[function_index].return_type = return_type;
  builder->functions[function_index].is_external = is_external;
  
  if (param_count > 0) {
    builder->functions[function_index].param_types = (int32_t*)malloc(param_count * sizeof(int32_t));
    if (builder->functions[function_index].param_types == NULL) {
      free(builder->functions[function_index].name);
      return -1;
    }
    
    memcpy(builder->functions[function_index].param_types, param_types, 
           param_count * sizeof(int32_t));
  } else {
    builder->functions[function_index].param_types = NULL;
  }
  
  builder->functions[function_index].param_count = param_count;
  
  builder->function_count++;
  
  /* Add the function to the function section */
  section_t* function_section = &builder->sections[SECTION_FUNCTION];
  
  /* Append the function index */
  if (!append_uint32(function_section, (uint32_t)function_index)) {
    return -1;
  }
  
  /* Append the function name */
  if (!append_string(function_section, name)) {
    return -1;
  }
  
  /* Append the return type */
  if (!append_uint32(function_section, (uint32_t)return_type)) {
    return -1;
  }
  
  /* Append the parameter count */
  if (!append_uint32(function_section, param_count)) {
    return -1;
  }
  
  /* Append the parameter types */
  for (uint32_t i = 0; i < param_count; i++) {
    if (!append_uint32(function_section, (uint32_t)param_types[i])) {
      return -1;
    }
  }
  
  /* Append is_external flag */
  if (!append_uint32(function_section, is_external ? 1 : 0)) {
    return -1;
  }
  
  return function_index;
}

int32_t coil_builder_add_global(coil_builder_t* builder, const char* name, 
                               int32_t type, const void* initializer, 
                               size_t initializer_size) {
  assert(builder != NULL);
  assert(name != NULL);
  assert(initializer != NULL || initializer_size == 0);
  
  /* Check if we need to resize the globals array */
  if (builder->global_count >= builder->global_capacity) {
    size_t new_capacity = builder->global_capacity == 0 ? 16 : builder->global_capacity * 2;
    global_entry_t* new_globals = (global_entry_t*)realloc(
      builder->globals, new_capacity * sizeof(global_entry_t)
    );
    
    if (new_globals == NULL) {
      return -1;
    }
    
    builder->globals = new_globals;
    builder->global_capacity = new_capacity;
  }
  
  /* Add the global */
  int32_t global_index = (int32_t)builder->global_count;
  builder->globals[global_index].name = strdup(name);
  if (builder->globals[global_index].name == NULL) {
    return -1;
  }
  
  builder->globals[global_index].type = type;
  
  if (initializer_size > 0) {
    builder->globals[global_index].initializer = (uint8_t*)malloc(initializer_size);
    if (builder->globals[global_index].initializer == NULL) {
      free(builder->globals[global_index].name);
      return -1;
    }
    
    memcpy(builder->globals[global_index].initializer, initializer, initializer_size);
  } else {
    builder->globals[global_index].initializer = NULL;
  }
  
  builder->globals[global_index].initializer_size = initializer_size;
  
  builder->global_count++;
  
  /* Add the global to the global section */
  section_t* global_section = &builder->sections[SECTION_GLOBAL];
  
  /* Append the global index */
  if (!append_uint32(global_section, (uint32_t)global_index)) {
    return -1;
  }
  
  /* Append the global name */
  if (!append_string(global_section, name)) {
    return -1;
  }
  
  /* Append the type */
  if (!append_uint32(global_section, (uint32_t)type)) {
    return -1;
  }
  
  /* Append the initializer size */
  if (!append_uint32(global_section, (uint32_t)initializer_size)) {
    return -1;
  }
  
  /* Append the initializer data */
  if (initializer_size > 0) {
    if (!append_to_section(global_section, initializer, initializer_size)) {
      return -1;
    }
  }
  
  return global_index;
}

bool coil_builder_begin_function_code(coil_builder_t* builder, int32_t function) {
  assert(builder != NULL);
  assert(function >= 0 && function < (int32_t)builder->function_count);
  assert(builder->current_function == NULL);
  
  /* Create a new function code structure */
  function_code_t* func_code = (function_code_t*)malloc(sizeof(function_code_t));
  if (func_code == NULL) {
    return false;
  }
  
  func_code->function = function;
  func_code->blocks = NULL;
  func_code->block_count = 0;
  func_code->block_capacity = 0;
  func_code->current_block = -1;
  
  builder->current_function = func_code;
  
  return true;
}

int32_t coil_builder_add_block(coil_builder_t* builder, const char* name) {
  assert(builder != NULL);
  assert(name != NULL);
  assert(builder->current_function != NULL);
  
  function_code_t* func_code = builder->current_function;
  
  /* Check if the block already exists */
  int32_t block_index = find_block_by_name(func_code, name);
  if (block_index >= 0) {
    func_code->current_block = block_index;
    return block_index;
  }
  
  /* Check if we need to resize the blocks array */
  if (func_code->block_count >= func_code->block_capacity) {
    size_t new_capacity = func_code->block_capacity == 0 ? 4 : func_code->block_capacity * 2;
    basic_block_t** new_blocks = (basic_block_t**)realloc(
      func_code->blocks, new_capacity * sizeof(basic_block_t*)
    );
    
    if (new_blocks == NULL) {
      return -1;
    }
    
    func_code->blocks = new_blocks;
    func_code->block_capacity = new_capacity;
  }
  
  /* Create a new block */
  basic_block_t* block = (basic_block_t*)malloc(sizeof(basic_block_t));
  if (block == NULL) {
    return -1;
  }
  
  block->name = strdup(name);
  if (block->name == NULL) {
    free(block);
    return -1;
  }
  
  block->code = NULL;
  block->code_size = 0;
  block->code_capacity = 0;
  
  /* Add the block */
  block_index = (int32_t)func_code->block_count;
  func_code->blocks[block_index] = block;
  func_code->block_count++;
  func_code->current_block = block_index;
  
  return block_index;
}

bool coil_builder_add_instruction(coil_builder_t* builder, uint8_t opcode, 
                                  uint8_t flags, uint8_t destination, 
                                  uint8_t* operands, uint8_t operand_count) {
  assert(builder != NULL);
  assert(builder->current_function != NULL);
  assert(builder->current_function->current_block >= 0);
  assert(operands != NULL || operand_count == 0);
  
  function_code_t* func_code = builder->current_function;
  basic_block_t* block = func_code->blocks[func_code->current_block];
  
  /* Ensure sufficient capacity */
  size_t required_size = 4 + operand_count;  /* opcode + flags + operand_count + destination + operands */
  if (block->code_size + required_size > block->code_capacity) {
    size_t new_capacity = block->code_capacity == 0 ? 64 : block->code_capacity * 2;
    while (new_capacity < block->code_size + required_size) {
      new_capacity *= 2;
    }
    
    uint8_t* new_code = (uint8_t*)realloc(block->code, new_capacity);
    if (new_code == NULL) {
      return false;
    }
    
    block->code = new_code;
    block->code_capacity = new_capacity;
  }
  
  /* Append the instruction */
  block->code[block->code_size++] = opcode;
  block->code[block->code_size++] = flags;
  block->code[block->code_size++] = operand_count;
  block->code[block->code_size++] = destination;
  
  /* Append the operands */
  for (uint8_t i = 0; i < operand_count; i++) {
    block->code[block->code_size++] = operands[i];
  }
  
  return true;
}

bool coil_builder_end_function_code(coil_builder_t* builder) {
  assert(builder != NULL);
  assert(builder->current_function != NULL);
  
  function_code_t* func_code = builder->current_function;
  
  /* Add the function code to the code section */
  section_t* code_section = &builder->sections[SECTION_CODE];
  
  /* Append the function index */
  if (!append_uint32(code_section, (uint32_t)func_code->function)) {
    return false;
  }
  
  /* Append the block count */
  if (!append_uint32(code_section, (uint32_t)func_code->block_count)) {
    return false;
  }
  
  /* Append each block */
  for (size_t i = 0; i < func_code->block_count; i++) {
    basic_block_t* block = func_code->blocks[i];
    
    /* Append the block name */
    if (!append_string(code_section, block->name)) {
      return false;
    }
    
    /* Append the code size */
    if (!append_uint32(code_section, (uint32_t)block->code_size)) {
      return false;
    }
    
    /* Append the code */
    if (!append_to_section(code_section, block->code, block->code_size)) {
      return false;
    }
  }
  
  /* Free the function code */
  free_function_code(func_code);
  builder->current_function = NULL;
  
  return true;
}

bool coil_builder_build(coil_builder_t* builder, uint8_t** output, size_t* size) {
  assert(builder != NULL);
  assert(output != NULL);
  assert(size != NULL);
  
  /* Calculate the total size */
  size_t total_size = 16;  /* Header size */
  total_size += SECTION_COUNT * 12;  /* Section table size */
  
  for (int i = 0; i < SECTION_COUNT; i++) {
    total_size += builder->sections[i].size;
    
    /* Pad to 4-byte boundary */
    total_size = (total_size + 3) & ~3;
  }
  
  /* Allocate the output buffer */
  uint8_t* buffer = (uint8_t*)malloc(total_size);
  if (buffer == NULL) {
    return false;
  }
  
  /* Write the header */
  coil_header_t header;
  header.magic = COIL_MAGIC;
  header.version = 0x00010000;  /* Version 1.0.0 */
  header.section_count = SECTION_COUNT;
  header.flags = 0;
  
  memcpy(buffer, &header, sizeof(header));
  
  /* Write the section table */
  section_header_t section_headers[SECTION_COUNT];
  size_t offset = 16 + SECTION_COUNT * 12;  /* Header + section table */
  
  for (int i = 0; i < SECTION_COUNT; i++) {
    section_headers[i].section_type = i;
    section_headers[i].offset = (uint32_t)offset;
    section_headers[i].size = (uint32_t)builder->sections[i].size;
    
    /* Copy section data */
    memcpy(buffer + offset, builder->sections[i].data, builder->sections[i].size);
    
    offset += builder->sections[i].size;
    
    /* Pad to 4-byte boundary */
    while (offset % 4 != 0) {
      buffer[offset++] = 0;
    }
  }
  
  /* Write the section table */
  memcpy(buffer + 16, section_headers, sizeof(section_headers));
  
  /* Set the output */
  *output = buffer;
  *size = total_size;
  
  return true;
}

type_encoding_t coil_create_type_encoding(type_category_t category, uint8_t width, 
                                         uint8_t qualifiers, uint16_t attributes) {
  return ((uint32_t)category << 28) | ((uint32_t)width << 20) | 
         ((uint32_t)qualifiers << 12) | attributes;
}

type_encoding_t coil_get_predefined_type(int type) {
  assert(type >= 0 && type < PREDEFINED_COUNT);
  
  return predefined_types[type];
}