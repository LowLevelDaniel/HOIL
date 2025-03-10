# HOIL/COIL Language System

This project implements a programming language ecosystem centered around:

- **HOIL (Human Oriented Intermediate Language)**: A high-level assembly-like language designed for readability
- **COIL (Computer Oriented Intermediate Language)**: A standardized low-level intermediate language
- **VM**: A virtual machine that executes COIL instructions

## Project Architecture

The project is designed with modularity in mind:

1. **HOIL Compiler**: Translates HOIL code to COIL format
2. **COIL Format**: A standardized binary format that can be implemented by different tools
3. **COIL VM**: Executes COIL instructions in a virtual environment
4. **COIL Debugger**: Helps debug COIL programs
5. **COIL Utilities**: Tools for working with COIL binary and text formats

## Project Structure

```
hoil-coil/
├── include/
│   ├── coil_format.h      # COIL-specific definitions
│   ├── hoil_format.h      # HOIL-specific definitions
│   └── coil_vm.h          # VM-specific declarations
├── src/
│   ├── hoil_to_coil.c     # HOIL to COIL converter
│   ├── coil_vm.c          # VM implementation
│   ├── coil_debugger.c    # Debugger for COIL
│   └── binary_coil_utilities.c  # Utilities for binary COIL
├── test/
│   ├── factorial.hoil     # Factorial example
│   └── fibonacci.hoil     # Fibonacci example
├── meson.build           # Build configuration
└── README.md             # This file
```

## Building

This project uses the Meson build system.

```bash
# Generate build configuration
meson setup builddir

# Build the project
meson compile -C builddir

# Run tests
meson test -C builddir
```

## Usage

### Converting HOIL to COIL

```bash
# Text output (for debugging)
./builddir/hoil_to_coil input.hoil output.coil

# Binary output (for execution)
./builddir/hoil_to_coil -b input.hoil output.coil
```

### Running COIL code

```bash
# Run a COIL binary file
./builddir/coil_vm -b input.coil

# Show statistics after execution
./builddir/coil_vm -b -s input.coil
```

### Converting between binary and text COIL

```bash
# Convert text COIL to binary
./builddir/coil_binary to-binary input.coil output.bin

# Convert binary COIL to text
./builddir/coil_binary to-text input.bin output.coil
```

### Debugging COIL code

```bash
# Debug a binary COIL file
./builddir/coil_debugger -b input.coil
```

## HOIL Language Reference

HOIL is a higher-level assembly language with the following features:

- Line-based instructions (one instruction per line)
- Comments starting with semicolon (`;`)
- Symbolic operation names for readability
- Comma-separated arguments
- Types (e.g., `dint` for default integer size)
- Variable references by identifier

### HOIL Categories and Instructions

```
VAL DEFV <type>, <id>, <value>      # Define a value with immediate value
VAL MOVV <type>, <dest_id>, <src_id> # Move value from source to destination
VAL LOAD <type>, <dest_id>, <addr_id> # Load from memory address
VAL STORE <type>, <addr_id>, <src_id> # Store to memory address

MATH ADD <dest_id>, <src_id1>, <src_id2> # Add values
MATH SUB <dest_id>, <src_id1>, <src_id2> # Subtract values
MATH MUL <dest_id>, <src_id1>, <src_id2> # Multiply values
MATH DIV <dest_id>, <src_id1>, <src_id2> # Divide values
MATH MOD <dest_id>, <src_id1>, <src_id2> # Modulo operation
MATH NEG <dest_id>, <src_id>             # Negate value

BIT AND <dest_id>, <src_id1>, <src_id2>  # Bitwise AND
BIT OR <dest_id>, <src_id1>, <src_id2>   # Bitwise OR
BIT XOR <dest_id>, <src_id1>, <src_id2>  # Bitwise XOR
BIT NOT <dest_id>, <src_id>              # Bitwise NOT
BIT SHL <dest_id>, <src_id>, <shift>     # Shift left
BIT SHR <dest_id>, <src_id>, <shift>     # Shift right

CF JMP <label>                  # Unconditional jump
CF JCOND <cond>, <id1>, <id2>, <label> # Conditional jump (cond: EQ, NE, LT, LE, GT, GE)
CF LABEL <label>                # Define a label
CF CALL <function>              # Call function
CF RET                          # Return from function
CF PUSH <id>                    # Push value onto stack
CF POP <id>                     # Pop value from stack
CF SYSC <syscall_num>, <args...> # System call
CF EXIT <status>                # Exit program
```

### HOIL Data Types

- `bool`: Boolean (1 bit stored in 1 byte)
- `int8`: 8-bit signed integer
- `int16`: 16-bit signed integer
- `int32`: 32-bit signed integer
- `dint`: Default integer (64-bit)
- `uint8`: 8-bit unsigned integer
- `uint16`: 16-bit unsigned integer
- `uint32`: 32-bit unsigned integer
- `uint64`: 64-bit unsigned integer
- `float32`: 32-bit floating point
- `float64`: 64-bit floating point
- `ptr`: Pointer type

## COIL Language Reference

COIL is a lower-level language with numeric operation codes and explicit memory management. Each COIL instruction consists of an operation code followed by arguments.

### COIL Format

The COIL format is designed to be stable and portable, allowing developers to build their own COIL assemblers and interpreters following the standard.

The binary format of each instruction consists of:
- Start marker (1 byte)
- Operation code (2 bytes)
- Type marker (1 byte)
- Type code (1 byte)
- Variable marker (1 byte)
- Variable address (2 bytes)
- Immediate marker (1 byte)
- Immediate value (8 bytes)
- End marker (1 byte)

### COIL Operation Codes

Operation codes are organized by category:

- Memory operations (0x00xx): `OP_ALLOC_IMM`, `OP_ALLOC_MEM`, etc.
- Arithmetic operations (0x01xx): `OP_ADD`, `OP_SUB`, etc.
- Bitwise operations (0x02xx): `OP_AND`, `OP_OR`, etc.
- Control flow (0x03xx): `OP_JMP`, `OP_JEQ`, etc.
- Function operations (0x04xx): `OP_CALL`, `OP_RET`, etc.
- System operations (0x05xx): `OP_SYSCALL`, `OP_EXIT`, etc.

## Docker Support

You can build and run the project using Docker:

```bash
# Build the Docker image
docker build -t hoil-coil .

# Run the Docker container
docker run -it hoil-coil

# Inside the container, you can run:
./builddir/hoil_to_coil -b test/factorial.hoil factorial.coil
./builddir/coil_vm -b factorial.coil
```

## License

This is free and unencumbered software released into the public domain.