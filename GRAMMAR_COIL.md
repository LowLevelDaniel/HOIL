# COIL Grammar Specification

COIL (Computer Oriented Intermediate Language) is a binary intermediate language designed for cross-processing unit and cross-architecture compatibility. It focuses on space efficiency and optimization capabilities.

## Binary Format

COIL instructions are encoded in a binary format optimized for size and processing efficiency. Each instruction follows this structure:

```
┌───────────────┬─────────┬─────────────┬───────┬────────────────┬───────────┬──────────────┬────────────┐
│ Start Marker  │ Op Code │ Type Marker │ Type  │ Variable Marker│ Address   │ Imm Marker   │ Imm Value  │ End Marker │
│ (1 byte)      │(2 bytes)│ (1 byte)    │(1 byte)│ (1 byte)      │ (2 bytes) │ (1 byte)     │ (8 bytes)  │ (1 byte)  │
└───────────────┴─────────┴─────────────┴───────┴────────────────┴───────────┴──────────────┴────────────┘
```

### Markers

- **Start Marker** (`0xC0`): Indicates the beginning of an instruction
- **Type Marker** (`0xC3`): Precedes the memory type code
- **Variable Marker** (`0xC1`): Precedes the variable address
- **Immediate Marker** (`0xC2`): Precedes the immediate value
- **End Marker** (`0xCF`): Indicates the end of an instruction

## Instruction Categories

COIL instructions are organized into categories based on their function:

### Memory Operations (0x00xx)

| Opcode    | Mnemonic  | Description                                 |
|-----------|-----------|---------------------------------------------|
| `0x0001`  | ALLOC_IMM | Allocate memory with immediate value        |
| `0x0002`  | ALLOC_MEM | Allocate memory with value from address     |
| `0x0003`  | MOVE      | Move value from source to destination       |
| `0x0004`  | LOAD      | Load value from memory address              |
| `0x0005`  | STORE     | Store value to memory address               |

### Arithmetic Operations (0x01xx)

| Opcode    | Mnemonic  | Description                                 |
|-----------|-----------|---------------------------------------------|
| `0x0101`  | ADD       | Add values                                  |
| `0x0102`  | SUB       | Subtract values                             |
| `0x0103`  | MUL       | Multiply values                             |
| `0x0104`  | DIV       | Divide values                               |
| `0x0105`  | MOD       | Modulo operation                            |
| `0x0106`  | NEG       | Negate value                                |

### Bitwise Operations (0x02xx)

| Opcode    | Mnemonic  | Description                                 |
|-----------|-----------|---------------------------------------------|
| `0x0201`  | AND       | Bitwise AND                                 |
| `0x0202`  | OR        | Bitwise OR                                  |
| `0x0203`  | XOR       | Bitwise XOR                                 |
| `0x0204`  | NOT       | Bitwise NOT                                 |
| `0x0205`  | SHL       | Shift left                                  |
| `0x0206`  | SHR       | Shift right                                 |

### Control Flow (0x03xx)

| Opcode    | Mnemonic  | Description                                 |
|-----------|-----------|---------------------------------------------|
| `0x0301`  | JMP       | Unconditional jump                          |
| `0x0302`  | JEQ       | Jump if equal                               |
| `0x0303`  | JNE       | Jump if not equal                           |
| `0x0304`  | JLT       | Jump if less than                           |
| `0x0305`  | JLE       | Jump if less or equal                       |
| `0x0306`  | JGT       | Jump if greater than                        |
| `0x0307`  | JGE       | Jump if greater or equal                    |

### Function Operations (0x04xx)

| Opcode    | Mnemonic  | Description                                 |
|-----------|-----------|---------------------------------------------|
| `0x0401`  | CALL      | Call function                               |
| `0x0402`  | RET       | Return from function                        |
| `0x0403`  | PUSH      | Push value onto stack                       |
| `0x0404`  | POP       | Pop value from stack                        |

### System Operations (0x05xx)

| Opcode    | Mnemonic  | Description                                 |
|-----------|-----------|---------------------------------------------|
| `0x0501`  | SYSCALL   | System call                                 |
| `0x0502`  | EXIT      | Exit program                                |

### Special Codes

| Opcode    | Mnemonic  | Description                                 |
|-----------|-----------|---------------------------------------------|
| `0xFFFE`  | LABEL_DEF | Label definition (not an actual instruction)|
| `0xFFFF`  | ARG_DATA  | Argument data (used with other instructions)|

## Memory Types

COIL supports various memory types, each with a specific type code:

| Type Code | Name        | Description                               | Size (bytes) |
|-----------|-------------|-------------------------------------------|--------------|
| `0x01`    | MEM_INT8    | 8-bit signed integer                      | 1            |
| `0x02`    | MEM_INT16   | 16-bit signed integer                     | 2            |
| `0x04`    | MEM_INT32   | 32-bit signed integer                     | 4            |
| `0x08`    | MEM_INT64   | 64-bit signed integer (default)           | 8            |
| `0x11`    | MEM_UINT8   | 8-bit unsigned integer                    | 1            |
| `0x12`    | MEM_UINT16  | 16-bit unsigned integer                   | 2            |
| `0x14`    | MEM_UINT32  | 32-bit unsigned integer                   | 4            |
| `0x18`    | MEM_UINT64  | 64-bit unsigned integer                   | 8            |
| `0x24`    | MEM_FLOAT32 | 32-bit floating point                     | 4            |
| `0x28`    | MEM_FLOAT64 | 64-bit floating point                     | 8            |
| `0x40`    | MEM_PTR     | Pointer type                              | 8            |
| `0x81`    | MEM_BOOL    | Boolean type                              | 1            |

## Value Types

COIL operates with four primary value types:

1. **Coil Variable**: An alias for a value in memory or register that can be marked for deletion
   - Represented by a memory address in the variable address field
   - Has an associated memory type

2. **Coil Symbol**: Holds a program address for code references
   - Used with instructions like JMP, CALL
   - Referenced by a label ID

3. **Coil Immediate**: Constant value stored directly in the instruction
   - Stored in the immediate value field (8 bytes)
   - Can represent integers, floats, or packed data depending on context

4. **Coil State**: Represents execution state for special operations
   - Used with state-preserving operations like PUSH/POP
   - Maintains stack or register state information

## Memory Model

COIL uses a flat memory model with a 16-bit address space (65536 bytes). The memory space is divided into:

- **Static Memory**: For global variables and constants
- **Stack**: For local variables and function calls
- **Call Stack**: For function return addresses

## Execution Flow

The COIL VM executes instructions in sequence, with control flow instructions altering the execution path:

1. **Sequential Execution**: Instructions are executed in order
2. **Jumps**: Unconditional (JMP) or conditional (JEQ, JNE, etc.) jumps
3. **Function Calls**: CALL pushes return address and jumps to function
4. **Returns**: RET pops return address and jumps back
5. **Exit**: EXIT terminates program execution with status code

## System Calls

COIL supports system calls using the SYSCALL instruction. Common system calls include:

- **1**: Write to file descriptor (e.g., stdout)
- **60**: Exit program

System call arguments are provided through the ARG_DATA instruction which follows the SYSCALL instruction.

## Planned Directives

Future versions of COIL will support directives for:

- **Section Definition**: Control memory layout
- **Includes**: Import code from other files
- **Libraries**: Link with external libraries
- **Raw Data**: Insert binary data directly
- **Alignment**: Control data alignment

## Binary to Text Format

For debugging and analysis, COIL binary can be converted to a text representation:

```
<opcode> <type> <address> <immediate>
```

Example:
```
0x0001 0x08 0x0000 0x0000000000000005
```

Which represents:
- Opcode: ALLOC_IMM (0x0001)
- Type: MEM_INT64 (0x08)
- Address: 0x0000
- Immediate value: 5