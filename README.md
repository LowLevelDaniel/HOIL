# HOILC: HOIL to COIL Compiler

HOILC is a compiler that translates the Human Oriented Intermediate Language (HOIL) to the Compiler Optimization Intermediate Language (COIL) binary format. It's designed to be safe, efficient, and follow high-quality coding standards.

## Overview

HOIL is a textual representation of COIL, providing a human-readable syntax while maintaining a direct 1:1 mapping to the COIL binary format. HOILC enables:

- Converting HOIL source code to COIL binary format
- Performing thorough type checking
- Optimizing code
- Generating efficient binary output

## Building the Compiler

HOILC uses the Meson build system:

```bash
# Clone the repository
git clone https://github.com/hoilc/hoilc.git
cd hoilc

# Set up the build directory
meson setup build
cd build

# Build the compiler
ninja

# Run tests
ninja test
```

## Usage

```bash
# Basic usage
hoilc -o output.coil input.hoil

# Verbose output
hoilc -v -o output.coil input.hoil

# Display version information
hoilc --version

# Display help
hoilc --help

# Dump COIL binary contents for inspection
coil_dump output.coil
```

## HOIL Language Features

HOIL supports:

- Module declarations
- Type definitions
- Constants
- Global variables
- Functions with multiple basic blocks
- Control flow using branches
- Simple instructions
- External function declarations

Here's a simple example of a HOIL program:

```
MODULE "example";

// Calculate the greatest common divisor
FUNCTION gcd(a: i32, b: i32) -> i32 {
    ENTRY:
        is_b_zero = CMP_EQ b, 0;
        BR is_b_zero, DONE, LOOP;
        
    LOOP:
        remainder = REM a, b;
        a = b;
        b = remainder;
        is_b_zero = CMP_EQ b, 0;
        BR is_b_zero, DONE, LOOP;
        
    DONE:
        RET a;
}

// Main function
FUNCTION main() -> i32 {
    ENTRY:
        result = CALL gcd(48, 18);
        RET result;
}
```

## Code Structure

The compiler is organized into several components:

- **Lexer**: Tokenizes the source code
- **Parser**: Builds an Abstract Syntax Tree (AST)
- **Type Checker**: Validates types and expressions
- **Code Generator**: Translates the AST to COIL binary format
- **Symbol Table**: Manages identifiers and their types
- **Error Handler**: Provides detailed error messages
- **Binary Format Handler**: Manages COIL binary format generation
- **Utilities**: Common functions for file I/O, memory management, etc.
- **Tools**: Additional utilities like `coil_dump` for inspecting COIL binaries

## Design Principles

HOILC is built with the following principles:

- **Memory Safety**: Complete bounds checking and careful memory management
- **Zero Dependencies**: Self-contained implementation without external libraries
- **Clear Error Messages**: Detailed error messages with source locations
- **Clean Code**: Follows NASA/Google C style guidelines with Doxygen documentation
- **Modular Design**: Well-separated components with clear interfaces

## License

HOILC is released under the MIT License. See the LICENSE file for details.

## Contributing

Contributions are welcome! Please see CONTRIBUTING.md for details on how to contribute to the project.