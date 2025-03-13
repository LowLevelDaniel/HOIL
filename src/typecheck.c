/**
 * @file typecheck.c
 * @brief Implementation of type checking for HOIL.
 * 
 * This file contains the implementation of the type checking system.
 * 
 * @author HOILC Team
 * @date 2025
 */

#include "../include/typecheck.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/**
 * @brief Type checker context structure.
 */
struct typecheck_context {
  error_context_t* error_ctx;   /**< Error context. */
  symbol_table_t* global_table; /**< Global symbol table. */
  symbol_table_t* current_table; /**< Current symbol table. */
  ast_node_t* current_function; /**< Current function being checked. */
  ast_node_t* void_type;        /**< Cached void type node. */
  ast_node_t* bool_type;        /**< Cached boolean type node. */
};

/**
 * @brief Create a basic type AST node.
 * 
 * @param type The AST node type.
 * @return The created type node, or NULL on allocation failure.
 */
static ast_node_t* create_basic_type(ast_node_type_t type) {
  ast_node_t* node = ast_create_node(type);
  if (node == NULL) {
    return NULL;
  }
  
  /* Default location */
  node->location.line = 0;
  node->location.column = 0;
  node->location.filename = NULL;
  
  return node;
}

/**
 * @brief Forward declarations for recursive type checking functions.
 */
static bool typecheck_type_def(typecheck_context_t* context, ast_node_t* type_def);
static bool typecheck_constant(typecheck_context_t* context, ast_node_t* constant);
static bool typecheck_global(typecheck_context_t* context, ast_node_t* global);
static bool typecheck_function(typecheck_context_t* context, ast_node_t* function);
static bool typecheck_extern_function(typecheck_context_t* context, ast_node_t* extern_function);
static bool typecheck_block(typecheck_context_t* context, ast_node_t* block, symbol_table_t* local_table);
static bool typecheck_statement(typecheck_context_t* context, ast_node_t* statement, symbol_table_t* local_table);
static bool typecheck_assignment(typecheck_context_t* context, ast_node_t* assignment, symbol_table_t* local_table);
static bool typecheck_instruction(typecheck_context_t* context, ast_node_t* instruction, symbol_table_t* local_table);
static bool typecheck_branch(typecheck_context_t* context, ast_node_t* branch, symbol_table_t* local_table);
static bool typecheck_return(typecheck_context_t* context, ast_node_t* ret, symbol_table_t* local_table);
static ast_node_t* resolve_type(typecheck_context_t* context, ast_node_t* type);
static ast_node_t* typecheck_expr(typecheck_context_t* context, ast_node_t* expr, symbol_table_t* local_table);

typecheck_context_t* typecheck_create_context(error_context_t* error_ctx) {
  assert(error_ctx != NULL);
  
  typecheck_context_t* context = (typecheck_context_t*)malloc(sizeof(typecheck_context_t));
  if (context == NULL) {
    return NULL;
  }
  
  context->error_ctx = error_ctx;
  context->global_table = symtable_create(NULL);
  if (context->global_table == NULL) {
    free(context);
    return NULL;
  }
  
  context->current_table = context->global_table;
  context->current_function = NULL;
  
  /* Create basic types */
  context->void_type = create_basic_type(AST_TYPE_VOID);
  context->bool_type = create_basic_type(AST_TYPE_BOOL);
  
  if (context->void_type == NULL || context->bool_type == NULL) {
    if (context->void_type != NULL) {
      ast_destroy_node(context->void_type);
    }
    if (context->bool_type != NULL) {
      ast_destroy_node(context->bool_type);
    }
    symtable_destroy(context->global_table);
    free(context);
    return NULL;
  }
  
  return context;
}

void typecheck_destroy_context(typecheck_context_t* context) {
  if (context == NULL) {
    return;
  }
  
  /* Free cached types */
  ast_destroy_node(context->void_type);
  ast_destroy_node(context->bool_type);
  
  /* Free symbol tables */
  symtable_destroy(context->global_table);
  
  free(context);
}

bool typecheck_module(typecheck_context_t* context, ast_node_t* module) {
  assert(context != NULL);
  assert(module != NULL);
  assert(module->type == AST_MODULE);
  
  /* Process declarations in order (two passes) */
  /* First pass: register type and function declarations */
  for (size_t i = 0; i < module->data.module.declarations.count; i++) {
    ast_node_t* decl = module->data.module.declarations.nodes[i];
    
    switch (decl->type) {
      case AST_TYPE_DEF: {
        /* Register type */
        symbol_entry_t* entry = symtable_add(context->global_table, 
                                            decl->data.type_def.name, 
                                            SYMBOL_TYPE, decl);
        if (entry == NULL) {
          error_report_at_node(context->error_ctx, HOILC_ERROR_SEMANTIC, decl,
                              "Duplicate type definition: %s", 
                              decl->data.type_def.name);
          return false;
        }
      }
      break;
      
      case AST_FUNCTION: {
        /* Register function */
        symbol_entry_t* entry = symtable_add(context->global_table, 
                                            decl->data.function.name, 
                                            SYMBOL_FUNCTION, decl);
        if (entry == NULL) {
          error_report_at_node(context->error_ctx, HOILC_ERROR_SEMANTIC, decl,
                              "Duplicate function definition: %s", 
                              decl->data.function.name);
          return false;
        }
      }
      break;
      
      case AST_EXTERN_FUNCTION: {
        /* Register external function */
        symbol_entry_t* entry = symtable_add(context->global_table, 
                                            decl->data.extern_function.name, 
                                            SYMBOL_FUNCTION, decl);
        if (entry == NULL) {
          error_report_at_node(context->error_ctx, HOILC_ERROR_SEMANTIC, decl,
                              "Duplicate function declaration: %s", 
                              decl->data.extern_function.name);
          return false;
        }
      }
      break;
      
      default:
        /* Skip other declarations in first pass */
        break;
    }
  }
  
  /* Second pass: check all declarations */
  for (size_t i = 0; i < module->data.module.declarations.count; i++) {
    ast_node_t* decl = module->data.module.declarations.nodes[i];
    bool success = false;
    
    switch (decl->type) {
      case AST_TYPE_DEF:
        success = typecheck_type_def(context, decl);
        break;
        
      case AST_CONSTANT:
        success = typecheck_constant(context, decl);
        break;
        
      case AST_GLOBAL:
        success = typecheck_global(context, decl);
        break;
        
      case AST_FUNCTION:
        success = typecheck_function(context, decl);
        break;
        
      case AST_EXTERN_FUNCTION:
        success = typecheck_extern_function(context, decl);
        break;
        
      default:
        error_report_at_node(context->error_ctx, HOILC_ERROR_INTERNAL, decl,
                            "Unknown declaration type");
        return false;
    }
    
    if (!success) {
      return false;
    }
  }
  
  return true;
}

/**
 * @brief Check if two type nodes are compatible.
 * 
 * @param context The type checker context.
 * @param type1 The first type node.
 * @param type2 The second type node.
 * @return true if the types are compatible, false otherwise.
 */
bool typecheck_are_types_compatible(typecheck_context_t* context, 
                                  ast_node_t* type1, ast_node_t* type2) {
  assert(context != NULL);
  assert(type1 != NULL);
  assert(type2 != NULL);
  assert(ast_is_type_node(type1));
  assert(ast_is_type_node(type2));
  
  /* Resolve named types */
  type1 = resolve_type(context, type1);
  type2 = resolve_type(context, type2);
  
  if (type1 == NULL || type2 == NULL) {
    return false;
  }
  
  /* Check if the types are the same */
  if (type1->type == type2->type) {
    switch (type1->type) {
      case AST_TYPE_VOID:
      case AST_TYPE_BOOL:
        return true;
        
      case AST_TYPE_INT:
        /* Integer types must have the same size and signedness */
        return type1->data.type_int.bits == type2->data.type_int.bits &&
               type1->data.type_int.is_signed == type2->data.type_int.is_signed;
        
      case AST_TYPE_FLOAT:
        /* Float types must have the same size */
        return type1->data.type_float.bits == type2->data.type_float.bits;
        
      case AST_TYPE_PTR:
        /* Pointer types are compatible if their element types are compatible */
        return typecheck_are_types_compatible(context, 
                                             type1->data.type_ptr.element_type, 
                                             type2->data.type_ptr.element_type);
        
      case AST_TYPE_VEC:
        /* Vector types are compatible if their element types are compatible and they have the same size */
        return type1->data.type_vec.size == type2->data.type_vec.size &&
               typecheck_are_types_compatible(context, 
                                             type1->data.type_vec.element_type, 
                                             type2->data.type_vec.element_type);
        
      case AST_TYPE_ARRAY:
        /* Array types are compatible if their element types are compatible and they have the same size */
        return type1->data.type_array.size == type2->data.type_array.size &&
               typecheck_are_types_compatible(context, 
                                             type1->data.type_array.element_type, 
                                             type2->data.type_array.element_type);
        
      case AST_TYPE_STRUCT:
        /* Structure types are compatible if they are the same structure */
        /* This is a simplification; in a full implementation, structural equality should be checked */
        return type1 == type2;
        
      case AST_TYPE_FUNCTION:
        /* Function types are compatible if their return types and parameter types are compatible */
        if (!typecheck_are_types_compatible(context, 
                                          type1->data.type_function.return_type, 
                                          type2->data.type_function.return_type)) {
          return false;
        }
        
        if (type1->data.type_function.parameter_types.count != 
            type2->data.type_function.parameter_types.count) {
          return false;
        }
        
        for (size_t i = 0; i < type1->data.type_function.parameter_types.count; i++) {
          if (!typecheck_are_types_compatible(context, 
                                             type1->data.type_function.parameter_types.nodes[i], 
                                             type2->data.type_function.parameter_types.nodes[i])) {
            return false;
          }
        }
        
        return true;
        
      case AST_TYPE_NAME:
        /* Named types should have been resolved, so this should not happen */
        assert(false);
        return false;
        
      default:
        assert(false);
        return false;
    }
  }
  
  /* Special case: null pointer (represented as integer 0) can be assigned to any pointer type */
  if (type1->type == AST_TYPE_PTR && type2->type == AST_TYPE_INT) {
    /* Only accept integer literals with value 0 as null pointers */
    /* This is checked elsewhere, not in the type system */
    return false;
  }
  
  /* Special case: allow implicit conversion between signed and unsigned integers of the same size */
  if (type1->type == AST_TYPE_INT && type2->type == AST_TYPE_INT) {
    return type1->data.type_int.bits == type2->data.type_int.bits;
  }
  
  /* Special case: allow implicit conversion between integers and floating point numbers */
  if ((type1->type == AST_TYPE_INT && type2->type == AST_TYPE_FLOAT) ||
      (type1->type == AST_TYPE_FLOAT && type2->type == AST_TYPE_INT)) {
    return true;
  }
  
  return false;
}

/**
 * @brief Resolve a type node to its underlying type.
 * 
 * @param context The type checker context.
 * @param type The type node to resolve.
 * @return The resolved type node, or NULL if the type cannot be resolved.
 */
static ast_node_t* resolve_type(typecheck_context_t* context, ast_node_t* type) {
  assert(context != NULL);
  assert(type != NULL);
  assert(ast_is_type_node(type));
  
  /* If the type is a named type, look it up in the symbol table */
  if (type->type == AST_TYPE_NAME) {
    const char* name = type->data.type_name.name;
    symbol_entry_t* entry = symtable_lookup(context->global_table, name, true);
    
    if (entry == NULL || symtable_get_kind(entry) != SYMBOL_TYPE) {
      error_report_at_node(context->error_ctx, HOILC_ERROR_TYPE, type,
                          "Unknown type: %s", name);
      return NULL;
    }
    
    ast_node_t* type_def = symtable_get_node(entry);
    assert(type_def->type == AST_TYPE_DEF);
    
    /* Create a structure type for the type definition */
    ast_node_t* struct_type = ast_create_node(AST_TYPE_STRUCT);
    if (struct_type == NULL) {
      error_report_at_node(context->error_ctx, HOILC_ERROR_INTERNAL, type,
                          "Memory allocation failed");
      return NULL;
    }
    
    /* Copy the fields from the type definition */
    struct_type->data.type_struct.fields = type_def->data.type_def.fields;
    
    /* Note: We're not deep-copying the fields here, which is a simplification.
       In a full implementation, proper copying or reference counting would be needed. */
    
    return struct_type;
  }
  
  return type;
}

/**
 * @brief Type check a type definition.
 * 
 * @param context The type checker context.
 * @param type_def The type definition to check.
 * @return true if the type definition is valid, false otherwise.
 */
static bool typecheck_type_def(typecheck_context_t* context, ast_node_t* type_def) {
  assert(context != NULL);
  assert(type_def != NULL);
  assert(type_def->type == AST_TYPE_DEF);
  
  /* Check each field type */
  for (size_t i = 0; i < type_def->data.type_def.fields.count; i++) {
    ast_node_t* field = type_def->data.type_def.fields.nodes[i];
    assert(field->type == AST_FIELD);
    
    /* Resolve the field type */
    ast_node_t* field_type = resolve_type(context, field->data.field.type);
    if (field_type == NULL) {
      return false;
    }
    
    /* Replace the field type with the resolved type */
    field->data.field.type = field_type;
  }
  
  /* Mark the type as defined */
  symbol_entry_t* entry = symtable_lookup(context->global_table, 
                                        type_def->data.type_def.name, false);
  assert(entry != NULL);
  symtable_mark_defined(entry);
  
  return true;
}

/**
 * @brief Type check a constant declaration.
 * 
 * @param context The type checker context.
 * @param constant The constant declaration to check.
 * @return true if the constant declaration is valid, false otherwise.
 */
static bool typecheck_constant(typecheck_context_t* context, ast_node_t* constant) {
  assert(context != NULL);
  assert(constant != NULL);
  assert(constant->type == AST_CONSTANT);
  
  /* Resolve the constant type */
  ast_node_t* const_type = resolve_type(context, constant->data.constant.type);
  if (const_type == NULL) {
    return false;
  }
  
  /* Replace the constant type with the resolved type */
  constant->data.constant.type = const_type;
  
  /* Type check the constant value */
  ast_node_t* value_type = typecheck_expr(context, constant->data.constant.value, 
                                         context->global_table);
  if (value_type == NULL) {
    return false;
  }
  
  /* Check that the value type is compatible with the constant type */
  if (!typecheck_are_types_compatible(context, const_type, value_type)) {
    error_report_at_node(context->error_ctx, HOILC_ERROR_TYPE, constant,
                        "Constant value type does not match declared type");
    return false;
  }
  
  /* Add the constant to the symbol table */
  symbol_entry_t* entry = symtable_add(context->global_table, constant->data.constant.name, 
                                      SYMBOL_CONSTANT, constant);
  if (entry == NULL) {
    error_report_at_node(context->error_ctx, HOILC_ERROR_SEMANTIC, constant,
                        "Duplicate constant: %s", constant->data.constant.name);
    return false;
  }
  
  /* Set the constant type */
  symtable_set_type(entry, const_type);
  
  /* Mark the constant as defined */
  symtable_mark_defined(entry);
  
  return true;
}

/**
 * @brief Type check a global variable declaration.
 * 
 * @param context The type checker context.
 * @param global The global variable declaration to check.
 * @return true if the global variable declaration is valid, false otherwise.
 */
static bool typecheck_global(typecheck_context_t* context, ast_node_t* global) {
  assert(context != NULL);
  assert(global != NULL);
  assert(global->type == AST_GLOBAL);
  
  /* Resolve the global variable type */
  ast_node_t* global_type = resolve_type(context, global->data.global.type);
  if (global_type == NULL) {
    return false;
  }
  
  /* Replace the global variable type with the resolved type */
  global->data.global.type = global_type;
  
  /* Check initializer, if present */
  if (global->data.global.initializer != NULL) {
    ast_node_t* init_type = typecheck_expr(context, global->data.global.initializer, 
                                          context->global_table);
    if (init_type == NULL) {
      return false;
    }
    
    /* Check that the initializer type is compatible with the global variable type */
    if (!typecheck_are_types_compatible(context, global_type, init_type)) {
      error_report_at_node(context->error_ctx, HOILC_ERROR_TYPE, global,
                          "Global variable initializer type does not match declared type");
      return false;
    }
  }
  
  /* Add the global variable to the symbol table */
  symbol_entry_t* entry = symtable_add(context->global_table, global->data.global.name, 
                                      SYMBOL_GLOBAL, global);
  if (entry == NULL) {
    error_report_at_node(context->error_ctx, HOILC_ERROR_SEMANTIC, global,
                        "Duplicate global variable: %s", global->data.global.name);
    return false;
  }
  
  /* Set the global variable type */
  symtable_set_type(entry, global_type);
  
  /* Mark the global variable as defined */
  symtable_mark_defined(entry);
  
  return true;
}

/**
 * @brief Type check a function declaration.
 * 
 * @param context The type checker context.
 * @param function The function declaration to check.
 * @return true if the function declaration is valid, false otherwise.
 */
static bool typecheck_function(typecheck_context_t* context, ast_node_t* function) {
  assert(context != NULL);
  assert(function != NULL);
  assert(function->type == AST_FUNCTION);
  
  /* Resolve the return type */
  ast_node_t* return_type = resolve_type(context, function->data.function.return_type);
  if (return_type == NULL) {
    return false;
  }
  
  /* Replace the return type with the resolved type */
  function->data.function.return_type = return_type;
  
  /* Create a local symbol table for the function */
  symbol_table_t* function_table = symtable_create_child(context->global_table);
  if (function_table == NULL) {
    error_report_at_node(context->error_ctx, HOILC_ERROR_INTERNAL, function,
                        "Memory allocation failed");
    return false;
  }
  
  /* Add parameters to the function table */
  for (size_t i = 0; i < function->data.function.parameters.count; i++) {
    ast_node_t* param = function->data.function.parameters.nodes[i];
    assert(param->type == AST_PARAMETER);
    
    /* Resolve the parameter type */
    ast_node_t* param_type = resolve_type(context, param->data.parameter.type);
    if (param_type == NULL) {
      symtable_destroy(function_table);
      return false;
    }
    
    /* Replace the parameter type with the resolved type */
    param->data.parameter.type = param_type;
    
    /* Add the parameter to the function table */
    symbol_entry_t* entry = symtable_add(function_table, param->data.parameter.name, 
                                        SYMBOL_PARAMETER, param);
    if (entry == NULL) {
      error_report_at_node(context->error_ctx, HOILC_ERROR_SEMANTIC, param,
                          "Duplicate parameter: %s", param->data.parameter.name);
      symtable_destroy(function_table);
      return false;
    }
    
    /* Set the parameter type */
    symtable_set_type(entry, param_type);
    
    /* Mark the parameter as defined */
    symtable_mark_defined(entry);
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
    
    /* Mark the block as defined */
    symtable_mark_defined(entry);
  }
  
  /* Mark the function as defined */
  symbol_entry_t* func_entry = symtable_lookup(context->global_table, 
                                              function->data.function.name, false);
  assert(func_entry != NULL);
  symtable_mark_defined(func_entry);
  symtable_set_type(func_entry, return_type);
  
  /* Set the current function and table */
  ast_node_t* previous_function = context->current_function;
  symbol_table_t* previous_table = context->current_table;
  context->current_function = function;
  context->current_table = function_table;
  
  /* Type check the function body */
  bool success = true;
  for (size_t i = 0; i < function->data.function.blocks.count; i++) {
    ast_node_t* block = function->data.function.blocks.nodes[i];
    if (!typecheck_block(context, block, function_table)) {
      success = false;
      break;
    }
  }
  
  /* Restore the previous function and table */
  context->current_function = previous_function;
  context->current_table = previous_table;
  
  /* Free the function table */
  symtable_destroy(function_table);
  
  return success;
}

/**
 * @brief Type check an external function declaration.
 * 
 * @param context The type checker context.
 * @param extern_function The external function declaration to check.
 * @return true if the external function declaration is valid, false otherwise.
 */
static bool typecheck_extern_function(typecheck_context_t* context, ast_node_t* extern_function) {
  assert(context != NULL);
  assert(extern_function != NULL);
  assert(extern_function->type == AST_EXTERN_FUNCTION);
  
  /* Resolve the return type */
  ast_node_t* return_type = resolve_type(context, extern_function->data.extern_function.return_type);
  if (return_type == NULL) {
    return false;
  }
  
  /* Replace the return type with the resolved type */
  extern_function->data.extern_function.return_type = return_type;
  
  /* Check parameter types */
  for (size_t i = 0; i < extern_function->data.extern_function.parameters.count; i++) {
    ast_node_t* param = extern_function->data.extern_function.parameters.nodes[i];
    assert(param->type == AST_PARAMETER);
    
    /* Resolve the parameter type */
    ast_node_t* param_type = resolve_type(context, param->data.parameter.type);
    if (param_type == NULL) {
      return false;
    }
    
    /* Replace the parameter type with the resolved type */
    param->data.parameter.type = param_type;
  }
  
  /* Mark the function as defined */
  symbol_entry_t* func_entry = symtable_lookup(context->global_table, 
                                              extern_function->data.extern_function.name, false);
  assert(func_entry != NULL);
  symtable_mark_defined(func_entry);
  symtable_set_type(func_entry, return_type);
  
  return true;
}

/**
 * @brief Type check a basic block.
 * 
 * @param context The type checker context.
 * @param block The basic block to check.
 * @param local_table The local symbol table.
 * @return true if the basic block is valid, false otherwise.
 */
static bool typecheck_block(typecheck_context_t* context, ast_node_t* block, 
                           symbol_table_t* local_table) {
  assert(context != NULL);
  assert(block != NULL);
  assert(block->type == AST_STMT_BLOCK);
  assert(local_table != NULL);
  
  /* Type check each statement in the block */
  for (size_t i = 0; i < block->data.stmt_block.statements.count; i++) {
    ast_node_t* statement = block->data.stmt_block.statements.nodes[i];
    if (!typecheck_statement(context, statement, local_table)) {
      return false;
    }
  }
  
  return true;
}

/**
 * @brief Type check a statement.
 * 
 * @param context The type checker context.
 * @param statement The statement to check.
 * @param local_table The local symbol table.
 * @return true if the statement is valid, false otherwise.
 */
static bool typecheck_statement(typecheck_context_t* context, ast_node_t* statement, 
                               symbol_table_t* local_table) {
  assert(context != NULL);
  assert(statement != NULL);
  assert(local_table != NULL);
  
  switch (statement->type) {
    case AST_STMT_ASSIGN:
      return typecheck_assignment(context, statement, local_table);
      
    case AST_STMT_INSTRUCTION:
      return typecheck_instruction(context, statement, local_table);
      
    case AST_STMT_BRANCH:
      return typecheck_branch(context, statement, local_table);
      
    case AST_STMT_RETURN:
      return typecheck_return(context, statement, local_table);
      
    default:
      error_report_at_node(context->error_ctx, HOILC_ERROR_INTERNAL, statement,
                          "Unknown statement type");
      return false;
  }
}

/**
 * @brief Type check an assignment statement.
 * 
 * @param context The type checker context.
 * @param assignment The assignment statement to check.
 * @param local_table The local symbol table.
 * @return true if the assignment statement is valid, false otherwise.
 */
static bool typecheck_assignment(typecheck_context_t* context, ast_node_t* assignment, 
                                symbol_table_t* local_table) {
  assert(context != NULL);
  assert(assignment != NULL);
  assert(assignment->type == AST_STMT_ASSIGN);
  assert(local_table != NULL);
  
  /* Look up the target variable in the local table */
  const char* target = assignment->data.stmt_assign.target;
  symbol_entry_t* entry = symtable_lookup(local_table, target, true);
  
  if (entry == NULL) {
    /* If not found, create a new local variable */
    ast_node_t* value = assignment->data.stmt_assign.value;
    ast_node_t* value_type = typecheck_expr(context, value, local_table);
    if (value_type == NULL) {
      return false;
    }
    
    /* Add the local variable to the symbol table */
    entry = symtable_add(local_table, target, SYMBOL_LOCAL, assignment);
    if (entry == NULL) {
      error_report_at_node(context->error_ctx, HOILC_ERROR_SEMANTIC, assignment,
                          "Failed to add local variable: %s", target);
      return false;
    }
    
    /* Set the variable type */
    symtable_set_type(entry, value_type);
    
    /* Mark the variable as defined */
    symtable_mark_defined(entry);
  } else {
    /* If found, check that the value type is compatible with the variable type */
    ast_node_t* var_type = symtable_get_type(entry);
    ast_node_t* value = assignment->data.stmt_assign.value;
    ast_node_t* value_type = typecheck_expr(context, value, local_table);
    if (value_type == NULL) {
      return false;
    }
    
    if (!typecheck_are_types_compatible(context, var_type, value_type)) {
      error_report_at_node(context->error_ctx, HOILC_ERROR_TYPE, assignment,
                          "Assignment value type does not match variable type");
      return false;
    }
  }
  
  return true;
}

/**
 * @brief Type check an instruction statement.
 * 
 * @param context The type checker context.
 * @param instruction The instruction statement to check.
 * @param local_table The local symbol table.
 * @return true if the instruction statement is valid, false otherwise.
 */
static bool typecheck_instruction(typecheck_context_t* context, ast_node_t* instruction, 
                                 symbol_table_t* local_table) {
  assert(context != NULL);
  assert(instruction != NULL);
  assert(instruction->type == AST_STMT_INSTRUCTION);
  assert(local_table != NULL);
  
  /* Type check each operand */
  ast_node_t** operand_types = NULL;
  if (instruction->data.stmt_instruction.operands.count > 0) {
    operand_types = (ast_node_t**)malloc(
      instruction->data.stmt_instruction.operands.count * sizeof(ast_node_t*)
    );
    
    if (operand_types == NULL) {
      error_report_at_node(context->error_ctx, HOILC_ERROR_INTERNAL, instruction,
                          "Memory allocation failed");
      return false;
    }
  }
  
  for (size_t i = 0; i < instruction->data.stmt_instruction.operands.count; i++) {
    ast_node_t* operand = instruction->data.stmt_instruction.operands.nodes[i];
    operand_types[i] = typecheck_expr(context, operand, local_table);
    if (operand_types[i] == NULL) {
      free(operand_types);
      return false;
    }
  }
  
  /* Check that the operand types are valid for the instruction */
  /* This is a simplified version; a full implementation would check specific requirements */
  /* for each instruction */
  
  /* Clean up */
  if (operand_types != NULL) {
    free(operand_types);
  }
  
  return true;
}

/**
 * @brief Type check a branch statement.
 * 
 * @param context The type checker context.
 * @param branch The branch statement to check.
 * @param local_table The local symbol table.
 * @return true if the branch statement is valid, false otherwise.
 */
static bool typecheck_branch(typecheck_context_t* context, ast_node_t* branch, 
                            symbol_table_t* local_table) {
  assert(context != NULL);
  assert(branch != NULL);
  assert(branch->type == AST_STMT_BRANCH);
  assert(local_table != NULL);
  
  /* Check condition if present */
  if (branch->data.stmt_branch.condition != NULL) {
    ast_node_t* cond_type = typecheck_expr(context, branch->data.stmt_branch.condition, 
                                          local_table);
    if (cond_type == NULL) {
      return false;
    }
    
    /* Check that the condition type is a boolean */
    if (!typecheck_are_types_compatible(context, context->bool_type, cond_type)) {
      error_report_at_node(context->error_ctx, HOILC_ERROR_TYPE, branch,
                          "Branch condition must be a boolean expression");
      return false;
    }
  }
  
  /* Check that the true target exists */
  const char* true_target = branch->data.stmt_branch.true_target;
  symbol_entry_t* true_entry = symtable_lookup(local_table, true_target, true);
  if (true_entry == NULL || symtable_get_kind(true_entry) != SYMBOL_BLOCK) {
    error_report_at_node(context->error_ctx, HOILC_ERROR_SEMANTIC, branch,
                        "Unknown branch target: %s", true_target);
    return false;
  }
  
  /* Check that the false target exists (if present) */
  if (branch->data.stmt_branch.false_target != NULL) {
    const char* false_target = branch->data.stmt_branch.false_target;
    symbol_entry_t* false_entry = symtable_lookup(local_table, false_target, true);
    if (false_entry == NULL || symtable_get_kind(false_entry) != SYMBOL_BLOCK) {
      error_report_at_node(context->error_ctx, HOILC_ERROR_SEMANTIC, branch,
                          "Unknown branch target: %s", false_target);
      return false;
    }
  }
  
  return true;
}

/**
 * @brief Type check a return statement.
 * 
 * @param context The type checker context.
 * @param ret The return statement to check.
 * @param local_table The local symbol table.
 * @return true if the return statement is valid, false otherwise.
 */
static bool typecheck_return(typecheck_context_t* context, ast_node_t* ret, 
                            symbol_table_t* local_table) {
  assert(context != NULL);
  assert(ret != NULL);
  assert(ret->type == AST_STMT_RETURN);
  assert(local_table != NULL);
  
  /* Get the function return type */
  assert(context->current_function != NULL);
  ast_node_t* func_ret_type = context->current_function->data.function.return_type;
  
  /* Check return value if present */
  if (ret->data.stmt_return.value != NULL) {
    ast_node_t* ret_type = typecheck_expr(context, ret->data.stmt_return.value, local_table);
    if (ret_type == NULL) {
      return false;
    }
    
    /* Check that the return value type is compatible with the function return type */
    if (!typecheck_are_types_compatible(context, func_ret_type, ret_type)) {
      error_report_at_node(context->error_ctx, HOILC_ERROR_TYPE, ret,
                          "Return value type does not match function return type");
      return false;
    }
  } else {
    /* Check that the function return type is void */
    if (!typecheck_are_types_compatible(context, func_ret_type, context->void_type)) {
      error_report_at_node(context->error_ctx, HOILC_ERROR_TYPE, ret,
                          "Empty return in non-void function");
      return false;
    }
  }
  
  return true;
}

/**
 * @brief Type check an expression and determine its type.
 * 
 * @param context The type checker context.
 * @param expr The expression to check.
 * @param local_table The local symbol table.
 * @return The expression type or NULL on error.
 */
static ast_node_t* typecheck_expr(typecheck_context_t* context, ast_node_t* expr, 
                                 symbol_table_t* local_table) {
  assert(context != NULL);
  assert(expr != NULL);
  assert(local_table != NULL);
  
  switch (expr->type) {
    case AST_EXPR_INTEGER: {
      /* Integer literal is always a 32-bit signed integer */
      ast_node_t* int_type = ast_create_node(AST_TYPE_INT);
      if (int_type == NULL) {
        error_report_at_node(context->error_ctx, HOILC_ERROR_INTERNAL, expr,
                            "Memory allocation failed");
        return NULL;
      }
      
      int_type->data.type_int.bits = 32;
      int_type->data.type_int.is_signed = true;
      
      return int_type;
    }
      
    case AST_EXPR_FLOAT: {
      /* Float literal is always a 64-bit floating point number */
      ast_node_t* float_type = ast_create_node(AST_TYPE_FLOAT);
      if (float_type == NULL) {
        error_report_at_node(context->error_ctx, HOILC_ERROR_INTERNAL, expr,
                            "Memory allocation failed");
        return NULL;
      }
      
      float_type->data.type_float.bits = 64;
      
      return float_type;
    }
      
    case AST_EXPR_STRING: {
      /* String literal is a pointer to 8-bit char */
      ast_node_t* char_type = ast_create_node(AST_TYPE_INT);
      if (char_type == NULL) {
        error_report_at_node(context->error_ctx, HOILC_ERROR_INTERNAL, expr,
                            "Memory allocation failed");
        return NULL;
      }
      
      char_type->data.type_int.bits = 8;
      char_type->data.type_int.is_signed = true;
      
      ast_node_t* ptr_type = ast_create_node(AST_TYPE_PTR);
      if (ptr_type == NULL) {
        ast_destroy_node(char_type);
        error_report_at_node(context->error_ctx, HOILC_ERROR_INTERNAL, expr,
                            "Memory allocation failed");
        return NULL;
      }
      
      ptr_type->data.type_ptr.element_type = char_type;
      ptr_type->data.type_ptr.memory_space = NULL;
      
      return ptr_type;
    }
      
    case AST_EXPR_IDENTIFIER: {
      /* Look up the identifier in the symbol table */
      const char* name = expr->data.expr_identifier.name;
      symbol_entry_t* entry = symtable_lookup(local_table, name, true);
      
      if (entry == NULL) {
        error_report_at_node(context->error_ctx, HOILC_ERROR_SEMANTIC, expr,
                            "Unknown identifier: %s", name);
        return NULL;
      }
      
      return symtable_get_type(entry);
    }
      
    case AST_EXPR_FIELD: {
      /* Type check the object expression */
      ast_node_t* obj_type = typecheck_expr(context, expr->data.expr_field.object, local_table);
      if (obj_type == NULL) {
        return NULL;
      }
      
      /* Make sure the object is a structure */
      if (obj_type->type != AST_TYPE_STRUCT) {
        error_report_at_node(context->error_ctx, HOILC_ERROR_TYPE, expr,
                            "Field access on non-structure type");
        return NULL;
      }
      
      /* Find the field in the structure */
      const char* field_name = expr->data.expr_field.field;
      for (size_t i = 0; i < obj_type->data.type_struct.fields.count; i++) {
        ast_node_t* field = obj_type->data.type_struct.fields.nodes[i];
        assert(field->type == AST_FIELD);
        
        if (strcmp(field->data.field.name, field_name) == 0) {
          return field->data.field.type;
        }
      }
      
      error_report_at_node(context->error_ctx, HOILC_ERROR_SEMANTIC, expr,
                          "Unknown field: %s", field_name);
      return NULL;
    }
      
    case AST_EXPR_CALL: {
      /* Type check the function expression */
      ast_node_t* func_type = typecheck_expr(context, expr->data.expr_call.function, local_table);
      if (func_type == NULL) {
        return NULL;
      }
      
      /* Make sure the function is a function type */
      if (func_type->type != AST_TYPE_FUNCTION) {
        error_report_at_node(context->error_ctx, HOILC_ERROR_TYPE, expr,
                            "Call to non-function type");
        return NULL;
      }
      
      /* Type check each argument */
      ast_node_t** arg_types = NULL;
      if (expr->data.expr_call.arguments.count > 0) {
        arg_types = (ast_node_t**)malloc(
          expr->data.expr_call.arguments.count * sizeof(ast_node_t*)
        );
        
        if (arg_types == NULL) {
          error_report_at_node(context->error_ctx, HOILC_ERROR_INTERNAL, expr,
                              "Memory allocation failed");
          return NULL;
        }
      }
      
      for (size_t i = 0; i < expr->data.expr_call.arguments.count; i++) {
        ast_node_t* arg = expr->data.expr_call.arguments.nodes[i];
        arg_types[i] = typecheck_expr(context, arg, local_table);
        if (arg_types[i] == NULL) {
          free(arg_types);
          return NULL;
        }
      }
      
      /* Check that the argument count matches */
      if (expr->data.expr_call.arguments.count != 
          func_type->data.type_function.parameter_types.count) {
        error_report_at_node(context->error_ctx, HOILC_ERROR_TYPE, expr,
                            "Argument count does not match parameter count");
        free(arg_types);
        return NULL;
      }
      
      /* Check that each argument type is compatible with the corresponding parameter type */
      for (size_t i = 0; i < expr->data.expr_call.arguments.count; i++) {
        ast_node_t* param_type = func_type->data.type_function.parameter_types.nodes[i];
        if (!typecheck_are_types_compatible(context, param_type, arg_types[i])) {
          error_report_at_node(context->error_ctx, HOILC_ERROR_TYPE, expr,
                              "Argument type does not match parameter type");
          free(arg_types);
          return NULL;
        }
      }
      
      /* Clean up */
      if (arg_types != NULL) {
        free(arg_types);
      }
      
      /* Return the function return type */
      return func_type->data.type_function.return_type;
    }
      
    default:
      error_report_at_node(context->error_ctx, HOILC_ERROR_INTERNAL, expr,
                          "Unknown expression type");
      return NULL;
  }
}

ast_node_t* typecheck_expression(typecheck_context_t* context, ast_node_t* expr, 
                                symbol_table_t* symtable) {
  return typecheck_expr(context, expr, symtable);
}

ast_node_t* typecheck_operation(typecheck_context_t* context, const char* opcode,
                               ast_node_t** operand_types, size_t operand_count) {
  assert(context != NULL);
  assert(opcode != NULL);
  assert(operand_types != NULL || operand_count == 0);
  
  /* This is a simplified version; a full implementation would check specific requirements */
  /* for each operation type */
  
  /* For now, just return the type of the first operand */
  if (operand_count > 0) {
    return operand_types[0];
  } else {
    /* No operands, return void */
    return context->void_type;
  }
}

symbol_table_t* typecheck_get_symbol_table(typecheck_context_t* context) {
  assert(context != NULL);
  
  return context->global_table;
}