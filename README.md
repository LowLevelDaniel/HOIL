# HOIL-COIL Intermediate Language System

## Overview

HOIL-COIL is a two-tier intermediate language system designed for cross-platform, cross-architecture code execution:

- **HOIL** (Human Oriented Intermediate Language): A higher-level assembly-like language designed for human readability while maintaining low-level control.
- **COIL** (Computer Oriented Intermediate Language): A binary intermediate representation focused on space efficiency and optimizability, serving as a compilation target.

## Architecture

```
┌───────────┐     ┌──────────────┐     ┌──────────────┐     ┌─────────────┐
│ HOIL Code │ ──▶ │ HOIL-to-COIL │ ──▶ │ COIL Binary  │ ──▶ │ COIL VM     │
└───────────┘     └──────────────┘     └──────────────┘     └─────────────┘
                                                │                 │
                                                │                 │
                                                ▼                 ▼
                                         ┌──────────────┐  ┌─────────────┐
                                         │ Architecture- │  │ Execution   │
                                         │ specific code │  │ Results     │
                                         └──────────────┘  └─────────────┘
```

### Key Features

- **Cross-processing unit compatibility**: COIL can be compiled for various processing units (CPU, GPU, TPU, NPU, FPGA, DSP, ASIC, MCU)
- **Cross-architecture design**: Works across different architectures (x86, ARM, etc.)
- **Space-efficient opcodes**: Optimized for minimal binary size
- **Hardware-specific optimizations**: Leverages native processor capabilities when available
- **Emulation fallbacks**: Provides software implementations when hardware features are unavailable

## Components

### 1. HOIL-to-COIL Converter

Translates human-readable HOIL code into COIL binary format:
- Parses HOIL instructions
- Manages memory allocation
- Resolves symbols and labels
- Generates optimized COIL binary output

### 2. COIL Virtual Machine

Executes COIL binary instructions:
- Simulates a virtual processor
- Manages memory and stack
- Handles control flow
- Implements system calls
- Provides detailed execution statistics

## Value Types

COIL operates on four primary value types:

1. **Coil Variable**: An alias for a value in memory or register, with lifecycle management
2. **Coil Symbol**: Holds a program address for code references
3. **Coil Immediate**: Constant value stored directly in the instruction
4. **Coil State**: Captures execution state for operations like stack manipulation

## Memory Model

COIL supports various memory types:
- 8/16/32/64-bit integers (signed/unsigned)
- 32/64-bit floating point
- Boolean
- Pointer types

## Building the Project

The project uses the Meson build system:

```bash
# Clone the repository
git clone https://github.com/yourusername/hoil-coil.git
cd hoil-coil

# Setup build directory
meson setup build
cd build

# Build the project
meson compile

# Run the tests
meson test
```

## Docker Support

A Dockerfile is provided for containerized development and testing:

```bash
# Build the Docker image
docker build -t hoil-coil .

# Run the container
docker run -it hoil-coil
```

## Usage Examples

### Converting HOIL to COIL

```bash
./hoil_to_coil -b input.hoil output.coil
```

Options:
- `-b`: Generate binary output (default is text format)

### Running COIL Code

```bash
./coil_vm -b -s program.coil
```

Options:
- `-b`: Binary input mode (default is text mode)
- `-s`: Show execution statistics

## Example Programs

The `test` directory contains sample HOIL programs:
- `factorial.hoil`: Calculates factorial of a number
- `fibonacci.hoil`: Generates Fibonacci sequence

## License

This project is released into the public domain under the [Unlicense](https://unlicense.org/).