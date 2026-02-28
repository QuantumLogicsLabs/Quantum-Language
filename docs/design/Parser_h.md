# Parser.h - Parser Class Interface and Parsing Methods

## Overview
`Parser.h` defines the interface for the parsing phase of the Quantum Language compiler. It declares the `Parser` class responsible for converting the token stream from the lexer into an Abstract Syntax Tree (AST), using a hybrid approach of recursive descent for statements and Pratt-style parsing for expressions.

## Architecture Overview

The parser interface provides:
- **AST Generation**: Main entry point for parsing token streams
- **Statement Parsing**: Recursive descent methods for all statement types
- **Expression Parsing**: Pratt-style methods for operator precedence
- **Token Management**: Utilities for token access and validation
- **Error Handling**: Custom exception types with position information

## Line-by-Line Analysis

### Header Guard and Dependencies (Lines 1-5)
```cpp
#pragma once
#include "Token.h"
#include "AST.h"
#include <vector>
#include <stdexcept>
```

**Dependency Analysis:**
- **`Token.h`**: Token type definitions and Token structure for input
- **`AST.h`**: AST node definitions for output tree structure
- **`vector`**: Token stream storage and AST node collections
- **`stdexcept`**: Base class for custom exception hierarchy

### Custom Exception Class (Lines 7-13)
```cpp
class ParseError : public std::runtime_error
{
public:
    int line, col;
    ParseError(const std::string &msg, int l, int c)
        : std::runtime_error(msg), line(l), col(c) {}
};
```

**ParseError Features:**
- **Inheritance**: Extends `std::runtime_error` for standard exception handling
- **Position Information**: Stores line and column for error location
- **Message Preservation**: Maintains error message from base class
- **Constructor**: Initializes all members with provided information

**Error Handling Benefits:**
- **Precise Location**: Exact position of parsing errors
- **Standard Interface**: Works with standard exception handling
- **Rich Context**: Line/column information for user-friendly errors
- **Type Safety**: Specific exception type for parsing errors

### Parser Class Declaration (Lines 15-77)

#### Class Definition (Line 15)
```cpp
class Parser
{
```

**Class Design:**
- **Single Responsibility**: Focused solely on parsing tokens into AST
- **Stateful Design**: Maintains position and state during parsing
- **Hybrid Approach**: Combines recursive descent and Pratt parsing

#### Public Interface (Lines 17-19)
```cpp
public:
    explicit Parser(std::vector<Token> tokens);
    ASTNodePtr parse();
```

**Public Methods Analysis:**

**Constructor (Line 18):**
```cpp
explicit Parser(std::vector<Token> tokens);
```
- **`explicit`**: Prevents implicit conversion from token vector
- **Parameter**: Takes token vector by value (move semantics optimize)
- **Purpose**: Initializes parser with token stream to parse

**Parse Method (Line 19):**
```cpp
ASTNodePtr parse();
```
- **Return Type**: Smart pointer to root AST node (typically BlockStmt)
- **Method**: Main entry point for parsing complete token stream
- **Ownership**: Returns unique pointer for automatic memory management

#### Private Members (Lines 21-77)

**Token Stream State (Lines 22-23):**
```cpp
private:
    std::vector<Token> tokens;
    size_t pos;
```

**State Analysis:**
- **`tokens`**: Complete token stream from lexer (owned by parser)
- **`pos`**: Current position in token stream (0-based index)
- **Ownership**: Parser takes ownership of token vector

**Token Access Methods (Lines 25-33):**
```cpp
    // Token helpers
    Token &current();
    Token &peek(int offset = 1);
    Token &consume();
    Token &expect(TokenType t, const std::string &msg);
    bool check(TokenType t) const;
    bool match(TokenType t);
    bool atEnd() const;
    void skipNewlines();
```

**Token Access Analysis:**

**`current()` (Line 26):**
```cpp
Token &current();
```
- **Purpose**: Returns current token without advancing position
- **Return**: Reference to current token for efficient access
- **Bounds Safety**: Must handle EOF token appropriately

**`peek()` (Line 27):**
```cpp
Token &peek(int offset = 1);
```
- **Purpose**: Lookahead token access without consuming
- **Default Offset**: Peeks 1 token ahead by default
- **Multi-token Lookahead**: Supports arbitrary offset distances
- **Operator Disambiguation**: Essential for expression parsing

**`consume()` (Line 28):**
```cpp
Token &consume();
```
- **Purpose**: Consumes current token and advances position
- **Return**: Reference to consumed token for convenience
- **Position Management**: Updates internal position counter

**`expect()` (Line 29):**
```cpp
Token &expect(TokenType t, const std::string &msg);
```
- **Purpose**: Consumes token only if it matches expected type
- **Error Generation**: Throws ParseError with custom message on mismatch
- **Validation**: Ensures specific token types appear where expected
- **Return**: Reference to consumed token on success

**`check()` (Line 30):**
```cpp
bool check(TokenType t) const;
```
- **Purpose**: Tests if current token matches type without consuming
- **Const Method**: Doesn't modify parser state
- **Conditional Logic**: Used in parsing decision making

**`match()` (Line 31):**
```cpp
bool match(TokenType t);
```
- **Purpose**: Consumes token only if it matches type
- **Return Value**: True if token matched and consumed, false otherwise
- **Convenience**: Combines check and consume operations

**`atEnd()` (Line 32):**
```cpp
bool atEnd() const;
```
- **Purpose**: Tests if parser has reached end of token stream
- **EOF Detection**: Typically checks for EOF_TOKEN type
- **Loop Control**: Used to control parsing loops

**`skipNewlines()` (Line 33):**
```cpp
void skipNewlines();
```
- **Purpose**: Skips newline tokens between statements
- **Whitespace Handling**: Treats newlines as statement separators
- **Flexible Formatting**: Allows flexible statement formatting

**Statement Parsing Methods (Lines 35-53):**
```cpp
    // Parsing methods
    ASTNodePtr parseStatement();
    ASTNodePtr parseBlock();
    ASTNodePtr parseBodyOrStatement(); // block OR single statement (brace-optional)
    ASTNodePtr parseVarDecl(bool isConst);
    ASTNodePtr parseFunctionDecl();
    ASTNodePtr parseClassDecl();
    ASTNodePtr parseIfStmt();
    ASTNodePtr parseWhileStmt();
    ASTNodePtr parseForStmt();
    ASTNodePtr parseReturnStmt();
    ASTNodePtr parsePrintStmt();
    ASTNodePtr parseInputStmt();
    ASTNodePtr parseCoutStmt(); // cout << x << y << endl
    ASTNodePtr parseCinStmt();  // cin >> x >> y
    ASTNodePtr parseImportStmt(bool isFrom = false);
    ASTNodePtr parseExprStmt();
    ASTNodePtr parseCTypeVarDecl(const std::string &typeHint); // int x = ...
    bool isCTypeKeyword(TokenType t) const;
```

**Statement Parsing Analysis:**

**Core Statement Methods:**
- **`parseStatement()`**: Main statement dispatcher based on token type
- **`parseBlock()`**: Parses blocks of statements in braces
- **`parseBodyOrStatement()`**: Flexible parsing for blocks or single statements

**Declaration Methods:**
- **`parseVarDecl()`**: Variable declarations with optional type hints and initializers
- **`parseFunctionDecl()`**: Function declarations with parameters and bodies
- **`parseClassDecl()`**: Class declarations with methods and inheritance
- **`parseCTypeVarDecl()`**: C-style typed variable declarations

**Control Flow Methods:**
- **`parseIfStmt()`**: If/elif/else statements with nested conditions
- **`parseWhileStmt()`**: While loops with condition and body
- **`parseForStmt()`**: For loops with iterator and iterable
- **`parseReturnStmt()`**: Return statements with optional values

**I/O Methods:**
- **`parsePrintStmt()`**: Print statements with multiple arguments
- **`parseInputStmt()`**: Input statements with prompts and targets
- **`parseCoutStmt()`**: C++ style cout output with << operators
- **`parseCinStmt()`**: C++ style cin input with >> operators

**Other Statements:**
- **`parseImportStmt()`**: Import statements for modules
- **`parseExprStmt()`**: Expression statements (function calls, assignments)
- **`isCTypeKeyword()`**: Utility to detect C-style type keywords

**Expression Parsing Methods (Lines 55-69):**
```cpp
    // Expression parsing (Pratt-style precedence)
    ASTNodePtr parseExpr();
    ASTNodePtr parseAssignment();
    ASTNodePtr parseOr();
    ASTNodePtr parseAnd();
    ASTNodePtr parseBitwise();
    ASTNodePtr parseEquality();
    ASTNodePtr parseComparison();
    ASTNodePtr parseShift();
    ASTNodePtr parseAddSub();
    ASTNodePtr parseMulDiv();
    ASTNodePtr parsePower();
    ASTNodePtr parseUnary();
    ASTNodePtr parsePostfix();
    ASTNodePtr parsePrimary();
```

**Pratt Parsing Analysis:**

**Precedence Chain (Lowest to Highest):**
1. **`parseExpr()`**: Entry point, calls `parseAssignment()`
2. **`parseAssignment()`**: Assignment operators (`=`, `+=`, etc.)
3. **`parseOr()`**: Logical OR (`||`)
4. **`parseAnd()`**: Logical AND (`&&`)
5. **`parseBitwise()`**: Bitwise OR, XOR, AND (`|`, `^`, `&`)
6. **`parseEquality()`**: Equality comparisons (`==`, `===`, `!=`, `!==`)
7. **`parseComparison()`**: Ordering comparisons (`<`, `>`, `<=`, `>=`)
8. **`parseShift()`**: Bitwise shifts (`<<`, `>>`)
9. **`parseAddSub()`**: Addition and subtraction (`+`, `-`)
10. **`parseMulDiv()`**: Multiplication and division (`*`, `/`, `%`)
11. **`parsePower()`**: Exponentiation (`**`) - right-associative
12. **`parseUnary()`**: Unary operators (`+`, `-`, `!`, `~`)
13. **`parsePostfix()`**: Postfix operators (`++`, `--`, function calls, indexing)
14. **`parsePrimary()`**: Base cases (literals, identifiers, grouping)

**Specialized Expression Methods (Lines 71-76):**
```cpp
    ASTNodePtr parseArrayLiteral();
    ASTNodePtr parseDictLiteral();
    ASTNodePtr parseLambda();
    ASTNodePtr parseArrowFunction(std::vector<std::string> params, int ln);
    std::vector<ASTNodePtr> parseArgList();
    std::vector<std::string> parseParamList();
```

**Specialized Expression Analysis:**

**Collection Literals:**
- **`parseArrayLiteral()`**: Array literals with elements `[1, 2, 3]`
- **`parseDictLiteral()`**: Dictionary literals with key-value pairs `{a: 1, b: 2}`

**Function Expressions:**
- **`parseLambda()`**: Lambda functions with `=>` syntax
- **`parseArrowFunction()`**: Arrow function parsing helper

**Utility Methods:**
- **`parseArgList()`**: Function argument lists in parentheses
- **`parseParamList()`**: Function parameter lists in definitions

## Design Patterns and Architecture

### Hybrid Parsing Strategy
```cpp
// Recursive Descent for Statements
ASTNodePtr parseStatement() {
    switch (current().type) {
        case LET: return parseVarDecl(false);
        case IF: return parseIfStmt();
        // ...
    }
}

// Pratt Parsing for Expressions
ASTNodePtr parseAssignment() {
    ASTNodePtr left = parseOr();
    while (match(ASSIGN) || match(PLUS_ASSIGN)) {
        // Handle assignment
    }
    return left;
}
```

**Benefits:**
- **Statement Clarity**: Recursive descent provides clear statement structure
- **Expression Flexibility**: Pratt parsing handles complex operator precedence
- **Maintainability**: Each strategy optimized for its domain
- **Extensibility**: Easy to add new statements or operators

### Visitor Pattern (Implied)
```cpp
// AST nodes will be visited by interpreter
ASTNodePtr parse();
```

**Benefits:**
- **Separation of Concerns**: Parsing separated from execution
- **Tree Structure**: Hierarchical representation of program structure
- **Type Safety**: Variant-based AST nodes with compile-time checking

### Factory Pattern (Implied)
```cpp
ASTNodePtr parseVarDecl(bool isConst) {
    return std::make_unique<ASTNode>(VarDecl{...}, line);
}
```

**Benefits:**
- **Consistent Creation**: Uniform AST node creation pattern
- **Memory Management**: Smart pointers handle cleanup automatically
- **Type Safety**: Compile-time checking of node types

## Integration with Compiler Pipeline

### Token Stream Input
```cpp
// In main.cpp
auto tokens = lexer.tokenize();
Parser parser(std::move(tokens));
auto ast = parser.parse();
```

**Integration Features:**
- **Token Ownership**: Parser takes ownership of token stream
- **AST Output**: Produces tree structure for interpreter
- **Error Propagation**: Position information flows through pipeline

### AST Generation
```cpp
// Parser creates hierarchical AST
auto block = std::make_unique<ASTNode>(BlockStmt{}, 0);
auto &stmts = block->as<BlockStmt>().statements;
stmts.push_back(parseStatement());
```

**AST Features:**
- **Tree Structure**: Hierarchical representation of program
- **Type Safety**: Variant-based nodes with compile-time checking
- **Position Information**: Source locations preserved for error reporting

## Error Handling Strategy

### Custom Exception Hierarchy
```cpp
class ParseError : public std::runtime_error {
    int line, col;
    // ...
};
```

**Error Features:**
- **Precise Location**: Line and column for all parsing errors
- **Rich Context**: Detailed error messages with expected vs actual
- **Standard Interface**: Works with standard exception handling
- **Type Safety**: Specific exception type for parsing errors

### Synchronization and Recovery
```cpp
void skipNewlines(); // Recovery point
Token &expect(TokenType t, const std::string &msg); // Validation
```

**Recovery Features:**
- **Synchronization Points**: Newlines serve as recovery boundaries
- **Validation**: `expect()` method ensures required tokens present
- **Graceful Degradation**: Attempts to continue after minor errors

## Performance Considerations

### Efficient Token Access
```cpp
Token &current() { return tokens[pos]; }
Token &consume() { return tokens[pos++]; }
```

**Optimizations:**
- **Direct Access**: O(1) token access with array indexing
- **Reference Returns**: Avoid copying tokens
- **Inline Potential**: Simple methods can be inlined by compiler

### Memory Management
```cpp
ASTNodePtr parse(); // Returns unique_ptr
std::make_unique<ASTNode>(...); // Efficient node creation
```

**Memory Features:**
- **Smart Pointers**: Automatic memory management
- **Move Semantics**: Efficient transfer of ownership
- **RAII**: Automatic cleanup when exceptions thrown

### Recursive Efficiency
```cpp
// Tail recursion optimization opportunities
ASTNodePtr parseExpr() { return parseAssignment(); }
```

**Optimization Features:**
- **Tail Calls**: Some methods could be optimized as tail calls
- **Minimal Stack Depth**: Reasonable recursion depth for typical programs
- **Compiler Optimization**: Modern compilers optimize recursive calls

## Extensibility Design

### New Statement Types
```cpp
// Easy to add new statement parsing methods
ASTNodePtr parseNewStatement();
```

**Extensibility Features:**
- **Dispatcher Pattern**: `parseStatement()` easily extended with new cases
- **Isolated Methods**: Each statement type has dedicated parser
- **Consistent Interface**: All parsers return same ASTNodePtr type

### New Expression Operators
```cpp
// Easy to add new precedence levels
ASTNodePtr parseNewPrecedence() {
    ASTNodePtr left = parseHigherPrecedence();
    while (match(NEW_OPERATOR)) {
        // Handle operator
    }
    return left;
}
```

**Operator Addition Benefits:**
- **Precedence Chain**: Easy to insert new precedence levels
- **Consistent Pattern**: All operators follow same parsing pattern
- **Associativity Control**: Easy to handle left/right associativity

## Why This Design Works

### Hybrid Approach Benefits
- **Best of Both Worlds**: Recursive descent for statements, Pratt for expressions
- **Clear Structure**: Statement parsing is straightforward and readable
- **Flexible Precedence**: Pratt parsing handles complex operator relationships
- **Maintainable**: Each parsing strategy optimized for its domain

### Comprehensive Language Support
- **All Constructs**: Every language feature has dedicated parsing method
- **Multi-language Syntax**: Supports Python, JavaScript, and C-style elements
- **Complex Features**: Template literals, lambdas, comprehensions
- **Future-proof**: Easy to add new language constructs

### Robust Error Handling
- **Precise Location**: Line and column for all errors
- **Rich Context**: Detailed error messages with expected tokens
- **Recovery Mechanisms**: Attempts to continue after minor errors
- **User Experience**: Helpful error messages for developers

The parser interface provides a comprehensive foundation for syntactic analysis, combining proven parsing techniques with modern C++ design principles to create a robust, maintainable, and extensible parsing system for the Quantum Language compiler.
