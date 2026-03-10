# testFile() Function - Comprehensive File Testing Engine

## Overview
The `testFile()` function is the core testing engine that performs comprehensive validation of Quantum Language source files. It executes a two-phase testing process: syntax validation through parsing, followed by execution testing with output capture. The function intelligently distinguishes between genuine program errors and expected input-driven failures in test mode.

## Complete Source Code

```cpp
static TestResult testFile(const std::string &path)
{
    TestResult res;
    res.path = path;

    std::ifstream file(path);
    if (!file.is_open())
    {
        res.error = "Could not open file";
        return res;
    }

    res.source = std::string((std::istreambuf_iterator<char>(file)),
                             std::istreambuf_iterator<char>());
    file.close();

    // ── Phase 1: Parse-only — catches all syntax errors.
    try
    {
        Parser parser(res.source);
        auto ast = parser.parse();
    }
    catch (const QuantumError &e)
    {
        res.error = e.kind + ": " + e.what();
        res.line = e.line;
        res.col = e.col;
        return res;
    }
    catch (const std::exception &e)
    {
        res.error = "Fatal: " + std::string(e.what());
        return res;
    }

    // ── Phase 2: Execute with output swallowed ────────────────────────────
    std::ostringstream sink;
    StreamGuard guard(
        std::cout.rdbuf(sink.rdbuf()),
        std::cerr.rdbuf(sink.rdbuf()));

    auto setErr = [&](const std::string &kind, const std::string &msg, int ln)
    {
        if (isInputDrivenError(msg))
            return;
        res.error = kind + ": " + msg;
        res.line = ln;
    };

    try
    {
        Interpreter interp;
        interp.evaluate(*ast);

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
            const std::string msg = e.what();
            if (msg.find("'main'") == std::string::npos &&
                msg.find("\"main\"") == std::string::npos)
                setErr(e.kind, msg, e.line);
        }
        catch (const QuantumError &e)
        {
            setErr(e.kind, e.what(), e.line);
        }
        catch (const std::exception &e)
        {
            std::string msg = e.what();
            if (!isInputDrivenError(msg))
                res.error = "Fatal in main(): " + msg;
        }
    }
    catch (const QuantumError &e)
    {
        if (!isInputDrivenError(e.what()))
            res.error = "Fatal: " + e.what();
    }
    catch (const std::exception &e)
    {
        std::string msg = e.what();
        if (!isInputDrivenError(msg))
            res.error = "Fatal: " + msg;
    }
    catch (...)
    {
        res.error = "Fatal: Unknown exception";
    }

    return res;
}
```

## Line-by-Line Analysis

### **Function Signature and Initialization (Lines 1-8)**
```cpp
static TestResult testFile(const std::string &path)
{
    TestResult res;
    res.path = path;
```
- **Return Type**: `TestResult` struct containing comprehensive test information
- **Path Storage**: Stores file path for reporting
- **Default Initialization**: Other members remain default (empty error, line 0)

### **File Reading (Lines 10-18)**
```cpp
std::ifstream file(path);
if (!file.is_open())
{
    res.error = "Could not open file";
    return res;
}

res.source = std::string((std::istreambuf_iterator<char>(file)),
                         std::istreambuf_iterator<char>());
file.close();
```
- **File Access Validation**: Immediate check for file accessibility
- **Error Handling**: Sets error message and returns early on failure
- **Source Code Reading**: Efficient iterator-based file reading
- **Resource Management**: Explicit file close for deterministic cleanup

### **Phase 1: Syntax Validation (Lines 20-35)**
```cpp
// ── Phase 1: Parse-only — catches all syntax errors.
try
{
    Parser parser(res.source);
    auto ast = parser.parse();
}
catch (const QuantumError &e)
{
    res.error = e.kind + ": " + e.what();
    res.line = e.line;
    res.col = e.col;
    return res;
}
catch (const std::exception &e)
{
    res.error = "Fatal: " + std::string(e.what());
    return res;
}
```
- **Parse-Only Testing**: Validates syntax without execution
- **Early Exit**: Returns immediately on syntax errors
- **Comprehensive Error Capture**: Both language-specific and system errors
- **Structured Error Information**: Preserves error kind, message, and location

### **Phase 2: Execution Testing Setup (Lines 37-45)**
```cpp
// ── Phase 2: Execute with output swallowed ────────────────────────────
std::ostringstream sink;
StreamGuard guard(
    std::cout.rdbuf(sink.rdbuf()),
    std::cerr.rdbuf(sink.rdbuf()));

auto setErr = [&](const std::string &kind, const std::string &msg, int ln)
{
    if (isInputDrivenError(msg))
        return;
    res.error = kind + ": " + msg;
    res.line = ln;
};
```
- **Output Capture**: Redirects stdout/stderr to string stream
- **RAII Protection**: StreamGuard ensures buffer restoration
- **Error Filter**: Lambda function filters input-driven errors
- **Test Isolation**: Prevents test output from affecting console

### **Execution Phase (Lines 47-75)**
```cpp
try
{
    Interpreter interp;
    interp.evaluate(*ast);

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
    // ... exception handling for main() execution
}
```
- **Interpreter Creation**: Fresh interpreter for each test
- **AST Execution**: Executes parsed syntax tree
- **Main Function Detection**: Checks for and calls main() if present
- **Nested Exception Handling**: Separate handling for global vs main() errors

### **Exception Handling Strategy (Lines 77-95)**
```cpp
catch (const QuantumError &e)
{
    if (!isInputDrivenError(e.what()))
        res.error = "Fatal: " + e.what();
}
catch (const std::exception &e)
{
    std::string msg = e.what();
    if (!isInputDrivenError(msg))
        res.error = "Fatal: " + msg;
}
catch (...)
{
    res.error = "Fatal: Unknown exception";
}
```
- **Input Error Filtering**: Ignores expected input-driven failures
- **Comprehensive Coverage**: Catches all exception types
- **Unknown Exception Handling**: Catch-all for unexpected errors
- **Consistent Error Format**: All errors follow same structure

## Design Patterns and Architecture

### **Two-Phase Testing Pattern**
```cpp
// Phase 1: Syntax Validation
Parser parser(source);
auto ast = parser.parse();  // Throws on syntax errors

// Phase 2: Execution Testing
Interpreter interp;
interp.evaluate(*ast);     // Throws on runtime errors
```
- **Early Error Detection**: Syntax errors caught before execution
- **Clear Separation**: Different error types handled separately
- **Efficient Testing**: No execution if syntax is invalid
- **Comprehensive Coverage**: Both parse-time and run-time errors

### **Output Isolation Pattern**
```cpp
std::ostringstream sink;
StreamGuard guard(
    std::cout.rdbuf(sink.rdbuf()),
    std::cerr.rdbuf(sink.rdbuf())
);
// Test execution here - all output captured
// Guard destructor restores streams automatically
```
- **Test Isolation**: Prevents test output from cluttering console
- **RAII Protection**: Automatic stream restoration even with exceptions
- **Clean Testing Environment**: Each test runs in isolation
- **Performance**: No disk I/O during test execution

### **Error Classification Pattern**
```cpp
auto setErr = [&](const std::string &kind, const std::string &msg, int ln) {
    if (isInputDrivenError(msg))
        return;  // Ignore input-related errors in test mode
    res.error = kind + ": " + msg;
    res.line = ln;
};
```
- **Intelligent Filtering**: Distinguishes expected vs. real errors
- **Context Awareness**: Understands testing environment
- **Consistent Interface**: Unified error setting mechanism
- **Lambda Encapsulation**: Captures error handling logic

## Technical Considerations

### **Performance Characteristics**
```cpp
// Time complexity analysis
// Phase 1: O(n) - parse entire source
// Phase 2: O(m) - execute AST (m = operations)
// Overall: O(n + m) - linear in source size + execution complexity

// Space complexity: O(n) - source + AST in memory
```
- **Linear Parsing**: Single pass through source code
- **Memory Efficiency**: AST and source stored simultaneously
- **Early Exit**: Syntax errors prevent unnecessary execution
- **Optimized for Testing**: Designed for batch processing

### **Error Handling Strategy**
```cpp
// Exception hierarchy handling
try {
    // Test execution
}
catch (const QuantumError &e) {
    // Language-specific errors
}
catch (const std::exception &e) {
    // Standard library errors
}
catch (...) {
    // Unknown exceptions
}
```
- **Specific to General**: Handle specific types first
- **Complete Coverage**: All possible exception types
- **Input Error Filtering**: Applied consistently across all handlers
- **Structured Information**: Preserves error context

### **Memory Management**
```cpp
// Resource management
{
    std::ifstream file(path);        // RAII file handle
    std::ostringstream sink;         // Output capture
    StreamGuard guard(...);         // Stream restoration
    Interpreter interp;             // Interpreter instance
    // All resources cleaned up automatically
}
```
- **RAII Throughout**: All resources managed by destructors
- **No Memory Leaks**: Automatic cleanup on all paths
- **Exception Safety**: Resources cleaned up even with exceptions
- **Stack Allocation**: Most objects created on stack

## Usage Patterns

### **Batch Testing**
```cpp
std::vector<TestResult> runTestSuite(const std::vector<std::string> &files) {
    std::vector<TestResult> results;
    
    for (const auto &file : files) {
        TestResult result = testFile(file);
        results.push_back(result);
        
        if (!result.error.empty()) {
            std::cout << "Failed: " << file << "\n";
        }
    }
    
    return results;
}
```

### **Continuous Integration**
```cpp
int runCI() {
    std::vector<std::string> testFiles = collectTestFiles("examples/");
    
    int failedCount = 0;
    for (const auto &file : testFiles) {
        TestResult result = testFile(file);
        if (!result.error.empty()) {
            failedCount++;
            logFailure(result);
        }
    }
    
    return failedCount > 0 ? 1 : 0;  // CI exit code
}
```

### **Development Testing**
```cpp
void quickTest(const std::string &filename) {
    TestResult result = testFile(filename);
    
    if (result.error.empty()) {
        std::cout << "✓ " << filename << " passed\n";
    } else {
        std::cout << "✗ " << filename << " failed\n";
        std::cout << "Error: " << result.error << "\n";
        
        if (result.line > 0) {
            std::string sourceLine = getSourceLine(result.source, result.line);
            std::cout << "Line " << result.line << ": " << sourceLine << "\n";
        }
    }
}
```

## Error Classification Examples

### **Syntax Errors (Phase 1)**
```cpp
// Invalid syntax
"print x +"  // Unexpected end of input
// Result: Parse error, immediate failure
```

### **Runtime Errors (Phase 2)**
```cpp
// Division by zero
"x = 1 / 0"  // Runtime error during execution
// Result: Runtime error, test failure
```

### **Input-Driven Errors (Ignored)**
```cpp
// Input function with redirected stdin
"name = input('Enter name: ')"  // Returns empty string
// Result: No error, test passes
```

### **Expected Behavior**
```cpp
// Valid program
"print 'Hello, World!'"  // Executes successfully
// Result: No error, test passes
```

## Advanced Features

### **Performance Metrics**
```cpp
struct EnhancedTestResult : TestResult {
    std::chrono::milliseconds parseTime;
    std::chrono::milliseconds executionTime;
    size_t memoryUsage;
    size_t astNodeCount;
};

static EnhancedTestResult testFileWithMetrics(const std::string &path) {
    EnhancedTestResult res;
    auto start = std::chrono::high_resolution_clock::now();
    
    // ... existing test logic ...
    
    auto end = std::chrono::high_resolution_clock::now();
    res.executionTime = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    return res;
}
```

### **Test Categories**
```cpp
enum class TestCategory {
    SYNTAX,      // Syntax validation only
    EXECUTION,   // Full execution testing
    PERFORMANCE,  // Performance benchmarking
    INTEGRATION  // Integration testing
};

static TestResult testFileWithCategory(const std::string &path, TestCategory category) {
    switch (category) {
        case TestCategory::SYNTAX:
            return testSyntaxOnly(path);
        case TestCategory::EXECUTION:
            return testFile(path);  // Existing implementation
        case TestCategory::PERFORMANCE:
            return testPerformance(path);
        case TestCategory::INTEGRATION:
            return testIntegration(path);
    }
}
```

### **Parallel Testing**
```cpp
#include <future>
#include <thread>

std::vector<TestResult> runParallelTests(const std::vector<std::string> &files) {
    std::vector<std::future<TestResult>> futures;
    
    for (const auto &file : files) {
        futures.push_back(std::async(std::launch::async, [file]() {
            return testFile(file);
        }));
    }
    
    std::vector<TestResult> results;
    for (auto &future : futures) {
        results.push_back(future.get());
    }
    
    return results;
}
```

## Why This Design Works

### **Comprehensiveness**
- **Two-Phase Testing**: Catches both syntax and runtime errors
- **Complete Error Coverage**: Handles all exception types
- **Context Preservation**: Maintains error location and source code
- **Intelligent Filtering**: Distinguishes expected vs. real errors

### **Reliability**
- **Exception Safe**: Robust error handling throughout
- **Resource Management**: RAII ensures proper cleanup
- **Isolation**: Each test runs in clean environment
- **Deterministic**: Same input produces same results

### **Efficiency**
- **Early Exit**: Syntax errors prevent unnecessary execution
- **Linear Performance**: Optimal for batch processing
- **Memory Efficient**: Minimal memory overhead per test
- **Parallel Ready**: Can be easily parallelized

### **Maintainability**
- **Clear Structure**: Two distinct phases
- **Modular Design**: Separate concerns for parsing and execution
- **Extensible**: Easy to add new test types
- **Well-Documented**: Clear intent and behavior

The `testFile()` function demonstrates exceptional software engineering design, providing a comprehensive, reliable, and efficient testing engine that perfectly serves the needs of the Quantum Language development workflow while maintaining clean separation of concerns and robust error handling.
