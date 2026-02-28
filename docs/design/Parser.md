# Parser.cpp - Recursive Descent Parser with Pratt-style Expression Parsing

## Overview
`Parser.cpp` implements the parsing phase of the Quantum Language compiler. It converts the token stream from the lexer into an Abstract Syntax Tree (AST) using a hybrid approach: recursive descent for statements and Pratt-style parsing for expressions. This combination provides excellent error handling, operator precedence management, and extensibility.

## Architecture Overview

The parser uses a two-phase approach:
1. **Statement Parsing**: Recursive descent for control structures, declarations, and statements
2. **Expression Parsing**: Pratt parsing (top-down operator precedence) for complex expressions

This hybrid approach provides:
- **Clear statement structure** with recursive descent
- **Flexible operator precedence** with Pratt parsing
- **Excellent error recovery** and reporting
- **Easy extensibility** for new language features

## Line-by-Line Analysis

### Includes and Dependencies (Lines 1-3)
```cpp
#include "../include/Parser.h"
#include <sstream>
```
- **Line 1**: Parser class interface and AST node definitions
- **Line 2**: String stream utilities for error message formatting

### Constructor (Line 4)
```cpp
Parser::Parser(std::vector<Token> tokens) : tokens(std::move(tokens)), pos(0) {}
```
- **Token Ownership**: Takes ownership of token vector using move semantics
- **Position Initialization**: Starts at first token (position 0)
- **Efficient Transfer**: Avoids copying the entire token vector

### Token Access Methods (Lines 6-31)

#### current(), peek(), consume() (Lines 6-13)
```cpp
Token &Parser::current() { return tokens[pos]; }
Token &Parser::peek(int offset)
{
    size_t p = pos + offset;
    return p < tokens.size() ? tokens[p] : tokens.back();
}
Token &Parser::consume() { return tokens[pos++]; }
```
- **Bounds Safety**: `peek()` returns last token if out of bounds (usually EOF)
- **Reference Returns**: Efficient access without copying
- **Position Management**: `consume()` advances position automatically

#### expect() Method (Lines 15-20)
```cpp
Token &Parser::expect(TokenType t, const std::string &msg)
{
    if (current().type != t)
        throw ParseError(msg + " (got '" + current().value + "')", current().line, current().col);
    return consume();
}
```
- **Error Generation**: Creates detailed error messages with expected vs actual tokens
- **Exception Throwing**: Uses `ParseError` with position information
- **Consumption**: Automatically consumes expected token on success

#### match() and check() Methods (Lines 22-31)
```cpp
bool Parser::check(TokenType t) const { return tokens[pos].type == t; }
bool Parser::match(TokenType t)
{
    if (check(t))
    {
        consume();
        return true;
    }
    return false;
}
```
- **Non-consuming Check**: `check()` verifies without advancing
- **Conditional Consumption**: `match()` consumes only on match
- **Convenience Methods**: Simplify common parsing patterns

### Utility Methods (Lines 32-38)
```cpp
bool Parser::atEnd() const { return tokens[pos].type == TokenType::EOF_TOKEN; }
void Parser::skipNewlines()
{
    while (check(TokenType::NEWLINE))
        consume();
}
```
- **EOF Detection**: Checks for end of token stream
- **Whitespace Handling**: Skips newlines for statement separation

### Main Entry Point (Lines 40-51)
```cpp
ASTNodePtr Parser::parse()
{
    auto block = std::make_unique<ASTNode>(BlockStmt{}, 0);
    auto &stmts = block->as<BlockStmt>().statements;
    skipNewlines();
    while (!atEnd())
    {
        stmts.push_back(parseStatement());
        skipNewlines();
    }
    return block;
}
```

**Parse Flow:**
1. **Root Block**: Creates top-level block statement
2. **Statement Loop**: Parses statements until EOF
3. **Newline Handling**: Skips whitespace between statements
4. **AST Construction**: Returns complete program as block

### Statement Parsing (Lines 53-100)

#### parseStatement() Method (Lines 53-100)
```cpp
ASTNodePtr Parser::parseStatement()
{
    skipNewlines();
    
    // Skip Python-style decorators
    while (check(TokenType::DECORATOR)) {
        consume(); // eat @
        if (check(TokenType::IDENTIFIER)) {
            consume(); // eat decorator name
            // Optional call parens e.g. @decorator(args)
            if (check(TokenType::LPAREN)) {
                consume(); // eat (
                int depth = 1;
                while (!atEnd() && depth > 0) {
                    if (check(TokenType::LPAREN)) depth++;
                    else if (check(TokenType::RPAREN)) depth--;
                    consume();
                }
            }
        }
        skipNewlines();
    }
    
    int ln = current().line;
    switch (current().type)
    {
    case TokenType::LET:
    {
        consume();
        return parseVarDecl(false);
    }
    case TokenType::CONST:
    {
        consume();
        return parseVarDecl(true);
    }
    // ... continues for all statement types ...
    }
}
```

**Statement Types Handled:**
1. **Variable Declarations**: `let`, `const`, C-style type hints
2. **Function Declarations**: `fn`, `def`, `function`
3. **Class Declarations**: `class` with inheritance
4. **Control Flow**: `if`, `while`, `for`, `break`, `continue`
5. **Exception Handling**: `try`, `except`, `finally`
6. **Return Statements**: `return` with optional value
7. **I/O Statements**: `print`, `input`, `cout`, `cin`
8. **Import Statements**: `import`, `from...import`
9. **Expression Statements**: Function calls, assignments

#### Decorator Support
```cpp
// Skip Python-style decorators (e.g. @property, @dataclass)
while (check(TokenType::DECORATOR)) {
    consume(); // eat @
    // ... handle decorator syntax ...
}
```
- **Python Compatibility**: Supports `@decorator` syntax
- **Future Feature**: Currently skipped, parsed for future implementation
- **Complex Arguments**: Handles `@decorator(args)` with nested parentheses

### Variable Declaration Parsing (Lines 150-180)
```cpp
ASTNodePtr Parser::parseVarDecl(bool isConst)
{
    int ln = current().line;
    
    // Check for C-style type hint
    std::string typeHint;
    if (isCTypeKeyword(current().type)) {
        typeHint = consume().value;
    }
    
    std::string name = expect(TokenType::IDENTIFIER, "Expected variable name").value;
    
    ASTNodePtr initializer = nullptr;
    if (match(TokenType::ASSIGN)) {
        initializer = parseExpr();
    }
    
    return std::make_unique<ASTNode>(VarDecl{isConst, name, std::move(initializer), typeHint}, ln);
}
```

**Variable Declaration Features:**
- **Const Detection**: Tracks constant vs mutable variables
- **Type Hints**: Optional C-style type annotations (`int x = 5`)
- **Initializers**: Optional assignment expressions
- **Error Handling**: Clear messages for missing identifiers

### Function Declaration Parsing (Lines 200-250)
```cpp
ASTNodePtr Parser::parseFunctionDecl()
{
    int ln = current().line;
    std::string name = expect(TokenType::IDENTIFIER, "Expected function name").value;
    
    expect(TokenType::LPAREN, "Expected '(' after function name");
    auto params = parseParamList();
    expect(TokenType::RPAREN, "Expected ')' after parameters");
    
    // Optional return type hint
    std::string returnType;
    if (match(TokenType::COLON)) {
        if (isCTypeKeyword(current().type)) {
            returnType = consume().value;
        }
    }
    
    auto body = parseBodyOrStatement();
    
    return std::make_unique<ASTNode>(FunctionDecl{name, std::move(params), std::move(body)}, ln);
}
```

**Function Features:**
- **Parameter Lists**: Support for multiple parameters
- **Return Type Hints**: Optional C-style return types
- **Flexible Bodies**: Single statement or block
- **Error Recovery**: Detailed error messages for syntax issues

### Expression Parsing - Pratt Style (Lines 400-800)

#### parseExpr() Entry Point (Lines 400-410)
```cpp
ASTNodePtr Parser::parseExpr()
{
    return parseAssignment();
}
```
- **Precedence Chain**: Starts with lowest precedence (assignment)
- **Chain of Responsibility**: Each level calls higher precedence

#### parseAssignment() (Lines 412-430)
```cpp
ASTNodePtr Parser::parseAssignment()
{
    ASTNodePtr target = parseOr();
    
    if (match(TokenType::ASSIGN) || match(TokenType::PLUS_ASSIGN) || 
        match(TokenType::MINUS_ASSIGN) || match(TokenType::STAR_ASSIGN) || 
        match(TokenType::SLASH_ASSIGN)) {
        Token op = tokens[pos - 1];
        ASTNodePtr value = parseAssignment(); // Right-associative
        return std::make_unique<ASTNode>(AssignExpr{op.value, std::move(target), std::move(value)}, op.line);
    }
    
    return target;
}
```

**Assignment Features:**
- **Multiple Operators**: `=`, `+=`, `-=`, `*=`, `/=`
- **Right-Associative**: `x = y = 5` works correctly
- **Target Validation**: Ensures left side is assignable (handled in interpreter)

#### Operator Precedence Chain
```cpp
ASTNodePtr Parser::parseOr()           // Lowest precedence
{
    ASTNodePtr left = parseAnd();
    while (match(TokenType::OR_OR)) {
        Token op = tokens[pos - 1];
        ASTNodePtr right = parseAnd();
        left = std::make_unique<ASTNode>(BinaryExpr{op.value, std::move(left), std::move(right)}, op.line);
    }
    return left;
}

ASTNodePtr Parser::parseAnd()
{
    ASTNodePtr left = parseBitwise();
    while (match(TokenType::AND_AND)) {
        Token op = tokens[pos - 1];
        ASTNodePtr right = parseBitwise();
        left = std::make_unique<ASTNode>(BinaryExpr{op.value, std::move(left), std::move(right)}, op.line);
    }
    return left;
}
```

**Precedence Hierarchy (Lowest to Highest):**
1. **Assignment**: `=`, `+=`, `-=`, `*=`, `/=`
2. **Logical OR**: `||`
3. **Logical AND**: `&&`
4. **Bitwise OR**: `|`
5. **Bitwise XOR**: `^`
6. **Bitwise AND**: `&`
7. **Equality**: `==`, `===`, `!=`, `!==`
8. **Comparison**: `<`, `>`, `<=`, `>=`
9. **Shift**: `<<`, `>>`
10. **Addition/Subtraction**: `+`, `-`
11. **Multiplication/Division**: `*`, `/`, `%`
12. **Power**: `**` (right-associative)
13. **Unary**: `+`, `-`, `!`, `~`
14. **Postfix**: `++`, `--`, `()`, `[]`, `.`
15. **Primary**: literals, identifiers, `(`expr`)`

#### parsePrimary() - Base Cases (Lines 700-800)
```cpp
ASTNodePtr Parser::parsePrimary()
{
    int ln = current().line;
    
    if (match(TokenType::NUMBER)) {
        Token num = tokens[pos - 1];
        return std::make_unique<ASTNode>(NumberLiteral{std::stod(num.value)}, ln);
    }
    
    if (match(TokenType::STRING)) {
        Token str = tokens[pos - 1];
        return std::make_unique<ASTNode>(StringLiteral{str.value}, ln);
    }
    
    if (match(TokenType::BOOL_TRUE)) {
        return std::make_unique<ASTNode>(BoolLiteral{true}, ln);
    }
    
    if (match(TokenType::BOOL_FALSE)) {
        return std::make_unique<ASTNode>(BoolLiteral{false}, ln);
    }
    
    if (match(TokenType::NIL)) {
        return std::make_unique<ASTNode>(NilLiteral{}, ln);
    }
    
    if (match(TokenType::IDENTIFIER)) {
        Token ident = tokens[pos - 1];
        return std::make_unique<ASTNode>(Identifier{ident.value}, ln);
    }
    
    if (match(TokenType::LPAREN)) {
        ASTNodePtr expr = parseExpr();
        expect(TokenType::RPAREN, "Expected ')' after expression");
        return expr;
    }
    
    // Array literals
    if (match(TokenType::LBRACKET)) {
        return parseArrayLiteral();
    }
    
    // Dictionary literals
    if (match(TokenType::LBRACE)) {
        return parseDictLiteral();
    }
    
    throw ParseError("Unexpected token: " + current().value, current().line, current().col);
}
```

**Primary Expressions:**
- **Literals**: Numbers, strings, booleans, nil
- **Identifiers**: Variable and function names
- **Grouping**: Parenthesized expressions
- **Collections**: Arrays and dictionaries
- **Error Handling**: Clear messages for unexpected tokens

### Specialized Parsing Methods

#### Array Literal Parsing (Lines 850-880)
```cpp
ASTNodePtr Parser::parseArrayLiteral()
{
    int ln = current().line;
    std::vector<ASTNodePtr> elements;
    
    if (!check(TokenType::RBRACKET)) {
        do {
            elements.push_back(parseExpr());
        } while (match(TokenType::COMMA));
    }
    
    expect(TokenType::RBRACKET, "Expected ']' after array elements");
    return std::make_unique<ASTNode>(ArrayLiteral{std::move(elements)}, ln);
}
```

#### Dictionary Literal Parsing (Lines 890-920)
```cpp
ASTNodePtr Parser::parseDictLiteral()
{
    int ln = current().line;
    std::vector<std::pair<ASTNodePtr, ASTNodePtr>> pairs;
    
    if (!check(TokenType::RBRACE)) {
        do {
            ASTNodePtr key = parseExpr();
            expect(TokenType::COLON, "Expected ':' after dictionary key");
            ASTNodePtr value = parseExpr();
            pairs.emplace_back(std::move(key), std::move(value));
        } while (match(TokenType::COMMA));
    }
    
    expect(TokenType::RBRACE, "Expected '}' after dictionary entries");
    return std::make_unique<ASTNode>(DictLiteral{std::move(pairs)}, ln);
}
```

#### Lambda Parsing (Lines 930-970)
```cpp
ASTNodePtr Parser::parseLambda()
{
    int ln = current().line;
    
    std::vector<std::string> params;
    if (match(TokenType::LPAREN)) {
        params = parseParamList();
        expect(TokenType::RPAREN, "Expected ')' after lambda parameters");
    } else if (check(TokenType::IDENTIFIER)) {
        params.push_back(consume().value);
    }
    
    expect(TokenType::FAT_ARROW, "Expected '=>' after lambda parameters");
    auto body = parseBodyOrStatement();
    
    return std::make_unique<ASTNode>(LambdaExpr{std::move(params), std::move(body)}, ln);
}
```

## Design Patterns and Architecture

### Pratt Parsing Pattern
```cpp
// Each precedence level follows this pattern:
ASTNodePtr parse<Level>() {
    ASTNodePtr left = parse<HigherLevel>();
    while (match(<operators>)) {
        Token op = tokens[pos - 1];
        ASTNodePtr right = parse<HigherLevel>();
        left = std::make_unique<ASTNode>(BinaryExpr{op.value, std::move(left), std::move(right)}, op.line);
    }
    return left;
}
```

**Benefits:**
- **Natural Precedence**: Easy to understand and modify
- **Extensible**: Simple to add new operators
- **Efficient**: No complex precedence tables
- **Clear Structure**: Each level has single responsibility

### Recursive Descent for Statements
```cpp
ASTNodePtr parseStatement() {
    switch (current().type) {
        case LET: return parseVarDecl(false);
        case CONST: return parseVarDecl(true);
        case IF: return parseIfStmt();
        // ... each statement type has its own parser
    }
}
```

**Benefits:**
- **Clear Structure**: Each statement type has dedicated parser
- **Error Recovery**: Isolated parsing for each statement
- **Maintainability**: Easy to add new statement types
- **Readability**: Straightforward control flow

### Factory Pattern for AST Nodes
```cpp
return std::make_unique<ASTNode>(BinaryExpr{op.value, std::move(left), std::move(right)}, op.line);
return std::make_unique<ASTNode>(VarDecl{isConst, name, std::move(initializer), typeHint}, ln);
```

**Benefits:**
- **Type Safety**: Compile-time checking of node types
- **Memory Management**: Automatic cleanup with smart pointers
- **Consistency**: Uniform node creation pattern
- **Extensibility**: Easy to add new node types

## Error Handling Strategy

### ParseError Generation
```cpp
throw ParseError("Expected ')' after expression", current().line, current().col);
```

**Error Information:**
- **Message**: Clear description of expected syntax
- **Position**: Precise line and column information
- **Context**: Current token value for debugging
- **Recovery**: Parser attempts to continue after errors

### Synchronization Points
```cpp
void Parser::skipNewlines() {
    while (check(TokenType::NEWLINE))
        consume();
}
```

**Recovery Strategy:**
- **Statement Boundaries**: Newlines serve as recovery points
- **Block Boundaries**: Braces provide clear synchronization
- **Declaration Boundaries**: Keywords help resynchronize parsing

## Performance Optimizations

### Move Semantics
```cpp
return std::make_unique<ASTNode>(BinaryExpr{op.value, std::move(left), std::move(right)}, op.line);
```

**Benefits:**
- **Reduced Allocations**: Avoids copying AST nodes
- **Better Cache Locality**: Sequential memory access
- **Faster Parsing**: Minimizes memory operations

### Efficient Token Access
```cpp
Token &current() { return tokens[pos]; }
```

**Benefits:**
- **Direct Access**: No bounds checking in hot paths
- **Reference Returns**: Avoids copying tokens
- **Inline Optimization**: Functions can be inlined by compiler

## Integration with Compiler Pipeline

### Token Stream Processing
```cpp
auto tokens = lexer.tokenize();
Parser parser(std::move(tokens));
auto ast = parser.parse();
```

### AST Construction
- **Tree Structure**: Hierarchical representation of program structure
- **Type Information**: Node types stored in variant for type safety
- **Position Information**: Source locations preserved for error reporting
- **Semantic Information**: Structure ready for semantic analysis

## Why This Design Works

### Hybrid Approach Benefits
- **Statement Clarity**: Recursive descent provides clear statement structure
- **Expression Flexibility**: Pratt parsing handles complex operator precedence
- **Maintainability**: Each parsing strategy optimized for its domain
- **Extensibility**: Easy to add new language features

### Error Handling Excellence
- **Precise Location**: Line and column for all errors
- **Context Information**: Rich error messages with expected vs actual
- **Recovery Mechanisms**: Attempts to continue parsing after errors
- **User Experience**: Helpful error messages for developers

### Performance Characteristics
- **Linear Time**: O(n) parsing for most constructs
- **Memory Efficiency**: Smart pointers and move semantics
- **Cache Friendly**: Sequential token processing
- **Optimizable**: Clear structure enables compiler optimizations

The parser provides a robust foundation for the Quantum Language compiler, handling the complexity of modern language features while maintaining excellent error handling and performance characteristics.
