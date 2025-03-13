/**
 * @file codegen.c
 * @brief Implementation of code generation for HOIL to COIL.
 * 
 * This file contains the implementation of the code generation system.
 * 
 * @author HOILC Team
 * @date 2025
 */

#include "../include/codegen.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/**
 * @brief Instruction mapping structure.
 */
typedef struct {
  const char* name;    /**< HOIL instruction name. */
  uint8_t opcode;      /**< COIL opcode. */
} instruction_mapping_t;

/**
 * @brief Code generator context structure.
 */
struct codegen_context {
  error_context_t* error_ctx;      /**< Error context. */
  symbol_table_t* symbol_table;    /**< Global symbol table. */
  coil_builder_t* builder;         /**< COIL binary builder. */
  
  /* State tracking */
  symbol_table_t* current_symtable; /**< Current symbol table. */
  int32_t* local_regs;              /**< Local register mappings. */
  size_t local_reg_count;          /**< Number of local registers. */
  size_t local_reg_capacity;       /**< Capacity of local registers array. */
  uint8_t next_reg;                /**< Next available register number. */
};

/**
 * @brief HOIL to COIL instruction mapping table.
 */
static const instruction_mapping_t instruction_map[] = {
  { "ADD", OPCODE_ADD },
  { "SUB", OPCODE_SUB },
  { "MUL", OPCODE_MUL },
  { "DIV", OPCODE_DIV },
  { "REM", OPCODE_REM },
  { "NEG", OPCODE_NEG },
  { "ABS", OPCODE_ABS },
  { "MIN", OPCODE_MIN },
  { "MAX", OPCODE_MAX },
  { "FMA", OPCODE_FMA },
  
  { "AND", OPCODE_AND },
  { "OR",  OPCODE_OR },
  { "XOR", OPCODE_XOR },
  { "NOT", OPCODE_NOT },
  { "SHL", OPCODE_SHL },
  { "SHR", OPCODE_SHR },
  
  { "CMP_EQ", OPCODE_CMP_EQ },
  { "CMP_NE", OPCODE_CMP_NE },
  { "CMP_LT", OPCODE_CMP_LT },
  { "CMP_LE", OPCODE_CMP_LE },
  { "CMP_GT", OPCODE_CMP_GT },
  { "CMP_GE", OPCODE_CMP_GE },
  
  { "LOAD", OPCODE_LOAD },
  { "STORE", OPCODE_STORE },
  { "LEA", OPCODE_LEA },
  { "FENCE", OPCODE_FENCE },
  
  { "BR", OPCODE_BR },
  { "BR_COND", OPCODE_BR_COND },
  { "SWITCH", OPCODE_SWITCH },
  { "CALL", OPCODE_CALL },
  { "RET", OPCODE_RET },
  
  { NULL, 0 }  /* Sentinel */
};

/**
 * @brief Forward declarations for recursive code generation functions.
 */
static bool codegen_module(codegen_context_t* context, ast_node_t* module);
static bool codegen_type_def(codegen_context_t* context, ast_node_t* type_def);
static bool codegen_constant(codegen_context_t* context, ast_node_t* constant);
static bool codegen_global(codegen_context_t* context, ast_node_t* global);
static bool codegen_function(codegen_context_t* context, ast_node_t* function);
static bool codegen_extern_function(codegen_context_t* context, ast_node_t* extern_function);
static bool codegen_block(codegen_context_t* context, ast_node_t* block, int32_t function_index);
static bool codegen_statement(codegen_context_t* context, ast_node_t* statement, int32_t function_index);
static bool codegen_assignment(codegen_context_t* context, ast_node_t* assignment, int32_t function_index);
static bool codegen_instruction(codegen_context_t* context, ast_node_t* instruction, int32_t function_index, uint8_t destination);
static bool codegen_branch(codegen_context_t* context, ast_node_t* branch, int32_t function_index);
static bool codegen_return(codegen_context_t* context, ast_node_t* ret, int32_t function_index);
static uint8_t codegen_expr(codegen_context_t* context, ast_node_t* expr, int32_t function_index);

codegen_context_t* codegen_create_context(error_context_t* error_ctx,
                                         symbol_table_t* symbol_table) {
  assert(error_ctx != NULL);
  assert(symbol_table != NULL);
  
  codegen_context_t* context = (codegen_context_t*)malloc(sizeof(codegen_context_t));
  if (context == NULL) {
    return NULL;
  }
  
  context->error_ctx = error_ctx;
  context->symbol_table = symbol_table;
  
  context->builder = coil_builder_create();
  if (context->builder == NULL) {
    free(context);
    return NULL;
  }
  
  context->current_symtable = NULL;
  context->local_regs = NULL;
  context->local_reg_count = 0;
  context->local_reg_capacity = 0;
  context->next_reg = 0;
  
  return context;
}

void codegen_destroy_context(codegen_context_t* context) {
  if (context == NULL) {
    return;
  }
  
  coil_builder_destroy(context->builder);
  free(context->local_regs);
  free(context);
}

bool codegen_generate(codegen_context_t* context, ast_node_t* module,
                     uint8_t** output, size_t* size) {
  assert(context != NULL);
  assert(module != NULL);
  assert(output != NULL);
  assert(size != NULL);
  
  /* Generate code for the module */
  if (!codegen_module(context, module)) {
    return false;
  }
  
  /* Build the COIL binary */
  if (!coil_builder_build(context->builder, output, size)) {
    error_report(context->error_ctx, HOILC_ERROR_INTERNAL,
                 "Failed to build COIL binary");
    return false;
  }
  
  return true;
}

coil_builder_t* codegen_get_builder(codegen_context_t* context) {
  assert(context != NULL);
  
  return context->builder;
}

int32_t codegen_map_type(codegen_context_t* context, ast_node_t* type_node) {
  assert(context != NULL);
  assert(type_node != NULL);
  
  switch (type_node->type) {
    case AST_TYPE_VOID:
      return PREDEFINED_VOID;
      
    case AST_TYPE_BOOL:
      return PREDEFINED_BOOL;
      
    case AST_TYPE_INT: {
      bool is_signed = type_node->data.type_int.is_signed;
      
      switch (type_node->data.type_int.bits) {
        case 8:
          return is_signed ? PREDEFINED_INT8 : PREDEFINED_UINT8;
        case 16:
          return is_signed ? PREDEFINED_INT16 : PREDEFINED_UINT16;
        case 32:
          return is_signed ? PREDEFINED_INT32 : PREDEFINED_UINT32;
        case 64:
          return is_signed ? PREDEFINED_INT64 : PREDEFINED_UINT64;
        default:
          error_report_at_node(context->error_ctx, HOILC_ERROR_INTERNAL, type_node,
                               "Unsupported integer bit width: %d", 
                               type_node->data.type_int.bits);
          return -1;
      }
    }
      
    case AST_TYPE_FLOAT: {
      switch (type_node->data.type_float.bits) {
        case 16:
          return PREDEFINED_FLOAT16;
        case 32:
          return PREDEFINED_FLOAT32;
        case 64:
          return PREDEFINED_FLOAT64;
        default:
          error_report_at_node(context->error_ctx, HOILC_ERROR_INTERNAL, type_node,
                               "Unsupported float bit width: %d", 
                               type_node->data.type_float.bits);
          return -1;
      }
    }
      
    case AST_TYPE_PTR: {
      /* Map the element type */
      int32_t element_type = codegen_map_type(context, type_node->data.type_ptr.element_type);
      if (element_type < 0) {
        return -1;
      }
      
      /* Create a pointer type encoding */
      type_encoding_t encoding = coil_create_type_encoding(
        TYPE_POINTER, 64, 0, 0
      );
      
      /* Add the pointer type */
      int32_t ptr_type = coil_builder_add_type(context->builder, encoding, NULL);
      if (ptr_type < 0) {
        error_report_at_node(context->error_ctx, HOILC_ERROR_INTERNAL, type_node,
                             "Failed to add pointer type");
        return -1;
      }
      
      return ptr_type;
    }
      
    case AST_TYPE_VEC: {
      /* Map the element type */
      int32_t element_type = codegen_map_type(context, type_node->data.type_vec.element_type);
      if (element_type < 0) {
        return -1;
      }
      
      /* Create a vector type encoding */
      type_encoding_t encoding = coil_create_type_encoding(
        TYPE_VECTOR, 0, 0, (uint16_t)type_node->data.type_vec.size
      );
      
      /* Add the vector type */
      int32_t vec_type = coil_builder_add_type(context->builder, encoding, NULL);
      if (vec_type < 0) {
        error_report_at_node(context->error_ctx, HOILC_ERROR_INTERNAL, type_node,
                             "Failed to add vector type");
        return -1;
      }
      
      return vec_type;
    }
      
    case AST_TYPE_ARRAY: {
      /* Map the element type */
      int32_t element_type = codegen_map_type(context, type_node->data.type_array.element_type);
      if (element_type < 0) {
        return -1;
      }
      
      /* Create an array type encoding */
      type_encoding_t encoding = coil_create_type_encoding(
        TYPE_ARRAY, 0, 0, (uint16_t)type_node->data.type_array.size
      );
      
      /* Add the array type */
      int32_t array_type = coil_builder_add_type(context->builder, encoding, NULL);
      if (array_type < 0) {
        error_report_at_node(context->error_ctx, HOILC_ERROR_INTERNAL, type_node,
                             "Failed to add array type");
        return -1;
      }
      
      return array_type;
    }
      
    case AST_TYPE_STRUCT: {
      /* Map the field types */
      int32_t* field_types = NULL;
      if (type_node->data.type_struct.fields.count > 0) {
        field_types = (int32_t*)malloc(
          type_node->data.type_struct.fields.count * sizeof(int32_t)
        );
        
        if (field_types == NULL) {
          error_report_at_node(context->error_ctx, HOILC_ERROR_INTERNAL, type_node,
                               "Memory allocation failed");
          return -1;
        }
      }
      
      for (size_t i = 0; i < type_node->data.type_struct.fields.count; i++) {
        ast_node_t* field = type_node->data.type_struct.fields.nodes[i];
        assert(field->type == AST_FIELD);
        
        field_types[i] = codegen_map_type(context, field->data.field.type);
        if (field_types[i] < 0) {
          free(field_types);
          return -1;
        }
      }
      
      /* Add the structure type */
      int32_t struct_type = coil_builder_add_struct_type(
        context->builder,
        field_types,
        (uint32_t)type_node->data.type_struct.fields.count,
        NULL
      );
      
      free(field_types);
      
      if (struct_type < 0) {
        error_report_at_node(context->error_ctx, HOILC_ERROR_INTERNAL, type_node,
                             "Failed to add structure type");
        return -1;
      }
      
      return struct_type;
    }
      
    case AST_TYPE_FUNCTION: {
      /* Map the return type */
      int32_t return_type = codegen_map_type(context, type_node->data.type_function.return_type);
      if (return_type < 0) {
        return -1;
      }
      
      /* Map the parameter types */
      int32_t* param_types = NULL;
      if (type_node->data.type_function.parameter_types.count > 0) {
        param_types = (int32_t*)malloc(
          type_node->data.type_function.parameter_types.count * sizeof(int32_t)
        );
        
        if (param_types == NULL) {
          error_report_at_node(context->error_ctx, HOILC_ERROR_INTERNAL, type_node,
                               "Memory allocation failed");
          return -1;
        }
      }
      
      for (size_t i = 0; i < type_node->data.type_function.parameter_types.count; i++) {
        ast_node_t* param_type = type_node->data.type_function.parameter_types.nodes[i];
        
        param_types[i] = codegen_map_type(context, param_type);
        if (param_types[i] < 0) {
          free(param_types);
          return -1;
        }
      }
      
      /* Create a function type encoding */
      type_encoding_t encoding = coil_create_type_encoding(
        TYPE_FUNCTION, 0, 0, 0
      );
      
      /* Add the function type */
      int32_t func_type = coil_builder_add_type(context->builder, encoding, NULL);
      
      free(param_types);
      
      if (func_type < 0) {
        error_report_at_node(context->error_ctx, HOILC_ERROR_INTERNAL, type_node,
                             "Failed to add function type");
        return -1;
      }
      
      return func_type;
    }
      
    case AST_TYPE_NAME: {
      /* Look up the type in the symbol table */
      symbol_entry_t* entry = symtable_lookup(context->symbol_table, 
                                             type_node->data.type_name.name, true);
      
      if (entry == NULL || symtable_get_kind(entry) != SYMBOL_TYPE) {
        error_report_at_node(context->error_ctx, HOILC_ERROR_TYPE, type_node,
                             "Unknown type: %s", type_node->data.type_name.name);
        return -1;
      }
      
      /* Get the type definition */
      ast_node_t* type_def = symtable_get_node(entry);
      assert(type_def->type == AST_TYPE_DEF);
      
      /* Map the structure type */
      ast_node_t* struct_type = ast_create_node(AST_TYPE_STRUCT);
      if (struct_type == NULL) {
        error_report_at_node(context->error_ctx, HOILC_ERROR_INTERNAL, type_node,
                             "Memory allocation failed");
        return -1;
      }
      
      /* Copy the fields */
      struct_type->data.type_struct.fields = type_def->data.type_def.fields;
      
      /* Map the structure type */
      int32_t mapped_type = codegen_map_type(context, struct_type);
      
      /* Don't destroy struct_type since it shares fields with type_def */
      free(struct_type);
      
      return mapped_type;
    }
      
    default:
      error_report_at_node(context->error_ctx, HOILC_ERROR_INTERNAL, type_node,
                           "Unknown type node: %d", type_node->type);
      return -1;
  }
}

uint8_t codegen_map_instruction(codegen_context_t* context, const char* instruction) {
  assert(context != NULL);
  assert(instruction != NULL);
  
  for (int i = 0; instruction_map[i].name != NULL; i++) {
    if (strcmp(instruction_map[i].name, instruction) == 0) {
      return instruction_map[i].opcode;
    }
  }
  
  error_report(context->error_ctx, HOILC_ERROR_INTERNAL,
               "Unknown instruction: %s", instruction);
  return 0;
}

bool codegen_generate_constant(codegen_context_t* context, ast_node_t* value,
                              void** output, size_t* size) {
  assert(context != NULL);
  assert(value != NULL);
  assert(output != NULL);
  assert(size != NULL);
  
  switch (value->type) {
    case AST_EXPR_INTEGER: {
      int64_t int_value = value->data.expr_integer.value;
      *output = malloc(sizeof(int64_t));
      if (*output == NULL) {
        error_report_at_node(context->error_ctx, HOILC_ERROR_INTERNAL, value,
                             "Memory allocation failed");
        return false;
      }
      
      memcpy(*output, &int_value, sizeof(int64_t));
      *size = sizeof(int64_t);
      return true;
    }
      
    case AST_EXPR_FLOAT: {
      double float_value = value->data.expr_float.value;
      *output = malloc(sizeof(double));
      if (*output == NULL) {
        error_report_at_node(context->error_ctx, HOILC_ERROR_INTERNAL, value,
                             "Memory allocation failed");
        return false;
      }
      
      memcpy(*output, &float_value, sizeof(double));
      *size = sizeof(double);
      return true;
    }
      
    case AST_EXPR_STRING: {
      const char* str_value = value->data.expr_string.value;
      size_t str_len = strlen(str_value) + 1;  /* Include null terminator */
      
      *output = malloc(str_len);
      if (*output == NULL) {
        error_report_at_node(context->error_ctx, HOILC_ERROR_INTERNAL, value,
                             "Memory allocation failed");
        return false;
      }
      
      memcpy(*output, str_value, str_len);
      *size = str_len;
      return true;
    }
      
    default:
      error_report_at_node(context->error_ctx, HOILC_ERROR_INTERNAL, value,
                           "Unsupported constant value type: %d", value->type);
      return false;
  }
}

/**
 * @brief Reset the local register tracking.
 * 
 * @param context The code generator context.
 */
static void reset_local_registers(codegen_context_t* context) {
  assert(context != NULL);
  
  free(context->local_regs);
  context->local_regs = NULL;
  context->local_reg_count = 0;
  context->local_reg_capacity = 0;
  context->next_reg = 0;
}

/**
 * @brief Add a local register mapping.
 * 
 * @param context The code generator context.
 * @param name The local variable name.
 * @return The register number, or 0xFF on error.
 */
static uint8_t add_local_register(codegen_context_t* context, const char* name) {
  assert(context != NULL);
  assert(name != NULL);
  
  /* Check if we need to resize the local registers array */
  if (context->local_reg_count >= context->local_reg_capacity) {
    size_t new_capacity = context->local_reg_capacity == 0 ? 16 : context->local_reg_capacity * 2;
    int32_t* new_regs = (int32_t*)realloc(
      context->local_regs, new_capacity * sizeof(int32_t)
    );
    
    if (new_regs == NULL) {
      error_report(context->error_ctx, HOILC_ERROR_INTERNAL,
                   "Memory allocation failed");
      return 0xFF;
    }
    
    context->local_regs = new_regs;
    context->local_reg_capacity = new_capacity;
  }
  
  /* Add the local register */
  uint8_t reg = context->next_reg++;
  
  if (reg >= 0xFF) {
    error_report(context->error_ctx, HOILC_ERROR_INTERNAL,
                 "Too many local registers");
    return 0xFF;
  }
  
  /* Store the register number in the symbol table entry */
  symbol_entry_t* entry = symtable_lookup(context->current_symtable, name, false);
  if (entry == NULL) {
    error_report(context->error_ctx, HOILC_ERROR_INTERNAL,
                 "Symbol not found in current scope: %s", name);
    return 0xFF;
  }
  
  context->local_regs[context->local_reg_count++] = reg;
  
  return reg;
}

/**
 * @brief Find the register number for a local variable.
 * 
 * @param context The code generator context.
 * @param name The local variable name.
 * @return The register number, or 0xFF if not found.
 */
static uint8_t find_local_register(codegen_context_t* context, const char* name) {
  assert(context != NULL);
  assert(name != NULL);
  
  /* Look up the symbol */
  symbol_entry_t* entry = symtable_lookup(context->current_symtable, name, true);
  if (entry == NULL) {
    error_report(context->error_ctx, HOILC_ERROR_INTERNAL,
                 "Symbol not found: %s", name);
    return 0xFF;
  }
  
  /* Check if it's a local variable */
  symbol_kind_t kind = symtable_get_kind(entry);
  if (kind != SYMBOL_LOCAL && kind != SYMBOL_PARAMETER) {
    error_report(context->error_ctx, HOILC_ERROR_INTERNAL,
                 "Symbol is not a local variable or parameter: %s", name);
    return 0xFF;
  }
  
  /* Find the register number */
  for (size_t i = 0; i < context->local_reg_count; i++) {
    if (context->local_regs[i] != 0xFF) {
      /* Find the entry in the symbol table */
      const char* sym_name = symtable_get_name(entry);
      if (strcmp(sym_name, name) == 0) {
        return (uint8_t)context->local_regs[i];
      }
    }
  }
  
  /* Not found, allocate a new register */
  return add_local_register(context, name);
}

/**
 * @brief Generate code for a module.
 * 
 * @param context The code generator context.
 * @param module The module AST node.
 * @return true on success, false on failure.
 */
static bool codegen_module(codegen_context_t* context, ast_node_t* module) {
  assert(context != NULL);
  assert(module != NULL);
  assert(module->type == AST_MODULE);
  
  /* Set the module name */
  if (!coil_builder_set_module_name(context->builder, module->data.module.name)) {
    error_report_at_node(context->error_ctx, HOILC_ERROR_INTERNAL, module,
                         "Failed to set module name");
    return false;
  }
  
  /* Process declarations */
  for (size_t i = 0; i < module->data.module.declarations.count; i++) {
    ast_node_t* decl = module->data.module.declarations.nodes[i];
    bool success = false;
    
    switch (decl->type) {
      case AST_TYPE_DEF:
        success = codegen_type_def(context, decl);
        break;
        
      case AST_CONSTANT:
        success = codegen_constant(context, decl);
        break;
        
      case AST_GLOBAL:
        success = codegen_global(context, decl);
        break;
        
      case AST_FUNCTION:
        success = codegen_function(context, decl);
        break;
        
      case AST_EXTERN_FUNCTION:
        success = codegen_extern_function(context, decl);
        break;
        
      default:
        error_report_at_node(context->error_ctx, HOILC_ERROR_INTERNAL, decl,
                             "Unknown declaration type: %d", decl->type);
        return false;
    }
    
    if (!success) {
      return false;
    }
  }
  
  return true;
}

/**
 * @brief Generate code for a type definition.
 * 
 * @param context The code generator context.
 * @param type_def The type definition AST node.
 * @return true on success, false on failure.
 */
static bool codegen_type_def(codegen_context_t* context, ast_node_t* type_def) {
  assert(context != NULL);
  assert(type_def != NULL);
  assert(type_def->type == AST_TYPE_DEF);
  
  /* Map the structure type */
  ast_node_t* struct_type = ast_create_node(AST_TYPE_STRUCT);
  if (struct_type == NULL) {
    error_report_at_node(context->error_ctx, HOILC_ERROR_INTERNAL, type_def,
                         "Memory allocation failed");
    return false;
  }
  
  /* Copy the fields */
  struct_type->data.type_struct.fields = type_def->data.type_def.fields;
  
  /* Map the structure type */
  int32_t type_index = codegen_map_type(context, struct_type);
  
  /* Don't destroy struct_type since it shares fields with type_def */
  free(struct_type);
  
  return type_index >= 0;
}

/**
 * @brief Generate code for a constant declaration.
 * 
 * @param context The code generator context.
 * @param constant The constant declaration AST node.
 * @return true on success, false on failure.
 */
static bool codegen_constant(codegen_context_t* context, ast_node_t* constant) {
  assert(context != NULL);
  assert(constant != NULL);
  assert(constant->type == AST_CONSTANT);
  
  /* Map the constant type */
  int32_t type_index = codegen_map_type(context, constant->data.constant.type);
  if (type_index < 0) {
    return false;
  }
  
  /* Generate the constant value */
  void* value_data = NULL;
  size_t value_size = 0;
  
  if (!codegen_generate_constant(context, constant->data.constant.value, 
                                &value_data, &value_size)) {
    return false;
  }
  
  /* Add the constant to the COIL binary */
  int32_t const_index = coil_builder_add_global(
    context->builder,
    constant->data.constant.name,
    type_index,
    value_data,
    value_size
  );
  
  free(value_data);
  
  if (const_index < 0) {
    error_report_at_node(context->error_ctx, HOILC_ERROR_INTERNAL, constant,
                         "Failed to add constant");
    return false;
  }
  
  return true;
}

/**
 * @brief Generate code for a global variable declaration.
 * 
 * @param context The code generator context.
 * @param global The global variable declaration AST node.
 * @return true on success, false on failure.
 */
static bool codegen_global(codegen_context_t* context, ast_node_t* global) {
  assert(context != NULL);
  assert(global != NULL);
  assert(global->type == AST_GLOBAL);
  
  /* Map the global variable type */
  int32_t type_index = codegen_map_type(context, global->data.global.type);
  if (type_index < 0) {
    return false;
  }
  
  /* Generate the initializer, if present */
  void* init_data = NULL;
  size_t init_size = 0;
  
  if (global->data.global.initializer != NULL) {
    if (!codegen_generate_constant(context, global->data.global.initializer, 
                                  &init_data, &init_size)) {
      return false;
    }
  }
  
  /* Add the global variable to the COIL binary */
  int32_t global_index = coil_builder_add_global(
    context->builder,
    global->data.global.name,
    type_index,
    init_data,
    init_size
  );
  
  if (init_data != NULL) {
    free(init_data);
  }
  
  if (global_index < 0) {
    error_report_at_node(context->error_ctx, HOILC_ERROR_INTERNAL, global,
                         "Failed to add global variable");
    return false;
  }
  
  return true;
}

/**
 * @brief Generate code for a function declaration.
 * 
 * @param context The code generator context.
 * @param function The function declaration AST node.
 * @return true on success, false on failure.
 */
static bool codegen_function(codegen_context_t* context, ast_node_t* function) {
  assert(context != NULL);
  assert(function != NULL);
  assert(function->type == AST_FUNCTION);
  
  /* Map the return type */
  int32_t return_type = codegen_map_type(context, function->data.function.return_type);
  if (return_type < 0) {
    return false;
  }
  
  /* Map the parameter types */
  int32_t* param_types = NULL;
  if (function->data.function.parameters.count > 0) {
    param_types = (int32_t*)malloc(
      function->data.function.parameters.count * sizeof(int32_t)
    );
    
    if (param_types == NULL) {
      error_report_at_node(context->error_ctx, HOILC_ERROR_INTERNAL, function,
                           "Memory allocation failed");
      return false;
    }
  }
  
  for (size_t i = 0; i < function->data.function.parameters.count; i++) {
    ast_node_t* param = function->data.function.parameters.nodes[i];
    assert(param->type == AST_PARAMETER);
    
    param_types[i] = codegen_map_type(context, param->data.parameter.type);
    if (param_types[i] < 0) {
      free(param_types);
      return false;
    }
  }
  
  /* Add the function to the COIL binary */
  int32_t function_index = coil_builder_add_function(
    context->builder,
    function->data.function.name,
    return_type,
    param_types,
    (uint32_t)function->data.function.parameters.count,
    false  /* Not external */
  );
  
  free(param_types);
  
  if (function_index < 0) {
    error_report_at_node(context->error_ctx, HOILC_ERROR_INTERNAL, function,
                         "Failed to add function");
    return false;
  }
  
  /* Create a local symbol table for the function */
  symbol_table_t* function_table = symtable_create_child(context->symbol_table);
  if (function_table == NULL) {
    error_report_at_node(context->error_ctx, HOILC_ERROR_INTERNAL, function,
                         "Memory allocation failed");
    return false;
  }
  
  /* Set the current symbol table */
  context->current_symtable = function_table;
  
  /* Reset local registers */
  reset_local_registers(context);
  
  /* Add parameters to the function table */
  for (size_t i = 0; i < function->data.function.parameters.count; i++) {
    ast_node_t* param = function->data.function.parameters.nodes[i];
    assert(param->type == AST_PARAMETER);
    
    /* Add the parameter to the function table */
    symbol_entry_t* entry = symtable_add(function_table, param->data.parameter.name, 
                                       SYMBOL_PARAMETER, param);
    if (entry == NULL) {
      error_report_at_node(context->error_ctx, HOILC_ERROR_SEMANTIC, param,
                          "Duplicate parameter: %s", param->data.parameter.name);
      symtable_destroy(function_table);
      return false;
    }
    
    /* Allocate a register for the parameter */
    uint8_t reg = add_local_register(context, param->data.parameter.name);
    if (reg == 0xFF) {
      symtable_destroy(function_table);
      return false;
    }
  }
  
  /* Add basic blocks to the function table */
  for (size_t i = 0; i < function->data.function.blocks.count; i++) {
    ast_node_t* block = function->data.function.blocks.nodes[i];
    assert(block->type == AST_STMT_BLOCK);
    
    /* Add the block to the function table */
    symbol_entry_t* entry = symtable_add(function_table, block->data.stmt_block.label, 
                                       SYMBOL_BLOCK, block);
    if (entry == NULL) {
      error_report_at_node(context->error_ctx, HOILC_ERROR_SEMANTIC, block,
                          "Duplicate block label: %s", block->data.stmt_block.label);
      symtable_destroy(function_table);
      return false;
    }
  }
  
  /* Begin generating function code */
  if (!coil_builder_begin_function_code(context->builder, function_index)) {
    error_report_at_node(context->error_ctx, HOILC_ERROR_INTERNAL, function,
                         "Failed to begin function code generation");
    symtable_destroy(function_table);
    return false;
  }
  
  /* Generate code for each basic block */
  bool success = true;
  for (size_t i = 0; i < function->data.function.blocks.count; i++) {
    ast_node_t* block = function->data.function.blocks.nodes[i];
    if (!codegen_block(context, block, function_index)) {
      success = false;
      break;
    }
  }
  
  /* End function code generation */
  if (success && !coil_builder_end_function_code(context->builder)) {
    error_report_at_node(context->error_ctx, HOILC_ERROR_INTERNAL, function,
                         "Failed to end function code generation");
    success = false;
  }
  
  /* Restore the symbol table */
  context->current_symtable = context->symbol_table;
  
  /* Free the function table */
  symtable_destroy(function_table);
  
  return success;
}

/**
 * @brief Generate code for an external function declaration.
 * 
 * @param context The code generator context.
 * @param extern_function The external function declaration AST node.
 * @return true on success, false on failure.
 */
static bool codegen_extern_function(codegen_context_t* context, ast_node_t* extern_function) {
  assert(context != NULL);
  assert(extern_function != NULL);
  assert(extern_function->type == AST_EXTERN_FUNCTION);
  
  /* Map the return type */
  int32_t return_type = codegen_map_type(context, extern_function->data.extern_function.return_type);
  if (return_type < 0) {
    return false;
  }
  
  /* Map the parameter types */
  int32_t* param_types = NULL;
  if (extern_function->data.extern_function.parameters.count > 0) {
    param_types = (int32_t*)malloc(
      extern_function->data.extern_function.parameters.count * sizeof(int32_t)
    );
    
    if (param_types == NULL) {
      error_report_at_node(context->error_ctx, HOILC_ERROR_INTERNAL, extern_function,
                           "Memory allocation failed");
      return false;
    }
  }
  
  for (size_t i = 0; i < extern_function->data.extern_function.parameters.count; i++) {
    ast_node_t* param = extern_function->data.extern_function.parameters.nodes[i];
    assert(param->type == AST_PARAMETER);
    
    param_types[i] = codegen_map_type(context, param->data.parameter.type);
    if (param_types[i] < 0) {
      free(param_types);
      return false;
    }
  }
  
  /* Add the function to the COIL binary */
  int32_t function_index = coil_builder_add_function(
    context->builder,
    extern_function->data.extern_function.name,
    return_type,
    param_types,
    (uint32_t)extern_function->data.extern_function.parameters.count,
    true  /* External */
  );
  
  free(param_types);
  
  if (function_index < 0) {
    error_report_at_node(context->error_ctx, HOILC_ERROR_INTERNAL, extern_function,
                         "Failed to add external function");
    return false;
  }
  
  return true;
}

/**
 * @brief Generate code for a basic block.
 * 
 * @param context The code generator context.
 * @param block The basic block AST node.
 * @param function_index The function index.
 * @return true on success, false on failure.
 */
static bool codegen_block(codegen_context_t* context, ast_node_t* block, 
                         int32_t function_index) {
  assert(context != NULL);
  assert(block != NULL);
  assert(block->type == AST_STMT_BLOCK);
  
  /* Add the block to the COIL binary */
  int32_t block_index = coil_builder_add_block(
    context->builder, 
    block->data.stmt_block.label
  );
  
  if (block_index < 0) {
    error_report_at_node(context->error_ctx, HOILC_ERROR_INTERNAL, block,
                         "Failed to add basic block");
    return false;
  }
  
  /* Generate code for each statement in the block */
  for (size_t i = 0; i < block->data.stmt_block.statements.count; i++) {
    ast_node_t* statement = block->data.stmt_block.statements.nodes[i];
    if (!codegen_statement(context, statement, function_index)) {
      return false;
    }
  }
  
  return true;
}

/**
 * @brief Generate code for a statement.
 * 
 * @param context The code generator context.
 * @param statement The statement AST node.
 * @param function_index The function index.
 * @return true on success, false on failure.
 */
static bool codegen_statement(codegen_context_t* context, ast_node_t* statement, 
                             int32_t function_index) {
  assert(context != NULL);
  assert(statement != NULL);
  
  switch (statement->type) {
    case AST_STMT_ASSIGN:
      return codegen_assignment(context, statement, function_index);
      
    case AST_STMT_INSTRUCTION:
      return codegen_instruction(context, statement, function_index, 0xFF);
      
    case AST_STMT_BRANCH:
      return codegen_branch(context, statement, function_index);
      
    case AST_STMT_RETURN:
      return codegen_return(context, statement, function_index);
      
    default:
      error_report_at_node(context->error_ctx, HOILC_ERROR_INTERNAL, statement,
                           "Unknown statement type: %d", statement->type);
      return false;
  }
}

/**
 * @brief Generate code for an assignment statement.
 * 
 * @param context The code generator context.
 * @param assignment The assignment statement AST node.
 * @param function_index The function index.
 * @return true on success, false on failure.
 */
static bool codegen_assignment(codegen_context_t* context, ast_node_t* assignment, 
                              int32_t function_index) {
  assert(context != NULL);
  assert(assignment != NULL);
  assert(assignment->type == AST_STMT_ASSIGN);
  
  /* Get or create a register for the target */
  uint8_t reg = find_local_register(context, assignment->data.stmt_assign.target);
  if (reg == 0xFF) {
    return false;
  }
  
  /* Generate code for the value (instruction) */
  if (assignment->data.stmt_assign.value->type == AST_STMT_INSTRUCTION) {
    return codegen_instruction(context, assignment->data.stmt_assign.value, 
                              function_index, reg);
  } else {
    error_report_at_node(context->error_ctx, HOILC_ERROR_INTERNAL, assignment,
                         "Assignment value is not an instruction");
    return false;
  }
}

/**
 * @brief Generate code for an instruction statement.
 * 
 * @param context The code generator context.
 * @param instruction The instruction statement AST node.
 * @param function_index The function index.
 * @param destination The destination register.
 * @return true on success, false on failure.
 */
static bool codegen_instruction(codegen_context_t* context, ast_node_t* instruction, 
                               int32_t function_index, uint8_t destination) {
  assert(context != NULL);
  assert(instruction != NULL);
  assert(instruction->type == AST_STMT_INSTRUCTION);
  
  /* Map the instruction opcode */
  uint8_t opcode = codegen_map_instruction(context, instruction->data.stmt_instruction.opcode);
  if (opcode == 0) {
    return false;
  }
  
  /* Generate code for each operand */
  uint8_t* operands = NULL;
  if (instruction->data.stmt_instruction.operands.count > 0) {
    operands = (uint8_t*)malloc(instruction->data.stmt_instruction.operands.count);
    if (operands == NULL) {
      error_report_at_node(context->error_ctx, HOILC_ERROR_INTERNAL, instruction,
                           "Memory allocation failed");
      return false;
    }
  }
  
  for (size_t i = 0; i < instruction->data.stmt_instruction.operands.count; i++) {
    ast_node_t* operand = instruction->data.stmt_instruction.operands.nodes[i];
    operands[i] = codegen_expr(context, operand, function_index);
    if (operands[i] == 0xFF) {
      free(operands);
      return false;
    }
  }
  
  /* Add the instruction to the COIL binary */
  bool success = coil_builder_add_instruction(
    context->builder,
    opcode,
    0,  /* No flags */
    destination,
    operands,
    (uint8_t)instruction->data.stmt_instruction.operands.count
  );
  
  if (operands != NULL) {
    free(operands);
  }
  
  if (!success) {
    error_report_at_node(context->error_ctx, HOILC_ERROR_INTERNAL, instruction,
                         "Failed to add instruction");
    return false;
  }
  
  return true;
}

/**
 * @brief Generate code for a branch statement.
 * 
 * @param context The code generator context.
 * @param branch The branch statement AST node.
 * @param function_index The function index.
 * @return true on success, false on failure.
 */
static bool codegen_branch(codegen_context_t* context, ast_node_t* branch, 
                          int32_t function_index) {
  assert(context != NULL);
  assert(branch != NULL);
  assert(branch->type == AST_STMT_BRANCH);
  
  if (branch->data.stmt_branch.condition != NULL) {
    /* Conditional branch */
    /* Generate code for the condition */
    uint8_t condition = codegen_expr(context, branch->data.stmt_branch.condition, function_index);
    if (condition == 0xFF) {
      return false;
    }
    
    /* BR_COND instruction */
    uint8_t opcode = OPCODE_BR_COND;
    uint8_t operands[3];
    operands[0] = condition;
    
    /* Find the true target block */
    symbol_entry_t* true_entry = symtable_lookup(context->current_symtable, 
                                              branch->data.stmt_branch.true_target, true);
    if (true_entry == NULL || symtable_get_kind(true_entry) != SYMBOL_BLOCK) {
      error_report_at_node(context->error_ctx, HOILC_ERROR_SEMANTIC, branch,
                          "Unknown branch target: %s", branch->data.stmt_branch.true_target);
      return false;
    }
    
    /* Set the target indexes */
    operands[1] = 1;  /* Block index (simplified) */
    
    /* Find the false target block */
    symbol_entry_t* false_entry = symtable_lookup(context->current_symtable, 
                                               branch->data.stmt_branch.false_target, true);
    if (false_entry == NULL || symtable_get_kind(false_entry) != SYMBOL_BLOCK) {
      error_report_at_node(context->error_ctx, HOILC_ERROR_SEMANTIC, branch,
                          "Unknown branch target: %s", branch->data.stmt_branch.false_target);
      return false;
    }
    
    operands[2] = 2;  /* Block index (simplified) */
    
    /* Add the branch instruction */
    if (!coil_builder_add_instruction(
          context->builder,
          opcode,
          0,  /* No flags */
          0xFF,  /* No destination */
          operands,
          3
        )) {
      error_report_at_node(context->error_ctx, HOILC_ERROR_INTERNAL, branch,
                          "Failed to add branch instruction");
      return false;
    }
  } else {
    /* Unconditional branch */
    /* BR instruction */
    uint8_t opcode = OPCODE_BR;
    uint8_t operands[1];
    
    /* Find the target block */
    symbol_entry_t* target_entry = symtable_lookup(context->current_symtable, 
                                               branch->data.stmt_branch.true_target, true);
    if (target_entry == NULL || symtable_get_kind(target_entry) != SYMBOL_BLOCK) {
      error_report_at_node(context->error_ctx, HOILC_ERROR_SEMANTIC, branch,
                          "Unknown branch target: %s", branch->data.stmt_branch.true_target);
      return false;
    }
    
    operands[0] = 1;  /* Block index (simplified) */
    
    /* Add the branch instruction */
    if (!coil_builder_add_instruction(
          context->builder,
          opcode,
          0,  /* No flags */
          0xFF,  /* No destination */
          operands,
          1
        )) {
      error_report_at_node(context->error_ctx, HOILC_ERROR_INTERNAL, branch,
                          "Failed to add branch instruction");
      return false;
    }
  }
  
  return true;
}

/**
 * @brief Generate code for a return statement.
 * 
 * @param context The code generator context.
 * @param ret The return statement AST node.
 * @param function_index The function index.
 * @return true on success, false on failure.
 */
static bool codegen_return(codegen_context_t* context, ast_node_t* ret, 
                          int32_t function_index) {
  assert(context != NULL);
  assert(ret != NULL);
  assert(ret->type == AST_STMT_RETURN);
  
  if (ret->data.stmt_return.value != NULL) {
    /* Return with value */
    /* Generate code for the return value */
    uint8_t value = codegen_expr(context, ret->data.stmt_return.value, function_index);
    if (value == 0xFF) {
      return false;
    }
    
    /* RET instruction */
    uint8_t opcode = OPCODE_RET;
    uint8_t operands[1];
    operands[0] = value;
    
    /* Add the return instruction */
    if (!coil_builder_add_instruction(
          context->builder,
          opcode,
          0,  /* No flags */
          0xFF,  /* No destination */
          operands,
          1
        )) {
      error_report_at_node(context->error_ctx, HOILC_ERROR_INTERNAL, ret,
                          "Failed to add return instruction");
      return false;
    }
  } else {
    /* Return void */
    /* RET instruction */
    uint8_t opcode = OPCODE_RET;
    
    /* Add the return instruction */
    if (!coil_builder_add_instruction(
          context->builder,
          opcode,
          0,  /* No flags */
          0xFF,  /* No destination */
          NULL,
          0
        )) {
      error_report_at_node(context->error_ctx, HOILC_ERROR_INTERNAL, ret,
                          "Failed to add return instruction");
      return false;
    }
  }
  
  return true;
}

/**
 * @brief Generate code for an expression and return the register containing the result.
 * 
 * @param context The code generator context.
 * @param expr The expression AST node.
 * @param function_index The function index.
 * @return The register number, or 0xFF on error.
 */
static uint8_t codegen_expr(codegen_context_t* context, ast_node_t* expr, 
                           int32_t function_index) {
  assert(context != NULL);
  assert(expr != NULL);
  
  switch (expr->type) {
    case AST_EXPR_INTEGER: {
      /* Integer literal */
      /* Allocate a temporary register */
      uint8_t reg = context->next_reg++;
      
      if (reg >= 0xFF) {
        error_report_at_node(context->error_ctx, HOILC_ERROR_INTERNAL, expr,
                             "Too many temporary registers");
        return 0xFF;
      }
      
      /* LOAD instruction */
      /* This is a simplification; in a full implementation, immediate values */
      /* would be encoded differently */
      uint8_t opcode = OPCODE_LOAD;
      
      /* Add the load instruction */
      if (!coil_builder_add_instruction(
            context->builder,
            opcode,
            0,  /* No flags */
            reg,
            NULL,
            0
          )) {
        error_report_at_node(context->error_ctx, HOILC_ERROR_INTERNAL, expr,
                            "Failed to add load instruction");
        return 0xFF;
      }
      
      return reg;
    }
      
    case AST_EXPR_FLOAT: {
      /* Float literal */
      /* Similar to integer literal */
      uint8_t reg = context->next_reg++;
      
      if (reg >= 0xFF) {
        error_report_at_node(context->error_ctx, HOILC_ERROR_INTERNAL, expr,
                             "Too many temporary registers");
        return 0xFF;
      }
      
      /* LOAD instruction */
      uint8_t opcode = OPCODE_LOAD;
      
      /* Add the load instruction */
      if (!coil_builder_add_instruction(
            context->builder,
            opcode,
            0,  /* No flags */
            reg,
            NULL,
            0
          )) {
        error_report_at_node(context->error_ctx, HOILC_ERROR_INTERNAL, expr,
                            "Failed to add load instruction");
        return 0xFF;
      }
      
      return reg;
    }
      
    case AST_EXPR_STRING: {
      /* String literal */
      /* Similar to other literals */
      uint8_t reg = context->next_reg++;
      
      if (reg >= 0xFF) {
        error_report_at_node(context->error_ctx, HOILC_ERROR_INTERNAL, expr,
                             "Too many temporary registers");
        return 0xFF;
      }
      
      /* LOAD instruction */
      uint8_t opcode = OPCODE_LOAD;
      
      /* Add the load instruction */
      if (!coil_builder_add_instruction(
            context->builder,
            opcode,
            0,  /* No flags */
            reg,
            NULL,
            0
          )) {
        error_report_at_node(context->error_ctx, HOILC_ERROR_INTERNAL, expr,
                            "Failed to add load instruction");
        return 0xFF;
      }
      
      return reg;
    }
      
    case AST_EXPR_IDENTIFIER: {
      /* Variable reference */
      /* Look up the variable */
      const char* name = expr->data.expr_identifier.name;
      
      /* Check if it's a local variable */
      uint8_t reg = find_local_register(context, name);
      if (reg != 0xFF) {
        return reg;
      }
      
      /* Not found, could be a global variable */
      /* This is a simplification; in a full implementation, globals would be handled differently */
      error_report_at_node(context->error_ctx, HOILC_ERROR_SEMANTIC, expr,
                           "Unknown identifier: %s", name);
      return 0xFF;
    }
      
    case AST_EXPR_FIELD: {
      /* Field access */
      /* This is a simplification; in a full implementation, field access would be handled differently */
      error_report_at_node(context->error_ctx, HOILC_ERROR_INTERNAL, expr,
                           "Field access not implemented");
      return 0xFF;
    }
      
    case AST_EXPR_CALL: {
      /* Function call */
      /* Generate code for the function expression */
      uint8_t func_reg = codegen_expr(context, expr->data.expr_call.function, function_index);
      if (func_reg == 0xFF) {
        return 0xFF;
      }
      
      /* Generate code for each argument */
      uint8_t* arg_regs = NULL;
      if (expr->data.expr_call.arguments.count > 0) {
        arg_regs = (uint8_t*)malloc(expr->data.expr_call.arguments.count);
        if (arg_regs == NULL) {
          error_report_at_node(context->error_ctx, HOILC_ERROR_INTERNAL, expr,
                               "Memory allocation failed");
          return 0xFF;
        }
      }
      
      for (size_t i = 0; i < expr->data.expr_call.arguments.count; i++) {
        ast_node_t* arg = expr->data.expr_call.arguments.nodes[i];
        arg_regs[i] = codegen_expr(context, arg, function_index);
        if (arg_regs[i] == 0xFF) {
          free(arg_regs);
          return 0xFF;
        }
      }
      
      /* Allocate a register for the result */
      uint8_t result_reg = context->next_reg++;
      
      if (result_reg >= 0xFF) {
        if (arg_regs != NULL) {
          free(arg_regs);
        }
        error_report_at_node(context->error_ctx, HOILC_ERROR_INTERNAL, expr,
                             "Too many temporary registers");
        return 0xFF;
      }
      
      /* CALL instruction */
      uint8_t opcode = OPCODE_CALL;
      uint8_t* operands = (uint8_t*)malloc(1 + expr->data.expr_call.arguments.count);
      if (operands == NULL) {
        if (arg_regs != NULL) {
          free(arg_regs);
        }
        error_report_at_node(context->error_ctx, HOILC_ERROR_INTERNAL, expr,
                             "Memory allocation failed");
        return 0xFF;
      }
      
      operands[0] = func_reg;
      for (size_t i = 0; i < expr->data.expr_call.arguments.count; i++) {
        operands[i + 1] = arg_regs[i];
      }
      
      /* Add the call instruction */
      bool success = coil_builder_add_instruction(
        context->builder,
        opcode,
        0,  /* No flags */
        result_reg,
        operands,
        (uint8_t)(1 + expr->data.expr_call.arguments.count)
      );
      
      free(operands);
      if (arg_regs != NULL) {
        free(arg_regs);
      }
      
      if (!success) {
        error_report_at_node(context->error_ctx, HOILC_ERROR_INTERNAL, expr,
                            "Failed to add call instruction");
        return 0xFF;
      }
      
      return result_reg;
    }
      
    default:
      error_report_at_node(context->error_ctx, HOILC_ERROR_INTERNAL, expr,
                           "Unknown expression type: %d", expr->type);
      return 0xFF;
  }
}