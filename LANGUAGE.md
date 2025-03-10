# HOIL Language Specification

HOIL (Human Oriented Intermediate Language) is a high-level assembly-like language designed for readability while maintaining translation capability to the lower-level COIL format.

## Syntax

- One instruction per line
- Comments start with semicolon (`;`)
- Instructions consist of categories and operations, followed by comma-separated arguments
- Case-sensitive
- Types explicitly specified

## Data Types

| Type Name | Description |
|-----------|-------------|
| `bool`    | Boolean (1 bit~ (optimized type stored as int8 for the most part but COIL assemblers / interpreters can pack booleans into the same int8 at different offsets if wanted)) |
| `int8`    | 8-bit signed integer |
| `int16`   | 16-bit signed integer |
| `int32`   | 32-bit signed integer |
| `dint`    | Default integer (64-bit) |
| `uint8`   | 8-bit unsigned integer |
| `uint16`  | 16-bit unsigned integer |
| `uint32`  | 32-bit unsigned integer |
| `uint64`  | 64-bit unsigned integer |
| `float32` | 32-bit floating point |
| `float64` | 64-bit floating point |
| `ptr`     | Pointer type |

## Instructions

### Value Operations

```
VAL DEFV <type>, <id>, <value>      # Define a value with immediate value
VAL MOVV <type>, <dest_id>, <src_id> # Move value from source to destination
VAL LOAD <type>, <dest_id>, <addr_id> # Load from memory address
VAL STORE <type>, <addr_id>, <src_id> # Store to memory address
```

### Arithmetic Operations

```
MATH ADD <dest_id>, <src_id1>, <src_id2> # Add values
MATH SUB <dest_id>, <src_id1>, <src_id2> # Subtract values
MATH MUL <dest_id>, <src_id1>, <src_id2> # Multiply values
MATH DIV <dest_id>, <src_id1>, <src_id2> # Divide values
MATH MOD <dest_id>, <src_id1>, <src_id2> # Modulo operation
MATH NEG <dest_id>, <src_id>             # Negate value
```

### Bitwise Operations

```
BIT AND <dest_id>, <src_id1>, <src_id2>  # Bitwise AND
BIT OR <dest_id>, <src_id1>, <src_id2>   # Bitwise OR
BIT XOR <dest_id>, <src_id1>, <src_id2>  # Bitwise XOR
BIT NOT <dest_id>, <src_id>              # Bitwise NOT
BIT SHL <dest_id>, <src_id>, <shift>     # Shift left
BIT SHR <dest_id>, <src_id>, <shift>     # Shift right
```

### Control Flow

```
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

### Memory Management

```
MEM ALLOC <id>, <size>          # Allocate memory
MEM FREE <id>                   # Free memory
MEM COPY <dest_id>, <src_id>, <size> # Copy memory
```

## Special Identifiers and Functions

- `id<num>` - Refers to a value (e.g., `id10` for the value 10)
- `&<id>` - Refers to the address of an identifier
- `SIZE(<id>)` - Returns the size in bytes of the data at the given identifier
- `SIZEOF(<type>)` - Returns the size in bytes of the specified type

## Example Program

```
; Calculate factorial of 5
VAL DEFV dint, 0, id5       ; n = 5
VAL DEFV dint, 1, id1       ; result = 1
VAL DEFV dint, 2, id1       ; i = 1

CF LABEL loop
CF JCOND GT, 0, 2, end      ; if n <= i, goto end

MATH MUL 1, 1, 2            ; result = result * i
MATH ADD 2, 2, id1          ; i = i + 1
CF JMP loop                  ; goto loop

CF LABEL end
CF SYSC 1, 1, &1, SIZE(1)   ; print result
CF EXIT 0                    ; exit program
```