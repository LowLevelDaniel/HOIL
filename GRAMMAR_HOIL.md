# HOIL Grammar Specification

HOIL (Human Oriented Intermediate Language) is a high-level assembly-like language designed as an intermediate representation that is human-readable while providing low-level access to system resources.

## Lexical Elements

### Comments
Comments begin with a semicolon (`;`) and continue to the end of the line:
```
; This is a comment
```

### Tokens
- **Keywords**: Operation prefixes and directives (e.g., `VAL`, `MATH`, `BIT`, `CF`)
- **Identifiers**: Symbol and label names (alphanumeric with underscores)
- **Literals**: Numeric values or string literals
- **Operators**: Mathematical and logical operators
- **Separators**: Commas, spaces, and line breaks

### Literals
- **Integer literals**: `123`, `-456`
- **Identifier literals**: `id5` (represents the value 5)
- **Character literals**: `id'A` (represents the ASCII value of 'A')
- **String literals**: Not directly supported; strings are constructed character by character

## Syntax

### Program Structure
A HOIL program consists of a sequence of instructions, each on a separate line:
```
<instruction> [<arguments>...] [; comment]
```

### Instruction Format
```
<category> <operation> [<arguments>...]
```

Where:
- `<category>` is one of the instruction categories (VAL, MATH, BIT, CF)
- `<operation>` is a specific operation within that category
- `<arguments>` are the operands for the operation

## Instruction Categories

### 1. Value Operations (VAL)
Used for memory allocation, value movement, and memory access.

#### Operations:
- `DEFV <type>, <symbol>, <value>`: Define a variable with an initial value
- `MOVV <type>, <dest>, <source>`: Move a value from source to destination
- `LOAD <type>, <dest>, <address>`: Load a value from memory address
- `STORE <type>, <address>, <source>`: Store a value to memory address

#### Example:
```
VAL DEFV dint, counter, id0      ; Define integer 'counter' with initial value 0
VAL MOVV dint, result, counter   ; Copy counter value to result
```

### 2. Mathematical Operations (MATH)
Perform arithmetic operations on values.

#### Operations:
- `ADD <dest>, <src1>, <src2>`: Addition
- `SUB <dest>, <src1>, <src2>`: Subtraction
- `MUL <dest>, <src1>, <src2>`: Multiplication
- `DIV <dest>, <src1>, <src2>`: Division
- `MOD <dest>, <src1>, <src2>`: Modulo
- `NEG <dest>, <src>`: Negation

#### Example:
```
MATH ADD sum, a, b      ; sum = a + b
MATH NEG negative, sum  ; negative = -sum
```

### 3. Bitwise Operations (BIT)
Perform bitwise operations on values.

#### Operations:
- `AND <dest>, <src1>, <src2>`: Bitwise AND
- `OR <dest>, <src1>, <src2>`: Bitwise OR
- `XOR <dest>, <src1>, <src2>`: Bitwise XOR
- `NOT <dest>, <src>`: Bitwise NOT
- `SHL <dest>, <src>, <shift>`: Shift left
- `SHR <dest>, <src>, <shift>`: Shift right

#### Example:
```
BIT AND result, flags, mask     ; result = flags & mask
BIT SHL value, value, 3         ; value = value << 3
```

### 4. Control Flow Operations (CF)
Control program execution flow.

#### Operations:
- `JMP <label>`: Unconditional jump
- `JCOND <condition>, <src1>, <src2>, <label>`: Conditional jump
- `LABEL <name>`: Define a label
- `CALL <function>`: Call a function
- `RET`: Return from function
- `PUSH <value>`: Push value onto stack
- `POP <variable>`: Pop value from stack
- `SYSC <number>, [args...]`: System call
- `EXIT <status>`: Exit program

#### Conditions:
- `EQ`: Equal
- `NE`: Not equal
- `LT`: Less than
- `LE`: Less or equal
- `GT`: Greater than
- `GE`: Greater or equal

#### Example:
```
CF LABEL loop_start
  MATH ADD counter, counter, one
  CF JCOND LT, counter, max, loop_start
CF SYSC 1, 1, &message, 10    ; Write 10 bytes from message to stdout
CF EXIT 0                      ; Exit with status 0
```

## Data Types

- `dint`: 64-bit signed integer (default)
- `int8`: 8-bit signed integer
- `int16`: 16-bit signed integer
- `int32`: 32-bit signed integer
- `uint8`: 8-bit unsigned integer
- `uint16`: 16-bit unsigned integer
- `uint32`: 32-bit unsigned integer
- `uint64`: 64-bit unsigned integer
- `float32`: 32-bit floating point
- `float64`: 64-bit floating point
- `bool`: Boolean (1 byte)
- `ptr`: Pointer type

## Symbol References

- Direct reference: `variable_name`
- Address reference: `&variable_name`
- Immediate value: `id123` (numeric literal)
- Character value: `id'A` (ASCII value of character)
- Special functions:
  - `SIZE(variable)`: Size of the variable
  - `SIZEOF(type)`: Size of the type

## Examples

### Hello World:
```
; Define message characters
VAL DEFV int8, msg1, id'H
VAL DEFV int8, msg2, id'e
VAL DEFV int8, msg3, id'l
VAL DEFV int8, msg4, id'l
VAL DEFV int8, msg5, id'o
VAL DEFV int8, msg6, id','
VAL DEFV int8, msg7, id' '
VAL DEFV int8, msg8, id'w
VAL DEFV int8, msg9, id'o
VAL DEFV int8, msg10, id'r
VAL DEFV int8, msg11, id'l
VAL DEFV int8, msg12, id'd
VAL DEFV int8, msg13, id'!
VAL DEFV int8, newline, id10

; Print each character
CF SYSC 1, 1, &msg1, 1
CF SYSC 1, 1, &msg2, 1
; ... (additional characters)
CF SYSC 1, 1, &msg13, 1
CF SYSC 1, 1, &newline, 1

; Exit program
CF SYSC 60, 0
```

### Simple Loop:
```
; Initialize variables
VAL DEFV dint, i, id0       ; Loop counter
VAL DEFV dint, max, id10    ; Loop maximum
VAL DEFV dint, one, id1     ; Constant 1

; Main loop
CF LABEL loop_start
  ; Check if i >= max
  MATH SUB temp, i, max
  CF JCOND GE, temp, one, loop_end
  
  ; Body of loop
  ; ... (operations)
  
  ; Increment counter
  MATH ADD i, i, one
  
  ; Loop back
  CF JMP loop_start

CF LABEL loop_end
CF EXIT 0
```