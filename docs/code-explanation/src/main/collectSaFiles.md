# collectSaFiles() Function - Recursive File Collection

## Overview
The `collectSaFiles()` function recursively traverses directories to collect all files with the `.sa` extension. It's a fundamental utility for the testing framework that enables comprehensive test discovery across complex directory structures, ensuring that no Quantum Language source files are missed during batch testing operations.

## Complete Source Code

```cpp
static void collectSaFiles(const fs::path &dir, std::vector<fs::path> &out)
{
    if (!fs::exists(dir) || !fs::is_directory(dir))
        return;

    for (const auto &entry : fs::directory_iterator(dir))
    {
        if (entry.is_directory())
        {
            collectSaFiles(entry.path(), out);
        }
        else if (entry.is_regular_file() && entry.path().extension() == ".sa")
        {
            out.push_back(entry.path());
        }
    }
}
```

## Line-by-Line Analysis

### **Function Signature (Line 1)**
```cpp
static void collectSaFiles(const fs::path &dir, std::vector<fs::path> &out)
```
- **`static`**: Internal linkage - only visible within this translation unit
- **`void`**: No return value - modifies output vector directly
- **`collectSaFiles`**: Descriptive name indicating .sa file collection purpose
- **`const fs::path &dir`**: Constant reference to directory path (fs = filesystem)
- **`std::vector<fs::path> &out`**: Reference to output vector for efficient accumulation

### **Directory Validation (Lines 3-5)**
```cpp
if (!fs::exists(dir) || !fs::is_directory(dir))
    return;
```
- **Existence Check**: Verifies directory exists on filesystem
- **Type Validation**: Confirms path is actually a directory
- **Early Return**: Exits silently for invalid directories
- **Defensive Programming**: Prevents crashes on invalid paths

### **Directory Iteration (Line 7)**
```cpp
for (const auto &entry : fs::directory_iterator(dir))
```
- **Range-Based For**: Modern C++ iteration syntax
- **`fs::directory_iterator`**: Iterates over directory contents
- **`entry`**: Directory entry object representing each item
- **Exception Safe**: Iterator handles filesystem errors gracefully

### **Recursive Directory Handling (Lines 9-12)**
```cpp
if (entry.is_directory())
{
    collectSaFiles(entry.path(), out);
}
```
- **Directory Detection**: Checks if entry is a subdirectory
- **Recursive Call**: Calls self for subdirectory traversal
- **Path Propagation**: Passes subdirectory path to recursive call
- **Depth-First Traversal**: Explores subdirectories completely

### **File Collection (Lines 13-16)**
```cpp
else if (entry.is_regular_file() && entry.path().extension() == ".sa")
{
    out.push_back(entry.path());
}
```
- **File Validation**: Confirms entry is a regular file (not symlink, etc.)
- **Extension Check**: Tests for `.sa` extension specifically
- **Path Storage**: Adds matching file path to output vector
- **Efficient Accumulation**: Uses vector's push_back for growth

## Design Patterns and Architecture

### **Recursive Traversal Pattern**
```cpp
// Directory tree traversal
void collectFiles(const fs::path &dir, std::vector<fs::path> &out) {
    for (const auto &entry : fs::directory_iterator(dir)) {
        if (entry.is_directory()) {
            collectFiles(entry.path(), out);  // Recursive step
        } else if (isTargetFile(entry)) {
            out.push_back(entry.path());      // Base case
        }
    }
}
```
- **Divide and Conquer**: Breaks problem into smaller subproblems
- **Base Case**: Regular files with matching extension
- **Recursive Step**: Subdirectories processed recursively
- **Termination**: Guaranteed by finite directory depth

### **Visitor Pattern**
```cpp
// File system visitor
struct SaFileVisitor {
    std::vector<fs::path> &files;
    
    void operator()(const fs::directory_entry &entry) {
        if (entry.is_regular_file() && entry.path().extension() == ".sa") {
            files.push_back(entry.path());
        }
    }
};

static void collectSaFiles(const fs::path &dir, std::vector<fs::path> &out) {
    SaFileVisitor visitor{out};
    traverseDirectory(dir, visitor);
}
```
- **Separation of Concerns**: Traversal logic separate from file selection
- **Extensible**: Easy to add new file types or criteria
- **Reusable**: Visitor can be used with different traversal strategies
- **Testable**: File selection logic can be unit tested independently

### **Filter Pattern**
```cpp
// File filtering abstraction
using FileFilter = std::function<bool(const fs::directory_entry&)>;

static void collectFiles(const fs::path &dir, 
                       std::vector<fs::path> &out, 
                       FileFilter filter) {
    for (const auto &entry : fs::directory_iterator(dir)) {
        if (entry.is_directory()) {
            collectFiles(entry.path(), out, filter);
        } else if (filter(entry)) {
            out.push_back(entry.path());
        }
    }
}

// Usage:
auto saFilter = [](const fs::directory_entry &entry) {
    return entry.is_regular_file() && entry.path().extension() == ".sa";
};
collectFiles(rootDir, files, saFilter);
```
- **Flexible Filtering**: Customizable file selection criteria
- **Reusable Function**: Same traversal for different file types
- **Composable**: Filters can be combined or chained
- **Expressive**: Lambda syntax for clear filter definitions

## Technical Considerations

### **Filesystem Library (C++17)**
```cpp
#include <filesystem>
namespace fs = std::filesystem;  // Common alias

// Key operations used:
fs::exists(path)          // Check if path exists
fs::is_directory(path)    // Check if path is directory
fs::directory_iterator(path) // Iterate directory contents
fs::is_regular_file(path) // Check if regular file
path.extension()          // Get file extension
```
- **Modern C++17**: Uses standard filesystem library
- **Cross-Platform**: Works on Windows, Linux, macOS
- **Type Safety**: Strong typing for path operations
- **Exception Handling**: Filesystem operations throw on errors

### **Path Handling**
```cpp
// Path operations and considerations
fs::path filePath = entry.path();  // Full path to file
std::string filename = filePath.filename().string();  // Just filename
std::string extension = filePath.extension().string(); // ".sa"
fs::path parent = filePath.parent_path();  // Directory containing file
```
- **Platform-Aware**: Handles path separators automatically
- **Unicode Support**: Proper Unicode path handling
- **Immutable Operations**: Path operations return new objects
- **Efficient Comparisons**: Path comparison is optimized

### **Performance Characteristics**
```cpp
// Time complexity analysis
// Let D = total number of directories
// Let F = total number of files
// Complexity: O(D + F) - visits each entry exactly once

// Space complexity: O(F) - stores all matching file paths
```
- **Linear Complexity**: Visits each filesystem entry once
- **Memory Usage**: Proportional to number of .sa files found
- **Recursive Overhead**: Function call stack proportional to directory depth
- **I/O Bound**: Performance limited by filesystem speed

### **Error Handling Strategy**
```cpp
// Current approach: Silent failure
if (!fs::exists(dir) || !fs::is_directory(dir))
    return;  // Silently ignore invalid directories

// Alternative: Error reporting
static std::optional<std::vector<fs::path>> collectSaFiles(const fs::path &dir) {
    if (!fs::exists(dir)) {
        throw std::runtime_error("Directory does not exist: " + dir.string());
    }
    if (!fs::is_directory(dir)) {
        throw std::runtime_error("Path is not a directory: " + dir.string());
    }
    // ... continue with collection
}
```
- **Current Choice**: Silent failure for robustness
- **Trade-off**: May miss errors but continues processing
- **Alternative**: Could throw exceptions for better error reporting
- **Context Dependent**: Choice depends on use case

## Usage Patterns

### **Basic Usage**
```cpp
std::vector<fs::path> saFiles;
collectSaFiles("examples/", saFiles);

std::cout << "Found " << saFiles.size() << " .sa files:\n";
for (const auto &file : saFiles) {
    std::cout << "  " << file.string() << "\n";
}
```

### **Test Suite Integration**
```cpp
static int runTestExamples(const std::string &examplesDir) {
    fs::path dir(examplesDir);
    std::vector<fs::path> files;
    collectSaFiles(dir, files);
    
    if (files.empty()) {
        std::cout << "No .sa files found in " << examplesDir << "\n";
        return 0;
    }
    
    // Run tests on collected files
    int failed = 0;
    for (const auto &file : files) {
        TestResult result = testFile(file.string());
        if (!result.error.empty()) {
            failed++;
        }
    }
    
    return failed > 0 ? 1 : 0;
}
```

### **Filtered Collection**
```cpp
// Collect files with additional criteria
std::vector<fs::path> collectTestFiles(const fs::path &dir) {
    std::vector<fs::path> allFiles;
    collectSaFiles(dir, allFiles);
    
    // Filter out test files (those with "test" in name)
    std::vector<fs::path> testFiles;
    for (const auto &file : allFiles) {
        std::string filename = file.filename().string();
        if (filename.find("test") != std::string::npos) {
            testFiles.push_back(file);
        }
    }
    
    return testFiles;
}
```

### **Parallel Processing**
```cpp
#include <algorithm>

// Sort files for consistent processing order
std::vector<fs::path> saFiles;
collectSaFiles("examples/", saFiles);
std::sort(saFiles.begin(), saFiles.end());

// Process in parallel (conceptual)
std::for_each(std::execution::par, saFiles.begin(), saFiles.end(),
    [](const fs::path &file) {
        TestResult result = testFile(file.string());
        // Process result...
    });
```

## Advanced Features

### **Progress Reporting**
```cpp
struct FileCollector {
    std::vector<fs::path> &files;
    std::function<void(int, int)> progressCallback;
    int totalProcessed = 0;
    
    void operator()(const fs::directory_entry &entry) {
        if (entry.is_regular_file() && entry.path().extension() == ".sa") {
            files.push_back(entry.path());
            totalProcessed++;
            
            if (progressCallback) {
                progressCallback(totalProcessed, files.size());
            }
        }
    }
};

static void collectSaFilesWithProgress(
    const fs::path &dir, 
    std::vector<fs::path> &out,
    std::function<void(int, int)> progress = nullptr
) {
    FileCollector collector{out, progress};
    traverseDirectory(dir, collector);
}
```

### **Exclusion Patterns**
```cpp
static void collectSaFilesExcluding(
    const fs::path &dir, 
    std::vector<fs::path> &out,
    const std::vector<std::string> &excludePatterns
) {
    if (!fs::exists(dir) || !fs::is_directory(dir))
        return;

    for (const auto &entry : fs::directory_iterator(dir)) {
        if (entry.is_directory()) {
            // Check if directory should be excluded
            std::string dirName = entry.path().filename().string();
            bool excluded = std::any_of(excludePatterns.begin(), excludePatterns.end(),
                [&dirName](const std::string &pattern) {
                    return dirName.find(pattern) != std::string::npos;
                });
            
            if (!excluded) {
                collectSaFilesExcluding(entry.path(), out, excludePatterns);
            }
        }
        else if (entry.is_regular_file() && 
                 entry.path().extension() == ".sa") {
            out.push_back(entry.path());
        }
    }
}
```

### **Symbolic Link Handling**
```cpp
static void collectSaFilesWithSymlinks(
    const fs::path &dir, 
    std::vector<fs::path> &out,
    bool followSymlinks = true
) {
    if (!fs::exists(dir) || !fs::is_directory(dir))
        return;

    for (const auto &entry : fs::directory_iterator(dir)) {
        if (entry.is_symlink()) {
            if (followSymlinks) {
                fs::path target = fs::read_symlink(entry.path());
                if (fs::exists(target)) {
                    if (fs::is_directory(target)) {
                        collectSaFilesWithSymlinks(target, out, followSymlinks);
                    } else if (fs::is_regular_file(target) && 
                               target.extension() == ".sa") {
                        out.push_back(target);
                    }
                }
            }
        }
        else if (entry.is_directory()) {
            collectSaFilesWithSymlinks(entry.path(), out, followSymlinks);
        }
        else if (entry.is_regular_file() && entry.path().extension() == ".sa") {
            out.push_back(entry.path());
        }
    }
}
```

## Debugging and Troubleshooting

### **Common Issues**

#### **Permission Errors**
```cpp
// Handle permission denied errors
static void collectSaFilesSafe(const fs::path &dir, std::vector<fs::path> &out) {
    try {
        for (const auto &entry : fs::directory_iterator(dir)) {
            // ... existing logic ...
        }
    }
    catch (const fs::filesystem_error &e) {
        std::cerr << "Warning: Cannot access " << dir << ": " << e.what() << "\n";
        // Continue with other directories
    }
}
```

#### **Circular Symbolic Links**
```cpp
// Detect and handle circular symlinks
static void collectSaFilesSafe(const fs::path &dir, std::vector<fs::path> &out,
                               std::set<fs::path> &visited) {
    if (visited.find(dir) != visited.end()) {
        return;  // Already visited, avoid infinite recursion
    }
    
    visited.insert(dir);
    
    // ... existing collection logic ...
}
```

#### **Large Directory Performance**
```cpp
// Optimize for large directories
static void collectSaFilesOptimized(const fs::path &dir, std::vector<fs::path> &out) {
    // Reserve space to avoid reallocations
    out.reserve(1000);  // Estimate based on typical usage
    
    // Use parallel directory iteration if available
    for (const auto &entry : fs::directory_iterator(dir, 
            fs::directory_options::skip_permission_denied)) {
        // ... existing logic ...
    }
}
```

## Why This Design Works

### **Simplicity**
- **Clear Purpose**: Obviously collects .sa files
- **Minimal Interface**: Simple function signature
- **Straightforward Logic**: Easy to understand and verify
- **Self-Documenting**: Function name explains behavior

### **Efficiency**
- **Single Pass**: Visits each filesystem entry once
- **Recursive Optimization**: Natural depth-first traversal
- **Memory Efficient**: Only stores matching files
- **Modern C++**: Uses efficient filesystem library

### **Reliability**
- **Cross-Platform**: Works on all major operating systems
- **Type Safe**: Strong typing prevents errors
- **Exception Safe**: Handles filesystem errors gracefully
- **Deterministic**: Same directory structure produces same results

### **Extensibility**
- **Filterable**: Easy to add file type filters
- **Customizable**: Can be modified for different collection criteria
- **Composable**: Can be combined with other utilities
- **Maintainable**: Clear structure for future enhancements

The `collectSaFiles()` function demonstrates excellent utility design, providing a simple, efficient, and reliable solution for recursive file discovery that perfectly serves the needs of the Quantum Language testing framework while maintaining clean code structure and cross-platform compatibility.
