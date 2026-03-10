# getSourceLine() Function - Source Code Line Extraction

## Overview
The `getSourceLine()` function extracts a specific line from source code text, providing essential context for error reporting and debugging. It handles edge cases like out-of-range line numbers and preserves original formatting, making it invaluable for displaying error locations in test reports and debugging output.

## Complete Source Code

```cpp
static std::string getSourceLine(const std::string &source, int lineNum)
{
    if (lineNum <= 0)
        return "";

    std::istringstream stream(source);
    std::string line;
    int currentLine = 1;

    while (currentLine < lineNum && std::getline(stream, line))
    {
        currentLine++;
    }

    if (currentLine == lineNum && std::getline(stream, line))
    {
        return line;
    }

    return "";
}
```

## Line-by-Line Analysis

### **Function Signature (Line 1)**
```cpp
static std::string getSourceLine(const std::string &source, int lineNum)
```
- **`static`**: Internal linkage - only visible within this translation unit
- **`std::string`**: Returns the extracted line (empty string if not found)
- **`getSourceLine`**: Descriptive name indicating line extraction purpose
- **`const std::string &source`**: Constant reference to source code for efficiency
- **`int lineNum`**: 1-based line number to extract

### **Input Validation (Lines 3-5)**
```cpp
if (lineNum <= 0)
    return "";
```
- **Early Return**: Handles invalid line numbers immediately
- **Zero/Negative Check**: Line numbers must be positive
- **Empty String**: Indicates invalid request
- **Defensive Programming**: Prevents undefined behavior

### **Stream Setup (Lines 7-9)**
```cpp
std::istringstream stream(source);
std::string line;
int currentLine = 1;
```
- **`std::istringstream`**: String stream for line-by-line processing
- **`source` Input**: Initializes stream with complete source code
- **`line` Variable**: Storage for each extracted line
- **`currentLine` Counter**: Tracks current line position (1-based)

### **Line Skipping Loop (Lines 11-15)**
```cpp
while (currentLine < lineNum && std::getline(stream, line))
{
    currentLine++;
}
```
- **Loop Condition**: Continue until reaching target line OR end of file
- **`std::getline(stream, line)`**: Extracts next line from stream
- **Line Advancement**: Increment counter for each line read
- **Early Termination**: Stops if EOF reached before target line

### **Target Line Extraction (Lines 17-21)**
```cpp
if (currentLine == lineNum && std::getline(stream, line))
{
    return line;
}
```
- **Position Check**: Verifies we're at the correct line
- **Line Extraction**: Gets the actual target line content
- **Success Return**: Returns the extracted line
- **Preserves Formatting**: Maintains original indentation and content

### **Default Return (Line 23)**
```cpp
return "";
```
- **Failure Case**: Empty string indicates line not found
- **EOF Handling**: When line number exceeds source length
- **Consistent Interface**: Always returns string, never throws
- **Clear Signal**: Empty string unambiguously indicates failure

## Design Patterns and Architecture

### **Iterator Pattern**
```cpp
// Sequential line iteration
std::istringstream stream(source);
std::string line;
int currentLine = 1;

while (std::getline(stream, line)) {
    if (currentLine == targetLine) {
        return line;  // Found the target
    }
    currentLine++;
}
```
- **Sequential Access**: Lines accessed in order
- **State Tracking**: Current position maintained
- **Early Termination**: Stop when target found
- **Efficient Traversal**: No unnecessary line storage

### **Guard Clause Pattern**
```cpp
// Input validation guard
if (lineNum <= 0)
    return "";  // Early return for invalid input

// Continue with valid input...
```
- **Precondition Check**: Validates inputs before processing
- **Early Exit**: Returns immediately on invalid input
- **Defensive Programming**: Prevents runtime errors
- **Clear Intent**: Obvious input validation

### **Resource Management Pattern**
```cpp
// Automatic resource cleanup
{
    std::istringstream stream(source);  // Resource acquisition
    // Use stream...
}  // Automatic resource release
```
- **RAII**: String stream manages its own resources
- **Scope-Bound**: Resource lifetime tied to scope
- **No Manual Cleanup**: Automatic destructor handles cleanup
- **Exception Safe**: Resources cleaned up even on exceptions

## Technical Considerations

### **Line Numbering Convention**
```cpp
// 1-based line numbering (standard in editors)
getSourceLine(source, 1);  // Returns first line
getSourceLine(source, 10); // Returns tenth line

// 0-based would be confusing for users
getSourceLine(source, 0);  // Returns "" (invalid)
```
- **User-Friendly**: Matches editor line numbers
- **Standard Convention**: Most editors use 1-based numbering
- **Error Reporting**: Consistent with compiler error messages
- **Intuitive**: Natural counting for humans

### **String Stream Efficiency**
```cpp
// Efficient string processing
std::istringstream stream(source);  // No copy, references source
std::getline(stream, line);        // Extracts line without reallocation
```
- **Zero-Copy Initialization**: String stream references original string
- **Incremental Processing**: Lines processed one at a time
- **Memory Efficient**: Only current line stored in memory
- **Cache Friendly**: Sequential memory access pattern

### **Edge Case Handling**
```cpp
// Various edge cases and their handling
getSourceLine(source, -1);   // Returns "" (invalid)
getSourceLine(source, 0);    // Returns "" (invalid)
getSourceLine(source, 1);     // Returns first line
getSourceLine(source, 999);  // Returns "" (out of range)
getSourceLine("", 1);        // Returns "" (empty source)
```
- **Invalid Line Numbers**: Negative and zero rejected
- **Out of Range**: Line numbers beyond source length
- **Empty Source**: Handles empty source code gracefully
- **Consistent Behavior**: Always returns string, never throws

### **Performance Characteristics**
```cpp
// Time complexity analysis
// Best case: lineNum = 1 (O(1))
// Average case: lineNum = n/2 (O(n))
// Worst case: lineNum = totalLines (O(n))

// Space complexity: O(1) - only stores current line
```
- **Linear Time**: Must read all preceding lines
- **Constant Space**: Only current line in memory
- **Early Termination**: Stops when target found
- **Optimal for Use Case**: Suitable for error reporting

## Usage Patterns

### **Error Reporting**
```cpp
void reportError(const TestResult &result) {
    std::cout << "Error in file: " << result.path << "\n";
    std::cout << "Line " << result.line << ": " << result.error << "\n";
    
    if (result.line > 0) {
        std::string sourceLine = getSourceLine(result.source, result.line);
        if (!sourceLine.empty()) {
            std::cout << "Code: " << sourceLine << "\n";
            
            // Show error position with caret
            if (result.col > 0) {
                int spaces = 6 + result.col - 1; // "Code: " offset
                std::cout << std::string(spaces, ' ') << "^\n";
            }
        }
    }
}
```

### **Test Result Documentation**
```cpp
void writeTestReport(const std::vector<TestResult> &results) {
    for (const auto &result : results) {
        if (!result.error.empty() && result.line > 0) {
            std::string sourceLine = getSourceLine(result.source, result.line);
            
            report << "File: " << result.path << "\n";
            report << "Line: " << result.line << "\n";
            report << "Error: " << result.error << "\n";
            report << "Code: " << sourceLine << "\n";
            report << "---\n";
        }
    }
}
```

### **Debugging Support**
```cpp
void debugErrorLocation(const std::string &source, int line, int col) {
    std::string errorLine = getSourceLine(source, line);
    
    if (!errorLine.empty()) {
        std::cout << "Error location:\n";
        std::cout << "Line " << line << ": " << errorLine << "\n";
        
        if (col > 0) {
            int leadingSpaces = 0;
            while (leadingSpaces < (int)errorLine.size() && 
                   (errorLine[leadingSpaces] == ' ' || errorLine[leadingSpaces] == '\t')) {
                leadingSpaces++;
            }
            
            int caretPos = col - 1;
            std::cout << std::string(caretPos, ' ') << "^\n";
        }
    }
}
```

### **Context Extraction**
```cpp
struct ErrorContext {
    std::string errorLine;
    std::string lineBefore;
    std::string lineAfter;
    
    ErrorContext(const std::string &source, int errorLine) {
        this->errorLine = getSourceLine(source, errorLine);
        this->lineBefore = getSourceLine(source, errorLine - 1);
        this->lineAfter = getSourceLine(source, errorLine + 1);
    }
};
```

## Advanced Features

### **Multi-Line Context**
```cpp
static std::vector<std::string> getSourceContext(
    const std::string &source, 
    int centerLine, 
    int contextLines = 3
) {
    std::vector<std::string> context;
    int startLine = std::max(1, centerLine - contextLines);
    int endLine = centerLine + contextLines;
    
    for (int line = startLine; line <= endLine; ++line) {
        std::string lineContent = getSourceLine(source, line);
        if (!lineContent.empty()) {
            context.push_back(lineContent);
        }
    }
    
    return context;
}
```

### **Line Number Mapping**
```cpp
struct LineInfo {
    int originalLine;
    int displayLine;
    std::string content;
    bool hasError;
};

static std::vector<LineInfo> getAnnotatedLines(
    const std::string &source, 
    int errorLine, 
    int errorCol
) {
    std::vector<LineInfo> lines;
    std::istringstream stream(source);
    std::string line;
    int currentLine = 1;
    
    while (std::getline(stream, line)) {
        LineInfo info;
        info.originalLine = currentLine;
        info.displayLine = currentLine;
        info.content = line;
        info.hasError = (currentLine == errorLine);
        
        lines.push_back(info);
        currentLine++;
    }
    
    return lines;
}
```

### **Efficient Line Indexing**
```cpp
class SourceLineIndex {
private:
    std::vector<size_t> lineOffsets;
    const std::string &source;
    
public:
    SourceLineIndex(const std::string &src) : source(src) {
        lineOffsets.push_back(0);  // First line starts at 0
        
        for (size_t i = 0; i < source.size(); ++i) {
            if (source[i] == '\n') {
                lineOffsets.push_back(i + 1);  // Next line starts after newline
            }
        }
    }
    
    std::string getLine(int lineNum) const {
        if (lineNum <= 0 || lineNum > (int)lineOffsets.size()) {
            return "";
        }
        
        size_t start = lineOffsets[lineNum - 1];
        size_t end = (lineNum < (int)lineOffsets.size()) ? 
                     lineOffsets[lineNum] : source.size();
        
        return source.substr(start, end - start);
    }
};
```

## Why This Design Works

### **Reliability**
- **Robust Edge Case Handling**: Handles all invalid inputs gracefully
- **Consistent Interface**: Always returns string, never throws
- **Deterministic Behavior**: Same input always produces same output
- **Memory Safe**: No buffer overflows or pointer issues

### **Efficiency**
- **Linear Time**: Optimal for sequential access pattern
- **Constant Space**: Minimal memory footprint
- **Early Termination**: Stops when target line found
- **No Unnecessary Copies**: Efficient string handling

### **Usability**
- **Intuitive Interface**: Simple function signature
- **Clear Return Values**: Empty string unambiguously indicates failure
- **Standard Conventions**: 1-based line numbering
- **Well-Documented**: Clear purpose and behavior

### **Maintainability**
- **Simple Implementation**: Easy to understand and modify
- **No Dependencies**: Uses only standard library
- **Testable**: Straightforward to unit test
- **Extensible**: Can be enhanced with additional features

The `getSourceLine()` function demonstrates excellent utility design, providing a reliable, efficient, and easy-to-use solution for source code line extraction that perfectly serves the needs of error reporting and debugging in the Quantum Language testing framework.
