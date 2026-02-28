# AST.h - Abstract Syntax Tree Node Definitions

## Overview
`AST.h` defines the complete Abstract Syntax Tree (AST) structure for the Quantum Language compiler. It contains all node types that represent different language constructs, using modern C++ `std::variant` to implement a type-safe, memory-efficient tree structure that serves as the intermediate representation between parsing and execution.

## Architecture Overview

The AST system provides:
- **Complete Language Coverage**: Every language construct has corresponding AST node
- **Type Safety**: Variant-based nodes with compile-time type checking
- **Memory Efficiency**: Smart pointers and variant storage minimize overhead
- **Extensibility**: Easy to add new node types for language evolution
- **Position Tracking**: Source location information for error reporting

## Line-by-Line Analysis

### Header Guard and Dependencies (Lines 1-5)
```cpp
#pragma once
#include <memory>
#include <string>
#include <vector>
#include <variant>
```

**Dependency Analysis:**
- **`memory`**: Smart pointers for automatic memory management
- **`string`**: String literals and identifiers
- **`vector`**: Collections for statements, expressions, parameters
- **`variant`**: Type-safe union for heterogeneous node storage

### Forward Declarations (Lines 7-9)
```cpp
// Forward declarations
struct ASTNode;
using ASTNodePtr = std::unique_ptr<ASTNode>;
```

**Forward Declaration Benefits:**
- **Break Circular Dependencies**: Prevents include loops
- **Type Alias**: `ASTNodePtr` provides convenient smart pointer type
- **Interface Clarity**: Clean separation between interface and implementation

### Expression Type Definitions (Lines 11-108)

#### Literal Types (Lines 13-27)
```cpp
// ─── Expression Types ───────────────────────────────────────────────────────

struct NumberLiteral
{
    double value;
};

struct StringLiteral
{
    std::string value;
};

struct BoolLiteral
{
    bool value;
};

struct NilLiteral
{
};
```

**Literal Analysis:**

**NumberLiteral (Lines 13-16):**
- **`value`**: Double-precision floating point for all numeric values
- **Unified Storage**: Integers and floats stored as double
- **Precision**: Sufficient for most scripting language needs

**StringLiteral (Lines 18-21):**
- **`value`**: String content without quotes
- **Unescaped**: Stores processed string with escape sequences resolved
- **Memory Efficient**: Uses std::string with small string optimization

**BoolLiteral (Lines 23-26):**
- **`value`**: Boolean true/false value
- **Compact**: Minimal memory footprint
- **Clear Semantics**: Unambiguous boolean representation

**NilLiteral (Lines 28-30):**
- **Empty Struct**: No data needed for nil representation
- **Type Safety**: Distinct from other literal types
- **Memory Efficient**: Zero-size type (typically 1 byte for uniqueness)

#### Identifier and Primary Expressions (Lines 29-32)
```cpp
struct Identifier
{
    std::string name;
};
```

**Identifier Features:**
- **`name`**: Variable or function name string
- **Resolution**: Resolved during execution through environment
- **Scope Handling**: Name resolution follows lexical scoping rules

#### Binary and Unary Operations (Lines 34-44)
```cpp
struct BinaryExpr
{
    std::string op;
    ASTNodePtr left, right;
};

struct UnaryExpr
{
    std::string op;
    ASTNodePtr operand;
};
```

**Operation Analysis:**

**BinaryExpr (Lines 34-38):**
- **`op`**: Operator string ("+", "-", "*", "/", "==", etc.)
- **`left`**: Left operand expression tree
- **`right`**: Right operand expression tree
- **Tree Structure**: Enables nested expressions and proper precedence

**UnaryExpr (Lines 40-44):**
- **`op`**: Unary operator string ("-", "!", "~", etc.)
- **`operand`**: Single operand expression
- **Prefix/Postfix**: Structure works for both (position handled by parser)

#### Assignment Expressions (Lines 46-51)
```cpp
struct AssignExpr
{
    std::string op; // = += -= *= /=
    ASTNodePtr target;
    ASTNodePtr value;
};
```

**Assignment Features:**
- **`op`**: Assignment operator ("=", "+=", "-=", "*=", "/=")
- **`target`**: L-value expression (variable, property, index)
- **`value`**: R-value expression to assign
- **Compound Operators**: Supports augmented assignment

#### Function Calls and Access (Lines 53-63)
```cpp
struct CallExpr
{
    ASTNodePtr callee;
    std::vector<ASTNodePtr> args;
};

struct IndexExpr
{
    ASTNodePtr object;
    ASTNodePtr index;
};
```

**Call and Access Analysis:**

**CallExpr (Lines 53-57):**
- **`callee`**: Function or method being called
- **`args`**: Vector of argument expressions
- **Variable Arity**: Supports any number of arguments
- **Nested Calls**: callee can be complex expression

**IndexExpr (Lines 59-63):**
- **`object`**: Object being indexed (array, string, dict)
- **`index`**: Index expression
- **Multi-dimensional**: Nesting enables multi-dimensional access

#### Slice Expressions (Lines 65-72)
```cpp
// Python slice: obj[start:stop:step]  — any part may be null (omitted)
struct SliceExpr
{
    ASTNodePtr object;
    ASTNodePtr start; // may be null → 0
    ASTNodePtr stop;  // may be null → end
    ASTNodePtr step;  // may be null → 1
};
```

**Slice Features:**
- **Python-style**: Supports familiar slice syntax
- **Optional Parts**: Each component can be omitted
- **Default Values**: null parts have sensible defaults
- **Flexibility**: Supports complex slicing operations

#### Member Access (Lines 74-78)
```cpp
struct MemberExpr
{
    ASTNodePtr object;
    std::string member;
};
```

**Member Access Features:**
- **`object`**: Object being accessed
- **`member`**: Member name string
- **Dot Notation**: Represents `obj.member` syntax
- **Chaining**: Enables `obj.member.submember` through nesting

#### Collection Literals (Lines 80-88)
```cpp
struct ArrayLiteral
{
    std::vector<ASTNodePtr> elements;
};

struct DictLiteral
{
    std::vector<std::pair<ASTNodePtr, ASTNodePtr>> pairs;
};
```

**Collection Analysis:**

**ArrayLiteral (Lines 80-83):**
- **`elements`**: Vector of element expressions
- **Mixed Types**: Elements can be different types
- **Empty Arrays**: Supports empty array literals
- **Nesting**: Elements can be complex expressions

**DictLiteral (Lines 85-88):**
- **`pairs`**: Vector of key-value pairs
- **Expression Keys**: Keys can be expressions (not just literals)
- **Duplicate Keys**: Parser handles duplicate key detection
- **Ordered**: Maintains insertion order (implementation-dependent)

#### Function Expressions (Lines 90-94)
```cpp
struct LambdaExpr
{
    std::vector<std::string> params;
    ASTNodePtr body;
};
```

**Lambda Features:**
- **`params`**: Parameter name strings
- **`body`**: Function body (typically BlockStmt or single statement)
- **Closures**: Environment captured during execution
- **First-class**: Can be passed as values, stored in variables

#### Ternary and Super Expressions (Lines 96-107)
```cpp
struct TernaryExpr
{
    ASTNodePtr condition;
    ASTNodePtr thenExpr;
    ASTNodePtr elseExpr;
};

// super(args) or super.method(args)
struct SuperExpr
{
    std::string method; // empty = super constructor call
};
```

**Special Expressions:**

**TernaryExpr (Lines 96-101):**
- **`condition`**: Conditional expression
- **`thenExpr`**: Expression evaluated if condition true
- **`elseExpr`**: Expression evaluated if condition false
- **Compact**: Alternative to if-else for expressions

**SuperExpr (Lines 103-107):**
- **`method`**: Method name (empty for constructor call)
- **Inheritance**: Used for calling parent class methods
- **Constructor Chaining**: Supports super() calls in constructors

### Statement Type Definitions (Lines 109-231)

#### Variable Declarations (Lines 111-117)
```cpp
struct VarDecl
{
    bool isConst;
    std::string name;
    ASTNodePtr initializer; // may be null
    std::string typeHint;   // e.g. "int", "float", "char", "" = none
};
```

**Variable Declaration Features:**
- **`isConst`**: Flag for constant vs mutable variables
- **`name`**: Variable identifier
- **`initializer`**: Optional initialization expression
- **`typeHint`**: Optional C-style type annotation

#### Function Declarations (Lines 119-124)
```cpp
struct FunctionDecl
{
    std::string name;
    std::vector<std::string> params;
    ASTNodePtr body; // BlockStmt
};
```

**Function Features:**
- **`name`**: Function identifier
- **`params`**: Parameter name list
- **`body`**: Function body as block statement
- **Closures**: Environment captured during execution

#### Control Flow Statements (Lines 126-151)
```cpp
struct ReturnStmt
{
    ASTNodePtr value; // may be null
};

struct IfStmt
{
    ASTNodePtr condition;
    ASTNodePtr thenBranch;
    // elif chains stored as else-if
    ASTNodePtr elseBranch; // may be null
};

struct WhileStmt
{
    ASTNodePtr condition;
    ASTNodePtr body;
};

struct ForStmt
{
    std::string var;
    std::string var2; // optional second variable for tuple unpacking: for k, v in ...
    ASTNodePtr iterable;
    ASTNodePtr body;
};
```

**Control Flow Analysis:**

**ReturnStmt (Lines 126-129):**
- **`value`**: Optional return value expression
- **Void Returns**: null value indicates void return
- **Nested Returns**: Proper handling through exception-based control flow

**IfStmt (Lines 131-137):**
- **`condition`**: Conditional expression
- **`thenBranch`**: Statement executed if condition true
- **`elseBranch`**: Optional else statement (may contain nested if for elif)
- **Chaining**: elif represented as nested if-else

**WhileStmt (Lines 139-143):**
- **`condition`**: Loop condition expression
- **`body`**: Loop body statement
- **Pre-test**: Condition checked before each iteration

**ForStmt (Lines 145-151):**
- **`var`**: Loop variable name
- **`var2`**: Optional second variable for tuple unpacking
- **`iterable`**: Iterable expression (array, string, range)
- **`body`**: Loop body statement

#### Advanced Constructs (Lines 153-170)
```cpp
// List comprehension: [expr for var in iterable (if cond)?]
struct ListComp
{
    ASTNodePtr expr;               // the value expression
    std::vector<std::string> vars; // loop variable(s) — supports tuple unpacking
    ASTNodePtr iterable;
    ASTNodePtr condition; // optional if-filter (may be null)
};

struct TupleLiteral
{
    std::vector<ASTNodePtr> elements;
};

struct BlockStmt
{
    std::vector<ASTNodePtr> statements;
};
```

**Advanced Features:**

**ListComp (Lines 154-160):**
- **`expr`**: Value expression for each iteration
- **`vars`**: Loop variable names (supports tuple unpacking)
- **`iterable`**: Source iterable
- **`condition`**: Optional filter condition
- **Python-style**: Familiar list comprehension syntax

**TupleLiteral (Lines 162-165):**
- **`elements`**: Tuple element expressions
- **Immutable**: Conceptually immutable (enforced at runtime)
- **Multiple Values**: Support for multiple return values

**BlockStmt (Lines 167-170):**
- **`statements`**: Vector of statements in block
- **Scope**: Creates new lexical scope
- **Empty Blocks**: Supports empty blocks

#### Expression and I/O Statements (Lines 172-187)
```cpp
struct ExprStmt
{
    ASTNodePtr expr;
};

struct PrintStmt
{
    std::vector<ASTNodePtr> args;
    bool newline;
};

struct InputStmt
{
    std::string target;
    ASTNodePtr prompt;
};
```

**Statement Types:**

**ExprStmt (Lines 172-175):**
- **`expr`**: Expression to evaluate
- **Side Effects**: Used for function calls, assignments
- **Discarded Values**: Result value typically ignored

**PrintStmt (Lines 177-181):**
- **`args`**: Vector of argument expressions
- **`newline`**: Whether to add newline after output
- **Multiple Arguments**: Supports print(a, b, c) syntax

**InputStmt (Lines 183-187):**
- **`target`**: Variable name to store input
- **`prompt`**: Optional prompt expression
- **User Input**: Reads from standard input

#### Exception Handling (Lines 189-212)
```cpp
struct BreakStmt
{
};

struct ContinueStmt
{
};

struct RaiseStmt
{
    ASTNodePtr value; // the exception value/message
};

struct ExceptClause
{
    std::string errorType; // e.g. "ValueError" — empty = bare except
    std::string alias;     // "as e" — empty if none
    std::shared_ptr<ASTNode> body;
};

struct TryStmt
{
    std::shared_ptr<ASTNode> body;
    std::vector<ExceptClause> handlers;
    std::shared_ptr<ASTNode> finallyBody; // may be null
};
```

**Exception Handling Analysis:**

**Control Flow Statements:**
- **`BreakStmt`**: Breaks out of loops
- **`ContinueStmt`**: Skips to next iteration
- **Empty Structs**: No additional data needed

**Exception Statements:**
- **`RaiseStmt`**: Raises exception with optional value
- **`ExceptClause`**: Single exception handler with type and alias
- **`TryStmt`**: Complete try-except-finally structure

#### Import and Class Statements (Lines 214-231)
```cpp
struct ImportStmt
{
    std::string module; // e.g. "abc" for `from abc import...`, empty for `import sys`
    struct Item {
        std::string name;
        std::string alias; // empty if no alias
    };
    std::vector<Item> imports;
};

struct ClassDecl
{
    std::string name;
    std::string base; // optional
    std::vector<ASTNodePtr> methods;
    std::vector<ASTNodePtr> staticMethods;
    std::vector<ASTNodePtr> fields;
};
```

**Module and OOP Statements:**

**ImportStmt (Lines 214-222):**
- **`module`**: Module name (empty for simple import)
- **`Item`**: Single import with name and optional alias
- **Multiple Imports**: Vector supports multiple imports per statement
- **Flexible Syntax**: Supports both `import` and `from...import`

**ClassDecl (Lines 224-231):**
- **`name`**: Class identifier
- **`base`**: Optional base class name for inheritance
- **`methods`**: Instance method declarations
- **`staticMethods`**: Static method declarations
- **`fields`**: Class field declarations

### AST Node Variant (Lines 235-248)
```cpp
using NodeVariant = std::variant<
    NumberLiteral, StringLiteral, BoolLiteral, NilLiteral,
    Identifier,
    BinaryExpr, UnaryExpr, AssignExpr,
    CallExpr, IndexExpr, SliceExpr, MemberExpr,
    ArrayLiteral, DictLiteral, LambdaExpr, ListComp, TupleLiteral,
    VarDecl, FunctionDecl, ReturnStmt,
    IfStmt, WhileStmt, ForStmt,
    BlockStmt, ExprStmt,
    PrintStmt, InputStmt,
    BreakStmt, ContinueStmt, SuperExpr,
    RaiseStmt, TryStmt,
    ImportStmt, ClassDecl,
    TernaryExpr>;
```

**Variant Features:**
- **Type Safety**: Compile-time checking of all node operations
- **Memory Efficiency**: Only stores active type, no overhead for unused types
- **Extensibility**: Easy to add new node types
- **Pattern Matching**: Clean dispatch with `std::visit`

### AST Node Wrapper (Lines 250-266)
```cpp
struct ASTNode
{
    NodeVariant node;
    int line = 0;

    template <typename T>
    ASTNode(T &&n, int ln = 0) : node(std::forward<T>(n)), line(ln) {}

    template <typename T>
    T &as() { return std::get<T>(node); }

    template <typename T>
    const T &as() const { return std::get<T>(node); }

    template <typename T>
    bool is() const { return std::holds_alternative<T>(node); }
};
```

**ASTNode Features:**

**Constructor (Lines 255-257):**
- **Perfect Forwarding**: Efficient construction of any node type
- **Line Tracking**: Stores source line for error reporting
- **Default Line**: Line 0 if not specified

**Access Methods (Lines 259-265):**
- **`as()`**: Cast to specific node type (throws if wrong type)
- **`is()`**: Check if node is specific type
- **Const Overloads**: Support for const access
- **Type Safety**: Compile-time and runtime type checking

## Design Patterns and Architecture

### Variant Pattern
```cpp
using NodeVariant = std::variant<NumberLiteral, StringLiteral, /* ... */>;
```

**Benefits:**
- **Type Safety**: Compile-time checking of node operations
- **Memory Efficiency**: No overhead for unused types
- **Pattern Matching**: Clean dispatch with `std::visit`
- **Extensibility**: Easy to add new node types

### Visitor Pattern (Implied)
```cpp
// AST nodes are visited by interpreter
void execute(ASTNode &node) {
    std::visit([this](auto &stmt) {
        using T = std::decay_t<decltype(stmt)>;
        if constexpr (std::is_same_v<T, VarDecl>) execVarDecl(stmt);
        // ... dispatch to appropriate handler
    }, node.node);
}
```

**Benefits:**
- **Separation of Concerns**: AST structure separated from execution
- **Extensibility**: Easy to add new node types and operations
- **Type Safety**: Compile-time dispatch to correct handlers

### Composite Pattern
```cpp
struct BlockStmt {
    std::vector<ASTNodePtr> statements;
};
```

**Benefits:**
- **Tree Structure**: Uniform treatment of individual and composite nodes
- **Recursive Operations**: Operations can be applied recursively
- **Hierarchical Structure**: Natural representation of program structure

## Performance Considerations

### Memory Layout
```cpp
struct ASTNode {
    NodeVariant node;  // Stack storage for small types
    int line = 0;      // Position information
};
```

**Optimizations:**
- **Stack Storage**: Small types stored inline in variant
- **Smart Pointers**: Large types use heap allocation with shared ownership
- **Cache Friendly**: Sequential access patterns for tree traversal

### Move Semantics
```cpp
template <typename T>
ASTNode(T &&n, int ln = 0) : node(std::forward<T>(n)), line(ln) {}
```

**Benefits:**
- **Efficient Construction**: Avoids copying large node structures
- **Perfect Forwarding**: Preserves value category of arguments
- **RAII**: Automatic resource management

### Type Safety
```cpp
template <typename T>
T &as() { return std::get<T>(node); }
```

**Safety Features:**
- **Runtime Checking**: `std::get` throws on type mismatch
- **Compile-time Interface**: Template methods provide clean interface
- **Debugging Support**: Clear error messages for type mismatches

## Integration with Compiler Pipeline

### Parser Output
```cpp
// Parser creates AST nodes
auto stmt = std::make_unique<ASTNode>(VarDecl{isConst, name, init, typeHint}, line);
return stmt;
```

**Integration Features:**
- **Tree Construction**: Parser builds hierarchical AST
- **Position Tracking**: Source locations preserved
- **Memory Management**: Smart pointers handle cleanup

### Interpreter Input
```cpp
// Interpreter consumes AST
void execute(ASTNode &node) {
    std::visit([this](auto &stmt) {
        // Dispatch to appropriate executor
    }, node.node);
}
```

**Execution Features:**
- **Tree Traversal**: Interpreter walks AST recursively
- **Type Dispatch**: Variant enables efficient type-based dispatch
- **Context Preservation**: Line numbers used for error reporting

## Why This Design Works

### Comprehensive Language Coverage
- **Complete Feature Set**: Every language construct has AST representation
- **Modern Language Features**: List comprehensions, lambdas, classes
- **Multi-paradigm Support**: Procedural, functional, OOP styles
- **Extensibility**: Easy to add new language constructs

### Type Safety and Performance
- **Modern C++**: Uses variant, smart pointers, move semantics
- **Memory Efficiency**: Optimized storage with minimal overhead
- **Compile-time Checking**: Variant provides type safety
- **Runtime Performance**: Efficient tree traversal and dispatch

### Maintainability and Clarity
- **Clear Structure**: Each node type has specific purpose
- **Self-documenting**: Node names clearly indicate purpose
- **Separation of Concerns**: AST structure separated from execution
- **Tooling Friendly**: Structure supports analysis tools and IDEs

The AST system provides a robust, type-safe foundation for the Quantum Language compiler, enabling accurate representation of program structure while maintaining excellent performance characteristics and extensibility for future language evolution.
