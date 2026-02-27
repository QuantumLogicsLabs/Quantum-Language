# Quantum Language Architecture

## Overview

Quantum Language is a modern, cybersecurity-focused scripting language implemented as a tree-walk interpreter in C++17. The language processes `.sa` (Quantum Script) files through a classic compilation pipeline: **Lexical Analysis → Parsing → Abstract Syntax Tree Generation → Interpretation**.

This document provides a comprehensive overview of how `.sa` files are compiled and executed, detailing the tokenizer, lexer, parser, and interpreter components.

## File Extension and Entry Point

- **File Extension**: `.sa` (Quantum Script)
- **Entry Point**: `src/main.cpp`
- **Language Version**: 1.0.0
- **Runtime**: Tree-walk interpreter

## Compilation Pipeline

The compilation process follows these distinct phases:

```
.sa Source File → Lexer → Token Stream → Parser → AST → Interpreter → Execution
```

### 1. Main Entry Point (`src/main.cpp`)

The `main.cpp` file serves as the orchestrator for the entire compilation process:

```cpp
// Key compilation flow in runFile():
Lexer lexer(source);           // Phase 1: Lexical analysis
auto tokens = lexer.tokenize(); // Generate token stream

Parser parser(std::move(tokens)); // Phase 2: Parsing
auto ast = parser.parse();        // Generate AST

Interpreter interp;               // Phase 3: Interpretation
interp.execute(*ast);             // Execute AST
```

The main function supports three modes:
- **REPL Mode**: Interactive parsing and execution line-by-line
- **File Mode**: Complete `.sa` file compilation and execution
- **Check Mode**: Syntax validation only (parses without execution)

## Phase 1: Lexical Analysis (Tokenizer/Lexer)

### Lexer Class (`src/Lexer.cpp`, `include/Lexer.h`)

The Lexer transforms raw source code into a stream of tokens. It operates character by character, recognizing patterns and categorizing them into lexical units.

#### Core Components

**State Management:**
```cpp
std::string src;  // Source code
size_t pos;       // Current position
int line, col;    // Line and column tracking
```

**Token Recognition Methods:**
- `readNumber()`: Handles decimal and hexadecimal numbers
- `readString()`: Processes string literals with escape sequences
- `readIdentifierOrKeyword()`: Distinguishes identifiers from keywords

#### Token Types (`include/Token.h`)

The lexer recognizes 50+ token types organized into categories:

**Literals:**
- `NUMBER`, `STRING`, `BOOL_TRUE`, `BOOL_FALSE`, `NIL`

**Keywords:**
- **Control Flow**: `let`, `const`, `fn`, `return`, `if`, `else`, `elif`, `while`, `for`, `in`, `break`, `continue`
- **I/O**: `print`, `input`
- **Logic**: `and`, `or`, `not`
- **Modules**: `import`
- **Cybersecurity (reserved)**: `scan`, `payload`, `encrypt`, `decrypt`, `hash`

**Operators:**
- **Arithmetic**: `+`, `-`, `*`, `/`, `%`, `**` (power)
- **Comparison**: `==`, `!=`, `<`, `>`, `<=`, `>=`
- **Assignment**: `=`, `+=`, `-=`, `*=`, `/=`
- **Bitwise**: `&`, `|`, `^`, `~`, `<<`, `>>`

**Delimiters:**
- `(`, `)`, `{`, `}`, `[`, `]`, `,`, `;`, `:`, `.`, `->`

#### Lexing Process

1. **Whitespace Handling**: Skips spaces and tabs, tracks newlines
2. **Comment Processing**: Skips `#` single-line comments
3. **Number Recognition**: 
   - Decimal numbers: `123`, `45.67`
   - Hexadecimal numbers: `0xFF`, `0x1A2B`
4. **String Processing**: Supports both single `'` and double `"` quotes with escape sequences
5. **Keyword/Identifier Distinction**: Uses hash map for O(1) keyword lookup
6. **Operator Parsing**: Handles multi-character operators (`==`, `!=`, `**`, `+=`, etc.)

#### Example Tokenization

For the input: `let x = 42 + "hello"`

Generated tokens:
```
LET(let)     IDENTIFIER(x)    ASSIGN(=)    NUMBER(42)    PLUS(+)    STRING(hello)
```

## Phase 2: Parsing (Syntax Analysis)

### Parser Class (`src/Parser.cpp`, `include/Parser.h`)

The Parser transforms the token stream into a structured Abstract Syntax Tree (AST) using a combination of **Recursive Descent** for statements and **Pratt Parsing** for expressions.

#### AST Structure (`include/AST.h`)

The AST uses a variant-based design with these node types:

**Expression Nodes:**
- `NumberLiteral`, `StringLiteral`, `BoolLiteral`, `NilLiteral`
- `Identifier` - Variable references
- `BinaryExpr` - Binary operations (`+`, `-`, `*`, `/`, etc.)
- `UnaryExpr` - Unary operations (`-`, `not`, `~`)
- `AssignExpr` - Assignment operations (`=`, `+=`, etc.)
- `CallExpr` - Function calls
- `IndexExpr` - Array/dict indexing `arr[i]`
- `MemberExpr` - Member access `obj.prop`
- `ArrayLiteral`, `DictLiteral` - Data structure literals
- `LambdaExpr` - Anonymous functions

**Statement Nodes:**
- `VarDecl` - Variable declarations (`let`, `const`)
- `FunctionDecl` - Function definitions
- `ReturnStmt` - Return statements
- `IfStmt` - Conditional statements with elif chains
- `WhileStmt` - While loops
- `ForStmt` - For-in loops
- `BlockStmt` - Code blocks
- `ExprStmt` - Expression statements
- `PrintStmt` - Print statements
- `ImportStmt` - Module imports
- `BreakStmt`, `ContinueStmt` - Loop control

#### Parsing Strategy

**Statement Parsing (Recursive Descent):**
```cpp
ASTNodePtr parseStatement() {
    switch (current().type) {
        case TokenType::LET:    return parseVarDecl(false);
        case TokenType::FN:     return parseFunctionDecl();
        case TokenType::IF:     return parseIfStmt();
        // ... other statement types
        default: return parseExprStmt();
    }
}
```

**Expression Parsing (Pratt Precedence):**
The parser uses operator precedence climbing with these levels (highest to lowest):
1. **Primary**: Literals, identifiers, parentheses
2. **Postfix**: Function calls, indexing, member access
3. **Unary**: `-`, `not`, `~`
4. **Power**: `**` (right-associative)
5. **Multiplication**: `*`, `/`, `%`
6. **Addition**: `+`, `-`
7. **Shift**: `<<`, `>>`
8. **Comparison**: `<`, `>`, `<=`, `>=`
9. **Equality**: `==`, `!=`
10. **Bitwise AND**: `&`
11. **Bitwise XOR**: `^`
12. **Bitwise OR**: `|`
13. **Logical AND**: `and`
14. **Logical OR**: `or`
15. **Assignment**: `=`, `+=`, `-=`, `*=`, `/=`

#### Example AST Generation

For the code: `let result = x * (y + 2)`

Generated AST structure:
```
BlockStmt
└── VarDecl (name: "result", isConst: false)
    └── AssignExpr (op: "=")
        ├── Identifier ("result")
        └── BinaryExpr (op: "*")
            ├── Identifier ("x")
            └── BinaryExpr (op: "+")
                ├── Identifier ("y")
                └── NumberLiteral (2.0)
```

## Phase 3: Interpretation (Execution)

### Interpreter Class (`src/Interpreter.cpp`)

The Interpreter executes the AST using a tree-walk approach with lexical scoping and dynamic typing.

#### Value System (`QuantumValue`)

The language uses a dynamic type system with these value types:
- **Number**: Double-precision floating point
- **String**: Text data
- **Boolean**: `true`/`false`
- **Nil**: Null value
- **Function**: User-defined and native functions
- **Array**: Ordered collections
- **Dictionary**: Key-value mappings

#### Environment and Scoping

```cpp
class Environment {
    std::shared_ptr<Environment> enclosing;  // Parent scope
    std::unordered_map<std::string, QuantumValue> values;
};
```

- **Lexical Scoping**: Variables resolved through parent environments
- **Function Closures**: Functions capture their defining environment
- **Global Scope**: Built-in functions and global variables

#### Execution Model

**Statement Execution:**
- **Variable Declaration**: Creates bindings in current environment
- **Function Declaration**: Creates function objects with closure
- **Control Flow**: Implements if/elif/else, while, for loops
- **Return**: Propagates return values up call stack

**Expression Evaluation:**
- **Literals**: Direct value creation
- **Variables**: Environment lookup
- **Binary Operations**: Type-aware arithmetic, comparison, logical operations
- **Function Calls**: Argument evaluation, environment creation, execution
- **Member Access**: Object property resolution

#### Built-in Functions

The interpreter registers native functions at startup:

**Type Conversion:**
- `num(value)`: Convert to number
- `str(value)`: Convert to string  
- `bool(value)`: Convert to boolean

**Mathematical:**
- `abs()`, `sqrt()`, `floor()`, `ceil()`, `round()`
- `pow()`, `log()`, `log2()`, `sin()`, `cos()`, `tan()`
- `min()`, `max()`

**I/O:**
- `__input__()`: Read user input (internal)
- `print()`: Output values

## Example Compilation Walkthrough

Let's trace the compilation of a simple `.sa` file:

**Source Code (`example.sa`):**
```sa
# Calculate factorial
fn factorial(n) {
    if n <= 1 {
        return 1
    }
    return n * factorial(n - 1)
}

let result = factorial(5)
print("Factorial of 5 is:", result)
```

### Step 1: Lexical Analysis

Generated token stream (simplified):
```
FN(fn) IDENTIFIER(factorial) LPAREN(()) IDENTIFIER(n) RPAREN()) LBRACE({) 
IF(if) IDENTIFIER(n) LTE(<=) NUMBER(1) LBRACE({) RETURN(return) NUMBER(1) 
RBRACE(}) RETURN(return) IDENTIFIER(n) STAR(*) IDENTIFIER(factorial) 
LPAREN(()) IDENTIFIER(n) MINUS(-) NUMBER(1) RPAREN()) RBRACE(}) 
LET(let) IDENTIFIER(result) ASSIGN(=) IDENTIFIER(factorial) LPAREN(()) 
NUMBER(5) RPAREN()) PRINT(print) LPAREN(()) STRING(Factorial of 5 is:) 
COMMA(,) IDENTIFIER(result) RPAREN())
```

### Step 2: Parsing

Generated AST structure:
```
BlockStmt
├── FunctionDecl (name: "factorial", params: ["n"])
│   └── BlockStmt
│       ├── IfStmt
│       │   ├── BinaryExpr (op: "<=")
│       │   │   ├── Identifier ("n")
│       │   │   └── NumberLiteral (1.0)
│       │   └── BlockStmt
│       │       └── ReturnStmt
│       │           └── NumberLiteral (1.0)
│       └── ReturnStmt
│           └── BinaryExpr (op: "*")
│               ├── Identifier ("n")
│               └── CallExpr
│                   ├── Identifier ("factorial")
│                   └── BinaryExpr (op: "-")
│                       ├── Identifier ("n")
│                       └── NumberLiteral (1.0)
├── VarDecl (name: "result", isConst: false)
│   └── AssignExpr (op: "=")
│       ├── Identifier ("result")
│       └── CallExpr
│           ├── Identifier ("factorial")
│           └── NumberLiteral (5.0)
└── PrintStmt
    ├── StringLiteral ("Factorial of 5 is:")
    └── Identifier ("result")
```

### Step 3: Interpretation

1. **Function Declaration**: Creates `factorial` function with closure
2. **Variable Declaration**: Evaluates `factorial(5)` call
   - Calls factorial with n=5
   - Recursive calls: factorial(4), factorial(3), factorial(2), factorial(1)
   - Returns 120
3. **Assignment**: Stores 120 in `result` variable
4. **Print**: Outputs "Factorial of 5 is: 120"

## Error Handling

The language implements comprehensive error handling at each phase:

### Lexical Errors
- **Unexpected characters**: Invalid symbols
- **Unterminated strings**: Missing closing quotes
- **Invalid numbers**: Malformed numeric literals

### Parse Errors
- **Syntax errors**: Unexpected tokens
- **Missing delimiters**: Unclosed brackets, parentheses
- **Invalid constructs**: Malformed statements

### Runtime Errors
- **Type errors**: Invalid operations on types
- **Name errors**: Undefined variables/functions
- **Runtime exceptions**: Division by zero, index out of bounds

## Performance Characteristics

### Time Complexity
- **Lexing**: O(n) - Linear scan of source
- **Parsing**: O(n) - Single pass with backtracking limited
- **Interpretation**: O(n × m) where n is AST size, m is operations per node

### Space Complexity
- **Tokens**: O(n) - Proportional to source size
- **AST**: O(n) - Tree structure proportional to tokens
- **Environments**: O(s × d) where s is scope size, d is call depth

## Cybersecurity Features

The language includes reserved keywords for future cybersecurity functionality:

- `scan`: Network/port scanning capabilities
- `payload`: Exploit payload creation
- `encrypt`/`decrypt`: Cryptographic operations
- `hash`: Hashing functions

These keywords are lexically recognized but not yet implemented, providing a foundation for security-focused language extensions.

## Build System

The project uses CMake for cross-platform compilation:

```cmake
# Key components
src/Lexer.cpp      # Lexical analysis
src/Parser.cpp     # Syntax analysis  
src/Interpreter.cpp # Execution engine
src/main.cpp       # Entry point
include/           # Header files
examples/          # Sample .sa files
```

## Conclusion

Quantum Language implements a clean, modular architecture following established compiler design principles. The separation of concerns between lexical analysis, parsing, and interpretation provides a solid foundation for language evolution and the addition of cybersecurity features. The use of modern C++ features ensures efficient memory management and type safety throughout the compilation pipeline.

The `.sa` file compilation process demonstrates how a simple scripting language can be implemented while maintaining extensibility for future security-focused enhancements.
