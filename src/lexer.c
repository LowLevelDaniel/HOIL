/**
 * @file lexer.c
 * @brief Implementation of the lexical analyzer for HOIL.
 * 
 * This file contains the implementation of the lexer functions.
 * 
 * @author HOILC Team
 * @date 2025
 */

#include "../include/lexer.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

/**
 * @brief Keyword mapping structure.
 */
typedef struct {
  const char* keyword;  /**< Keyword string. */
  token_type_t type;    /**< Corresponding token type. */
} keyword_map_t;

/**
 * @brief Instruction mapping structure.
 */
typedef struct {
  const char* instruction;  /**< Instruction string. */
  token_type_t type;        /**< Corresponding token type. */
} instruction_map_t;

/**
 * @brief Lexer structure implementation.
 */
struct lexer {
  const char* source;    /**< Source code. */
  size_t length;         /**< Source code length. */
  size_t position;       /**< Current position in source. */
  size_t line;           /**< Current line (1-based). */
  size_t column;         /**< Current column (1-based). */
  size_t token_start;    /**< Start position of current token. */
  int token_line;        /**< Line of current token. */
  int token_column;      /**< Column of current token. */
  token_t peeked_token;  /**< Peeked token. */
  bool has_peeked;       /**< Whether a token has been peeked. */
};

/* Keyword table */
static const keyword_map_t keywords[] = {
  {"MODULE",   TOKEN_MODULE},
  {"TARGET",   TOKEN_TARGET},
  {"TYPE",     TOKEN_TYPE},
  {"CONSTANT", TOKEN_CONSTANT},
  {"GLOBAL",   TOKEN_GLOBAL},
  {"EXTERN",   TOKEN_EXTERN},
  {"FUNCTION", TOKEN_FUNCTION},
  {"ENTRY",    TOKEN_ENTRY},
  {"void",     TOKEN_VOID},
  {"bool",     TOKEN_BOOL},
  {"i8",       TOKEN_I8},
  {"i16",      TOKEN_I16},
  {"i32",      TOKEN_I32},
  {"i64",      TOKEN_I64},
  {"u8",       TOKEN_U8},
  {"u16",      TOKEN_U16},
  {"u32",      TOKEN_U32},
  {"u64",      TOKEN_U64},
  {"f16",      TOKEN_F16},
  {"f32",      TOKEN_F32},
  {"f64",      TOKEN_F64},
  {"ptr",      TOKEN_PTR},
  {"vec",      TOKEN_VEC},
  {"array",    TOKEN_ARRAY},
  {NULL,       TOKEN_ERROR}  /* Sentinel */
};

/* Instruction table */
static const instruction_map_t instructions[] = {
  {"ADD",     TOKEN_ADD},
  {"SUB",     TOKEN_SUB},
  {"MUL",     TOKEN_MUL},
  {"DIV",     TOKEN_DIV},
  {"REM",     TOKEN_REM},
  {"NEG",     TOKEN_NEG},
  {"AND",     TOKEN_AND},
  {"OR",      TOKEN_OR},
  {"XOR",     TOKEN_XOR},
  {"NOT",     TOKEN_NOT},
  {"SHL",     TOKEN_SHL},
  {"SHR",     TOKEN_SHR},
  {"CMP_EQ",  TOKEN_CMP_EQ},
  {"CMP_NE",  TOKEN_CMP_NE},
  {"CMP_LT",  TOKEN_CMP_LT},
  {"CMP_LE",  TOKEN_CMP_LE},
  {"CMP_GT",  TOKEN_CMP_GT},
  {"CMP_GE",  TOKEN_CMP_GE},
  {"LOAD",    TOKEN_LOAD},
  {"STORE",   TOKEN_STORE},
  {"LEA",     TOKEN_LEA},
  {"BR",      TOKEN_BR},
  {"CALL",    TOKEN_CALL},
  {"RET",     TOKEN_RET},
  {NULL,      TOKEN_ERROR}  /* Sentinel */
};

/* Token type names for debugging */
static const char* token_names[TOKEN_COUNT] = {
  "EOF",           /* TOKEN_EOF */
  "ERROR",         /* TOKEN_ERROR */
  "(",             /* TOKEN_LPAREN */
  ")",             /* TOKEN_RPAREN */
  "{",             /* TOKEN_LBRACE */
  "}",             /* TOKEN_RBRACE */
  "[",             /* TOKEN_LBRACKET */
  "]",             /* TOKEN_RBRACKET */
  ",",             /* TOKEN_COMMA */
  ".",             /* TOKEN_DOT */
  ";",             /* TOKEN_SEMICOLON */
  ":",             /* TOKEN_COLON */
  "->",            /* TOKEN_ARROW */
  "=",             /* TOKEN_EQUAL */
  "<",             /* TOKEN_LESS */
  ">",             /* TOKEN_GREATER */
  "MODULE",        /* TOKEN_MODULE */
  "TARGET",        /* TOKEN_TARGET */
  "TYPE",          /* TOKEN_TYPE */
  "CONSTANT",      /* TOKEN_CONSTANT */
  "GLOBAL",        /* TOKEN_GLOBAL */
  "EXTERN",        /* TOKEN_EXTERN */
  "FUNCTION",      /* TOKEN_FUNCTION */
  "ENTRY",         /* TOKEN_ENTRY */
  "void",          /* TOKEN_VOID */
  "bool",          /* TOKEN_BOOL */
  "i8",            /* TOKEN_I8 */
  "i16",           /* TOKEN_I16 */
  "i32",           /* TOKEN_I32 */
  "i64",           /* TOKEN_I64 */
  "u8",            /* TOKEN_U8 */
  "u16",           /* TOKEN_U16 */
  "u32",           /* TOKEN_U32 */
  "u64",           /* TOKEN_U64 */
  "f16",           /* TOKEN_F16 */
  "f32",           /* TOKEN_F32 */
  "f64",           /* TOKEN_F64 */
  "ptr",           /* TOKEN_PTR */
  "vec",           /* TOKEN_VEC */
  "array",         /* TOKEN_ARRAY */
  "IDENTIFIER",    /* TOKEN_IDENTIFIER */
  "INTEGER",       /* TOKEN_INTEGER */
  "FLOAT",         /* TOKEN_FLOAT */
  "STRING",        /* TOKEN_STRING */
  "ADD",           /* TOKEN_ADD */
  "SUB",           /* TOKEN_SUB */
  "MUL",           /* TOKEN_MUL */
  "DIV",           /* TOKEN_DIV */
  "REM",           /* TOKEN_REM */
  "NEG",           /* TOKEN_NEG */
  "AND",           /* TOKEN_AND */
  "OR",            /* TOKEN_OR */
  "XOR",           /* TOKEN_XOR */
  "NOT",           /* TOKEN_NOT */
  "SHL",           /* TOKEN_SHL */
  "SHR",           /* TOKEN_SHR */
  "CMP_EQ",        /* TOKEN_CMP_EQ */
  "CMP_NE",        /* TOKEN_CMP_NE */
  "CMP_LT",        /* TOKEN_CMP_LT */
  "CMP_LE",        /* TOKEN_CMP_LE */
  "CMP_GT",        /* TOKEN_CMP_GT */
  "CMP_GE",        /* TOKEN_CMP_GE */
  "LOAD",          /* TOKEN_LOAD */
  "STORE",         /* TOKEN_STORE */
  "LEA",           /* TOKEN_LEA */
  "BR",            /* TOKEN_BR */
  "CALL",          /* TOKEN_CALL */
  "RET",           /* TOKEN_RET */
};

/**
 * @brief Check if a character is valid in an identifier.
 * 
 * @param c The character to check.
 * @return true if the character is valid in an identifier, false otherwise.
 */
static bool is_identifier_char(char c) {
  return isalnum(c) || c == '_';
}

/**
 * @brief Get the current character in the source.
 * 
 * @param lexer The lexer.
 * @return The current character or '\0' if at the end of the source.
 */
static char lexer_current_char(lexer_t* lexer) {
  if (lexer->position >= lexer->length) {
    return '\0';
  }
  return lexer->source[lexer->position];
}

/**
 * @brief Get the next character in the source.
 * 
 * @param lexer The lexer.
 * @return The next character or '\0' if at the end of the source.
 */
static char lexer_peek_char(lexer_t* lexer) {
  if (lexer->position + 1 >= lexer->length) {
    return '\0';
  }
  return lexer->source[lexer->position + 1];
}

/**
 * @brief Advance to the next character in the source.
 * 
 * @param lexer The lexer.
 */
static void lexer_advance(lexer_t* lexer) {
  char c = lexer_current_char(lexer);
  
  if (c == '\0') {
    return;
  }
  
  lexer->position++;
  lexer->column++;
  
  if (c == '\n') {
    lexer->line++;
    lexer->column = 1;
  }
}

/**
 * @brief Skip whitespace and comments in the source.
 * 
 * @param lexer The lexer.
 */
static void lexer_skip_whitespace_and_comments(lexer_t* lexer) {
  while (true) {
    char c = lexer_current_char(lexer);
    
    if (isspace(c)) {
      lexer_advance(lexer);
      continue;
    }
    
    if (c == '/' && lexer_peek_char(lexer) == '/') {
      /* Skip line comment */
      lexer_advance(lexer);
      lexer_advance(lexer);
      
      while (lexer_current_char(lexer) != '\n' && 
             lexer_current_char(lexer) != '\0') {
        lexer_advance(lexer);
      }
      
      continue;
    }
    
    if (c == '/' && lexer_peek_char(lexer) == '*') {
      /* Skip block comment */
      lexer_advance(lexer);
      lexer_advance(lexer);
      
      while (!(lexer_current_char(lexer) == '*' && 
               lexer_peek_char(lexer) == '/')) {
        if (lexer_current_char(lexer) == '\0') {
          /* Unterminated comment */
          return;
        }
        
        lexer_advance(lexer);
      }
      
      /* Skip the closing '*/
      lexer_advance(lexer);
      lexer_advance(lexer);
      
      continue;
    }
    
    break;
  }
}

/**
 * @brief Initialize a token.
 * 
 * @param lexer The lexer.
 * @param token The token to initialize.
 * @param type The token type.
 */
static void init_token(lexer_t* lexer, token_t* token, token_type_t type) {
  token->type = type;
  token->start = lexer->source + lexer->token_start;
  token->length = lexer->position - lexer->token_start;
  token->line = lexer->token_line;
  token->column = lexer->token_column;
}

/**
 * @brief Scan an identifier or keyword.
 * 
 * @param lexer The lexer.
 * @param token The token to fill.
 */
static void scan_identifier_or_keyword(lexer_t* lexer, token_t* token) {
  /* Consume the identifier */
  while (is_identifier_char(lexer_current_char(lexer))) {
    lexer_advance(lexer);
  }
  
  /* Check if it's a keyword or instruction */
  size_t length = lexer->position - lexer->token_start;
  const char* start = lexer->source + lexer->token_start;
  
  /* Check against keywords */
  for (const keyword_map_t* k = keywords; k->keyword != NULL; k++) {
    if (strlen(k->keyword) == length && 
        strncmp(start, k->keyword, length) == 0) {
      init_token(lexer, token, k->type);
      return;
    }
  }
  
  /* Check against instructions */
  for (const instruction_map_t* i = instructions; i->instruction != NULL; i++) {
    if (strlen(i->instruction) == length && 
        strncmp(start, i->instruction, length) == 0) {
      init_token(lexer, token, i->type);
      return;
    }
  }
  
  /* It's a regular identifier */
  init_token(lexer, token, TOKEN_IDENTIFIER);
}

/**
 * @brief Scan a number literal.
 * 
 * @param lexer The lexer.
 * @param token The token to fill.
 */
static void scan_number(lexer_t* lexer, token_t* token) {
  bool is_float = false;
  
  /* Consume digits before decimal point */
  while (isdigit(lexer_current_char(lexer))) {
    lexer_advance(lexer);
  }
  
  /* Check for decimal point */
  if (lexer_current_char(lexer) == '.' && 
      isdigit(lexer_peek_char(lexer))) {
    is_float = true;
    lexer_advance(lexer);
    
    /* Consume digits after decimal point */
    while (isdigit(lexer_current_char(lexer))) {
      lexer_advance(lexer);
    }
  }
  
  /* Check for scientific notation */
  if ((lexer_current_char(lexer) == 'e' || 
       lexer_current_char(lexer) == 'E')) {
    is_float = true;
    lexer_advance(lexer);
    
    /* Optional sign */
    if (lexer_current_char(lexer) == '+' || 
        lexer_current_char(lexer) == '-') {
      lexer_advance(lexer);
    }
    
    /* Must have at least one digit */
    if (!isdigit(lexer_current_char(lexer))) {
      /* Invalid scientific notation */
      init_token(lexer, token, TOKEN_ERROR);
      return;
    }
    
    /* Consume digits in exponent */
    while (isdigit(lexer_current_char(lexer))) {
      lexer_advance(lexer);
    }
  }
  
  /* Set token type based on whether it's an integer or float */
  if (is_float) {
    init_token(lexer, token, TOKEN_FLOAT);
    
    /* Convert string to double */
    char buffer[64];
    size_t len = lexer->position - lexer->token_start;
    
    if (len >= sizeof(buffer)) {
      /* Number too long */
      token->type = TOKEN_ERROR;
      return;
    }
    
    memcpy(buffer, lexer->source + lexer->token_start, len);
    buffer[len] = '\0';
    
    token->value.float_value = strtod(buffer, NULL);
  } else {
    init_token(lexer, token, TOKEN_INTEGER);
    
    /* Convert string to integer */
    char buffer[32];
    size_t len = lexer->position - lexer->token_start;
    
    if (len >= sizeof(buffer)) {
      /* Number too long */
      token->type = TOKEN_ERROR;
      return;
    }
    
    memcpy(buffer, lexer->source + lexer->token_start, len);
    buffer[len] = '\0';
    
    token->value.int_value = strtoll(buffer, NULL, 10);
  }
}

/**
 * @brief Scan a string literal.
 * 
 * @param lexer The lexer.
 * @param token The token to fill.
 */
static void scan_string(lexer_t* lexer, token_t* token) {
  /* Skip the opening quote */
  lexer_advance(lexer);
  
  /* Mark the real start after the quote */
  size_t content_start = lexer->position;
  
  /* Scan until closing quote or end of file */
  while (lexer_current_char(lexer) != '"' && 
         lexer_current_char(lexer) != '\0') {
    /* Handle escape sequences */
    if (lexer_current_char(lexer) == '\\') {
      lexer_advance(lexer);
      if (lexer_current_char(lexer) == '\0') {
        break;
      }
    }
    
    lexer_advance(lexer);
  }
  
  /* Check if we ended with a closing quote */
  if (lexer_current_char(lexer) != '"') {
    /* Unterminated string */
    init_token(lexer, token, TOKEN_ERROR);
    return;
  }
  
  /* Record the end before the closing quote */
  size_t content_end = lexer->position;
  
  /* Skip the closing quote */
  lexer_advance(lexer);
  
  /* Initialize the token */
  init_token(lexer, token, TOKEN_STRING);
  
  /* Adjust the start and length to exclude the quotes */
  token->start = lexer->source + content_start;
  token->length = content_end - content_start;
}

/**
 * @brief Scan the next token from the source.
 * 
 * @param lexer The lexer.
 * @param token The token to fill.
 */
static void scan_token(lexer_t* lexer, token_t* token) {
  /* Skip whitespace and comments */
  lexer_skip_whitespace_and_comments(lexer);
  
  /* Mark the start of the token */
  lexer->token_start = lexer->position;
  lexer->token_line = (int)lexer->line;
  lexer->token_column = (int)lexer->column;
  
  /* Check for end of file */
  if (lexer_current_char(lexer) == '\0') {
    init_token(lexer, token, TOKEN_EOF);
    return;
  }
  
  char c = lexer_current_char(lexer);
  lexer_advance(lexer);
  
  /* Identify token type */
  switch (c) {
    case '(': init_token(lexer, token, TOKEN_LPAREN); break;
    case ')': init_token(lexer, token, TOKEN_RPAREN); break;
    case '{': init_token(lexer, token, TOKEN_LBRACE); break;
    case '}': init_token(lexer, token, TOKEN_RBRACE); break;
    case '[': init_token(lexer, token, TOKEN_LBRACKET); break;
    case ']': init_token(lexer, token, TOKEN_RBRACKET); break;
    case ',': init_token(lexer, token, TOKEN_COMMA); break;
    case '.': init_token(lexer, token, TOKEN_DOT); break;
    case ';': init_token(lexer, token, TOKEN_SEMICOLON); break;
    case ':': init_token(lexer, token, TOKEN_COLON); break;
    case '=': init_token(lexer, token, TOKEN_EQUAL); break;
    case '<': init_token(lexer, token, TOKEN_LESS); break;
    case '>': init_token(lexer, token, TOKEN_GREATER); break;
    
    case '-':
      /* Check for arrow token "->" */
      if (lexer_current_char(lexer) == '>') {
        lexer_advance(lexer);
        init_token(lexer, token, TOKEN_ARROW);
      } else {
        /* Standalone minus or the start of a negative number */
        if (isdigit(lexer_current_char(lexer))) {
          scan_number(lexer, token);
        } else {
          init_token(lexer, token, TOKEN_ERROR);
        }
      }
      break;
    
    case '"':
      /* String literal */
      scan_string(lexer, token);
      break;
    
    default:
      /* Identifier, keyword, or number */
      if (isalpha(c) || c == '_') {
        /* Go back to include the first character */
        lexer->position--;
        lexer->column--;
        scan_identifier_or_keyword(lexer, token);
      } else if (isdigit(c)) {
        /* Go back to include the first digit */
        lexer->position--;
        lexer->column--;
        scan_number(lexer, token);
      } else {
        /* Unrecognized character */
        init_token(lexer, token, TOKEN_ERROR);
      }
      break;
  }
}

/* Public API implementation */

lexer_t* lexer_create(const char* source, size_t length) {
  if (source == NULL) {
    return NULL;
  }
  
  lexer_t* lexer = (lexer_t*)malloc(sizeof(lexer_t));
  if (lexer == NULL) {
    return NULL;
  }
  
  lexer->source = source;
  lexer->length = length;
  lexer->position = 0;
  lexer->line = 1;
  lexer->column = 1;
  lexer->token_start = 0;
  lexer->token_line = 1;
  lexer->token_column = 1;
  lexer->has_peeked = false;
  
  return lexer;
}

void lexer_destroy(lexer_t* lexer) {
  if (lexer != NULL) {
    free(lexer);
  }
}

bool lexer_next_token(lexer_t* lexer, token_t* token) {
  assert(lexer != NULL);
  assert(token != NULL);
  
  /* If we've peeked a token, return it */
  if (lexer->has_peeked) {
    *token = lexer->peeked_token;
    lexer->has_peeked = false;
    return token->type != TOKEN_ERROR && token->type != TOKEN_EOF;
  }
  
  /* Scan a new token */
  scan_token(lexer, token);
  
  return token->type != TOKEN_ERROR && token->type != TOKEN_EOF;
}

bool lexer_peek_token(lexer_t* lexer, token_t* token) {
  assert(lexer != NULL);
  assert(token != NULL);
  
  /* If we've already peeked, return the peeked token */
  if (lexer->has_peeked) {
    *token = lexer->peeked_token;
    return token->type != TOKEN_ERROR && token->type != TOKEN_EOF;
  }
  
  /* Scan a new token and store it */
  scan_token(lexer, &lexer->peeked_token);
  lexer->has_peeked = true;
  
  /* Copy the token to the output */
  *token = lexer->peeked_token;
  
  return token->type != TOKEN_ERROR && token->type != TOKEN_EOF;
}

const char* token_type_name(token_type_t type) {
  if (type >= 0 && type < TOKEN_COUNT) {
    return token_names[type];
  }
  return "UNKNOWN";
}

char* token_to_string(const token_t* token, char* buffer, size_t size) {
  assert(token != NULL);
  assert(buffer != NULL);
  assert(size > 0);
  
  if (token->length + 1 > size) {
    /* Buffer too small, truncate */
    size_t copy_length = size - 1;
    memcpy(buffer, token->start, copy_length);
    buffer[copy_length] = '\0';
  } else {
    /* Copy the token content */
    memcpy(buffer, token->start, token->length);
    buffer[token->length] = '\0';
  }
  
  return buffer;
}

bool token_is_type(token_type_t type) {
  return type >= TOKEN_VOID && type <= TOKEN_ARRAY;
}

bool token_is_instruction(token_type_t type) {
  return type >= TOKEN_ADD && type <= TOKEN_RET;
}