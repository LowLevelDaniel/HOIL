# HOIL/COIL Language System

This project implements a simple language ecosystem with:

- **HOIL** (Human Oriented Intermediate Language): A high-level assembly-like language
- **COIL** (Computer Oriented Intermediate Language): A lower-level intermediate language
- **HOIL-to-COIL converter**: Translates HOIL code to COIL
- **COIL interpreter**: Executes COIL instructions

## Project Structure

```
hoil-coil/
├── include/
│   └── hoil_coil_common.h  # Common definitions
├── src/
│   ├── hoil_to_coil.c      # HOIL to COIL converter
│   └── coil_interpreter.c  # COIL interpreter
├── test/
│   ├── example.hoil        # Example HOIL program
│   └── example.coil        # Generated COIL code
├── meson.build            # Meson build configuration
└── README.md              # This file
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
./builddir/hoil_to_coil input.hoil output.coil
```

### Running COIL code

```bash
./builddir/coil_interpreter input.coil
```

### Complete Example

```bash
# Convert example HOIL to COIL
./builddir/hoil_to_coil test/example.hoil test/example.coil

# Run the COIL code
./builddir/coil_interpreter test/example.coil
```

## HOIL Language Reference

HOIL is an assembly-like language with the following features:
- Line-based instructions (one instruction per line)
- Comments starting with semicolon (`;`)
- Symbolic operation names for readability
- Comma-separated arguments
- Types (e.g., `dint` for default integer size)
- Variable references by identifier

### HOIL Instructions

```
VAL DEFV <type>, <id>, <value>      # Define a value with specified type
VAL MOVV <type>, <dest_id>, <src_id> # Define a value from source
MATH ADD <dest_id>, <src_id1>, <src_id2> # Add values
MATH SUB <dest_id>, <src_id1>, <src_id2> # Subtract values
MATH MUL <dest_id>, <src_id1>, <src_id2> # Multiply values
MATH DIV <dest_id>, <src_id1>, <src_id2> # Divide values
CF SYSC <syscall_num>, <args...>     # System call
```

### HOIL Data Types

- `dint`: Default integer size (typically 64-bit)
- `int8`: 8-bit integer
- `int16`: 16-bit integer
- `int32`: 32-bit integer

### HOIL Intrinsics

- `SIZE(<id>)`: Get the size of the variable at the given identifier

## COIL Language Reference

COIL is a lower-level language with numeric operation codes and explicit memory management. Each COIL instruction consists of an operation code followed by arguments.

### COIL Format

```
<op_code> <arg1> <arg2> ... <argN>
```

### COIL Operation Codes

- `0001`: Allocate memory with immediate value
- `0002`: Allocate memory with value from another address
- `0101`: Add values
- `0102`: Subtract values
- `0103`: Multiply values
- `0104`: Divide values
- `0201`: System call

## Docker Support

You can build and run the project using Docker:

```bash
# Build the Docker image
docker build -t hoil-coil .

# Run the Docker container
docker run -it hoil-coil

# Inside the container, you can run:
./builddir/hoil_to_coil examples/example.hoil examples/example.coil
./builddir/coil_interpreter examples/example.coil
```

## License

[MIT License](LICENSE)