# CVM++ — Custom Virtual Machine + Compiler

CVM++ is a small, self-contained scripting language implemented in C++17.
Source code is tokenized, parsed into an abstract syntax tree, compiled into
custom bytecode, and executed on a stack-based virtual machine. The whole
toolchain is in this repository — no external dependencies beyond the C++
standard library.

## Architecture

```
   source.cvm
       │
       ▼
   ┌─────────┐      tokens      ┌─────────┐       AST        ┌──────────┐
   │  Lexer  │ ───────────────▶ │ Parser  │ ───────────────▶ │ Compiler │
   └─────────┘                  └─────────┘                  └────┬─────┘
                                                                  │ bytecode
                                                                  ▼
                                                              ┌───────┐
                                                              │  VM   │ ─▶ stdout
                                                              └───────┘
```

Each stage lives in its own pair of files:

| Stage    | Header              | Source                |
|----------|---------------------|-----------------------|
| Tokens   | `include/token.h`   | (header-only enum)    |
| Lexer    | `include/lexer.h`   | `src/lexer.cpp`       |
| AST      | `include/ast.h`     | `src/ast.cpp`         |
| Parser   | `include/parser.h`  | `src/parser.cpp`      |
| Bytecode | `include/bytecode.h`| `src/bytecode.cpp`    |
| Compiler | `include/compiler.h`| `src/compiler.cpp`    |
| VM       | `include/vm.h`      | `src/vm.cpp`          |
| REPL     | `include/repl.h`    | `src/repl.cpp`        |
| CLI      |                     | `src/main.cpp`        |

## Language

### Types
- **Integer** — 64-bit signed
- **Boolean** — `true` / `false`

### Statements

```text
let x = 10;            // variable declaration
x = x + 1;             // assignment
print(x);              // print expression to stdout
input(x);              // read one integer line from stdin into x

if (cond) { ... }      // conditional
if (cond) { ... } else { ... }

while (cond) { ... }   // loop
```

### Operators

| Category    | Operators                         |
|-------------|-----------------------------------|
| Arithmetic  | `+` `-` `*` `/`                   |
| Comparison  | `==` `!=` `<` `>` `<=` `>=`       |
| Unary       | `-` (negation) `!` (logical not)  |
| Assignment  | `=`                               |

Integer division truncates toward zero. Comparisons require matching types.
Logical `!` requires a boolean operand.

### Grammar (BNF)

```text
program        → statement* EOF
statement      → varDecl | printStmt | inputStmt | ifStmt | whileStmt | block | exprStmt
varDecl        → "let" IDENTIFIER ("=" expression)? ";"
printStmt      → "print" "(" expression ")" ";"
inputStmt      → "input" "(" IDENTIFIER ")" ";"
ifStmt         → "if" "(" expression ")" statement ("else" statement)?
whileStmt      → "while" "(" expression ")" statement
block          → "{" statement* "}"
exprStmt       → expression ";"

expression     → assignment
assignment     → IDENTIFIER "=" assignment | equality
equality       → comparison ( ("==" | "!=") comparison )*
comparison     → term ( ("<" | ">" | "<=" | ">=") term )*
term           → factor ( ("+" | "-") factor )*
factor         → unary  ( ("*" | "/") unary  )*
unary          → ("-" | "!") unary | primary
primary        → NUMBER | "true" | "false" | IDENTIFIER | "(" expression ")"
```

## Bytecode

Instructions are 1-byte opcodes. Some take a 16-bit little-endian operand
(constant index, variable slot, or absolute jump target into the code array).

| Opcode             | Operand   | Effect                                              |
|--------------------|-----------|-----------------------------------------------------|
| `HALT`             |           | stop execution                                      |
| `LOAD_CONST`       | u16 idx   | push `constants[idx]`                               |
| `LOAD_VAR`         | u16 idx   | push `variables[idx]`                               |
| `STORE_VAR`        | u16 idx   | `variables[idx] = pop()`                            |
| `POP`              |           | discard top of stack                                |
| `ADD/SUB/MUL/DIV`  |           | int arithmetic (pops two, pushes one)               |
| `NEGATE`           |           | int negation                                        |
| `NOT`              |           | boolean negation                                    |
| `CMP_EQ` / `CMP_NEQ` |         | type-aware equality, pushes bool                    |
| `CMP_LT/GT/LE/GE`  |           | int comparison, pushes bool                         |
| `JUMP`             | u16 tgt   | `ip = tgt`                                          |
| `JUMP_IF_FALSE`    | u16 tgt   | if `!pop()` then `ip = tgt`                         |
| `PRINT`            |           | write `pop()` + newline to stdout                   |
| `INPUT`            | u16 idx   | read line from stdin, parse int into `variables[idx]` |

## Building

### CMake (recommended)

```bash
mkdir build && cd build
cmake ..
cmake --build .
ctest --output-on-failure
```

This produces `build/cvm` (the CLI), plus three test binaries.

### Manual g++

From the project root:

```bash
g++ -std=c++17 -O2 -Wall -Wextra -Iinclude \
    src/lexer.cpp src/parser.cpp src/ast.cpp src/bytecode.cpp \
    src/compiler.cpp src/vm.cpp src/repl.cpp src/main.cpp \
    -o cvm
```

## Running

### Execute a script

```bash
./cvm scripts/variables.cvm
```

Expected output:

```
10
20
30
55
11
```

### REPL

```bash
./cvm
```

```
CVM++ interactive shell. Type 'exit' to quit, 'run <file>' to load a script.
>>> let x = 41;
>>> print(x + 1);
42
>>> run scripts/loops.cvm
5
4
3
2
1
55
>>> exit
```

Note: each REPL submission compiles and executes as its own program — there
is no variable persistence between submissions yet. Use a script file or
`run <file>` for multi-statement programs that share state.

### Calculator example with piped input

```bash
printf "5\n7\n" | ./cvm scripts/calculator.cvm
```

Output:

```
12
-2
35
```

## Debug flags

| Flag         | What it dumps to stderr                                  |
|--------------|----------------------------------------------------------|
| `--tokens`   | the token stream emitted by the lexer                    |
| `--ast`      | the parsed AST, indented                                  |
| `--bytecode` | a disassembly of the compiled chunk                       |
| `--trace`    | each VM instruction as it executes, with the stack state  |

Combine freely:

```bash
./cvm --bytecode --trace scripts/loops.cvm
```

## VS Code setup

A minimal `.vscode/c_cpp_properties.json` works out of the box:

```json
{
  "configurations": [{
    "name": "cvm",
    "includePath": [ "${workspaceFolder}/include" ],
    "cStandard": "c11",
    "cppStandard": "c++17"
  }],
  "version": 4
}
```

Build with the CMake Tools extension (Ctrl-Shift-P → "CMake: Configure"),
then run / debug the `cvm` target.

## Project layout

```
CVMPlusPlus/
├── include/        # public headers, one per stage
├── src/            # implementations
├── scripts/        # sample .cvm programs
├── tests/          # self-contained test binaries (no framework)
├── CMakeLists.txt
└── README.md
```

## Future extensions

The current language is intentionally minimal. Natural next steps:

- **Functions**: add `fn name(args) { ... }`, a `CALL`/`RETURN` opcode pair,
  and a call frame stack.
- **Lexical scopes**: track per-block variable maps in the Compiler instead
  of a single flat table.
- **Strings**: extend the `Value` tagged union and add string concatenation.
- **Arrays / maps**: introduce reference types and an object heap.
- **`for` loops**: desugar to `while` in the parser.
- **REPL state persistence**: keep a long-lived Chunk + variable table across
  inputs.
- **Constant folding / peephole optimization**: optimize the chunk after
  compilation.

## License

This is a teaching project. Use it however you like.
