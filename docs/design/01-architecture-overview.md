# Quantum Language Architecture Overview

## 🏗️ System Architecture

Quantum Language is a dynamically-typed scripting language implemented in C++17 with a tree-walk interpreter architecture. The project follows a classic compiler/interpreter design pattern with clear separation of concerns.

## 📋 Core Components

### 1. Frontend (Compilation Pipeline)
```
Source Code (.sa) → Lexer → Parser → AST → TypeChecker → Interpreter
```

#### **Lexer** (`src/Lexer.cpp`, `include/Lexer.h`)
- **Purpose**: Tokenizes source code into lexical tokens
- **Input**: Raw source code strings
- **Output**: Stream of tokens for the parser
- **Key Features**:
  - Multi-syntax support (Python, JavaScript, C++ style)
  - String interpolation handling
  - Comment stripping
  - Unicode support

#### **Parser** (`src/Parser.cpp`, `include/Parser.h`)
- **Purpose**: Parses token stream into Abstract Syntax Tree (AST)
- **Algorithm**: Recursive Descent Parser
- **Grammar Support**:
  - Expressions (binary, unary, ternary)
  - Statements (if, while, for, functions, classes)
  - Declarations (variables, functions, classes)
  - Advanced features (closures, comprehensions, slices)

#### **TypeChecker** (`src/TypeChecker.cpp`, `include/TypeChecker.h`)
- **Purpose**: Performs semantic analysis and type checking
- **Algorithm**: Static analysis with environment-based scoping
- **Key Features**:
  - Dynamic type validation with optional type hints
  - Variable scope management with lexical environments
  - Built-in function registration
  - Type mismatch detection and warning system
  - Function signature validation

### Type System Design
```cpp
// Type Environment Chain
struct TypeEnv {
    std::map<std::string, std::string> vars;  // Variable → Type mapping
    std::shared_ptr<TypeEnv> parent;          // Parent environment for scoping
    
    void define(const std::string& name, const std::string& type);
    std::string resolve(const std::string& name);  // Look up in chain
};

// Type Checking Process
std::string checkNode(const ASTNodePtr& node, std::shared_ptr<TypeEnv> env);
```

### Supported Types
- **Primitive Types**: `float`, `string`, `bool`, `any`
- **Complex Types**: `fn` (function), `void` (no return value)
- **Type Hints**: Optional static type annotations
- **Built-in Types**: Pre-registered global functions and types

### Type Checking Features
1. **Variable Declaration Checking**
   ```cpp
   // Type hint vs initializer compatibility
   if (!vd.typeHint.empty() && vd.typeHint != "any" && 
       initType != "any" && vd.typeHint != initType) {
       // Emit type mismatch warning
   }
   ```

2. **Function Signature Validation**
   ```cpp
   // Parameter type checking in nested environments
   auto subEnv = std::make_shared<TypeEnv>(env);
   for (size_t i = 0; i < fd.params.size(); ++i) {
       std::string pType = fd.paramTypes[i].empty() ? "any" : fd.paramTypes[i];
       subEnv->define(fd.params[i], pType);
   }
   ```

3. **Expression Type Inference**
   ```cpp
   // Binary expression type inference
   if (be.op == "+" || be.op == "-" || be.op == "*" || be.op == "/") 
       return "float";  // Arithmetic operations
   if (be.op == "==" || be.op == "!=" || be.op == "<" || be.op == ">") 
       return "bool";   // Comparison operations
   ```

4. **Environment Chain Resolution**
   ```cpp
   // Lexical scoping with parent chain lookup
   std::string resolve(const std::string& name) {
       if (vars.count(name)) return vars[name];      // Current scope
       if (parent) return parent->resolve(name);  // Parent scopes
       return "any";                                 // Global fallback
   }
   ```

### Error Handling
- **StaticTypeError**: Type-related errors with line information
- **Warning System**: Non-fatal type mismatches with warnings
- **Graceful Degradation**: Continue checking after type errors
- **User-friendly Messages**: Clear type mismatch descriptions

### Built-in Function Registration
```cpp
// Pre-registered global functions
globalEnv->define("print", "any");     // Accepts any types
globalEnv->define("input", "string");   // Returns string
globalEnv->define("len", "int");       // Returns integer
globalEnv->define("sha256", "string");  // Cryptographic function
```

### Type Safety Features
- **Optional Static Typing**: Type hints for better error detection
- **Runtime Flexibility**: Dynamic typing with static validation
- **Early Error Detection**: Catch type errors before execution
- **IDE Support**: Type information for language services

### TypeChecker Folder Structure
```
src/
├── TypeChecker.cpp              # Type checker implementation
│   ├── TypeChecker()            # Constructor with built-in registration
│   ├── check(nodes)             # Check multiple AST nodes
│   ├── check(node)              # Check single AST node
│   └── checkNode(node, env)     # Core type checking logic
│       ├── Literal handling     # Number, String, Bool literals
│       ├── Variable checking    # Identifier resolution
│       ├── Declaration checking # Variable declarations with hints
│       ├── Function validation  # Parameter and return types
│       ├── Block scoping        # Environment chain management
│       ├── Expression inference # Binary/Unary expression types
│       └── Call validation      # Function call type checking
│
include/
├── TypeChecker.h                # Type checker interface
│   ├── StaticTypeError          # Type error exception class
│   ├── TypeEnv                  # Type environment structure
│   │   ├── vars                 # Variable → Type mapping
│   │   ├── parent               # Parent environment link
│   │   ├── define()             # Add variable to environment
│   │   └── resolve()            # Look up variable in chain
│   └── TypeChecker              # Main type checker class
│       ├── globalEnv            # Global type environment
│       ├── check()              # Public interface methods
│       └── checkNode()          # Core checking algorithm
```

### 2. Backend (Runtime)

#### **Interpreter** (`src/Interpreter.cpp`, `include/Interpreter.h`)
- **Architecture**: Tree-walk interpreter
- **Execution Model**: Visit pattern over AST nodes
- **Key Components**:
  - Environment management (scopes, closures)
  - Function call handling
  - Built-in function registration
  - Method dispatch system

#### **Value System** (`src/Value.cpp`, `include/Value.h`)
- **Type System**: Dynamic typing with variant-based storage
- **Supported Types**:
  - Primitive: `nil`, `bool`, `number`, `string`
  - Composite: `array`, `dict` (object)
  - Functional: `function`, `native` (built-in)
  - OOP: `class`, `instance`
  - Advanced: `pointer` (C++ style)

### 3. AST Structure (`include/AST.h`)
- **Design**: Variant-based node types
- **Expression Nodes**:
  - Literals (number, string, bool, nil)
  - Variables and identifiers
  - Operations (binary, unary, assign)
  - Function calls and lambdas
  - Array/dict literals and comprehensions
  - Member access and indexing
  - Slicing (Python-style)
- **Statement Nodes**:
  - Variable declarations
  - Control flow (if, while, for)
  - Function and class declarations
  - Return statements
  - Import statements
  - Print and input statements

## 🔄 Data Flow

### Compilation Flow
1. **Source Reading**: File contents loaded into memory
2. **Lexical Analysis**: Source → Token stream
3. **Syntax Analysis**: Tokens → AST
4. **Semantic Analysis**: AST validation and type checking
5. **Execution**: Tree-walk interpretation

### Runtime Flow
1. **Environment Setup**: Global scope creation with built-ins
2. **AST Traversal**: Depth-first execution
3. **Value Evaluation**: Expression computation
4. **Side Effects**: Variable assignment, I/O operations
5. **Result Propagation**: Return values and error handling

## 🏛️ Design Patterns

### 1. Visitor Pattern
- **Used In**: Interpreter for AST traversal
- **Benefit**: Clean separation of node types and operations

### 2. Variant Pattern (Type-safe Union)
- **Used In**: AST nodes and Value types
- **Benefit**: Type safety without inheritance hierarchies

### 3. Environment Chain
- **Used In**: Variable scoping and closures
- **Pattern**: Linked list of symbol tables
- **Features**: Lexical scoping, closure capture

### 4. Factory Pattern
- **Used In**: Value creation and AST node construction
- **Benefit**: Centralized object creation logic

## 🔧 Memory Management

### Smart Pointers
- **`std::unique_ptr`**: AST node ownership
- **`std::shared_ptr`**: Shared resources (functions, environments)
- **RAII**: Automatic resource cleanup

### Garbage Collection
- **Strategy**: Reference counting via shared_ptr
- **Circular References**: Handled by weak references where needed
- **Memory Safety**: No manual memory management required

## 🚀 Performance Considerations

### Optimizations
1. **Release Build**: `-O2` optimization flags
2. **Inline Functions**: Critical path optimizations
3. **String Interning**: Shared string storage
4. **Function Caching**: Native function lookup optimization

### Trade-offs
- **Interpreted vs Compiled**: Simplicity over raw speed
- **Dynamic Typing**: Flexibility over static optimization
- **Tree-walk vs Bytecode**: Simplicity over execution speed

## 🛡️ Error Handling

### Exception Strategy
- **Compile-time**: ParseError with position information
- **Runtime**: RuntimeError with stack traces
- **Type Errors**: TypeError with context

### Error Recovery
- **Graceful Degradation**: Continue parsing after errors
- **Multiple Error Reporting**: Collect multiple errors before failing
- **User-friendly Messages**: Clear error descriptions with line numbers

## 🔌 Extensibility

### Built-in Function System
- **Registration**: Runtime function registration
- **Native Functions**: C++ integration points
- **Method Dispatch**: Object-oriented method calls

### Module System
- **Import Statements**: File-based module loading
- **Namespace Isolation**: Separate environments per module
- **Circular Import Detection**: Prevent infinite recursion

## 📊 Architecture Metrics

- **Core Modules**: 7 source files
- **Lines of Code**: ~50,000+ (including examples)
- **AST Node Types**: 30+ expression/statement types
- **Value Types**: 11 distinct runtime types
- **Built-in Functions**: 100+ native functions

---

*This architecture provides a solid foundation for a scripting language while maintaining simplicity and extensibility.*
