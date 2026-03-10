# runTestExamples() Function - Comprehensive Test Suite Executor

## Overview
The `runTestExamples()` function is the main test suite executor that orchestrates comprehensive testing of all Quantum Language source files in a directory tree. It combines file discovery, batch execution, result aggregation, and detailed reporting into a single, automated testing workflow that provides both immediate console feedback and persistent test result documentation.

## Complete Source Code

```cpp
static int runTestExamples(const std::string &examplesDir)
{
    fs::path dir(examplesDir);
    if (!fs::exists(dir) || !fs::is_directory(dir))
    {
        std::cerr << Colors::RED << "[Error] " << Colors::RESET
                  << "Directory not found: " << examplesDir << "\n";
        return 1;
    }

    // Redirect stdin → NUL/dev/null so programs that call input() / cin
    // don't hang waiting for keyboard input.
    redirectStdinToNull();
    g_testMode = true;

    // Collect & sort files
    std::vector<fs::path> files;
    collectSaFiles(dir, files);

    if (files.empty())
    {
        std::cout << Colors::YELLOW << "[Warning] " << Colors::RESET
                  << "No .sa files found in " << examplesDir << "\n";
        return 0;
    }

    std::sort(files.begin(), files.end());

    // Console header
    std::cout << Colors::CYAN << Colors::BOLD
              << "\n══════════════════════════════════════════════════\n"
              << "  QUANTUM LANGUAGE TEST SUITE\n"
              << "══════════════════════════════════════════════════\n"
              << Colors::RESET;

    std::cout << Colors::CYAN << "  Directory: " << Colors::YELLOW 
              << fs::absolute(dir).string() << Colors::RESET << "\n";
    std::cout << Colors::CYAN << "  Files found: " << Colors::YELLOW 
              << files.size() << Colors::RESET << "\n\n";

    // Run every file
    std::vector<TestResult> failures;
    int passed = 0;

    for (const auto &filePath : files)
    {
        std::string pathStr = filePath.string();
        std::string displayPath = fs::relative(filePath, dir).string();

        std::cout << Colors::WHITE << "Testing: " << Colors::YELLOW 
                  << displayPath << Colors::RESET << " ... ";

        TestResult tr = testFile(pathStr);
        tr.path = displayPath;

        if (tr.error.empty())
        {
            std::cout << Colors::GREEN << Colors::BOLD << "✓ PASS" 
                      << Colors::RESET << "\n";
            passed++;
        }
        else
        {
            std::cout << Colors::RED << Colors::BOLD << "✗ FAIL" 
                      << Colors::RESET << "\n";

            // Show the offending source line with a caret pointer
            if (tr.line > 0)
            {
                std::string srcLine = getSourceLine(tr.source, tr.line);
                if (!srcLine.empty())
                {
                    std::cout << Colors::RED << "  " << tr.line << ": " 
                              << srcLine << Colors::RESET << "\n";
                    if (tr.col > 0)
                    {
                        int spaces = 4 + tr.col - 1; // "  N: " offset
                        std::cout << std::string(spaces, ' ') << "^\n";
                    }
                }
            }

            std::cout << Colors::RED << "  Error: " << tr.error 
                      << Colors::RESET << "\n";

            failures.push_back(tr);
        }
    }

    int total = static_cast<int>(files.size());
    int failed = static_cast<int>(failures.size());

    // Console summary
    std::cout << Colors::CYAN << Colors::BOLD
              << "\n══════════════════════════════════════════════════\n"
              << Colors::RESET;

    if (failed == 0)
    {
        std::cout << Colors::GREEN << Colors::BOLD
                  << "  ✓ All " << total << " file(s) passed!\n"
                  << Colors::RESET;
    }
    else
    {
        std::cout << Colors::GREEN << "  Passed : " << passed << " / " << total << "\n"
                  << Colors::RESET
                  << Colors::RED << "  Failed : " << failed << " / " << total << "\n"
                  << Colors::RESET;
    }

    std::cout << Colors::CYAN << Colors::BOLD
              << "══════════════════════════════════════════════════\n\n"
              << Colors::RESET;

    // ── Write test_results.txt ────────────────────────────────────────────
    const std::string reportPath = "test_results.txt";
    std::ofstream report(reportPath);

    if (!report.is_open())
    {
        std::cerr << Colors::RED << "[Error] " << Colors::RESET
                  << "Could not write report to: " << reportPath << "\n";
        return failed > 0 ? 1 : 0;
    }

    report << "================================================================================\n";
    report << "  Quantum Language — Test Results\n";
    report << "================================================================================\n";
    report << "  Directory : " << fs::absolute(dir).string() << "\n";
    report << "  Total     : " << total << "\n";
    report << "  Passed    : " << passed << "\n";
    report << "  Failed    : " << failed << "\n";
    report << "================================================================================\n\n";

    if (failures.empty())
    {
        report << "All files passed — no errors found.\n";
    }
    else
    {
        for (size_t i = 0; i < failures.size(); ++i)
        {
            const auto &f = failures[i];

            report << "################################################################################\n";
            report << "  FAILED FILE #" << (i + 1) << "\n";
            report << "################################################################################\n";
            report << "  Path  : " << f.path << "\n";
            report << "  Error : " << f.error << "\n";
            if (f.line > 0)
            {
                report << "  Line  : " << f.line;
                if (f.col > 0)
                    report << ":" << f.col;
                report << "\n";
                std::string srcLine = getSourceLine(f.source, f.line);
                if (!srcLine.empty())
                {
                    report << "  Code  : " << srcLine << "\n";
                    if (f.col > 0)
                    {
                        int lead = 0;
                        while (lead < (int)srcLine.size() &&
                               (srcLine[lead] == ' ' || srcLine[lead] == '\t'))
                            ++lead;
                        int caretPos = 10 + f.col - 1; // "  Code  : " is 10 chars
                        report << std::string(caretPos, ' ') << "^\n";
                    }
                }
            }
            report << "--------------------------------------------------------------------------------\n";
            report << "  Source Code:\n";
            report << "--------------------------------------------------------------------------------\n";
            report << f.source << "\n";
            report << "################################################################################\n\n";
        }
    }

    report.close();

    std::cout << Colors::CYAN << "  Report saved to: "
              << Colors::YELLOW << reportPath << Colors::RESET << "\n\n";

    return failed > 0 ? 1 : 0;
}
```

## Line-by-Line Analysis

### **Function Signature and Validation (Lines 1-12)**
```cpp
static int runTestExamples(const std::string &examplesDir)
{
    fs::path dir(examplesDir);
    if (!fs::exists(dir) || !fs::is_directory(dir))
    {
        std::cerr << Colors::RED << "[Error] " << Colors::RESET
                  << "Directory not found: " << examplesDir << "\n";
        return 1;
    }
```
- **Return Type**: `int` - exit code (0 for success, 1 for failure)
- **Path Validation**: Checks directory existence and type
- **Error Reporting**: Colored, formatted error message
- **Early Exit**: Returns failure code for invalid directory

### **Test Environment Setup (Lines 14-17)**
```cpp
redirectStdinToNull();
g_testMode = true;
```
- **Input Redirection**: Prevents blocking on input() calls
- **Test Mode Flag**: Enables test-specific behavior in interpreter
- **Global State**: Sets up environment for all subsequent tests
- **Non-Blocking**: Ensures automated testing doesn't hang

### **File Collection and Validation (Lines 19-31)**
```cpp
std::vector<fs::path> files;
collectSaFiles(dir, files);

if (files.empty())
{
    std::cout << Colors::YELLOW << "[Warning] " << Colors::RESET
              << "No .sa files found in " << examplesDir << "\n";
    return 0;
}

std::sort(files.begin(), files.end());
```
- **Recursive Discovery**: Finds all .sa files in directory tree
- **Empty Check**: Handles case with no test files
- **Deterministic Order**: Sorts for consistent test execution
- **Success Return**: No files is not an error condition

### **Console Header Display (Lines 33-42)**
```cpp
std::cout << Colors::CYAN << Colors::BOLD
          << "\n══════════════════════════════════════════════════\n"
          << "  QUANTUM LANGUAGE TEST SUITE\n"
          << "══════════════════════════════════════════════════\n"
          << Colors::RESET;

std::cout << Colors::CYAN << "  Directory: " << Colors::YELLOW 
          << fs::absolute(dir).string() << Colors::RESET << "\n";
std::cout << Colors::CYAN << "  Files found: " << Colors::YELLOW 
          << files.size() << Colors::RESET << "\n\n";
```
- **Professional Header**: Unicode box drawing with title
- **Test Information**: Directory path and file count
- **Color Coding**: Cyan for structure, yellow for data
- **Visual Separation**: Clear start of test execution

### **Test Execution Loop (Lines 44-82)**
```cpp
std::vector<TestResult> failures;
int passed = 0;

for (const auto &filePath : files)
{
    std::string pathStr = filePath.string();
    std::string displayPath = fs::relative(filePath, dir).string();

    std::cout << Colors::WHITE << "Testing: " << Colors::YELLOW 
              << displayPath << Colors::RESET << " ... ";

    TestResult tr = testFile(pathStr);
    tr.path = displayPath;

    if (tr.error.empty())
    {
        std::cout << Colors::GREEN << Colors::BOLD << "✓ PASS" 
                  << Colors::RESET << "\n";
        passed++;
    }
    else
    {
        // Error reporting and failure collection
    }
}
```
- **Progress Tracking**: Shows current file being tested
- **Relative Paths**: Displays paths relative to test directory
- **Immediate Feedback**: Pass/fail shown immediately
- **Failure Collection**: Stores detailed failure information

### **Error Display Logic (Lines 58-79)**
```cpp
if (tr.line > 0)
{
    std::string srcLine = getSourceLine(tr.source, tr.line);
    if (!srcLine.empty())
    {
        std::cout << Colors::RED << "  " << tr.line << ": " 
                  << srcLine << Colors::RESET << "\n";
        if (tr.col > 0)
        {
            int spaces = 4 + tr.col - 1; // "  N: " offset
            std::cout << std::string(spaces, ' ') << "^\n";
        }
    }
}

std::cout << Colors::RED << "  Error: " << tr.error 
          << Colors::RESET << "\n";
```
- **Context Display**: Shows source line where error occurred
- **Error Pointer**: Caret points to exact error location
- **Color Coding**: Red for errors, clear visibility
- **Detailed Information**: Line number, source, and error message

### **Console Summary (Lines 84-103)**
```cpp
int total = static_cast<int>(files.size());
int failed = static_cast<int>(failures.size());

std::cout << Colors::CYAN << Colors::BOLD
          << "\n══════════════════════════════════════════════════\n"
          << Colors::RESET;

if (failed == 0)
{
    std::cout << Colors::GREEN << Colors::BOLD
              << "  ✓ All " << total << " file(s) passed!\n"
              << Colors::RESET;
}
else
{
    std::cout << Colors::GREEN << "  Passed : " << passed << " / " << total << "\n"
              << Colors::RESET
              << Colors::RED << "  Failed : " << failed << " / " << total << "\n"
              << Colors::RESET;
}
```
- **Summary Statistics**: Total, passed, and failed counts
- **Success Emphasis**: Special message for all-pass scenario
- **Clear Breakdown**: Detailed counts when failures exist
- **Visual Separation**: Boxed summary section

### **Report File Generation (Lines 105-175)**
```cpp
const std::string reportPath = "test_results.txt";
std::ofstream report(reportPath);

// Write comprehensive report with:
// - Test summary statistics
// - Detailed failure information
// - Complete source code for failed files
// - Error location pointers
```
- **Persistent Documentation**: Saves detailed results to file
- **Comprehensive Information**: All failure details preserved
- **Debugging Support**: Complete source code for analysis
- **Professional Format**: Structured, readable report layout

## Design Patterns and Architecture

### **Test Orchestrator Pattern**
```cpp
// Test execution orchestration
int runTestSuite(const std::string &testDir) {
    // 1. Setup test environment
    setupTestEnvironment();
    
    // 2. Discover test files
    auto files = discoverTestFiles(testDir);
    
    // 3. Execute tests with reporting
    auto results = executeTests(files);
    
    // 4. Generate reports
    generateReports(results);
    
    // 5. Return exit code
    return results.hasFailures() ? 1 : 0;
}
```
- **Sequential Phases**: Clear test execution pipeline
- **Single Responsibility**: Each phase has distinct purpose
- **Comprehensive Coverage**: All aspects of testing addressed
- **Professional Output**: Both console and file reports

### **Progress Reporting Pattern**
```cpp
// Real-time progress feedback
for (const auto &file : files) {
    std::cout << "Testing: " << file << " ... ";
    
    auto result = testFile(file);
    
    if (result.passed) {
        std::cout << "✓ PASS\n";
    } else {
        std::cout << "✗ FAIL\n";
        showFailureDetails(result);
    }
}
```
- **Immediate Feedback**: Results shown as tests complete
- **Visual Indicators**: Clear pass/fail symbols
- **Error Context**: Detailed failure information
- **Progress Tracking**: User can see test progress

### **Report Generation Pattern**
```cpp
// Structured report generation
void generateTestReport(const std::vector<TestResult> &results) {
    std::ofstream report("test_results.txt");
    
    // Executive summary
    writeSummary(report, results);
    
    // Detailed failures
    for (const auto &failure : results.getFailures()) {
        writeFailureDetails(report, failure);
    }
}
```
- **Executive Summary**: High-level test statistics
- **Detailed Analysis**: Complete failure information
- **Persistent Storage**: Results saved for later analysis
- **Professional Format**: Structured, readable layout

## Technical Considerations

### **Performance Characteristics**
```cpp
// Time complexity analysis
// Let F = number of .sa files
// Let L = average lines per file
// Total complexity: O(F * (parse_time + execution_time))
// Typically: O(F * L) for parsing + O(F * operations) for execution

// Space complexity: O(F * L) for storing failure source code
```
- **Linear Scaling**: Performance scales with number of files
- **Memory Management**: Only failure source code stored
- **I/O Bound**: Limited by filesystem and parsing speed
- **Optimization Opportunities**: Parallel test execution

### **Error Handling Strategy**
```cpp
// Multi-level error handling
1. Directory validation - immediate return
2. File collection - graceful handling of empty directories
3. Individual test execution - continue on individual failures
4. Report generation - handle file write failures
```
- **Graceful Degradation**: Continues testing despite individual failures
- **Comprehensive Coverage**: All error cases handled
- **User Feedback**: Clear error messages at each level
- **Robust Recovery**: Partial results available even with some failures

### **Output Management**
```cpp
// Dual output strategy
Console Output:
- Real-time progress
- Immediate pass/fail feedback
- Error summaries
- Visual indicators

File Output:
- Persistent documentation
- Detailed failure analysis
- Complete source code
- Machine-readable format
```
- **Comprehensive Coverage**: Both immediate and persistent output
- **Different Formats**: Optimized for different use cases
- **Professional Presentation**: Consistent formatting
- **Debugging Support**: Detailed information for problem analysis

## Usage Patterns

### **Command Line Usage**
```bash
# Test default examples directory
quantum --test examples

# Test specific directory
quantum --test /path/to/test/files

# CI/CD integration
quantum --test tests && echo "All tests passed"
```

### **Integration with Build Systems**
```cmake
# CMake integration
add_custom_target(test_quantum
    COMMAND quantum --test ${CMAKE_SOURCE_DIR}/examples
    DEPENDS quantum
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running Quantum Language test suite"
)
```

### **Automated Testing**
```python
# Python script for automated testing
import subprocess
import sys

def run_quantum_tests():
    result = subprocess.run(['quantum', '--test', 'examples'], 
                          capture_output=True, text=True)
    
    if result.returncode == 0:
        print("✓ All tests passed")
        return True
    else:
        print("✗ Some tests failed")
        print(result.stdout)
        print(result.stderr)
        return False

if __name__ == "__main__":
    success = run_quantum_tests()
    sys.exit(0 if success else 1)
```

## Advanced Features

### **Parallel Test Execution**
```cpp
#include <future>
#include <thread>

static int runTestExamplesParallel(const std::string &examplesDir) {
    std::vector<fs::path> files;
    collectSaFiles(examplesDir, files);
    
    // Execute tests in parallel
    std::vector<std::future<TestResult>> futures;
    for (const auto &file : files) {
        futures.push_back(std::async(std::launch::async, [file]() {
            return testFile(file.string());
        }));
    }
    
    // Collect results
    std::vector<TestResult> results;
    for (auto &future : futures) {
        results.push_back(future.get());
    }
    
    // Generate report...
    return generateReport(results);
}
```

### **Test Filtering**
```cpp
static int runFilteredTests(const std::string &examplesDir, 
                           const std::string &pattern) {
    std::vector<fs::path> allFiles;
    collectSaFiles(examplesDir, allFiles);
    
    // Filter files by pattern
    std::vector<fs::path> filteredFiles;
    for (const auto &file : allFiles) {
        if (file.string().find(pattern) != std::string::npos) {
            filteredFiles.push_back(file);
        }
    }
    
    return runTestFiles(filteredFiles);
}
```

### **Performance Benchmarking**
```cpp
struct BenchmarkResult {
    std::string file;
    std::chrono::milliseconds parseTime;
    std::chrono::milliseconds executionTime;
    size_t memoryUsage;
};

static std::vector<BenchmarkResult> runBenchmarks(const std::string &examplesDir) {
    // Implementation with timing and memory measurement
}
```

## Why This Design Works

### **Comprehensiveness**
- **Complete Workflow**: From discovery to reporting
- **Detailed Information**: All failure details preserved
- **Multiple Output Formats**: Console and file reports
- **Professional Presentation**: Clear, structured output

### **Usability**
- **Simple Interface**: Single function call
- **Clear Feedback**: Real-time progress and results
- **Error Context**: Detailed failure information
- **Persistent Results**: Reports saved for analysis

### **Reliability**
- **Robust Error Handling**: All edge cases covered
- **Graceful Degradation**: Continues despite individual failures
- **Deterministic Behavior**: Same results on same input
- **Cross-Platform**: Works on all major systems

### **Maintainability**
- **Modular Design**: Separate concerns for each phase
- **Clear Structure**: Easy to understand and modify
- **Extensible**: Simple to add new features
- **Well-Documented**: Clear intent and behavior

The `runTestExamples()` function demonstrates exceptional software engineering design, providing a comprehensive, reliable, and user-friendly testing framework that perfectly serves the needs of the Quantum Language development workflow while maintaining professional standards for test automation and reporting.
