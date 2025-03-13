/**
 * @file ast.c
 * @brief Implementation of the Abstract Syntax Tree for HOIL.
 * 
 * This file contains the implementation of AST node creation and manipulation.
 * 
 * @author HOILC Team
 * @date 2025
 */

#include "../include/ast.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/**
 * @brief Safely duplicate a string.
 * 
 * @param str The string to duplicate.
 * @return A newly allocated copy of the string, or NULL on allocation failure.
 */
static char* safe_strdup(const char* str) {
  if (str == NULL) {
    return NULL;
  }
  
  char* dup = strdup(str);
  if (dup == NULL) {
    /* Handle memory allocation failure */
    return NULL;
  }
  
  return dup;
}

ast_node_t* ast_create_node(ast_node_type_t type) {
  ast_node_t* node = (ast_node_t*)calloc(1, sizeof(ast_node_t));
  if (node == NULL) {
    return NULL;
  }
  
  node->type = type;
  
  /* Initialize location to unknown */
  node->location.line = 0;
  node->location.column = 0;
  node->location.filename = NULL;
  
  return node;
}

void ast_destroy_node(ast_node_t* node) {
  if (node == NULL) {
    return;
  }
  
  /* Free resources based on node type */
  switch (node->type) {
    case AST_MODULE:
      free(node->data.module.name);
      ast_destroy_node_list(&node->data.module.declarations);
      break;
      
    case AST_TARGET:
      free(node->data.target.device_class);
      ast_destroy_node_list(&node->data.target.required_features);
      ast_destroy_node_list(&node->data.target.preferred_features);
      break;
      
    case AST_TYPE_DEF:
      free(node->data.type_def.name);
      ast_destroy_node_list(&node->data.type_def.fields);
      break;
      
    case AST_CONSTANT:
      free(node->data.constant.name);
      ast_destroy_node(node->data.constant.type);
      ast_destroy_node(node->data.constant.value);
      break;
      
    case AST_GLOBAL:
      free(node->data.global.name);
      ast_destroy_node(node->data.global.type);
      if (node->data.global.initializer) {
        ast_destroy_node(node->data.global.initializer);
      }
      break;
      
    case AST_FUNCTION:
      free(node->data.function.name);
      ast_destroy_node_list(&node->data.function.parameters);
      ast_destroy_node(node->data.function.return_type);
      ast_destroy_node_list(&node->data.function.blocks);
      if (node->data.function.target) {
        ast_destroy_node(node->data.function.target);
      }
      break;
      
    case AST_EXTERN_FUNCTION:
      free(node->data.extern_function.name);
      ast_destroy_node_list(&node->data.extern_function.parameters);
      ast_destroy_node(node->data.extern_function.return_type);
      break;
      
    case AST_TYPE_PTR:
      ast_destroy_node(node->data.type_ptr.element_type);
      free(node->data.type_ptr.memory_space);
      break;
      
    case AST_TYPE_VEC:
      ast_destroy_node(node->data.type_vec.element_type);
      break;
      
    case AST_TYPE_ARRAY:
      ast_destroy_node(node->data.type_array.element_type);
      break;
      
    case AST_TYPE_STRUCT:
      ast_destroy_node_list(&node->data.type_struct.fields);
      break;
      
    case AST_TYPE_FUNCTION:
      ast_destroy_node_list(&node->data.type_function.parameter_types);
      ast_destroy_node(node->data.type_function.return_type);
      break;
      
    case AST_TYPE_NAME:
      free(node->data.type_name.name);
      break;
      
    case AST_EXPR_STRING:
      free(node->data.expr_string.value);
      break;
      
    case AST_EXPR_IDENTIFIER:
      free(node->data.expr_identifier.name);
      break;
      
    case AST_EXPR_FIELD:
      ast_destroy_node(node->data.expr_field.object);
      free(node->data.expr_field.field);
      break;
      
    case AST_EXPR_INDEX:
      ast_destroy_node(node->data.expr_index.array);
      ast_destroy_node(node->data.expr_index.index);
      break;
      
    case AST_EXPR_CALL:
      ast_destroy_node(node->data.expr_call.function);
      ast_destroy_node_list(&node->data.expr_call.arguments);
      break;
      
    case AST_STMT_BLOCK:
      free(node->data.stmt_block.label);
      ast_destroy_node_list(&node->data.stmt_block.statements);
      break;
      
    case AST_STMT_ASSIGN:
      free(node->data.stmt_assign.target);
      ast_destroy_node(node->data.stmt_assign.value);
      break;
      
    case AST_STMT_INSTRUCTION:
      free(node->data.stmt_instruction.opcode);
      ast_destroy_node_list(&node->data.stmt_instruction.operands);
      break;
      
    case AST_STMT_BRANCH:
      if (node->data.stmt_branch.condition) {
        ast_destroy_node(node->data.stmt_branch.condition);
      }
      free(node->data.stmt_branch.true_target);
      free(node->data.stmt_branch.false_target);
      break;
      
    case AST_STMT_RETURN:
      if (node->data.stmt_return.value) {
        ast_destroy_node(node->data.stmt_return.value);
      }
      break;
      
    case AST_PARAMETER:
      free(node->data.parameter.name);
      ast_destroy_node(node->data.parameter.type);
      break;
      
    case AST_FIELD:
      free(node->data.field.name);
      ast_destroy_node(node->data.field.type);
      break;
      
    /* Types that don't need additional cleanup */
    case AST_TYPE_VOID:
    case AST_TYPE_BOOL:
    case AST_TYPE_INT:
    case AST_TYPE_FLOAT:
    case AST_EXPR_INTEGER:
    case AST_EXPR_FLOAT:
      /* Nothing to free */
      break;
  }
  
  /* Free the node itself */
  free(node);
}

ast_node_list_t* ast_create_node_list(void) {
  ast_node_list_t* list = (ast_node_list_t*)malloc(sizeof(ast_node_list_t));
  if (list == NULL) {
    return NULL;
  }
  
  list->nodes = NULL;
  list->count = 0;
  list->capacity = 0;
  
  return list;
}

bool ast_add_node(ast_node_list_t* list, ast_node_t* node) {
  assert(list != NULL);
  assert(node != NULL);
  
  /* Check if we need to resize the array */
  if (list->count >= list->capacity) {
    size_t new_capacity = list->capacity == 0 ? 4 : list->capacity * 2;
    ast_node_t** new_nodes = (ast_node_t**)realloc(
      list->nodes, new_capacity * sizeof(ast_node_t*)
    );
    
    if (new_nodes == NULL) {
      /* Memory allocation failed */
      return false;
    }
    
    list->nodes = new_nodes;
    list->capacity = new_capacity;
  }
  
  /* Add the node to the list */
  list->nodes[list->count++] = node;
  
  return true;
}

void ast_destroy_node_list(ast_node_list_t* list) {
  if (list == NULL) {
    return;
  }
  
  /* Free all nodes in the list */
  for (size_t i = 0; i < list->count; i++) {
    ast_destroy_node(list->nodes[i]);
  }
  
  /* Free the array */
  free(list->nodes);
  
  /* Reset the list */
  list->nodes = NULL;
  list->count = 0;
  list->capacity = 0;
}

ast_node_t* ast_create_module(const char* name) {
  ast_node_t* node = ast_create_node(AST_MODULE);
  if (node == NULL) {
    return NULL;
  }
  
  node->data.module.name = safe_strdup(name);
  if (node->data.module.name == NULL) {
    ast_destroy_node(node);
    return NULL;
  }
  
  /* Initialize the declarations list */
  node->data.module.declarations.nodes = NULL;
  node->data.module.declarations.count = 0;
  node->data.module.declarations.capacity = 0;
  
  return node;
}

ast_node_t* ast_create_function(const char* name, ast_node_t* return_type) {
  assert(return_type != NULL);
  
  ast_node_t* node = ast_create_node(AST_FUNCTION);
  if (node == NULL) {
    return NULL;
  }
  
  node->data.function.name = safe_strdup(name);
  if (node->data.function.name == NULL) {
    ast_destroy_node(node);
    return NULL;
  }
  
  node->data.function.return_type = return_type;
  node->data.function.target = NULL;
  
  /* Initialize the parameters and blocks lists */
  node->data.function.parameters.nodes = NULL;
  node->data.function.parameters.count = 0;
  node->data.function.parameters.capacity = 0;
  
  node->data.function.blocks.nodes = NULL;
  node->data.function.blocks.count = 0;
  node->data.function.blocks.capacity = 0;
  
  return node;
}

ast_node_t* ast_create_block(const char* label) {
  ast_node_t* node = ast_create_node(AST_STMT_BLOCK);
  if (node == NULL) {
    return NULL;
  }
  
  node->data.stmt_block.label = safe_strdup(label);
  if (node->data.stmt_block.label == NULL) {
    ast_destroy_node(node);
    return NULL;
  }
  
  /* Initialize the statements list */
  node->data.stmt_block.statements.nodes = NULL;
  node->data.stmt_block.statements.count = 0;
  node->data.stmt_block.statements.capacity = 0;
  
  return node;
}

ast_node_t* ast_create_assignment(const char* target, ast_node_t* value) {
  assert(value != NULL);
  
  ast_node_t* node = ast_create_node(AST_STMT_ASSIGN);
  if (node == NULL) {
    return NULL;
  }
  
  node->data.stmt_assign.target = safe_strdup(target);
  if (node->data.stmt_assign.target == NULL) {
    ast_destroy_node(node);
    return NULL;
  }
  
  node->data.stmt_assign.value = value;
  
  return node;
}

ast_node_t* ast_create_instruction(const char* opcode) {
  ast_node_t* node = ast_create_node(AST_STMT_INSTRUCTION);
  if (node == NULL) {
    return NULL;
  }
  
  node->data.stmt_instruction.opcode = safe_strdup(opcode);
  if (node->data.stmt_instruction.opcode == NULL) {
    ast_destroy_node(node);
    return NULL;
  }
  
  /* Initialize the operands list */
  node->data.stmt_instruction.operands.nodes = NULL;
  node->data.stmt_instruction.operands.count = 0;
  node->data.stmt_instruction.operands.capacity = 0;
  
  return node;
}

ast_node_t* ast_create_identifier(const char* name) {
  ast_node_t* node = ast_create_node(AST_EXPR_IDENTIFIER);
  if (node == NULL) {
    return NULL;
  }
  
  node->data.expr_identifier.name = safe_strdup(name);
  if (node->data.expr_identifier.name == NULL) {
    ast_destroy_node(node);
    return NULL;
  }
  
  return node;
}

ast_node_t* ast_create_integer(int64_t value) {
  ast_node_t* node = ast_create_node(AST_EXPR_INTEGER);
  if (node == NULL) {
    return NULL;
  }
  
  node->data.expr_integer.value = value;
  
  return node;
}

ast_node_t* ast_create_float(double value) {
  ast_node_t* node = ast_create_node(AST_EXPR_FLOAT);
  if (node == NULL) {
    return NULL;
  }
  
  node->data.expr_float.value = value;
  
  return node;
}

ast_node_t* ast_create_string(const char* value) {
  ast_node_t* node = ast_create_node(AST_EXPR_STRING);
  if (node == NULL) {
    return NULL;
  }
  
  node->data.expr_string.value = safe_strdup(value);
  if (node->data.expr_string.value == NULL) {
    ast_destroy_node(node);
    return NULL;
  }
  
  return node;
}

void ast_set_location(ast_node_t* node, int line, int column, const char* filename) {
  assert(node != NULL);
  
  node->location.line = line;
  node->location.column = column;
  node->location.filename = filename;  /* Not duplicated, expected to be a persistent string */
}

bool ast_is_type_node(const ast_node_t* node) {
  if (node == NULL) {
    return false;
  }
  
  switch (node->type) {
    case AST_TYPE_VOID:
    case AST_TYPE_BOOL:
    case AST_TYPE_INT:
    case AST_TYPE_FLOAT:
    case AST_TYPE_PTR:
    case AST_TYPE_VEC:
    case AST_TYPE_ARRAY:
    case AST_TYPE_STRUCT:
    case AST_TYPE_FUNCTION:
    case AST_TYPE_NAME:
      return true;
      
    default:
      return false;
  }
}

bool ast_is_expression_node(const ast_node_t* node) {
  if (node == NULL) {
    return false;
  }
  
  switch (node->type) {
    case AST_EXPR_INTEGER:
    case AST_EXPR_FLOAT:
    case AST_EXPR_STRING:
    case AST_EXPR_IDENTIFIER:
    case AST_EXPR_FIELD:
    case AST_EXPR_INDEX:
    case AST_EXPR_CALL:
      return true;
      
    default:
      return false;
  }
}

bool ast_is_statement_node(const ast_node_t* node) {
  if (node == NULL) {
    return false;
  }
  
  switch (node->type) {
    case AST_STMT_BLOCK:
    case AST_STMT_ASSIGN:
    case AST_STMT_INSTRUCTION:
    case AST_STMT_BRANCH:
    case AST_STMT_RETURN:
      return true;
      
    default:
      return false;
  }
}