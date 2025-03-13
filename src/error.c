/**
 * @file error.c
 * @brief Implementation of error handling for HOILC.
 * 
 * This file contains the implementation of the error handling system.
 * 
 * @author HOILC Team
 * @date 2025
 */

#include "../include/error.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

/**
 * @brief Error context structure implementation.
 */
struct error_context {
  bool has_error;                      /**< Whether an error has occurred. */
  hoilc_result_t result;               /**< Error result code. */
  char message[ERROR_MESSAGE_MAX];     /**< Error message. */
  bool has_location;                   /**< Whether the error has a location. */
  int line;                            /**< Error line number. */
  int column;                          /**< Error column number. */
  const char* filename;                /**< Error filename. */
};

error_context_t* error_create_context(void) {
  error_context_t* context = (error_context_t*)malloc(sizeof(error_context_t));
  if (context == NULL) {
    return NULL;
  }
  
  context->has_error = false;
  context->result = HOILC_SUCCESS;
  context->message[0] = '\0';
  context->has_location = false;
  context->line = 0;
  context->column = 0;
  context->filename = NULL;
  
  return context;
}

void error_destroy_context(error_context_t* context) {
  if (context != NULL) {
    free(context);
  }
}

/**
 * @brief Format an error message with variable arguments.
 * 
 * @param context The error context.
 * @param format The error message format.
 * @param args The variable arguments.
 */
static void error_format_message(error_context_t* context, const char* format, va_list args) {
  assert(context != NULL);
  assert(format != NULL);
  
  vsnprintf(context->message, ERROR_MESSAGE_MAX, format, args);
  
  /* Ensure null-termination */
  context->message[ERROR_MESSAGE_MAX - 1] = '\0';
}

void error_report_at(error_context_t* context, hoilc_result_t result,
                     const source_location_t* location, const char* format, ...) {
  assert(context != NULL);
  assert(format != NULL);
  
  /* Only record the first error */
  if (context->has_error) {
    return;
  }
  
  context->has_error = true;
  context->result = result;
  
  /* Format the message */
  va_list args;
  va_start(args, format);
  error_format_message(context, format, args);
  va_end(args);
  
  /* Set the location */
  if (location != NULL) {
    context->has_location = true;
    context->line = location->line;
    context->column = location->column;
    context->filename = location->filename;
  } else {
    context->has_location = false;
  }
}

void error_report_at_node(error_context_t* context, hoilc_result_t result,
                          const ast_node_t* node, const char* format, ...) {
  assert(context != NULL);
  assert(format != NULL);
  
  /* Only record the first error */
  if (context->has_error) {
    return;
  }
  
  context->has_error = true;
  context->result = result;
  
  /* Format the message */
  va_list args;
  va_start(args, format);
  error_format_message(context, format, args);
  va_end(args);
  
  /* Set the location from the node */
  if (node != NULL) {
    context->has_location = true;
    context->line = node->location.line;
    context->column = node->location.column;
    context->filename = node->location.filename;
  } else {
    context->has_location = false;
  }
}

void error_report(error_context_t* context, hoilc_result_t result,
                  const char* format, ...) {
  assert(context != NULL);
  assert(format != NULL);
  
  /* Only record the first error */
  if (context->has_error) {
    return;
  }
  
  context->has_error = true;
  context->result = result;
  context->has_location = false;
  
  /* Format the message */
  va_list args;
  va_start(args, format);
  error_format_message(context, format, args);
  va_end(args);
}

bool error_occurred(const error_context_t* context) {
  assert(context != NULL);
  
  return context->has_error;
}

hoilc_result_t error_get_result(const error_context_t* context) {
  assert(context != NULL);
  
  return context->result;
}

const char* error_get_message(const error_context_t* context) {
  assert(context != NULL);
  
  if (!context->has_error) {
    return NULL;
  }
  
  return context->message;
}

bool error_get_location(const error_context_t* context, int* line, int* column,
                        const char** filename) {
  assert(context != NULL);
  
  if (!context->has_error || !context->has_location) {
    return false;
  }
  
  if (line != NULL) {
    *line = context->line;
  }
  
  if (column != NULL) {
    *column = context->column;
  }
  
  if (filename != NULL) {
    *filename = context->filename;
  }
  
  return true;
}

void error_clear(error_context_t* context) {
  assert(context != NULL);
  
  context->has_error = false;
  context->result = HOILC_SUCCESS;
  context->message[0] = '\0';
  context->has_location = false;
  context->line = 0;
  context->column = 0;
  context->filename = NULL;
}