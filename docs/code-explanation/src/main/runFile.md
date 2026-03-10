# runFile() Function - Script File Execution Engine

## Overview
The `runFile()` function is the core script execution engine for Quantum Language. It handles reading `.sa` files from disk, parsing them into an Abstract Syntax Tree (AST), and executing them through the interpreter. It provides comprehensive error handling and supports both scripts with and without `main()` functions.

## Complete Source Code

```cpp
static void runFile(const std::string &path)
{
    std::ifstream file(path);
    if (!file.is_open())
    {
        std::cerr << Colors::RED << Colors::BOLD
                  << "\n  X Error: Could not open file '" << path << "'" << Colors::RESET;
        return;
    }

    std::string source((std::istreambuf_iterator<char>(file)),
                      std::istreambuf_iterator<char>());
    file.close();

    try
    {
        // Parse the source code into an AST
        Parser parser(source);
        auto ast = parser.parse();

        // Execute the AST
        Interpreter interp;
        interp.evaluate(*ast);

        // If there's a main() function, call it
        try
        {
            auto mainFn = interp.globals->get("main");
            if (mainFn.isFunction() &&
                std::holds_alternative<std::shared_ptr<QuantumFunction>>(mainFn.data))
            {
                CallExpr ce;
                ce.callee = std::make_unique<ASTNode>(Identifier{"main"}, 0);
                ASTNode callNode(std::move(ce), 0);
                interp.evaluate(callNode);
            }
        }
        catch (const ReturnSignal &)
        {
            // Normal return from main() — not an error.
        }
        catch (const NameError &e)
        {
            // Only swallow "Undefined variable: 'main'" (no main() defined).
            // Any other NameError thrown inside main() is a real bug.
            const std::string msg = e.what();
            if (msg.find("'main'") == std::string::npos &&
                msg.find("\"main\"") == std::string::npos)
            {
                std::cerr << Colors::RED << Colors::BOLD
                          << "\n  X " << e.kind << Colors::RESET;
                if (e.line > 0)
                    std::cerr << " (line " << e.line << ")";
                std::cerr << Colors::RESET << "\n  " << e.what() << "\n";
            }
        }
        catch (const QuantumError &e)
        {
            // All runtime errors inside main() are real failures.
            std::cerr << Colors::RED << Colors::BOLD
                      << "\n  X " << e.kind << Colors::RESET;
            if (e.line > 0)
                std::cerr << " (line " << e.line << ")";
            std::cerr << Colors::RESET << "\n  " << e.what() << "\n";
        }
    }
    catch (const QuantumError &e)
    {
        std::cerr << Colors::RED << Colors::BOLD
                  << "\n  X " << e.kind << Colors::RESET;
        if (e.line > 0)
            std::cerr << " (line " << e.line << ")";
        std::cerr << Colors::RESET << "\n  " << e.what() << "\n";
    }
    catch (const std::exception &e)
    {
        std::cerr << Colors::RED << Colors::BOLD
                  << "\n  X Fatal: " << Colors::RESET << e.what() << "\n";
    }
}
```

## Line-by-Line Analysis

### **Function Signature (Line 1)**
```cpp
static void runFile(const std::string &path)
```
- **`static`**: Internal linkage - only visible within this translation unit
- **`void`**: No return value - errors are printed to stderr
- **`runFile`**: Descriptive name indicating file execution purpose
- **`const std::string &path`**: Constant reference to file path for efficiency

### **File Opening (Lines 3-9)**
```cpp
std::ifstream file(path);
if (!file.is_open())
{
    std::cerr << Colors::RED << Colors::BOLD
              << "\n  X Error: Could not open file '" << path << "'" << Colors::RESET;
    return;
}
```

**File Handling Analysis:**
- **`std::ifstream`**: Input file stream for reading
- **Error Checking**: Immediate validation of file accessibility
- **User-Friendly Error**: Colored, formatted error message with file path
- **Early Return**: Graceful exit on file access failure
- **Color Coding**: Red bold text for error visibility

### **Source Code Reading (Lines 11-15)**
```cpp
std::string source((std::istreambuf_iterator<char>(file)),
                  std::istreambuf_iterator<char>());
file.close();
```

**Reading Strategy:**
- **Efficient Reading**: Uses iterator-based reading for entire file
- **Single Allocation**: Reads entire file into memory at once
- **Automatic String Construction**: String built from iterator range
- **Resource Management**: Explicit file close for deterministic cleanup

### **Parse-Execute Phase (Lines 17-26)**
```cpp
try
{
    // Parse the source code into an AST
    Parser parser(source);
    auto ast = parser.parse();

    // Execute the AST
    Interpreter interp;
    interp.evaluate(*ast);
```

**Two-Phase Execution:**
1. **Parsing Phase**: Source code → Abstract Syntax Tree
2. **Execution Phase**: AST → Runtime behavior
- **Exception Safety**: Wrapped in try-catch for error handling
- **Parser Object**: Created on stack, automatic cleanup
- **Interpreter Object**: Fresh interpreter for each file
- **AST Dereference**: `*ast` since parse() returns unique_ptr

### **Main Function Detection (Lines 28-38)**
```cpp
// If there's a main() function, call it
try
{
    auto mainFn = interp.globals->get("main");
    if (mainFn.isFunction() &&
        std::holds_alternative<std::shared_ptr<QuantumFunction>>(mainFn.data))
    {
        CallExpr ce;
        ce.callee = std::make_unique<ASTNode>(Identifier{"main"}, 0);
        ASTNode callNode(std::move(ce), 0);
        interp.evaluate(callNode);
    }
}
```

**Main Function Logic:**
- **Global Scope Lookup**: Searches for "main" in global namespace
- **Type Checking**: Verifies it's a function, not a variable
- **Variant Checking**: Uses `std::holds_alternative` for type safety
- **Call Construction**: Builds CallExpr AST node dynamically
- **Execution**: Evaluates the constructed call expression

### **Return Signal Handling (Lines 40-43)**
```cpp
catch (const ReturnSignal &)
{
    // Normal return from main() — not an error.
}
```

**Return Signal Analysis:**
- **Exception as Control Flow**: ReturnSignal used for function returns
- **Silent Catching**: Normal program termination, not an error
- **No Logging**: Return from main() is expected behavior

### **NameError Handling (Lines 45-58)**
```cpp
catch (const NameError &e)
{
    // Only swallow "Undefined variable: 'main'" (no main() defined).
    // Any other NameError thrown inside main() is a real bug.
    const std::string msg = e.what();
    if (msg.find("'main'") == std::string::npos &&
        msg.find("\"main\"") == std::string::npos)
    {
        std::cerr << Colors::RED << Colors::BOLD
                  << "\n  X " << e.kind << Colors::RESET;
        if (e.line > 0)
            std::cerr << " (line " << e.line << ")";
        std::cerr << Colors::RESET << "\n  " << e.what() << "\n";
    }
}
```

**NameError Strategy:**
- **Selective Handling**: Only ignores "main not defined" errors
- **String Matching**: Checks error message for main-related content
- **Error Reporting**: Full error details for real NameErrors
- **Line Information**: Includes line number when available

### **QuantumError Handling (Lines 60-67)**
```cpp
catch (const QuantumError &e)
{
    // All runtime errors inside main() are real failures.
    std::cerr << Colors::RED << Colors::BOLD
              << "\n  X " << e.kind << Colors::RESET;
    if (e.line > 0)
        std::cerr << " (line " << e.line << ")";
    std::cerr << Colors::RESET << "\n  " << e.what() << "\n";
}
```

**Runtime Error Handling:**
- **No Filtering**: All QuantumErrors are reported
- **Structured Information**: Error kind, line number, and message
- **Consistent Formatting**: Same error display format as other errors
- **Line Context**: Provides location information for debugging

### **Top-Level Exception Handling (Lines 69-76)**
```cpp
catch (const QuantumError &e)
{
    // Parse-time or global execution errors
    std::cerr << Colors::RED << Colors::BOLD
              << "\n  X " << e.kind << Colors::RESET;
    if (e.line > 0)
        std::cerr << " (line " << e.line << ")";
    std::cerr << Colors::RESET << "\n  " << e.what() << "\n";
}
catch (const std::exception &e)
{
    std::cerr << Colors::RED << Colors::BOLD
              << "\n  X Fatal: " << Colors::RESET << e.what() << "\n";
}
```

**Exception Hierarchy:**
- **Specific First**: QuantumError caught before generic std::exception
- **Fatal Errors**: Generic exceptions marked as "Fatal"
- **Consistent Display**: All errors follow same visual format
- **Complete Coverage**: Catches all possible exception types

## Design Patterns and Architecture

### **Two-Phase Execution Model**
```
Source Code → Parser → AST → Interpreter → Execution
                    ↓
              Main Function Call (if exists)
```
- **Separation of Concerns**: Parsing and execution are distinct phases
- **Error Isolation**: Parse errors caught before execution begins
- **Optional Main**: Scripts can run without main() function
- **Clean State**: Fresh interpreter for each file

### **Exception Handling Strategy**
```
Try-Catch Layers:
1. Parse/Execute (QuantumError)
2. Main Function Call (ReturnSignal, NameError, QuantumError)
3. Top-Level (QuantumError, std::exception)
```
- **Layered Defense**: Multiple exception handling layers
- **Selective Filtering**: Some errors ignored (missing main)
- **Context Preservation**: Line numbers and error types maintained
- **User-Friendly Output**: Colored, formatted error messages

### **Resource Management**
- **RAII**: File streams and parser objects auto-cleanup
- **Smart Pointers**: AST managed with unique_ptr
- **Deterministic Cleanup**: Explicit file.close() for clarity
- **Exception Safety**: Resources cleaned up even on errors

## Technical Considerations

### **Performance**
- **Single File Read**: Entire file read at once for efficiency
- **In-Memory Parsing**: AST built completely before execution
- **Stack Allocation**: Most objects created on stack
- **Minimal Dynamic Allocation**: Only AST and source string

### **Memory Management**
- **Automatic Cleanup**: RAII handles most resource management
- **Smart Pointers**: AST ownership clearly defined
- **Exception Safety**: No memory leaks even with exceptions
- **Efficient String Handling**: Move semantics where possible

### **Error Reporting**
- **Structured Information**: Error type, location, message
- **Visual Hierarchy**: Color coding and formatting
- **Context Preservation**: Line numbers maintained through parsing
- **User-Friendly**: Clear, actionable error messages

## Usage Scenarios

### **Simple Script Execution**
```bash
quantum hello.sa
```
- File read, parsed, executed
- No main() function needed
- Global code runs immediately

### **Program with Main Function**
```bash
quantum program.sa
```
- Global code executes first
- Then main() function called
- Return from main() handled gracefully

### **Error Handling**
- **File Not Found**: Clear error with file path
- **Syntax Errors**: Line number and error description
- **Runtime Errors**: Context and stack information
- **Missing Main**: Treated as normal, not an error

## Why This Design Works

### **Robustness**
- **Comprehensive Error Handling**: All error cases covered
- **Graceful Degradation**: Missing main() not treated as failure
- **Resource Safety**: RAII ensures proper cleanup
- **Exception Safety**: Strong exception guarantee

### **Flexibility**
- **Optional Main**: Supports both script and program styles
- **Clear Separation**: Parsing and execution are distinct
- **Extensible**: Easy to add new error types or phases
- **Maintainable**: Well-structured, documented code

### **User Experience**
- **Clear Error Messages**: Formatted, colored, informative
- **Line Numbers**: Precise error location information
- **Consistent Interface**: Same behavior for all file types
- **Professional Appearance**: Polished error reporting

The `runFile()` function demonstrates exceptional software engineering practices, providing a robust, flexible, and user-friendly script execution engine that handles all edge cases gracefully while maintaining clean separation of concerns and comprehensive error reporting.
