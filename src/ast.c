/**
 * @file ast.c
 * @brief Implementation of Abstract Syntax Tree functions for HOIL
 * 
 * This file implements functions for creating, manipulating, and destroying
 * Abstract Syntax Tree (AST) nodes that represent HOIL code.
 *
 * @author Generated by Claude
 * @date 2025-03-13
 */

#include "ast.h"
#include "error_handling.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/**
 * @brief Helper function to duplicate a string
 * 
 * @param str String to duplicate
 * @return Duplicated string or NULL on error
 */
static char* strdup_safe(const char* str) {
  if (str == NULL) {
    return NULL;
  }
  
  size_t len = strlen(str);
  char* dup = (char*)malloc(len + 1);
  if (dup == NULL) {
    error_report(ERROR_MEMORY, "Failed to allocate memory for string duplication");
    return NULL;
  }
  
  memcpy(dup, str, len + 1);
  return dup;
}

/**
 * @brief Create a new empty module
 * 
 * @param name Module name
 * @return New module or NULL on error
 */
ast_module_t* ast_module_create(const char* name) {
  if (name == NULL) {
    error_report(ERROR_INVALID_ARGUMENT, "Module name cannot be NULL");
    return NULL;
  }
  
  ast_module_t* module = (ast_module_t*)malloc(sizeof(ast_module_t));
  if (module == NULL) {
    error_report(ERROR_MEMORY, "Failed to allocate memory for module");
    return NULL;
  }
  
  module->name = strdup_safe(name);
  if (module->name == NULL) {
    free(module);
    return NULL;
  }
  
  module->types = NULL;
  module->type_count = 0;
  module->globals = NULL;
  module->global_count = 0;
  module->constants = NULL;
  module->constant_count = 0;
  module->functions = NULL;
  module->function_count = 0;
  module->target = NULL;
  
  return module;
}

/**
 * @brief Destroy a module and all its contents
 * 
 * @param module Module to destroy
 */
void ast_module_destroy(ast_module_t* module) {
  if (module == NULL) {
    return;
  }
  
  // Free module name
  if (module->name != NULL) {
    free((void*)module->name);
    module->name = NULL;
  }

  // Free types
  if (module->types != NULL) {
    for (uint32_t i = 0; i < module->type_count; i++) {
      if (module->types[i] != NULL) {
          // Free type name
          if (module->types[i]->name != NULL) {
            free((void*)module->types[i]->name);
          }
          
          // Free structure fields if present
          if (module->types[i]->fields != NULL) {
            for (uint32_t j = 0; j < module->types[i]->field_count; j++) {
              if (module->types[i]->fields[j].name != NULL) {
                free((void*)module->types[i]->fields[j].name);
              }
              if (module->types[i]->fields[j].type != NULL) {
                ast_type_destroy(module->types[i]->fields[j].type);
              }
            }
            free(module->types[i]->fields);
          }
          
          // Check if structure name needs to be freed
          if (module->types[i]->type.category == TYPE_STRUCTURE && 
            module->types[i]->type.structure.name != NULL &&
            module->types[i]->type.structure.name != module->types[i]->name) {
            free((void*)module->types[i]->type.structure.name);
          }
          free(module->types[i]);
        }
    }
    free(module->types);
    module->types = NULL;
  }
  
  // Free globals
  for (uint32_t i = 0; i < module->global_count; i++) {
    ast_global_destroy(module->globals[i]);
  }
  free(module->globals);
  
  // Free constants
  for (uint32_t i = 0; i < module->constant_count; i++) {
    ast_constant_destroy(module->constants[i]);
  }
  free(module->constants);
  
  // Free functions
  for (uint32_t i = 0; i < module->function_count; i++) {
    ast_function_destroy(module->functions[i]);
  }
  free(module->functions);
  
  // Free target
  if (module->target != NULL) {
    ast_target_destroy(module->target);
  }
  
  free(module);
}

/**
 * @brief Create a new type definition
 * 
 * @param name Type name
 * @param category Type category
 * @return New type definition or NULL on error
 */
ast_type_def_t* ast_type_def_create(const char* name, type_category_t category) {
  if (name == NULL) {
    error_report(ERROR_INVALID_ARGUMENT, "Type name cannot be NULL");
    return NULL;
  }
  
  ast_type_def_t* type_def = (ast_type_def_t*)malloc(sizeof(ast_type_def_t));
  if (type_def == NULL) {
    error_report(ERROR_MEMORY, "Failed to allocate memory for type definition");
    return NULL;
  }
  
  type_def->name = strdup_safe(name);
  if (type_def->name == NULL) {
    free(type_def);
    return NULL;
  }
  
  type_def->type.category = category;
  type_def->type.qualifier_flags = 0;
  
  type_def->fields = NULL;
  type_def->field_count = 0;
  type_def->size = 0;
  type_def->alignment = 0;
  
  return type_def;
}

/**
 * @brief Create a new basic type
 * 
 * @param category Type category
 * @return New type or NULL on error
 */
ast_type_t* ast_type_create(type_category_t category) {
  ast_type_t* type = (ast_type_t*)malloc(sizeof(ast_type_t));
  if (type == NULL) {
    error_report(ERROR_MEMORY, "Failed to allocate memory for type");
    return NULL;
  }
  
  type->category = category;
  type->qualifier_flags = 0;
  
  // Initialize based on category
  switch (category) {
    case TYPE_VOID:
    case TYPE_BOOLEAN:
      // No additional initialization needed
      break;
      
    case TYPE_INTEGER:
      type->integer.bit_width = 32;  // Default to 32-bit
      type->integer.is_unsigned = false;
      break;
      
    case TYPE_FLOAT:
      type->float_type.bit_width = 32;  // Default to 32-bit
      break;
      
    case TYPE_POINTER:
      type->pointer.element_type = NULL;
      type->pointer.space = MEMORY_DEFAULT;
      break;
      
    case TYPE_VECTOR:
      type->vector.element_type = NULL;
      type->vector.element_count = 0;
      break;
      
    case TYPE_ARRAY:
      type->array.element_type = NULL;
      type->array.element_count = 0;
      break;
      
    case TYPE_STRUCTURE:
      type->structure.name = NULL;
      type->structure.def = NULL;
      break;
      
    case TYPE_FUNCTION:
      type->function.return_type = NULL;
      type->function.param_types = NULL;
      type->function.param_count = 0;
      type->function.is_vararg = false;
      break;
  }
  
  return type;
}

/**
 * @brief Create a new integer type
 * 
 * @param bit_width Bit width (8, 16, 32, 64)
 * @param is_unsigned Whether type is unsigned
 * @return New type or NULL on error
 */
ast_type_t* ast_type_create_integer(uint32_t bit_width, bool is_unsigned) {
  // Validate bit width
  if (bit_width != 8 && bit_width != 16 && bit_width != 32 && bit_width != 64) {
    error_report(ERROR_INVALID_ARGUMENT, "Invalid integer bit width: %u", bit_width);
    return NULL;
  }
  
  ast_type_t* type = ast_type_create(TYPE_INTEGER);
  if (type == NULL) {
    return NULL;
  }
  
  type->integer.bit_width = bit_width;
  type->integer.is_unsigned = is_unsigned;
  
  if (is_unsigned) {
    type->qualifier_flags |= QUALIFIER_UNSIGNED;
  }
  
  return type;
}

/**
 * @brief Create a new float type
 * 
 * @param bit_width Bit width (16, 32, 64)
 * @return New type or NULL on error
 */
ast_type_t* ast_type_create_float(uint32_t bit_width) {
  // Validate bit width
  if (bit_width != 16 && bit_width != 32 && bit_width != 64) {
    error_report(ERROR_INVALID_ARGUMENT, "Invalid float bit width: %u", bit_width);
    return NULL;
  }
  
  ast_type_t* type = ast_type_create(TYPE_FLOAT);
  if (type == NULL) {
    return NULL;
  }
  
  type->float_type.bit_width = bit_width;
  
  return type;
}

/**
 * @brief Create a new pointer type
 * 
 * @param element_type Type being pointed to
 * @param space Memory space
 * @return New type or NULL on error
 */
ast_type_t* ast_type_create_pointer(ast_type_t* element_type, memory_space_t space) {
  if (element_type == NULL) {
    error_report(ERROR_INVALID_ARGUMENT, "Element type cannot be NULL");
    return NULL;
  }
  
  ast_type_t* type = ast_type_create(TYPE_POINTER);
  if (type == NULL) {
    return NULL;
  }
  
  // Make a copy of the element type
  type->pointer.element_type = (ast_type_t*)malloc(sizeof(ast_type_t));
  if (type->pointer.element_type == NULL) {
    error_report(ERROR_MEMORY, "Failed to allocate memory for pointer element type");
    free(type);
    return NULL;
  }
  
  memcpy(type->pointer.element_type, element_type, sizeof(ast_type_t));
  type->pointer.space = space;
  
  return type;
}

/**
 * @brief Create a new vector type
 * 
 * @param element_type Element type
 * @param element_count Number of elements
 * @return New type or NULL on error
 */
ast_type_t* ast_type_create_vector(ast_type_t* element_type, uint32_t element_count) {
  if (element_type == NULL) {
    error_report(ERROR_INVALID_ARGUMENT, "Element type cannot be NULL");
    return NULL;
  }
  
  if (element_count == 0) {
    error_report(ERROR_INVALID_ARGUMENT, "Vector element count must be greater than 0");
    return NULL;
  }
  
  ast_type_t* type = ast_type_create(TYPE_VECTOR);
  if (type == NULL) {
    return NULL;
  }
  
  // Make a copy of the element type
  type->vector.element_type = (ast_type_t*)malloc(sizeof(ast_type_t));
  if (type->vector.element_type == NULL) {
    error_report(ERROR_MEMORY, "Failed to allocate memory for vector element type");
    free(type);
    return NULL;
  }
  
  memcpy(type->vector.element_type, element_type, sizeof(ast_type_t));
  type->vector.element_count = element_count;
  
  return type;
}

/**
 * @brief Create a new array type
 * 
 * @param element_type Element type
 * @param element_count Number of elements (0 for unknown/runtime)
 * @return New type or NULL on error
 */
ast_type_t* ast_type_create_array(ast_type_t* element_type, uint32_t element_count) {
  if (element_type == NULL) {
    error_report(ERROR_INVALID_ARGUMENT, "Element type cannot be NULL");
    return NULL;
  }
  
  ast_type_t* type = ast_type_create(TYPE_ARRAY);
  if (type == NULL) {
    return NULL;
  }
  
  // Make a copy of the element type
  type->array.element_type = (ast_type_t*)malloc(sizeof(ast_type_t));
  if (type->array.element_type == NULL) {
    error_report(ERROR_MEMORY, "Failed to allocate memory for array element type");
    free(type);
    return NULL;
  }
  
  memcpy(type->array.element_type, element_type, sizeof(ast_type_t));
  type->array.element_count = element_count;
  
  return type;
}

/**
 * @brief Create a new structure type
 * 
 * @param name Structure name
 * @param def Structure definition (NULL if forward declaration)
 * @return New type or NULL on error
 */
ast_type_t* ast_type_create_structure(const char* name, ast_type_def_t* def) {
  if (name == NULL) {
    error_report(ERROR_INVALID_ARGUMENT, "Structure name cannot be NULL");
    return NULL;
  }
  
  ast_type_t* type = ast_type_create(TYPE_STRUCTURE);
  if (type == NULL) {
    return NULL;
  }
  
  type->structure.name = strdup_safe(name);
  if (type->structure.name == NULL) {
    free(type);
    return NULL;
  }
  
  type->structure.def = def;
  
  return type;
}

/**
 * @brief Create a new function type
 * 
 * @param return_type Return type
 * @param param_types Parameter types
 * @param param_count Number of parameters
 * @param is_vararg Whether function takes variable arguments
 * @return New type or NULL on error
 */
ast_type_t* ast_type_create_function(ast_type_t* return_type, ast_type_t** param_types, 
                                    uint32_t param_count, bool is_vararg) {
  if (return_type == NULL) {
    error_report(ERROR_INVALID_ARGUMENT, "Return type cannot be NULL");
    return NULL;
  }
  
  ast_type_t* type = ast_type_create(TYPE_FUNCTION);
  if (type == NULL) {
    return NULL;
  }
  
  // Make a copy of the return type
  type->function.return_type = (ast_type_t*)malloc(sizeof(ast_type_t));
  if (type->function.return_type == NULL) {
    error_report(ERROR_MEMORY, "Failed to allocate memory for function return type");
    free(type);
    return NULL;
  }
  
  memcpy(type->function.return_type, return_type, sizeof(ast_type_t));
  
  // Make copies of parameter types if any
  if (param_count > 0 && param_types != NULL) {
    type->function.param_types = (ast_type_t**)malloc(param_count * sizeof(ast_type_t*));
    if (type->function.param_types == NULL) {
      error_report(ERROR_MEMORY, "Failed to allocate memory for function parameter types");
      free(type->function.return_type);
      free(type);
      return NULL;
    }
    
    for (uint32_t i = 0; i < param_count; i++) {
      if (param_types[i] == NULL) {
        error_report(ERROR_INVALID_ARGUMENT, "Parameter type %u cannot be NULL", i);
        for (uint32_t j = 0; j < i; j++) {
          free(type->function.param_types[j]);
        }
        free(type->function.param_types);
        free(type->function.return_type);
        free(type);
        return NULL;
      }
      
      type->function.param_types[i] = (ast_type_t*)malloc(sizeof(ast_type_t));
      if (type->function.param_types[i] == NULL) {
        error_report(ERROR_MEMORY, "Failed to allocate memory for function parameter type %u", i);
        for (uint32_t j = 0; j < i; j++) {
          free(type->function.param_types[j]);
        }
        free(type->function.param_types);
        free(type->function.return_type);
        free(type);
        return NULL;
      }
      
      memcpy(type->function.param_types[i], param_types[i], sizeof(ast_type_t));
    }
  } else {
    type->function.param_types = NULL;
  }
  
  type->function.param_count = param_count;
  type->function.is_vararg = is_vararg;
  
  return type;
}

/**
 * @brief Destroy a type
 * 
 * @param type Type to destroy
 */
void ast_type_destroy(ast_type_t* type) {
  if (type == NULL) {
    return;
  }
  
  // Free resources based on type category
  switch (type->category) {
    case TYPE_POINTER:
      if (type->pointer.element_type != NULL) {
        ast_type_destroy(type->pointer.element_type);
      }
      break;
      
    case TYPE_VECTOR:
      if (type->vector.element_type != NULL) {
        ast_type_destroy(type->vector.element_type);
      }
      break;
      
    case TYPE_ARRAY:
      if (type->array.element_type != NULL) {
        ast_type_destroy(type->array.element_type);
      }
      break;
      
    case TYPE_STRUCTURE:
      if (type->structure.name != NULL) {
        free((void*)type->structure.name);
      }
      break;
      
    case TYPE_FUNCTION:
      if (type->function.return_type != NULL) {
        ast_type_destroy(type->function.return_type);
      }
      
      if (type->function.param_types != NULL) {
        for (uint32_t i = 0; i < type->function.param_count; i++) {
          if (type->function.param_types[i] != NULL) {
            ast_type_destroy(type->function.param_types[i]);
          }
        }
        free(type->function.param_types);
      }
      break;
      
    default:
      // No resources to free for other types
      break;
  }
  
  free(type);
}

/**
 * @brief Create a new constant
 * 
 * @param name Constant name
 * @param type Constant type
 * @return New constant or NULL on error
 */
ast_constant_t* ast_constant_create(const char* name, ast_type_t* type) {
  if (name == NULL) {
    error_report(ERROR_INVALID_ARGUMENT, "Constant name cannot be NULL");
    return NULL;
  }
  
  if (type == NULL) {
    error_report(ERROR_INVALID_ARGUMENT, "Constant type cannot be NULL");
    return NULL;
  }
  
  ast_constant_t* constant = (ast_constant_t*)malloc(sizeof(ast_constant_t));
  if (constant == NULL) {
    error_report(ERROR_MEMORY, "Failed to allocate memory for constant");
    return NULL;
  }
  
  constant->name = strdup_safe(name);
  if (constant->name == NULL) {
    free(constant);
    return NULL;
  }
  
  // Make a copy of the type
  constant->type = (ast_type_t*)malloc(sizeof(ast_type_t));
  if (constant->type == NULL) {
    error_report(ERROR_MEMORY, "Failed to allocate memory for constant type");
    free((void*)constant->name);
    free(constant);
    return NULL;
  }
  
  memcpy(constant->type, type, sizeof(ast_type_t));
  
  // Default value type based on the type category
  switch (type->category) {
    case TYPE_INTEGER:
      constant->value_type = CONSTANT_INTEGER;
      constant->int_value = 0;
      break;
      
    case TYPE_FLOAT:
      constant->value_type = CONSTANT_FLOAT;
      constant->float_value = 0.0;
      break;
      
    case TYPE_BOOLEAN:
      constant->value_type = CONSTANT_BOOLEAN;
      constant->bool_value = false;
      break;
      
    case TYPE_ARRAY:
      constant->value_type = CONSTANT_ARRAY;
      constant->array.elements = NULL;
      constant->array.element_count = 0;
      break;
      
    case TYPE_STRUCTURE:
      constant->value_type = CONSTANT_STRUCT;
      constant->structure.fields = NULL;
      constant->structure.field_count = 0;
      break;
      
    default:
      // Default to integer for other types
      constant->value_type = CONSTANT_INTEGER;
      constant->int_value = 0;
      break;
  }
  
  return constant;
}

/**
 * @brief Destroy a constant
 * 
 * @param constant Constant to destroy
 */
void ast_constant_destroy(ast_constant_t* constant) {
  if (constant == NULL) {
    return;
  }
  
  // Free name
  free((void*)constant->name);
  
  // Free type
  if (constant->type != NULL) {
    ast_type_destroy(constant->type);
  }
  
  // Free resources based on value type
  switch (constant->value_type) {
    case CONSTANT_STRING:
      if (constant->string_value != NULL) {
        free((void*)constant->string_value);
      }
      break;
      
    case CONSTANT_ARRAY:
      if (constant->array.elements != NULL) {
        for (uint32_t i = 0; i < constant->array.element_count; i++) {
          if (constant->array.elements[i] != NULL) {
            ast_constant_destroy(constant->array.elements[i]);
          }
        }
        free(constant->array.elements);
      }
      break;
      
    case CONSTANT_STRUCT:
      if (constant->structure.fields != NULL) {
        for (uint32_t i = 0; i < constant->structure.field_count; i++) {
          if (constant->structure.fields[i] != NULL) {
            ast_constant_destroy(constant->structure.fields[i]);
          }
        }
        free(constant->structure.fields);
      }
      break;
      
    default:
      // No resources to free for other value types
      break;
  }
  
  free(constant);
}

/**
 * @brief Create a new global variable
 * 
 * @param name Global name
 * @param type Global type
 * @return New global or NULL on error
 */
ast_global_t* ast_global_create(const char* name, ast_type_t* type) {
  if (name == NULL) {
    error_report(ERROR_INVALID_ARGUMENT, "Global name cannot be NULL");
    return NULL;
  }
  
  if (type == NULL) {
    error_report(ERROR_INVALID_ARGUMENT, "Global type cannot be NULL");
    return NULL;
  }
  
  ast_global_t* global = (ast_global_t*)malloc(sizeof(ast_global_t));
  if (global == NULL) {
    error_report(ERROR_MEMORY, "Failed to allocate memory for global");
    return NULL;
  }
  
  global->name = strdup_safe(name);
  if (global->name == NULL) {
    free(global);
    return NULL;
  }
  
  // Make a copy of the type
  global->type = (ast_type_t*)malloc(sizeof(ast_type_t));
  if (global->type == NULL) {
    error_report(ERROR_MEMORY, "Failed to allocate memory for global type");
    free((void*)global->name);
    free(global);
    return NULL;
  }
  
  memcpy(global->type, type, sizeof(ast_type_t));
  
  global->has_initializer = false;
  global->initializer = NULL;
  
  return global;
}

/**
 * @brief Destroy a global variable
 * 
 * @param global Global to destroy
 */
void ast_global_destroy(ast_global_t* global) {
  if (global == NULL) {
    return;
  }
  
  // Free name
  free((void*)global->name);
  
  // Free type
  if (global->type != NULL) {
    ast_type_destroy(global->type);
  }
  
  // Free initializer if present
  if (global->has_initializer && global->initializer != NULL) {
    ast_constant_destroy(global->initializer);
  }
  
  free(global);
}

/**
 * @brief Create a new function
 * 
 * @param name Function name
 * @param return_type Return type
 * @param is_extern Whether function is external
 * @return New function or NULL on error
 */
ast_function_t* ast_function_create(const char* name, ast_type_t* return_type, bool is_extern) {
  if (name == NULL) {
    error_report(ERROR_INVALID_ARGUMENT, "Function name cannot be NULL");
    return NULL;
  }
  
  if (return_type == NULL) {
    error_report(ERROR_INVALID_ARGUMENT, "Return type cannot be NULL");
    return NULL;
  }
  
  ast_function_t* function = (ast_function_t*)malloc(sizeof(ast_function_t));
  if (function == NULL) {
    error_report(ERROR_MEMORY, "Failed to allocate memory for function");
    return NULL;
  }
  
  function->name = strdup_safe(name);
  if (function->name == NULL) {
    free(function);
    return NULL;
  }
  
  // Make a copy of the return type
  function->return_type = (ast_type_t*)malloc(sizeof(ast_type_t));
  if (function->return_type == NULL) {
    error_report(ERROR_MEMORY, "Failed to allocate memory for function return type");
    free((void*)function->name);
    free(function);
    return NULL;
  }
  
  memcpy(function->return_type, return_type, sizeof(ast_type_t));
  
  function->parameters = NULL;
  function->parameter_count = 0;
  function->is_vararg = false;
  function->is_extern = is_extern;
  function->blocks = NULL;
  function->block_count = 0;
  function->target = NULL;
  
  return function;
}

/**
 * @brief Destroy a function
 * 
 * @param function Function to destroy
 */
void ast_function_destroy(ast_function_t* function) {
  if (function == NULL) {
    return;
  }
  
  // Free name
  free((void*)function->name);
  
  // Free return type
  if (function->return_type != NULL) {
    ast_type_destroy(function->return_type);
  }
  
  // Free parameters
  if (function->parameters != NULL) {
    for (uint32_t i = 0; i < function->parameter_count; i++) {
      free((void*)function->parameters[i].name);
      if (function->parameters[i].type != NULL) {
        ast_type_destroy(function->parameters[i].type);
      }
    }
    free(function->parameters);
  }
  
  // Free basic blocks
  if (function->blocks != NULL) {
    for (uint32_t i = 0; i < function->block_count; i++) {
      if (function->blocks[i] != NULL) {
        ast_basic_block_destroy(function->blocks[i]);
      }
    }
    free(function->blocks);
  }
  
  // Free target
  if (function->target != NULL) {
    ast_target_destroy(function->target);
  }
  
  free(function);
}

/**
 * @brief Create a new basic block
 * 
 * @param name Block name
 * @param is_entry Whether this is an entry block
 * @return New basic block or NULL on error
 */
ast_basic_block_t* ast_basic_block_create(const char* name, bool is_entry) {
  if (name == NULL) {
    error_report(ERROR_INVALID_ARGUMENT, "Block name cannot be NULL");
    return NULL;
  }
  
  ast_basic_block_t* block = (ast_basic_block_t*)malloc(sizeof(ast_basic_block_t));
  if (block == NULL) {
    error_report(ERROR_MEMORY, "Failed to allocate memory for basic block");
    return NULL;
  }
  
  block->name = strdup_safe(name);
  if (block->name == NULL) {
    free(block);
    return NULL;
  }
  
  block->instructions = NULL;
  block->instruction_count = 0;
  block->is_entry = is_entry;
  
  return block;
}

/**
 * @brief Destroy a basic block
 * 
 * @param block Basic block to destroy
 */
void ast_basic_block_destroy(ast_basic_block_t* block) {
  if (block == NULL) {
    return;
  }
  
  // Free name
  free((void*)block->name);
  
  // Free instructions
  if (block->instructions != NULL) {
    for (uint32_t i = 0; i < block->instruction_count; i++) {
      if (block->instructions[i] != NULL) {
        ast_instruction_destroy(block->instructions[i]);
      }
    }
    free(block->instructions);
  }
  
  free(block);
}

/**
 * @brief Create a new instruction
 * 
 * @param opcode Instruction opcode
 * @return New instruction or NULL on error
 */
ast_instruction_t* ast_instruction_create(opcode_t opcode) {
  ast_instruction_t* instruction = (ast_instruction_t*)malloc(sizeof(ast_instruction_t));
  if (instruction == NULL) {
    error_report(ERROR_MEMORY, "Failed to allocate memory for instruction");
    return NULL;
  }
  
  instruction->opcode = opcode;
  instruction->result = NULL;
  instruction->operands = NULL;
  instruction->operand_count = 0;
  
  // Initialize opcode-specific fields
  if (opcode == OPCODE_CALL) {
    instruction->call_info.is_vararg = false;
    instruction->call_info.return_type = NULL;
  } else if (opcode == OPCODE_BR) {
    instruction->branch_info.is_conditional = false;
    instruction->branch_info.true_label = NULL;
    instruction->branch_info.false_label = NULL;
  } else if (opcode == OPCODE_RET) {
    instruction->return_info.has_value = false;
  }
  
  return instruction;
}

/**
 * @brief Set instruction result
 * 
 * @param instruction Instruction to modify
 * @param result Result operand
 * @return true if successful, false otherwise
 */
bool ast_instruction_set_result(ast_instruction_t* instruction, ast_operand_t* result) {
  if (instruction == NULL) {
    error_report(ERROR_INVALID_ARGUMENT, "Instruction cannot be NULL");
    return false;
  }
  
  // Free previous result if any
  if (instruction->result != NULL) {
    ast_operand_destroy(instruction->result);
  }
  
  instruction->result = result;
  return true;
}

/**
 * @brief Add operand to instruction
 * 
 * @param instruction Instruction to modify
 * @param operand Operand to add
 * @return true if successful, false otherwise
 */
bool ast_instruction_add_operand(ast_instruction_t* instruction, ast_operand_t* operand) {
  if (instruction == NULL) {
    error_report(ERROR_INVALID_ARGUMENT, "Instruction cannot be NULL");
    return false;
  }
  
  if (operand == NULL) {
    error_report(ERROR_INVALID_ARGUMENT, "Operand cannot be NULL");
    return false;
  }
  
  // Expand operands array
  ast_operand_t** new_operands = (ast_operand_t**)realloc(instruction->operands, 
                                                        (instruction->operand_count + 1) * sizeof(ast_operand_t*));
  if (new_operands == NULL) {
    error_report(ERROR_MEMORY, "Failed to allocate memory for instruction operand");
    return false;
  }
  
  instruction->operands = new_operands;
  instruction->operands[instruction->operand_count] = operand;
  instruction->operand_count++;
  
  return true;
}

/**
 * @brief Destroy an instruction
 * 
 * @param instruction Instruction to destroy
 */
void ast_instruction_destroy(ast_instruction_t* instruction) {
  if (instruction == NULL) {
    return;
  }
  
  // Free result operand if any
  if (instruction->result != NULL) {
    ast_operand_destroy(instruction->result);
  }
  
  // Free source operands
  if (instruction->operands != NULL) {
    for (uint32_t i = 0; i < instruction->operand_count; i++) {
      if (instruction->operands[i] != NULL) {
        ast_operand_destroy(instruction->operands[i]);
      }
    }
    free(instruction->operands);
  }
  
  // Free opcode-specific fields
  if (instruction->opcode == OPCODE_CALL && instruction->call_info.return_type != NULL) {
    ast_type_destroy(instruction->call_info.return_type);
  }
  
  free(instruction);
}

/**
 * @brief Create a new operand
 * 
 * @param type Operand type
 * @param value_type Type of the value
 * @return New operand or NULL on error
 */
ast_operand_t* ast_operand_create(operand_type_t type, ast_type_t* value_type) {
  ast_operand_t* operand = (ast_operand_t*)malloc(sizeof(ast_operand_t));
  if (operand == NULL) {
    error_report(ERROR_MEMORY, "Failed to allocate memory for operand");
    return NULL;
  }
  
  operand->type = type;
  
  // Make a copy of the value type if provided
  if (value_type != NULL) {
    operand->value_type = (ast_type_t*)malloc(sizeof(ast_type_t));
    if (operand->value_type == NULL) {
      error_report(ERROR_MEMORY, "Failed to allocate memory for operand value type");
      free(operand);
      return NULL;
    }
    
    memcpy(operand->value_type, value_type, sizeof(ast_type_t));
  } else {
    operand->value_type = NULL;
  }
  
  // Initialize based on operand type
  switch (type) {
    case OPERAND_LOCAL:
    case OPERAND_REGISTER:
      operand->local.name = NULL;
      break;
      
    case OPERAND_GLOBAL:
      operand->global.name = NULL;
      break;
      
    case OPERAND_CONSTANT:
      operand->constant.constant = NULL;
      break;
      
    case OPERAND_FUNCTION:
      operand->function.name = NULL;
      break;
      
    case OPERAND_BLOCK:
      operand->block.name = NULL;
      break;
      
    default:
      // No initialization needed for other types
      break;
  }
  
  return operand;
}

/**
 * @brief Create a local variable operand
 * 
 * @param name Variable name
 * @param type Variable type
 * @return New operand or NULL on error
 */
ast_operand_t* ast_operand_create_local(const char* name, ast_type_t* type) {
  if (name == NULL) {
    error_report(ERROR_INVALID_ARGUMENT, "Local variable name cannot be NULL");
    return NULL;
  }
  
  if (type == NULL) {
    error_report(ERROR_INVALID_ARGUMENT, "Local variable type cannot be NULL");
    return NULL;
  }
  
  ast_operand_t* operand = ast_operand_create(OPERAND_LOCAL, type);
  if (operand == NULL) {
    return NULL;
  }
  
  operand->local.name = strdup_safe(name);
  if (operand->local.name == NULL) {
    ast_operand_destroy(operand);
    return NULL;
  }
  
  return operand;
}

/**
 * @brief Create a global variable operand
 * 
 * @param name Global variable name
 * @param type Global variable type
 * @return New operand or NULL on error
 */
ast_operand_t* ast_operand_create_global(const char* name, ast_type_t* type) {
  if (name == NULL) {
    error_report(ERROR_INVALID_ARGUMENT, "Global variable name cannot be NULL");
    return NULL;
  }
  
  if (type == NULL) {
    error_report(ERROR_INVALID_ARGUMENT, "Global variable type cannot be NULL");
    return NULL;
  }
  
  ast_operand_t* operand = ast_operand_create(OPERAND_GLOBAL, type);
  if (operand == NULL) {
    return NULL;
  }
  
  operand->global.name = strdup_safe(name);
  if (operand->global.name == NULL) {
    ast_operand_destroy(operand);
    return NULL;
  }
  
  return operand;
}

/**
 * @brief Create a constant operand
 * 
 * @param constant Constant value
 * @return New operand or NULL on error
 */
ast_operand_t* ast_operand_create_constant(ast_constant_t* constant) {
  if (constant == NULL) {
    error_report(ERROR_INVALID_ARGUMENT, "Constant cannot be NULL");
    return NULL;
  }
  
  ast_operand_t* operand = ast_operand_create(OPERAND_CONSTANT, constant->type);
  if (operand == NULL) {
    return NULL;
  }
  
  operand->constant.constant = constant;
  
  return operand;
}

/**
 * @brief Create a function reference operand
 * 
 * @param name Function name
 * @param type Function type
 * @return New operand or NULL on error
 */
ast_operand_t* ast_operand_create_function(const char* name, ast_type_t* type) {
  if (name == NULL) {
    error_report(ERROR_INVALID_ARGUMENT, "Function name cannot be NULL");
    return NULL;
  }
  
  if (type == NULL) {
    error_report(ERROR_INVALID_ARGUMENT, "Function type cannot be NULL");
    return NULL;
  }
  
  ast_operand_t* operand = ast_operand_create(OPERAND_FUNCTION, type);
  if (operand == NULL) {
    return NULL;
  }
  
  operand->function.name = strdup_safe(name);
  if (operand->function.name == NULL) {
    ast_operand_destroy(operand);
    return NULL;
  }
  
  return operand;
}

/**
 * @brief Create a basic block reference operand
 * 
 * @param name Block name
 * @return New operand or NULL on error
 */
ast_operand_t* ast_operand_create_block(const char* name) {
  if (name == NULL) {
    error_report(ERROR_INVALID_ARGUMENT, "Block name cannot be NULL");
    return NULL;
  }
  
  ast_operand_t* operand = ast_operand_create(OPERAND_BLOCK, NULL);
  if (operand == NULL) {
    return NULL;
  }
  
  operand->block.name = strdup_safe(name);
  if (operand->block.name == NULL) {
    ast_operand_destroy(operand);
    return NULL;
  }
  
  return operand;
}

/**
 * @brief Destroy an operand
 * 
 * @param operand Operand to destroy
 */
void ast_operand_destroy(ast_operand_t* operand) {
  if (operand == NULL) {
    return;
  }
  
  // Free value type if present
  if (operand->value_type != NULL) {
    ast_type_destroy(operand->value_type);
  }
  
  // Free resources based on operand type
  switch (operand->type) {
    case OPERAND_LOCAL:
    case OPERAND_REGISTER:
      if (operand->local.name != NULL) {
        free((void*)operand->local.name);
      }
      break;
      
    case OPERAND_GLOBAL:
      if (operand->global.name != NULL) {
        free((void*)operand->global.name);
      }
      break;
      
    case OPERAND_FUNCTION:
      if (operand->function.name != NULL) {
        free((void*)operand->function.name);
      }
      break;
      
    case OPERAND_BLOCK:
      if (operand->block.name != NULL) {
        free((void*)operand->block.name);
      }
      break;
      
    case OPERAND_CONSTANT:
      // We don't own the constant, so don't free it
      break;
      
    default:
      // No resources to free for other types
      break;
  }
  
  free(operand);
}

/**
 * @brief Create a new target specification
 * 
 * @param device_class Target device class
 * @return New target or NULL on error
 */
ast_target_t* ast_target_create(const char* device_class) {
  if (device_class == NULL) {
    error_report(ERROR_INVALID_ARGUMENT, "Device class cannot be NULL");
    return NULL;
  }
  
  ast_target_t* target = (ast_target_t*)malloc(sizeof(ast_target_t));
  if (target == NULL) {
    error_report(ERROR_MEMORY, "Failed to allocate memory for target");
    return NULL;
  }
  
  target->device_class = strdup_safe(device_class);
  if (target->device_class == NULL) {
    free(target);
    return NULL;
  }
  
  target->required_features = NULL;
  target->required_count = 0;
  target->preferred_features = NULL;
  target->preferred_count = 0;
  
  return target;
}

/**
 * @brief Add required feature to target
 * 
 * @param target Target to modify
 * @param feature Feature name
 * @return true if successful, false otherwise
 */
bool ast_target_add_required_feature(ast_target_t* target, const char* feature) {
  if (target == NULL) {
    error_report(ERROR_INVALID_ARGUMENT, "Target cannot be NULL");
    return false;
  }
  
  if (feature == NULL) {
    error_report(ERROR_INVALID_ARGUMENT, "Feature name cannot be NULL");
    return false;
  }
  
  // Expand required features array
  const char** new_features = (const char**)realloc(target->required_features, 
                                                  (target->required_count + 1) * sizeof(const char*));
  if (new_features == NULL) {
    error_report(ERROR_MEMORY, "Failed to allocate memory for target required feature");
    return false;
  }
  
  target->required_features = new_features;
  
  // Add the new feature
  target->required_features[target->required_count] = strdup_safe(feature);
  if (target->required_features[target->required_count] == NULL) {
    return false;
  }
  
  target->required_count++;
  
  return true;
}

/**
 * @brief Add preferred feature to target
 * 
 * @param target Target to modify
 * @param feature Feature name
 * @return true if successful, false otherwise
 */
bool ast_target_add_preferred_feature(ast_target_t* target, const char* feature) {
  if (target == NULL) {
    error_report(ERROR_INVALID_ARGUMENT, "Target cannot be NULL");
    return false;
  }
  
  if (feature == NULL) {
    error_report(ERROR_INVALID_ARGUMENT, "Feature name cannot be NULL");
    return false;
  }
  
  // Expand preferred features array
  const char** new_features = (const char**)realloc(target->preferred_features, 
                                                  (target->preferred_count + 1) * sizeof(const char*));
  if (new_features == NULL) {
    error_report(ERROR_MEMORY, "Failed to allocate memory for target preferred feature");
    return false;
  }
  
  target->preferred_features = new_features;
  
  // Add the new feature
  target->preferred_features[target->preferred_count] = strdup_safe(feature);
  if (target->preferred_features[target->preferred_count] == NULL) {
    return false;
  }
  
  target->preferred_count++;
  
  return true;
}

/**
 * @brief Destroy a target
 * 
 * @param target Target to destroy
 */
void ast_target_destroy(ast_target_t* target) {
  if (target == NULL) {
    return;
  }
  
  // Free device class
  free((void*)target->device_class);
  
  // Free required features
  if (target->required_features != NULL) {
    for (uint32_t i = 0; i < target->required_count; i++) {
      free((void*)target->required_features[i]);
    }
    free(target->required_features);
  }
  
  // Free preferred features
  if (target->preferred_features != NULL) {
    for (uint32_t i = 0; i < target->preferred_count; i++) {
      free((void*)target->preferred_features[i]);
    }
    free(target->preferred_features);
  }
  
  free(target);
}