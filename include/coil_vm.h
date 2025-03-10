/**
* @file coil_vm.h
* @brief Definition of the COIL Virtual Machine
*
* This file contains declarations for the COIL Virtual Machine (VM), which
* executes compiled COIL instructions.
*/

#ifndef COIL_VM_H
#define COIL_VM_H

#include <stdio.h>
#include <stdbool.h>
#include "coil_format.h"

/**
* @brief Maximum memory size in bytes
*/
#define STATIC_MEMORY_SIZE 65536

/**
* @brief Maximum stack size in bytes
*/
#define STACK_SIZE 4096

/**
* @brief Maximum call stack size (for function returns)
*/
#define CALL_STACK_SIZE 256

/**
* @brief Maximum number of labels
*/
#define MAX_LABELS 256

/**
* @brief Label entry structure
*/
typedef struct {
  uint16_t label_id;         /**< Label identifier */
  long file_position;        /**< File position for this label */
} label_entry_t;

/**
* @brief VM state structure
*/
typedef struct {
  uint8_t memory[STATIC_MEMORY_SIZE];   /**< Static memory space */
  size_t memory_used;                   /**< Amount of memory used */
  
  uint8_t stack[STACK_SIZE];            /**< Data stack */
  size_t stack_used;                    /**< Amount of stack used */
  
  long call_stack[CALL_STACK_SIZE];     /**< Call stack for function returns */
  size_t call_stack_used;               /**< Amount of call stack used */
  
  label_entry_t labels[MAX_LABELS];     /**< Label table */
  size_t label_count;                   /**< Number of labels defined */
  
  FILE *input_file;                     /**< Input file for instructions */
  bool binary_mode;                     /**< Flag for binary input format */
  
  uint64_t instruction_count;           /**< Number of instructions executed */
  bool running;                         /**< Flag to control execution */
  
  int exit_code;                        /**< Exit code when program terminates */
} vm_state_t;

/**
* @brief Initialize the VM state
* 
* @param state Pointer to the VM state
* @param input_file Input file for instructions
* @param binary_mode Flag for binary input format
* @return 0 on success, non-zero on failure
*/
int initialize_vm(vm_state_t *state, FILE *input_file, bool binary_mode);

/**
* @brief Run the virtual machine
* 
* @param state Pointer to the VM state
* @return Exit code from the program
*/
int run_vm(vm_state_t *state);

/**
* @brief Print VM statistics
* 
* @param state Pointer to the VM state
*/
void print_statistics(vm_state_t *state);

/**
* @brief Get a pointer to a memory address
* 
* @param state Pointer to the VM state
* @param addr Memory address
* @return Pointer to the memory location, or NULL if invalid
*/
void* get_memory_ptr(vm_state_t *state, uint16_t addr);

/**
* @brief Push a value onto the stack
* 
* @param state Pointer to the VM state
* @param value Value to push
* @param size Size of the value in bytes
* @return 0 on success, non-zero on failure
*/
int stack_push(vm_state_t *state, const void *value, size_t size);

/**
* @brief Pop a value from the stack
* 
* @param state Pointer to the VM state
* @param value Pointer to store the popped value
* @param size Size of the value in bytes
* @return 0 on success, non-zero on failure
*/
int stack_pop(vm_state_t *state, void *value, size_t size);

#endif /* COIL_VM_H */