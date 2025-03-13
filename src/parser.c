/**
 * @file parser.c
 * @brief Implementation of the parser for HOIL.
 * 
 * This file contains the implementation of the parser functions to convert
 * tokens into an abstract syntax tree.
 * 
 * @author HOILC Team
 * @date 2025
 */

#include "../include/parser.h"
#include "../include/error.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>

/**
 * @brief Parser structure implementation.
 */
struct parser {
  lexer_t* lexer;                /**< Lexer for reading tokens. */
  token_t current;               /**< Current token. */
  bool has_error;                /**< Whether an error has occurred. */
  parser_error_t error;          /**< Last error. */
  const char* filename;          /**< Source filename. */
};

/**
 * @brief Set the parser error message.
 * 
 * @param parser The parser.
 * @param message The error message.
 */
static void parser_set_error(parser_t* parser, const char* message) {
  assert(parser != NULL);
  assert(message != NULL);
  
  parser->has_error = true;
  parser->error.message = strdup(message);
  parser->error.location.line = parser->current.line;
  parser->error.location.column = parser->current.column;
  parser->error.location.filename = parser->filename;
}

/**
 * @brief Advance to the next token.
 * 
 * @param parser The parser.
 * @return true on success, false on error.
 */
static bool parser_advance(parser_t* parser) {
  assert(parser != NULL);
  
  if (!lexer_next_token(parser->lexer, &parser->current)) {
    if (parser->current.type == TOKEN_ERROR) {
      char message[64];
      snprintf(message, sizeof(message), "Unexpected character: '%c'", 
              *parser->current.start);
      parser_set_error(parser, strdup(message));
      return false;
    }
    
    /* End of file is not an error */
    return parser->current.type != TOKEN_ERROR;
  }
  
  return true;
}

/**
 * @brief Check if the current token has the specified type.
 * 
 * @param parser The parser.
 * @param type The token type to check.
 * @return true if the current token has the specified type, false otherwise.
 */
static bool parser_check(parser_t* parser, token_type_t type) {
  assert(parser != NULL);
  
  return parser->current.type == type;
}

/**
 * @brief Match and consume the current token if it has the specified type.
 * 
 * @param parser The parser.
 * @param type The token type to match.
 * @return true if the token was matched and consumed, false otherwise.
 */
static bool parser_match(parser_t* parser, token_type_t type) {
  assert(parser != NULL);
  
  if (parser_check(parser, type)) {
    parser_advance(parser);
    return true;
  }
  
  return false;
}

/**
 * @brief Expect and consume a token of the specified type.
 * 
 * @param parser The parser.
 * @param type The expected token type.
 * @param error_message The error message if the token is not found.
 * @return true if the token was consumed, false otherwise.
 */
static bool parser_expect(parser_t* parser, token_type_t type, 
                          const char* error_message) {
  assert(parser != NULL);
  assert(error_message != NULL);
  
  if (parser_check(parser, type)) {
    parser_advance(parser);
    return true;
  }
  
  parser_set_error(parser, strdup(error_message));
  return false;
}

/**
 * @brief Copy token content to a null-terminated string.
 * 
 * @param token The token.
 * @return A newly allocated string containing the token content, or NULL on failure.
 */
static char* token_to_str(const token_t* token) {
  assert(token != NULL);
  
  char* str = (char*)malloc(token->length + 1);
  if (str == NULL) {
    return NULL;
  }
  
  memcpy(str, token->start, token->length);
  str[token->length] = '\0';
  
  return str;
}

/*
 * Forward declarations for recursive parsing functions
 */
static ast_node_t* parse_module(parser_t* parser);
static ast_node_t* parse_type_def(parser_t* parser);
static ast_node_t* parse_constant(parser_t* parser);
static ast_node_t* parse_global(parser_t* parser);
static ast_node_t* parse_function(parser_t* parser);
static ast_node_t* parse_extern_function(parser_t* parser);
static ast_node_t* parse_type(parser_t* parser);
static ast_node_t* parse_expression(parser_t* parser);
static ast_node_t* parse_block(parser_t* parser);
static ast_node_t* parse_statement(parser_t* parser);
static ast_node_t* parse_instruction(parser_t* parser);
static ast_node_t* parse_branch(parser_t* parser);
static ast_node_t* parse_return(parser_t* parser);
static ast_node_t* parse_assignment(parser_t* parser);

parser_t* parser_create(lexer_t* lexer, const char* filename) {
  assert(lexer != NULL);
  
  parser_t* parser = (parser_t*)malloc(sizeof(parser_t));
  if (parser == NULL) {
    return NULL;
  }
  
  parser->lexer = lexer;
  parser->has_error = false;
  parser->error.message = NULL;
  parser->error.location.line = 0;
  parser->error.location.column = 0;
  parser->error.location.filename = filename;
  parser->filename = filename;
  
  /* Prime the parser with the first token */
  if (!lexer_next_token(lexer, &parser->current)) {
    if (parser->current.type == TOKEN_ERROR) {
      parser_set_error(parser, strdup("Lexical error at start of file"));
    }
  }
  
  return parser;
}

void parser_destroy(parser_t* parser) {
  if (parser == NULL) {
    return;
  }
  
  /* Free the error message if it exists */
  if (parser->error.message != NULL) {
    free(parser->error.message);
  }
  
  free(parser);
}

ast_node_t* parser_parse_module(parser_t* parser) {
  assert(parser != NULL);
  
  return parse_module(parser);
}

parser_error_t parser_get_error(const parser_t* parser) {
  assert(parser != NULL);
  
  return parser->error;
}

bool parser_has_error(const parser_t* parser) {
  assert(parser != NULL);
  
  return parser->has_error;
}

/**
 * @brief Parse a module declaration.
 * 
 * @param parser The parser.
 * @return The parsed module AST node, or NULL on error.
 */
static ast_node_t* parse_module(parser_t* parser) {
  /* Expect MODULE keyword */
  if (!parser_expect(parser, TOKEN_MODULE, "Expected 'MODULE' at start of file")) {
    return NULL;
  }
  
  /* Expect a string literal for the module name */
  if (!parser_expect(parser, TOKEN_STRING, "Expected module name string")) {
    return NULL;
  }
  
  /* Get the module name */
  char* module_name = token_to_str(&parser->current);
  if (module_name == NULL) {
    parser_set_error(parser, strdup("Memory allocation error for module name"));
    return NULL;
  }
  
  /* Create the module node */
  ast_node_t* module = ast_create_module(module_name);
  free(module_name);
  
  if (module == NULL) {
    parser_set_error(parser, strdup("Memory allocation error for module node"));
    return NULL;
  }
  
  /* Set the module location */
  ast_set_location(module, parser->current.line, parser->current.column, 
                   parser->filename);
  
  /* Expect semicolon after module declaration */
  if (!parser_expect(parser, TOKEN_SEMICOLON, "Expected ';' after module name")) {
    ast_destroy_node(module);
    return NULL;
  }
  
  /* Parse declarations until end of file */
  while (!parser_check(parser, TOKEN_EOF)) {
    ast_node_t* declaration = NULL;
    
    /* Check the declaration type */
    switch (parser->current.type) {
      case TOKEN_TYPE:
        declaration = parse_type_def(parser);
        break;
        
      case TOKEN_CONSTANT:
        declaration = parse_constant(parser);
        break;
        
      case TOKEN_GLOBAL:
        declaration = parse_global(parser);
        break;
        
      case TOKEN_EXTERN:
        declaration = parse_extern_function(parser);
        break;
        
      case TOKEN_FUNCTION:
        declaration = parse_function(parser);
        break;
        
      default: {
        /* Unexpected token */
        char error[64];
        snprintf(error, sizeof(error), "Unexpected token in module declaration: %s", 
                token_type_name(parser->current.type));
        parser_set_error(parser, strdup(error));
        ast_destroy_node(module);
        return NULL;
      }
    }
    
    /* Check if declaration parsing failed */
    if (declaration == NULL) {
      ast_destroy_node(module);
      return NULL;
    }
    
    /* Add the declaration to the module */
    if (!ast_add_node(&module->data.module.declarations, declaration)) {
      parser_set_error(parser, strdup("Memory allocation error adding declaration"));
      ast_destroy_node(declaration);
      ast_destroy_node(module);
      return NULL;
    }
  }
  
  return module;
}

/**
 * @brief Parse a type definition.
 * 
 * @param parser The parser.
 * @return The parsed type definition AST node, or NULL on error.
 */
static ast_node_t* parse_type_def(parser_t* parser) {
  /* Expect TYPE keyword */
  if (!parser_expect(parser, TOKEN_TYPE, "Expected 'TYPE' keyword")) {
    return NULL;
  }
  
  /* Expect an identifier for the type name */
  if (!parser_expect(parser, TOKEN_IDENTIFIER, "Expected type name identifier")) {
    return NULL;
  }
  
  /* Get the type name */
  char* type_name = token_to_str(&parser->current);
  if (type_name == NULL) {
    parser_set_error(parser, strdup("Memory allocation error for type name"));
    return NULL;
  }
  
  /* Create the type definition node */
  ast_node_t* type_def = ast_create_node(AST_TYPE_DEF);
  if (type_def == NULL) {
    free(type_name);
    parser_set_error(parser, strdup("Memory allocation error for type definition"));
    return NULL;
  }
  
  /* Set the type name */
  type_def->data.type_def.name = type_name;
  
  /* Set the type definition location */
  ast_set_location(type_def, parser->current.line, parser->current.column, 
                  parser->filename);
  
  /* Initialize the fields list */
  type_def->data.type_def.fields.nodes = NULL;
  type_def->data.type_def.fields.count = 0;
  type_def->data.type_def.fields.capacity = 0;
  
  /* Expect opening brace */
  if (!parser_expect(parser, TOKEN_LBRACE, "Expected '{' after type name")) {
    ast_destroy_node(type_def);
    return NULL;
  }
  
  /* Parse fields until closing brace */
  while (!parser_check(parser, TOKEN_RBRACE)) {
    /* Expect field name identifier */
    if (!parser_expect(parser, TOKEN_IDENTIFIER, "Expected field name identifier")) {
      ast_destroy_node(type_def);
      return NULL;
    }
    
    /* Get the field name */
    char* field_name = token_to_str(&parser->current);
    if (field_name == NULL) {
      parser_set_error(parser, strdup("Memory allocation error for field name"));
      ast_destroy_node(type_def);
      return NULL;
    }
    
    /* Expect colon */
    if (!parser_expect(parser, TOKEN_COLON, "Expected ':' after field name")) {
      free(field_name);
      ast_destroy_node(type_def);
      return NULL;
    }
    
    /* Parse the field type */
    ast_node_t* field_type = parse_type(parser);
    if (field_type == NULL) {
      free(field_name);
      ast_destroy_node(type_def);
      return NULL;
    }
    
    /* Create the field node */
    ast_node_t* field = ast_create_node(AST_FIELD);
    if (field == NULL) {
      free(field_name);
      ast_destroy_node(field_type);
      ast_destroy_node(type_def);
      parser_set_error(parser, strdup("Memory allocation error for field node"));
      return NULL;
    }
    
    /* Set the field properties */
    field->data.field.name = field_name;
    field->data.field.type = field_type;
    
    /* Set the field location */
    ast_set_location(field, parser->current.line, parser->current.column, 
                    parser->filename);
    
    /* Add the field to the type definition */
    if (!ast_add_node(&type_def->data.type_def.fields, field)) {
      ast_destroy_node(field);
      ast_destroy_node(type_def);
      parser_set_error(parser, strdup("Memory allocation error adding field"));
      return NULL;
    }
    
    /* Expect comma or closing brace */
    if (parser_check(parser, TOKEN_COMMA)) {
      parser_advance(parser);
    } else if (parser_check(parser, TOKEN_RBRACE)) {
      break;
    } else {
      parser_set_error(parser, strdup("Expected ',' or '}' after field"));
      ast_destroy_node(type_def);
      return NULL;
    }
  }
  
  /* Expect closing brace */
  if (!parser_expect(parser, TOKEN_RBRACE, "Expected '}' at end of type definition")) {
    ast_destroy_node(type_def);
    return NULL;
  }
  
  return type_def;
}

/**
 * @brief Parse a type.
 * 
 * @param parser The parser.
 * @return The parsed type AST node, or NULL on error.
 */
static ast_node_t* parse_type(parser_t* parser) {
  ast_node_t* type = NULL;
  
  /* Check for basic types */
  switch (parser->current.type) {
    case TOKEN_VOID:
      type = ast_create_node(AST_TYPE_VOID);
      parser_advance(parser);
      break;
      
    case TOKEN_BOOL:
      type = ast_create_node(AST_TYPE_BOOL);
      parser_advance(parser);
      break;
      
    case TOKEN_I8:
    case TOKEN_I16:
    case TOKEN_I32:
    case TOKEN_I64:
    case TOKEN_U8:
    case TOKEN_U16:
    case TOKEN_U32:
    case TOKEN_U64:
      type = ast_create_node(AST_TYPE_INT);
      if (type != NULL) {
        /* Set the integer type properties */
        switch (parser->current.type) {
          case TOKEN_I8:
            type->data.type_int.bits = 8;
            type->data.type_int.is_signed = true;
            break;
          case TOKEN_I16:
            type->data.type_int.bits = 16;
            type->data.type_int.is_signed = true;
            break;
          case TOKEN_I32:
            type->data.type_int.bits = 32;
            type->data.type_int.is_signed = true;
            break;
          case TOKEN_I64:
            type->data.type_int.bits = 64;
            type->data.type_int.is_signed = true;
            break;
          case TOKEN_U8:
            type->data.type_int.bits = 8;
            type->data.type_int.is_signed = false;
            break;
          case TOKEN_U16:
            type->data.type_int.bits = 16;
            type->data.type_int.is_signed = false;
            break;
          case TOKEN_U32:
            type->data.type_int.bits = 32;
            type->data.type_int.is_signed = false;
            break;
          case TOKEN_U64:
            type->data.type_int.bits = 64;
            type->data.type_int.is_signed = false;
            break;
          default:
            break;
        }
      }
      parser_advance(parser);
      break;
      
    case TOKEN_F16:
    case TOKEN_F32:
    case TOKEN_F64:
      type = ast_create_node(AST_TYPE_FLOAT);
      if (type != NULL) {
        /* Set the float type properties */
        switch (parser->current.type) {
          case TOKEN_F16:
            type->data.type_float.bits = 16;
            break;
          case TOKEN_F32:
            type->data.type_float.bits = 32;
            break;
          case TOKEN_F64:
            type->data.type_float.bits = 64;
            break;
          default:
            break;
        }
      }
      parser_advance(parser);
      break;
      
    case TOKEN_PTR:
      /* Pointer type */
      parser_advance(parser);
      
      /* Expect < for type parameter */
      if (!parser_expect(parser, TOKEN_LESS, "Expected '<' after 'ptr'")) {
        return NULL;
      }
      
      /* Parse the element type */
      ast_node_t* element_type = parse_type(parser);
      if (element_type == NULL) {
        return NULL;
      }
      
      /* Create the pointer type node */
      type = ast_create_node(AST_TYPE_PTR);
      if (type == NULL) {
        ast_destroy_node(element_type);
        parser_set_error(parser, strdup("Memory allocation error for pointer type"));
        return NULL;
      }
      
      /* Set the pointer type properties */
      type->data.type_ptr.element_type = element_type;
      type->data.type_ptr.memory_space = NULL;
      
      /* Check for optional memory space */
      if (parser_match(parser, TOKEN_COMMA)) {
        /* Expect memory space identifier */
        if (!parser_expect(parser, TOKEN_IDENTIFIER, "Expected memory space identifier")) {
          ast_destroy_node(type);
          return NULL;
        }
        
        /* Get the memory space name */
        type->data.type_ptr.memory_space = token_to_str(&parser->current);
        if (type->data.type_ptr.memory_space == NULL) {
          ast_destroy_node(type);
          parser_set_error(parser, strdup("Memory allocation error for memory space"));
          return NULL;
        }
      }
      
      /* Expect > to close type parameter */
      if (!parser_expect(parser, TOKEN_GREATER, "Expected '>' to close pointer type")) {
        ast_destroy_node(type);
        return NULL;
      }
      break;
      
    case TOKEN_VEC:
      /* Vector type */
      parser_advance(parser);
      
      /* Expect < for type parameters */
      if (!parser_expect(parser, TOKEN_LESS, "Expected '<' after 'vec'")) {
        return NULL;
      }
      
      /* Parse the element type */
      element_type = parse_type(parser);
      if (element_type == NULL) {
        return NULL;
      }
      
      /* Expect comma */
      if (!parser_expect(parser, TOKEN_COMMA, "Expected ',' after vector element type")) {
        ast_destroy_node(element_type);
        return NULL;
      }
      
      /* Expect vector size */
      if (!parser_expect(parser, TOKEN_INTEGER, "Expected vector size")) {
        ast_destroy_node(element_type);
        return NULL;
      }
      
      /* Get the vector size */
      uint32_t vector_size = (uint32_t)parser->current.value.int_value;
      
      /* Create the vector type node */
      type = ast_create_node(AST_TYPE_VEC);
      if (type == NULL) {
        ast_destroy_node(element_type);
        parser_set_error(parser, strdup("Memory allocation error for vector type"));
        return NULL;
      }
      
      /* Set the vector type properties */
      type->data.type_vec.element_type = element_type;
      type->data.type_vec.size = vector_size;
      
      /* Expect > to close type parameters */
      if (!parser_expect(parser, TOKEN_GREATER, "Expected '>' to close vector type")) {
        ast_destroy_node(type);
        return NULL;
      }
      break;
      
    case TOKEN_ARRAY:
      /* Array type */
      parser_advance(parser);
      
      /* Expect < for type parameters */
      if (!parser_expect(parser, TOKEN_LESS, "Expected '<' after 'array'")) {
        return NULL;
      }
      
      /* Parse the element type */
      element_type = parse_type(parser);
      if (element_type == NULL) {
        return NULL;
      }
      
      /* Create the array type node */
      type = ast_create_node(AST_TYPE_ARRAY);
      if (type == NULL) {
        ast_destroy_node(element_type);
        parser_set_error(parser, strdup("Memory allocation error for array type"));
        return NULL;
      }
      
      /* Set the array type properties */
      type->data.type_array.element_type = element_type;
      type->data.type_array.size = 0;  /* Default to unsized array */
      
      /* Check for optional array size */
      if (parser_match(parser, TOKEN_COMMA)) {
        /* Expect array size */
        if (!parser_expect(parser, TOKEN_INTEGER, "Expected array size")) {
          ast_destroy_node(type);
          return NULL;
        }
        
        /* Get the array size */
        type->data.type_array.size = (uint32_t)parser->current.value.int_value;
      }
      
      /* Expect > to close type parameters */
      if (!parser_expect(parser, TOKEN_GREATER, "Expected '>' to close array type")) {
        ast_destroy_node(type);
        return NULL;
      }
      break;
      
    case TOKEN_IDENTIFIER:
      /* Named type */
      type = ast_create_node(AST_TYPE_NAME);
      if (type != NULL) {
        /* Set the type name */
        type->data.type_name.name = token_to_str(&parser->current);
        if (type->data.type_name.name == NULL) {
          ast_destroy_node(type);
          parser_set_error(parser, strdup("Memory allocation error for type name"));
          return NULL;
        }
      }
      parser_advance(parser);
      break;
      
    case TOKEN_FUNCTION:
      /* Function type */
      parser_advance(parser);
      
      /* Expect opening parenthesis */
      if (!parser_expect(parser, TOKEN_LPAREN, "Expected '(' after 'function'")) {
        return NULL;
      }
      
      /* Create function type node */
      type = ast_create_node(AST_TYPE_FUNCTION);
      if (type == NULL) {
        parser_set_error(parser, strdup("Memory allocation error for function type"));
        return NULL;
      }
      
      /* Initialize parameter types list */
      type->data.type_function.parameter_types.nodes = NULL;
      type->data.type_function.parameter_types.count = 0;
      type->data.type_function.parameter_types.capacity = 0;
      
      /* Parse parameter types (if any) */
      if (!parser_check(parser, TOKEN_RPAREN)) {
        do {
          /* Parse parameter type */
          ast_node_t* param_type = parse_type(parser);
          if (param_type == NULL) {
            ast_destroy_node(type);
            return NULL;
          }
          
          /* Add parameter type to list */
          if (!ast_add_node(&type->data.type_function.parameter_types, param_type)) {
            ast_destroy_node(param_type);
            ast_destroy_node(type);
            parser_set_error(parser, strdup("Memory allocation error adding parameter type"));
            return NULL;
          }
        } while (parser_match(parser, TOKEN_COMMA));
      }
      
      /* Expect closing parenthesis */
      if (!parser_expect(parser, TOKEN_RPAREN, "Expected ')' after function parameters")) {
        ast_destroy_node(type);
        return NULL;
      }
      
      /* Expect arrow */
      if (!parser_expect(parser, TOKEN_ARROW, "Expected '->' after function parameters")) {
        ast_destroy_node(type);
        return NULL;
      }
      
      /* Parse return type */
      ast_node_t* return_type = parse_type(parser);
      if (return_type == NULL) {
        ast_destroy_node(type);
        return NULL;
      }
      
      /* Set the return type */
      type->data.type_function.return_type = return_type;
      break;
      
    default: {
      /* Unexpected token */
      char error[64];
      snprintf(error, sizeof(error), "Unexpected token in type: %s", 
              token_type_name(parser->current.type));
      parser_set_error(parser, strdup(error));
      return NULL;
    }
  }
  
  if (type == NULL) {
    parser_set_error(parser, strdup("Memory allocation error for type"));
    return NULL;
  }
  
  /* Set the type location */
  ast_set_location(type, parser->current.line, parser->current.column, 
                   parser->filename);
  
  return type;
}

/**
 * @brief Parse a constant declaration.
 * 
 * @param parser The parser.
 * @return The parsed constant AST node, or NULL on error.
 */
static ast_node_t* parse_constant(parser_t* parser) {
  /* Expect CONSTANT keyword */
  if (!parser_expect(parser, TOKEN_CONSTANT, "Expected 'CONSTANT' keyword")) {
    return NULL;
  }
  
  /* Expect constant name identifier */
  if (!parser_expect(parser, TOKEN_IDENTIFIER, "Expected constant name identifier")) {
    return NULL;
  }
  
  /* Get the constant name */
  char* constant_name = token_to_str(&parser->current);
  if (constant_name == NULL) {
    parser_set_error(parser, strdup("Memory allocation error for constant name"));
    return NULL;
  }
  
  /* Expect colon */
  if (!parser_expect(parser, TOKEN_COLON, "Expected ':' after constant name")) {
    free(constant_name);
    return NULL;
  }
  
  /* Parse constant type */
  ast_node_t* constant_type = parse_type(parser);
  if (constant_type == NULL) {
    free(constant_name);
    return NULL;
  }
  
  /* Expect equals sign */
  if (!parser_expect(parser, TOKEN_EQUAL, "Expected '=' after constant type")) {
    free(constant_name);
    ast_destroy_node(constant_type);
    return NULL;
  }
  
  /* Parse constant value expression */
  ast_node_t* constant_value = parse_expression(parser);
  if (constant_value == NULL) {
    free(constant_name);
    ast_destroy_node(constant_type);
    return NULL;
  }
  
  /* Create constant node */
  ast_node_t* constant = ast_create_node(AST_CONSTANT);
  if (constant == NULL) {
    free(constant_name);
    ast_destroy_node(constant_type);
    ast_destroy_node(constant_value);
    parser_set_error(parser, strdup("Memory allocation error for constant"));
    return NULL;
  }
  
  /* Set constant properties */
  constant->data.constant.name = constant_name;
  constant->data.constant.type = constant_type;
  constant->data.constant.value = constant_value;
  
  /* Set constant location */
  ast_set_location(constant, parser->current.line, parser->current.column, 
                  parser->filename);
  
  /* Expect semicolon */
  if (!parser_expect(parser, TOKEN_SEMICOLON, "Expected ';' after constant definition")) {
    ast_destroy_node(constant);
    return NULL;
  }
  
  return constant;
}

/**
 * @brief Parse a global variable declaration.
 * 
 * @param parser The parser.
 * @return The parsed global variable AST node, or NULL on error.
 */
static ast_node_t* parse_global(parser_t* parser) {
  /* Expect GLOBAL keyword */
  if (!parser_expect(parser, TOKEN_GLOBAL, "Expected 'GLOBAL' keyword")) {
    return NULL;
  }
  
  /* Expect global variable name identifier */
  if (!parser_expect(parser, TOKEN_IDENTIFIER, "Expected global variable name identifier")) {
    return NULL;
  }
  
  /* Get the global variable name */
  char* global_name = token_to_str(&parser->current);
  if (global_name == NULL) {
    parser_set_error(parser, strdup("Memory allocation error for global variable name"));
    return NULL;
  }
  
  /* Expect colon */
  if (!parser_expect(parser, TOKEN_COLON, "Expected ':' after global variable name")) {
    free(global_name);
    return NULL;
  }
  
  /* Parse global variable type */
  ast_node_t* global_type = parse_type(parser);
  if (global_type == NULL) {
    free(global_name);
    return NULL;
  }
  
  /* Create global variable node */
  ast_node_t* global = ast_create_node(AST_GLOBAL);
  if (global == NULL) {
    free(global_name);
    ast_destroy_node(global_type);
    parser_set_error(parser, strdup("Memory allocation error for global variable"));
    return NULL;
  }
  
  /* Set global variable properties */
  global->data.global.name = global_name;
  global->data.global.type = global_type;
  global->data.global.initializer = NULL;  /* No initializer by default */
  
  /* Set global variable location */
  ast_set_location(global, parser->current.line, parser->current.column, 
                  parser->filename);
  
  /* Check for optional initializer */
  if (parser_match(parser, TOKEN_EQUAL)) {
    /* Parse initializer expression */
    ast_node_t* initializer = parse_expression(parser);
    if (initializer == NULL) {
      ast_destroy_node(global);
      return NULL;
    }
    
    /* Set the initializer */
    global->data.global.initializer = initializer;
  }
  
  /* Expect semicolon */
  if (!parser_expect(parser, TOKEN_SEMICOLON, "Expected ';' after global variable declaration")) {
    ast_destroy_node(global);
    return NULL;
  }
  
  return global;
}

/**
 * @brief Parse a function declaration.
 * 
 * @param parser The parser.
 * @return The parsed function AST node, or NULL on error.
 */
static ast_node_t* parse_function(parser_t* parser) {
  /* Expect FUNCTION keyword */
  if (!parser_expect(parser, TOKEN_FUNCTION, "Expected 'FUNCTION' keyword")) {
    return NULL;
  }
  
  /* Expect function name identifier */
  if (!parser_expect(parser, TOKEN_IDENTIFIER, "Expected function name identifier")) {
    return NULL;
  }
  
  /* Get the function name */
  char* function_name = token_to_str(&parser->current);
  if (function_name == NULL) {
    parser_set_error(parser, strdup("Memory allocation error for function name"));
    return NULL;
  }
  
  /* Expect opening parenthesis */
  if (!parser_expect(parser, TOKEN_LPAREN, "Expected '(' after function name")) {
    free(function_name);
    return NULL;
  }
  
  /* Create function node */
  ast_node_t* function = ast_create_node(AST_FUNCTION);
  if (function == NULL) {
    free(function_name);
    parser_set_error(parser, strdup("Memory allocation error for function"));
    return NULL;
  }
  
  /* Set function name */
  function->data.function.name = function_name;
  
  /* Initialize parameters list */
  function->data.function.parameters.nodes = NULL;
  function->data.function.parameters.count = 0;
  function->data.function.parameters.capacity = 0;
  
  /* Initialize blocks list */
  function->data.function.blocks.nodes = NULL;
  function->data.function.blocks.count = 0;
  function->data.function.blocks.capacity = 0;
  
  /* Set function location */
  ast_set_location(function, parser->current.line, parser->current.column, 
                  parser->filename);
  
  /* Parse parameters (if any) */
  if (!parser_check(parser, TOKEN_RPAREN)) {
    do {
      /* Expect parameter name identifier */
      if (!parser_expect(parser, TOKEN_IDENTIFIER, "Expected parameter name identifier")) {
        ast_destroy_node(function);
        return NULL;
      }
      
      /* Get the parameter name */
      char* param_name = token_to_str(&parser->current);
      if (param_name == NULL) {
        ast_destroy_node(function);
        parser_set_error(parser, strdup("Memory allocation error for parameter name"));
        return NULL;
      }
      
      /* Expect colon */
      if (!parser_expect(parser, TOKEN_COLON, "Expected ':' after parameter name")) {
        free(param_name);
        ast_destroy_node(function);
        return NULL;
      }
      
      /* Parse parameter type */
      ast_node_t* param_type = parse_type(parser);
      if (param_type == NULL) {
        free(param_name);
        ast_destroy_node(function);
        return NULL;
      }
      
      /* Create parameter node */
      ast_node_t* param = ast_create_node(AST_PARAMETER);
      if (param == NULL) {
        free(param_name);
        ast_destroy_node(param_type);
        ast_destroy_node(function);
        parser_set_error(parser, strdup("Memory allocation error for parameter"));
        return NULL;
      }
      
      /* Set parameter properties */
      param->data.parameter.name = param_name;
      param->data.parameter.type = param_type;
      
      /* Set parameter location */
      ast_set_location(param, parser->current.line, parser->current.column, 
                      parser->filename);
      
      /* Add parameter to function */
      if (!ast_add_node(&function->data.function.parameters, param)) {
        ast_destroy_node(param);
        ast_destroy_node(function);
        parser_set_error(parser, strdup("Memory allocation error adding parameter"));
        return NULL;
      }
    } while (parser_match(parser, TOKEN_COMMA));
  }
  
  /* Expect closing parenthesis */
  if (!parser_expect(parser, TOKEN_RPAREN, "Expected ')' after function parameters")) {
    ast_destroy_node(function);
    return NULL;
  }
  
  /* Expect arrow */
  if (!parser_expect(parser, TOKEN_ARROW, "Expected '->' after function parameters")) {
    ast_destroy_node(function);
    return NULL;
  }
  
  /* Parse return type */
  ast_node_t* return_type = parse_type(parser);
  if (return_type == NULL) {
    ast_destroy_node(function);
    return NULL;
  }
  
  /* Set function return type */
  function->data.function.return_type = return_type;
  
  /* Check for optional target specification */
  if (parser_check(parser, TOKEN_TARGET)) {
    /* Parse target specification (simplified for now) */
    ast_node_t* target = ast_create_node(AST_TARGET);
    if (target == NULL) {
      ast_destroy_node(function);
      parser_set_error(parser, strdup("Memory allocation error for target"));
      return NULL;
    }
    
    /* For now, skip target specification */
    /* This should be properly parsed in a full implementation */
    parser_advance(parser);
    
    /* Expect target specifier (string or identifier) */
    if (parser_check(parser, TOKEN_STRING) || parser_check(parser, TOKEN_IDENTIFIER)) {
      parser_advance(parser);
    } else {
      ast_destroy_node(target);
      ast_destroy_node(function);
      parser_set_error(parser, strdup("Expected target specifier"));
      return NULL;
    }
    
    /* Set function target */
    function->data.function.target = target;
  }
  
  /* Expect opening brace */
  if (!parser_expect(parser, TOKEN_LBRACE, "Expected '{' to start function body")) {
    ast_destroy_node(function);
    return NULL;
  }
  
  /* Parse basic blocks */
  while (!parser_check(parser, TOKEN_RBRACE)) {
    /* Parse a basic block */
    ast_node_t* block = parse_block(parser);
    if (block == NULL) {
      ast_destroy_node(function);
      return NULL;
    }
    
    /* Add block to function */
    if (!ast_add_node(&function->data.function.blocks, block)) {
      ast_destroy_node(block);
      ast_destroy_node(function);
      parser_set_error(parser, strdup("Memory allocation error adding block"));
      return NULL;
    }
  }
  
  /* Expect closing brace */
  if (!parser_expect(parser, TOKEN_RBRACE, "Expected '}' to end function body")) {
    ast_destroy_node(function);
    return NULL;
  }
  
  return function;
}

/**
 * @brief Parse an external function declaration.
 * 
 * @param parser The parser.
 * @return The parsed external function AST node, or NULL on error.
 */
static ast_node_t* parse_extern_function(parser_t* parser) {
  /* Expect EXTERN keyword */
  if (!parser_expect(parser, TOKEN_EXTERN, "Expected 'EXTERN' keyword")) {
    return NULL;
  }
  
  /* Expect FUNCTION keyword */
  if (!parser_expect(parser, TOKEN_FUNCTION, "Expected 'FUNCTION' keyword after 'EXTERN'")) {
    return NULL;
  }
  
  /* Expect function name identifier */
  if (!parser_expect(parser, TOKEN_IDENTIFIER, "Expected external function name identifier")) {
    return NULL;
  }
  
  /* Get the function name */
  char* function_name = token_to_str(&parser->current);
  if (function_name == NULL) {
    parser_set_error(parser, strdup("Memory allocation error for external function name"));
    return NULL;
  }
  
  /* Expect opening parenthesis */
  if (!parser_expect(parser, TOKEN_LPAREN, "Expected '(' after external function name")) {
    free(function_name);
    return NULL;
  }
  
  /* Create external function node */
  ast_node_t* extern_function = ast_create_node(AST_EXTERN_FUNCTION);
  if (extern_function == NULL) {
    free(function_name);
    parser_set_error(parser, strdup("Memory allocation error for external function"));
    return NULL;
  }
  
  /* Set external function name */
  extern_function->data.extern_function.name = function_name;
  
  /* Initialize parameters list */
  extern_function->data.extern_function.parameters.nodes = NULL;
  extern_function->data.extern_function.parameters.count = 0;
  extern_function->data.extern_function.parameters.capacity = 0;
  
  /* Set external function as not variadic by default */
  extern_function->data.extern_function.is_variadic = false;
  
  /* Set external function location */
  ast_set_location(extern_function, parser->current.line, parser->current.column, 
                  parser->filename);
  
  /* Parse parameters (if any) */
  if (!parser_check(parser, TOKEN_RPAREN)) {
    do {
      /* Check for variadic parameter */
      if (parser_match(parser, TOKEN_DOT)) {
        /* Expect two more dots for "..." */
        if (!parser_expect(parser, TOKEN_DOT, "Expected '...' for variadic parameter")) {
          ast_destroy_node(extern_function);
          return NULL;
        }
        if (!parser_expect(parser, TOKEN_DOT, "Expected '...' for variadic parameter")) {
          ast_destroy_node(extern_function);
          return NULL;
        }
        
        /* Mark the function as variadic */
        extern_function->data.extern_function.is_variadic = true;
        
        /* Variadic parameter must be the last parameter */
        break;
      }
      
      /* Expect parameter name identifier */
      if (!parser_expect(parser, TOKEN_IDENTIFIER, "Expected parameter name identifier")) {
        ast_destroy_node(extern_function);
        return NULL;
      }
      
      /* Get the parameter name */
      char* param_name = token_to_str(&parser->current);
      if (param_name == NULL) {
        ast_destroy_node(extern_function);
        parser_set_error(parser, strdup("Memory allocation error for parameter name"));
        return NULL;
      }
      
      /* Expect colon */
      if (!parser_expect(parser, TOKEN_COLON, "Expected ':' after parameter name")) {
        free(param_name);
        ast_destroy_node(extern_function);
        return NULL;
      }
      
      /* Parse parameter type */
      ast_node_t* param_type = parse_type(parser);
      if (param_type == NULL) {
        free(param_name);
        ast_destroy_node(extern_function);
        return NULL;
      }
      
      /* Create parameter node */
      ast_node_t* param = ast_create_node(AST_PARAMETER);
      if (param == NULL) {
        free(param_name);
        ast_destroy_node(param_type);
        ast_destroy_node(extern_function);
        parser_set_error(parser, strdup("Memory allocation error for parameter"));
        return NULL;
      }
      
      /* Set parameter properties */
      param->data.parameter.name = param_name;
      param->data.parameter.type = param_type;
      
      /* Set parameter location */
      ast_set_location(param, parser->current.line, parser->current.column, 
                      parser->filename);
      
      /* Add parameter to function */
      if (!ast_add_node(&extern_function->data.extern_function.parameters, param)) {
        ast_destroy_node(param);
        ast_destroy_node(extern_function);
        parser_set_error(parser, strdup("Memory allocation error adding parameter"));
        return NULL;
      }
    } while (parser_match(parser, TOKEN_COMMA));
  }
  
  /* Expect closing parenthesis */
  if (!parser_expect(parser, TOKEN_RPAREN, "Expected ')' after external function parameters")) {
    ast_destroy_node(extern_function);
    return NULL;
  }
  
  /* Expect arrow */
  if (!parser_expect(parser, TOKEN_ARROW, "Expected '->' after external function parameters")) {
    ast_destroy_node(extern_function);
    return NULL;
  }
  
  /* Parse return type */
  ast_node_t* return_type = parse_type(parser);
  if (return_type == NULL) {
    ast_destroy_node(extern_function);
    return NULL;
  }
  
  /* Set external function return type */
  extern_function->data.extern_function.return_type = return_type;
  
  /* Expect semicolon */
  if (!parser_expect(parser, TOKEN_SEMICOLON, "Expected ';' after external function declaration")) {
    ast_destroy_node(extern_function);
    return NULL;
  }
  
  return extern_function;
}

/**
 * @brief Parse a basic block.
 * 
 * @param parser The parser.
 * @return The parsed basic block AST node, or NULL on error.
 */
static ast_node_t* parse_block(parser_t* parser) {
  /* Expect block label identifier */
  if (!parser_expect(parser, TOKEN_IDENTIFIER, "Expected block label identifier")) {
    return NULL;
  }
  
  /* Get the block label */
  char* block_label = token_to_str(&parser->current);
  if (block_label == NULL) {
    parser_set_error(parser, strdup("Memory allocation error for block label"));
    return NULL;
  }
  
  /* Expect colon */
  if (!parser_expect(parser, TOKEN_COLON, "Expected ':' after block label")) {
    free(block_label);
    return NULL;
  }
  
  /* Create block node */
  ast_node_t* block = ast_create_node(AST_STMT_BLOCK);
  if (block == NULL) {
    free(block_label);
    parser_set_error(parser, strdup("Memory allocation error for block"));
    return NULL;
  }
  
  /* Set block label */
  block->data.stmt_block.label = block_label;
  
  /* Initialize statements list */
  block->data.stmt_block.statements.nodes = NULL;
  block->data.stmt_block.statements.count = 0;
  block->data.stmt_block.statements.capacity = 0;
  
  /* Set block location */
  ast_set_location(block, parser->current.line, parser->current.column, 
                  parser->filename);
  
  /* Parse statements until the next block or end of function */
  while (!parser_check(parser, TOKEN_IDENTIFIER) || 
         !lexer_peek_token(parser->lexer, &parser->current) || 
         parser->current.type != TOKEN_COLON) {
    /* Check for end of function */
    if (parser_check(parser, TOKEN_RBRACE)) {
      break;
    }
    
    /* Parse a statement */
    ast_node_t* statement = parse_statement(parser);
    if (statement == NULL) {
      ast_destroy_node(block);
      return NULL;
    }
    
    /* Add statement to block */
    if (!ast_add_node(&block->data.stmt_block.statements, statement)) {
      ast_destroy_node(statement);
      ast_destroy_node(block);
      parser_set_error(parser, strdup("Memory allocation error adding statement"));
      return NULL;
    }
  }
  
  return block;
}

/**
 * @brief Parse a statement.
 * 
 * @param parser The parser.
 * @return The parsed statement AST node, or NULL on error.
 */
static ast_node_t* parse_statement(parser_t* parser) {
  /* Check statement type */
  if (parser_check(parser, TOKEN_IDENTIFIER)) {
    token_t peek;
    if (lexer_peek_token(parser->lexer, &peek)) {
      if (peek.type == TOKEN_EQUAL) {
        /* Assignment statement */
        return parse_assignment(parser);
      }
    }
  }
  
  /* Check for return statement */
  if (parser_check(parser, TOKEN_RET)) {
    return parse_return(parser);
  }
  
  /* Check for branch statement */
  if (parser_check(parser, TOKEN_BR)) {
    return parse_branch(parser);
  }
  
  /* Instruction statement */
  return parse_instruction(parser);
}

/**
 * @brief Parse an assignment statement.
 * 
 * @param parser The parser.
 * @return The parsed assignment statement AST node, or NULL on error.
 */
static ast_node_t* parse_assignment(parser_t* parser) {
  /* Expect target identifier */
  if (!parser_expect(parser, TOKEN_IDENTIFIER, "Expected target identifier for assignment")) {
    return NULL;
  }
  
  /* Get the target name */
  char* target = token_to_str(&parser->current);
  if (target == NULL) {
    parser_set_error(parser, strdup("Memory allocation error for assignment target"));
    return NULL;
  }
  
  /* Expect equals sign */
  if (!parser_expect(parser, TOKEN_EQUAL, "Expected '=' after assignment target")) {
    free(target);
    return NULL;
  }
  
  /* Parse value expression (instruction) */
  ast_node_t* value = parse_instruction(parser);
  if (value == NULL) {
    free(target);
    return NULL;
  }
  
  /* Create assignment node */
  ast_node_t* assignment = ast_create_node(AST_STMT_ASSIGN);
  if (assignment == NULL) {
    free(target);
    ast_destroy_node(value);
    parser_set_error(parser, strdup("Memory allocation error for assignment"));
    return NULL;
  }
  
  /* Set assignment properties */
  assignment->data.stmt_assign.target = target;
  assignment->data.stmt_assign.value = value;
  
  /* Set assignment location */
  ast_set_location(assignment, parser->current.line, parser->current.column, 
                  parser->filename);
  
  return assignment;
}

/**
 * @brief Parse an instruction statement.
 * 
 * @param parser The parser.
 * @return The parsed instruction statement AST node, or NULL on error.
 */
static ast_node_t* parse_instruction(parser_t* parser) {
  /* Check if the token is an instruction */
  if (!token_is_instruction(parser->current.type)) {
    char error[64];
    snprintf(error, sizeof(error), "Expected instruction, got %s", 
             token_type_name(parser->current.type));
    parser_set_error(parser, strdup(error));
    return NULL;
  }
  
  /* Get the instruction opcode */
  char* opcode = token_to_str(&parser->current);
  if (opcode == NULL) {
    parser_set_error(parser, strdup("Memory allocation error for instruction opcode"));
    return NULL;
  }
  
  /* Create instruction node */
  ast_node_t* instruction = ast_create_node(AST_STMT_INSTRUCTION);
  if (instruction == NULL) {
    free(opcode);
    parser_set_error(parser, strdup("Memory allocation error for instruction"));
    return NULL;
  }
  
  /* Set instruction opcode */
  instruction->data.stmt_instruction.opcode = opcode;
  
  /* Initialize operands list */
  instruction->data.stmt_instruction.operands.nodes = NULL;
  instruction->data.stmt_instruction.operands.count = 0;
  instruction->data.stmt_instruction.operands.capacity = 0;
  
  /* Set instruction location */
  ast_set_location(instruction, parser->current.line, parser->current.column, 
                  parser->filename);
  
  /* Advance past the opcode */
  parser_advance(parser);
  
  /* Parse operands */
  bool first_operand = true;
  while (!parser_check(parser, TOKEN_SEMICOLON)) {
    /* Expect comma between operands (except for the first one) */
    if (!first_operand) {
      if (!parser_expect(parser, TOKEN_COMMA, "Expected ',' between operands")) {
        ast_destroy_node(instruction);
        return NULL;
      }
    }
    
    /* Parse operand expression */
    ast_node_t* operand = parse_expression(parser);
    if (operand == NULL) {
      ast_destroy_node(instruction);
      return NULL;
    }
    
    /* Add operand to instruction */
    if (!ast_add_node(&instruction->data.stmt_instruction.operands, operand)) {
      ast_destroy_node(operand);
      ast_destroy_node(instruction);
      parser_set_error(parser, strdup("Memory allocation error adding operand"));
      return NULL;
    }
    
    first_operand = false;
  }
  
  /* Expect semicolon */
  if (!parser_expect(parser, TOKEN_SEMICOLON, "Expected ';' after instruction")) {
    ast_destroy_node(instruction);
    return NULL;
  }
  
  return instruction;
}

/**
 * @brief Parse a branch statement.
 * 
 * @param parser The parser.
 * @return The parsed branch statement AST node, or NULL on error.
 */
static ast_node_t* parse_branch(parser_t* parser) {
  /* Expect BR keyword */
  if (!parser_expect(parser, TOKEN_BR, "Expected 'BR' keyword")) {
    return NULL;
  }
  
  /* Create branch node */
  ast_node_t* branch = ast_create_node(AST_STMT_BRANCH);
  if (branch == NULL) {
    parser_set_error(parser, strdup("Memory allocation error for branch"));
    return NULL;
  }
  
  /* Initialize branch properties */
  branch->data.stmt_branch.condition = NULL;
  branch->data.stmt_branch.true_target = NULL;
  branch->data.stmt_branch.false_target = NULL;
  
  /* Set branch location */
  ast_set_location(branch, parser->current.line, parser->current.column, 
                  parser->filename);
  
  /* Check for condition (optional) */
  if (!parser_check(parser, TOKEN_IDENTIFIER) || parser->current.length == 6) {
    /* Unconditional branch (condition is "ALWAYS" or similar) */
    parser_advance(parser);
    
    /* Expect comma */
    if (!parser_expect(parser, TOKEN_COMMA, "Expected ',' after branch condition")) {
      ast_destroy_node(branch);
      return NULL;
    }
    
    /* Expect target block label */
    if (!parser_expect(parser, TOKEN_IDENTIFIER, "Expected target block label")) {
      ast_destroy_node(branch);
      return NULL;
    }
    
    /* Get the target label */
    branch->data.stmt_branch.true_target = token_to_str(&parser->current);
    if (branch->data.stmt_branch.true_target == NULL) {
      ast_destroy_node(branch);
      parser_set_error(parser, strdup("Memory allocation error for branch target"));
      return NULL;
    }
  } else {
    /* Conditional branch */
    ast_node_t* condition = parse_expression(parser);
    if (condition == NULL) {
      ast_destroy_node(branch);
      return NULL;
    }
    
    /* Set branch condition */
    branch->data.stmt_branch.condition = condition;
    
    /* Expect comma */
    if (!parser_expect(parser, TOKEN_COMMA, "Expected ',' after branch condition")) {
      ast_destroy_node(branch);
      return NULL;
    }
    
    /* Expect true target block label */
    if (!parser_expect(parser, TOKEN_IDENTIFIER, "Expected true target block label")) {
      ast_destroy_node(branch);
      return NULL;
    }
    
    /* Get the true target label */
    branch->data.stmt_branch.true_target = token_to_str(&parser->current);
    if (branch->data.stmt_branch.true_target == NULL) {
      ast_destroy_node(branch);
      parser_set_error(parser, strdup("Memory allocation error for branch true target"));
      return NULL;
    }
    
    /* Expect comma */
    if (!parser_expect(parser, TOKEN_COMMA, "Expected ',' after true target")) {
      ast_destroy_node(branch);
      return NULL;
    }
    
    /* Expect false target block label */
    if (!parser_expect(parser, TOKEN_IDENTIFIER, "Expected false target block label")) {
      ast_destroy_node(branch);
      return NULL;
    }
    
    /* Get the false target label */
    branch->data.stmt_branch.false_target = token_to_str(&parser->current);
    if (branch->data.stmt_branch.false_target == NULL) {
      ast_destroy_node(branch);
      parser_set_error(parser, strdup("Memory allocation error for branch false target"));
      return NULL;
    }
  }
  
  /* Expect semicolon */
  if (!parser_expect(parser, TOKEN_SEMICOLON, "Expected ';' after branch statement")) {
    ast_destroy_node(branch);
    return NULL;
  }
  
  return branch;
}

/**
 * @brief Parse a return statement.
 * 
 * @param parser The parser.
 * @return The parsed return statement AST node, or NULL on error.
 */
static ast_node_t* parse_return(parser_t* parser) {
  /* Expect RET keyword */
  if (!parser_expect(parser, TOKEN_RET, "Expected 'RET' keyword")) {
    return NULL;
  }
  
  /* Create return node */
  ast_node_t* ret = ast_create_node(AST_STMT_RETURN);
  if (ret == NULL) {
    parser_set_error(parser, strdup("Memory allocation error for return statement"));
    return NULL;
  }
  
  /* Initialize return properties */
  ret->data.stmt_return.value = NULL;
  
  /* Set return location */
  ast_set_location(ret, parser->current.line, parser->current.column, 
                  parser->filename);
  
  /* Check for return value (optional) */
  if (!parser_check(parser, TOKEN_SEMICOLON)) {
    /* Parse return value expression */
    ast_node_t* value = parse_expression(parser);
    if (value == NULL) {
      ast_destroy_node(ret);
      return NULL;
    }
    
    /* Set return value */
    ret->data.stmt_return.value = value;
  }
  
  /* Expect semicolon */
  if (!parser_expect(parser, TOKEN_SEMICOLON, "Expected ';' after return statement")) {
    ast_destroy_node(ret);
    return NULL;
  }
  
  return ret;
}

/**
 * @brief Parse an expression.
 * 
 * @param parser The parser.
 * @return The parsed expression AST node, or NULL on error.
 */
static ast_node_t* parse_expression(parser_t* parser) {
  ast_node_t* expr = NULL;
  
  /* Parse expression based on token type */
  switch (parser->current.type) {
    case TOKEN_INTEGER:
      /* Integer literal */
      expr = ast_create_node(AST_EXPR_INTEGER);
      if (expr != NULL) {
        expr->data.expr_integer.value = parser->current.value.int_value;
      }
      parser_advance(parser);
      break;
      
    case TOKEN_FLOAT:
      /* Float literal */
      expr = ast_create_node(AST_EXPR_FLOAT);
      if (expr != NULL) {
        expr->data.expr_float.value = parser->current.value.float_value;
      }
      parser_advance(parser);
      break;
      
    case TOKEN_STRING:
      /* String literal */
      expr = ast_create_node(AST_EXPR_STRING);
      if (expr != NULL) {
        expr->data.expr_string.value = token_to_str(&parser->current);
        if (expr->data.expr_string.value == NULL) {
          ast_destroy_node(expr);
          parser_set_error(parser, strdup("Memory allocation error for string literal"));
          return NULL;
        }
      }
      parser_advance(parser);
      break;
      
    case TOKEN_IDENTIFIER:
      /* Identifier */
      expr = ast_create_node(AST_EXPR_IDENTIFIER);
      if (expr != NULL) {
        expr->data.expr_identifier.name = token_to_str(&parser->current);
        if (expr->data.expr_identifier.name == NULL) {
          ast_destroy_node(expr);
          parser_set_error(parser, strdup("Memory allocation error for identifier"));
          return NULL;
        }
      }
      parser_advance(parser);
      
      /* Check for field access */
      if (parser_match(parser, TOKEN_DOT)) {
        /* Expect field name identifier */
        if (!parser_expect(parser, TOKEN_IDENTIFIER, "Expected field name identifier")) {
          ast_destroy_node(expr);
          return NULL;
        }
        
        /* Get the field name */
        char* field_name = token_to_str(&parser->current);
        if (field_name == NULL) {
          ast_destroy_node(expr);
          parser_set_error(parser, strdup("Memory allocation error for field name"));
          return NULL;
        }
        
        /* Create field access node */
        ast_node_t* field_access = ast_create_node(AST_EXPR_FIELD);
        if (field_access == NULL) {
          free(field_name);
          ast_destroy_node(expr);
          parser_set_error(parser, strdup("Memory allocation error for field access"));
          return NULL;
        }
        
        /* Set field access properties */
        field_access->data.expr_field.object = expr;
        field_access->data.expr_field.field = field_name;
        
        /* Set field access location */
        ast_set_location(field_access, parser->current.line, parser->current.column, 
                        parser->filename);
        
        /* Replace expr with field access */
        expr = field_access;
      }
      
      /* Check for function call */
      if (parser_match(parser, TOKEN_LPAREN)) {
        /* Create function call node */
        ast_node_t* call = ast_create_node(AST_EXPR_CALL);
        if (call == NULL) {
          ast_destroy_node(expr);
          parser_set_error(parser, strdup("Memory allocation error for function call"));
          return NULL;
        }
        
        /* Set function call properties */
        call->data.expr_call.function = expr;
        
        /* Initialize arguments list */
        call->data.expr_call.arguments.nodes = NULL;
        call->data.expr_call.arguments.count = 0;
        call->data.expr_call.arguments.capacity = 0;
        
        /* Set function call location */
        ast_set_location(call, parser->current.line, parser->current.column, 
                        parser->filename);
        
        /* Parse arguments (if any) */
        if (!parser_check(parser, TOKEN_RPAREN)) {
          do {
            /* Parse argument expression */
            ast_node_t* argument = parse_expression(parser);
            if (argument == NULL) {
              ast_destroy_node(call);
              return NULL;
            }
            
            /* Add argument to function call */
            if (!ast_add_node(&call->data.expr_call.arguments, argument)) {
              ast_destroy_node(argument);
              ast_destroy_node(call);
              parser_set_error(parser, strdup("Memory allocation error adding argument"));
              return NULL;
            }
          } while (parser_match(parser, TOKEN_COMMA));
        }
        
        /* Expect closing parenthesis */
        if (!parser_expect(parser, TOKEN_RPAREN, "Expected ')' after function arguments")) {
          ast_destroy_node(call);
          return NULL;
        }
        
        /* Replace expr with function call */
        expr = call;
      }
      break;
      
    default: {
      /* Unexpected token */
      char error[64];
      snprintf(error, sizeof(error), "Unexpected token in expression: %s", 
              token_type_name(parser->current.type));
      parser_set_error(parser, strdup(error));
      return NULL;
    }
  }
  
  if (expr == NULL) {
    parser_set_error(parser, strdup("Memory allocation error for expression"));
    return NULL;
  }
  
  /* Set expression location */
  ast_set_location(expr, parser->current.line, parser->current.column, 
                  parser->filename);
  
  return expr;
}