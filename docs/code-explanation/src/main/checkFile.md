# checkFile() Function - Syntax Validation Engine

## Overview
The `checkFile()` function provides syntax-only validation for Quantum Language source files. It parses files without executing them, making it ideal for quick syntax checking, IDE integration, and pre-commit validation. The function focuses exclusively on detecting syntax errors and structural issues.

## Complete Source Code

```cpp
static int checkFile(const std::string &path)
{
    std::ifstream file(path);
    if (!file.is_open())
    {
        std::cerr << Colors::RED << Colors::BOLD
                  << "\n  X Error: Could not open file '" << path << "'" << Colors::RESET;
        return 1;
    }

    std::string source((std::istreambuf_iterator<char>(file)),
                      std::istreambuf_iterator<char>());
    file.close();

    try
    {
        // Parse only - don't execute
        Parser parser(source);
        auto ast = parser.parse();
        
        std::cout << Colors::GREEN << Colors::BOLD
                  << "\n  ✓ Syntax check passed: " << path << Colors::RESET << "\n";
        return 0;
    }
    catch (const QuantumError &e)
    {
        std::cerr << Colors::RED << Colors::BOLD
                  << "\n  X " << e.kind << Colors::RESET;
        if (e.line > 0)
            std::cerr << " (line " << e.line << ")";
        std::cerr << Colors::RESET << "\n  " << e.what() << "\n";
        return 1;
    }
    catch (const std::exception &e)
    {
        std::cerr << Colors::RED << Colors::BOLD
                  << "\n  X Fatal: " << Colors::RESET << e.what() << "\n";
        return 1;
    }
}
```

## Line-by-Line Analysis

### **Function Signature (Line 1)**
```cpp
static int checkFile(const std::string &path)
```
- **`static`**: Internal linkage - only visible within this translation unit
- **`int`**: Returns exit code (0 for success, 1 for failure)
- **`checkFile`**: Descriptive name indicating syntax validation purpose
- **`const std::string &path`**: Constant reference to file path for efficiency

### **File Opening (Lines 3-9)**
```cpp
std::ifstream file(path);
if (!file.is_open())
{
    std::cerr << Colors::RED << Colors::BOLD
              << "\n  X Error: Could not open file '" << path << "'" << Colors::RESET;
    return 1;
}
```

**File Handling Analysis:**
- **Immediate Validation**: Checks file accessibility before processing
- **Error Return**: Returns 1 (failure) on file access issues
- **Consistent Error Format**: Same error display as runFile()
- **User Feedback**: Clear indication of file access failure

### **Source Code Reading (Lines 11-15)**
```cpp
std::string source((std::istreambuf_iterator<char>(file)),
                  std::istreambuf_iterator<char>());
file.close();
```

**Reading Strategy:**
- **Efficient Reading**: Iterator-based reading for entire file
- **Memory Efficiency**: Single allocation for source code
- **Resource Management**: Explicit file close for deterministic cleanup
- **Consistent Approach**: Same reading method as runFile()

### **Parse-Only Validation (Lines 17-25)**
```cpp
try
{
    // Parse only - don't execute
    Parser parser(source);
    auto ast = parser.parse();
    
    std::cout << Colors::GREEN << Colors::BOLD
              << "\n  ✓ Syntax check passed: " << path << Colors::RESET << "\n";
    return 0;
}
```

**Validation Logic:**
- **Parser Construction**: Creates Parser object with source code
- **AST Generation**: Calls parse() to build Abstract Syntax Tree
- **No Execution**: Interpreter is never created or used
- **Success Message**: Green checkmark with file path on success
- **Success Return**: Returns 0 (success) on successful parsing

### **Error Handling (Lines 27-39)**
```cpp
catch (const QuantumError &e)
{
    std::cerr << Colors::RED << Colors::BOLD
              << "\n  X " << e.kind << Colors::RESET;
    if (e.line > 0)
        std::cerr << " (line " << e.line << ")";
    std::cerr << Colors::RESET << "\n  " << e.what() << "\n";
    return 1;
}
catch (const std::exception &e)
{
    std::cerr << Colors::RED << Colors::BOLD
              << "\n  X Fatal: " << Colors::RESET << e.what() << "\n";
    return 1;
}
```

**Exception Strategy:**
- **Specific Handling**: QuantumError for language-specific errors
- **Generic Handling**: std::exception for system-level errors
- **Consistent Format**: Same error display as other functions
- **Error Return**: Returns 1 (failure) on any exception
- **Line Information**: Includes line numbers when available

## Design Patterns and Architecture

### **Parse-Only Architecture**
```
Source File → File Read → Parser → AST → Success/Failure
                                    ↓
                            No Execution Phase
```
- **Single Purpose**: Only validates syntax, no runtime behavior
- **Fast Execution**: No interpreter overhead
- **IDE Friendly**: Suitable for real-time syntax checking
- **Lightweight**: Minimal resource usage

### **Error Handling Pattern**
```
Try-Catch Structure:
1. File Operations (File Access)
2. Parse Operations (QuantumError)
3. System Errors (std::exception)
```
- **Layered Defense**: Multiple exception handling layers
- **Consistent Interface**: Same error format across all functions
- **Clear Return Codes**: 0 for success, 1 for failure
- **Informative Output**: Error types and locations provided

### **Resource Management**
- **RAII**: File stream auto-cleanup
- **Stack Allocation**: Parser and AST on stack
- **Deterministic Cleanup**: Explicit file.close()
- **Exception Safety**: No resource leaks on errors

## Technical Considerations

### **Performance**
- **Fast Validation**: No execution overhead
- **Single Pass**: Parse once, validate immediately
- **Memory Efficient**: AST built then discarded
- **Minimal I/O**: Only file reading, no output operations

### **Memory Usage**
- **Source String**: Entire file in memory
- **AST Nodes**: Temporary during parsing
- **Parser Object**: Small stack footprint
- **Automatic Cleanup**: All resources released on function exit

### **Error Detection**
- **Syntax Errors**: Invalid grammar, missing tokens
- **Structural Errors**: Invalid AST construction
- **File Errors**: Access permissions, missing files
- **System Errors**: Memory allocation, I/O failures

## Usage Scenarios

### **Command Line Usage**
```bash
quantum --check script.sa
```
- Quick syntax validation
- Returns appropriate exit codes
- Suitable for scripting and automation

### **IDE Integration**
```cpp
// In IDE plugin
if (checkFile(filename) == 0) {
    // Syntax is valid
} else {
    // Show error markers
}
```
- Real-time syntax checking
- Error highlighting
- Linting integration

### **CI/CD Pipeline**
```bash
# In build script
quantum --check src/**/*.sa
if [ $? -ne 0 ]; then
    echo "Syntax errors found"
    exit 1
fi
```
- Pre-commit validation
- Automated testing
- Quality gates

### **Development Workflow**
```bash
# During development
quantum --check new_feature.sa
quantum new_feature.sa  # Run if syntax is valid
```
- Rapid development cycle
- Early error detection
- Iterative development

## Comparison with runFile()

| Aspect | checkFile() | runFile() |
|--------|-------------|-----------|
| **Purpose** | Syntax validation | Full execution |
| **Speed** | Fast (parse only) | Slower (parse + execute) |
| **Side Effects** | None | May produce output, modify state |
| **Error Types** | Syntax only | Syntax + runtime |
| **Resource Usage** | Minimal | Higher (interpreter) |
| **Use Cases** | Linting, CI/CD | Running programs |

## Why This Design Works

### **Efficiency**
- **Parse-Only Focus**: No unnecessary execution overhead
- **Fast Feedback**: Immediate syntax validation
- **Resource Conscious**: Minimal memory and CPU usage
- **Scalable**: Can check many files quickly

### **Reliability**
- **Comprehensive Error Detection**: Catches all syntax issues
- **Consistent Interface**: Same error format as other tools
- **Clear Return Codes**: Standard Unix exit conventions
- **Robust Error Handling**: All exception cases covered

### **Integration**
- **CLI Friendly**: Suitable for command-line usage
- **IDE Compatible**: Works well with editor integration
- **Automation Ready**: Perfect for CI/CD pipelines
- **Standard Interface**: Follows common tool conventions

### **User Experience**
- **Clear Success Indicator**: Green checkmark on success
- **Detailed Error Messages**: Line numbers and descriptions
- **Fast Feedback**: Quick validation results
- **Professional Output**: Consistent, colored formatting

The `checkFile()` function demonstrates excellent tool design principles, providing a focused, efficient, and reliable syntax validation engine that integrates perfectly into development workflows while maintaining consistency with the broader Quantum Language toolchain.
