# main() Function - Application Entry Point and Command-Line Interface

## Overview
The `main()` function serves as the primary entry point for the Quantum Language interpreter. It handles command-line argument parsing, coordinates different execution modes (REPL, file execution, testing, help), and manages cross-platform console initialization. This function orchestrates the entire application workflow and provides the user-facing interface.

## Complete Source Code

```cpp
int main(int argc, char *argv[])
{
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif

    if (argc == 1)
    {
        runREPL();
        return 0;
    }

    std::string arg = argv[1];

    if (arg == "--help" || arg == "-h")
    {
        printBanner();
        printHelp(argv[0]);
        return 0;
    }

    if (arg == "--aura")
    {
        printBanner();
        printAura();
        return 0;
    }

    if (arg == "--version" || arg == "-v")
    {
        std::cout << "Quantum Language v1.0.0\n"
                  << "Runtime: Tree-walk interpreter\n"
                  << "Built By Muhammad Saad Amin\n";
        return 0;
    }

    if (arg == "--check" && argc >= 3)
    {
        return checkFile(argv[2]);
    }

    if (arg == "--test")
    {
        std::string targetDir = "examples";
        if (argc >= 3)
            targetDir = argv[2];
        return runTestExamples(targetDir);
    }

    runFile(arg);
    return 0;
}
```

## Line-by-Line Analysis

### **Function Signature (Line 1)**
```cpp
int main(int argc, char *argv[])
```
- **`int`**: Return code (0 for success, non-zero for failure)
- **`argc`**: Argument count - number of command-line arguments
- **`argv`**: Argument vector - array of C-style strings
- **Standard Entry Point**: Conforms to C/C++ standard

### **Cross-Platform Console Setup (Lines 3-7)**
```cpp
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif
```
- **Conditional Compilation**: Windows-specific code only
- **UTF-8 Support**: Sets console to UTF-8 code page
- **Output Console**: `SetConsoleOutputCP(CP_UTF8)` for output
- **Input Console**: `SetConsoleCP(CP_UTF8)` for input
- **Unicode Compatibility**: Ensures proper character display

### **REPL Mode (Lines 9-13)**
```cpp
if (argc == 1)
{
    runREPL();
    return 0;
}
```
- **No Arguments**: `argc == 1` means only program name
- **Interactive Mode**: Starts Read-Eval-Print Loop
- **Success Return**: Returns 0 after REPL completion
- **Default Behavior**: REPL is the default interactive mode

### **Help Command (Lines 15-21)**
```cpp
if (arg == "--help" || arg == "-h")
{
    printBanner();
    printHelp(argv[0]);
    return 0;
}
```
- **Multiple Forms**: Accepts both `--help` and `-h`
- **Banner Display**: Shows program branding first
- **Help Content**: Displays usage information
- **Successful Exit**: Returns 0 after showing help

### **Aura Display (Lines 23-29)**
```cpp
if (arg == "--aura")
{
    printBanner();
    printAura();
    return 0;
}
```
- **Special Command**: `--aura` shows achievement display
- **Visual Impact**: Combines banner with achievements
- **Marketing Feature**: Demonstrates project capabilities
- **User Engagement**: Impressive visual presentation

### **Version Information (Lines 31-37)**
```cpp
if (arg == "--version" || arg == "-v")
{
    std::cout << "Quantum Language v1.0.0\n"
              << "Runtime: Tree-walk interpreter\n"
              << "Built By Muhammad Saad Amin\n";
    return 0;
}
```
- **Version Command**: Accepts `--version` and `-v`
- **Semantic Versioning**: Clear version number format
- **Runtime Information**: Specifies interpreter type
- **Author Attribution**: Credits the developer

### **Syntax Check Command (Lines 39-43)**
```cpp
if (arg == "--check" && argc >= 3)
{
    return checkFile(argv[2]);
}
```
- **Validation Mode**: `--check <file>` for syntax checking
- **Argument Validation**: Requires at least 3 arguments
- **Direct Return**: Passes exit code from checkFile
- **Development Tool**: Useful for IDE integration

### **Test Command (Lines 45-53)**
```cpp
if (arg == "--test")
{
    std::string targetDir = "examples";
    if (argc >= 3)
        targetDir = argv[2];
    return runTestExamples(targetDir);
}
```
- **Testing Mode**: `--test [directory]` for batch testing
- **Default Directory**: Uses "examples" if none specified
- **Optional Argument**: Directory parameter is optional
- **Exit Code**: Returns test suite exit code

### **File Execution (Lines 55-57)**
```cpp
runFile(arg);
return 0;
```
- **Default Case**: Treat first argument as filename
- **Direct Execution**: Passes to runFile function
- **Success Return**: Returns 0 after execution
- **File Mode**: Standard script execution

## Design Patterns and Architecture

### **Command Pattern**
```cpp
// Command dispatch pattern
struct Command {
    std::string name;
    std::vector<std::string> aliases;
    std::function<int(int, char**)> handler;
    std::string description;
};

std::vector<Command> commands = {
    {"help", {"-h", "--help"}, handleHelp, "Show help information"},
    {"version", {"-v", "--version"}, handleVersion, "Show version info"},
    {"test", {"--test"}, handleTest, "Run test suite"},
    {"check", {"--check"}, handleCheck, "Check file syntax"}
};
```
- **Command Registration**: Centralized command definitions
- **Handler Functions**: Each command has dedicated handler
- **Alias Support**: Multiple names for same command
- **Extensible**: Easy to add new commands

### **Strategy Pattern**
```cpp
// Execution strategy selection
enum class ExecutionMode {
    REPL,
    FILE_EXECUTION,
    HELP,
    VERSION,
    TEST,
    CHECK,
    AURA
};

ExecutionMode determineMode(int argc, char *argv[]) {
    if (argc == 1) return ExecutionMode::REPL;
    if (std::string(argv[1]) == "--help") return ExecutionMode::HELP;
    // ... other checks
    return ExecutionMode::FILE_EXECUTION;
}
```
- **Mode Selection**: Determines execution strategy
- **Clean Separation**: Each mode handled separately
- **Extensible**: Easy to add new execution modes
- **Type Safety**: Enum prevents invalid modes

### **Factory Pattern**
```cpp
// Application mode factory
class ApplicationMode {
public:
    virtual ~ApplicationMode() = default;
    virtual int execute(int argc, char *argv[]) = 0;
};

class REPLMode : public ApplicationMode {
public:
    int execute(int argc, char *argv[]) override {
        runREPL();
        return 0;
    }
};

class FileExecutionMode : public ApplicationMode {
private:
    std::string filename;
public:
    FileExecutionMode(const std::string &file) : filename(file) {}
    int execute(int argc, char *argv[]) override {
        runFile(filename);
        return 0;
    }
};
```
- **Mode Objects**: Each execution mode as separate class
- **Polymorphic Interface**: Common interface for all modes
- **Encapsulation**: Mode-specific logic encapsulated
- **Testability**: Each mode can be tested independently

## Technical Considerations

### **Argument Parsing Strategy**
```cpp
// Current approach: Simple if-else chain
if (arg == "--help") { /* ... */ }
else if (arg == "--version") { /* ... */ }
else if (arg == "--test") { /* ... */ }

// Alternative: Argument parsing library
#include <args.hxx>
args::ArgumentParser parser("Quantum Language Interpreter");
args::HelpFlag help(parser, {"help", "h"}, "Display help");
args::ValueFlag<std::string> file(parser, {"file"}, "Script to run");
args::Flag test(parser, {"test"}, "Run test suite");
```
- **Current Simplicity**: Easy to understand and maintain
- **Limited Complexity**: No complex argument structures
- **Future Enhancement**: Could use library for advanced features
- **Performance**: Minimal overhead for simple parsing

### **Error Handling Strategy**
```cpp
// Current approach: Minimal error handling
if (arg == "--check" && argc >= 3) {
    return checkFile(argv[2]);
}

// Enhanced error handling
if (arg == "--check") {
    if (argc < 3) {
        std::cerr << "Error: --check requires a file argument\n";
        printHelp(argv[0]);
        return 1;
    }
    return checkFile(argv[2]);
}
```
- **Current Choice**: Simple validation, relies on function-level checks
- **Trade-off**: Cleaner main() function but less immediate feedback
- **User Experience**: Error messages come from called functions
- **Maintainability**: Centralized error handling in specialized functions

### **Cross-Platform Compatibility**
```cpp
// Windows console setup
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif

// Alternative: Cross-platform console library
#include <boost/asio.hpp> or other console libraries
```
- **Platform Detection**: Compile-time platform identification
- **Unicode Support**: Ensures proper character display
- **Limited Scope**: Only handles console encoding
- **Future Enhancement**: Could use cross-platform libraries

## Usage Patterns

### **Basic Usage**
```bash
# Start REPL (no arguments)
quantum

# Execute script file
quantum script.sa

# Show help
quantum --help
quantum -h

# Show version
quantum --version
quantum -v
```

### **Development Usage**
```bash
# Syntax checking
quantum --check script.sa

# Run test suite
quantum --test
quantum --test examples
quantum --test /path/to/tests

# Show achievements
quantum --aura
```

### **Integration Usage**
```bash
# In shell scripts
if quantum --check script.sa; then
    echo "Syntax OK"
    quantum script.sa
else
    echo "Syntax Error"
    exit 1
fi

# In CI/CD pipelines
quantum --test tests/
if [ $? -eq 0 ]; then
    echo "All tests passed"
else
    echo "Some tests failed"
    exit 1
fi
```

## Advanced Features

### **Configuration File Support**
```cpp
struct Config {
    std::string defaultTestDir = "examples";
    bool showBanner = true;
    bool enableColors = true;
    std::string logLevel = "info";
};

static Config loadConfig(const std::string &configFile) {
    Config config;
    // Load from JSON/YAML/INI file
    return config;
}

int main(int argc, char *argv[]) {
    Config config = loadConfig("quantum.conf");
    
    // Apply configuration
    if (!config.enableColors) {
        Colors::disable();
    }
    
    // ... rest of main logic
}
```

### **Advanced Argument Parsing**
```cpp
#include <getopt.h>

int main(int argc, char *argv[]) {
    int option;
    std::string testDir = "examples";
    bool verbose = false;
    
    static struct option long_options[] = {
        {"help", no_argument, 0, 'h'},
        {"version", no_argument, 0, 'v'},
        {"test", required_argument, 0, 't'},
        {"verbose", no_argument, 0, 'V'},
        {0, 0, 0, 0}
    };
    
    while ((option = getopt_long(argc, argv, "hvt:V", long_options, NULL)) != -1) {
        switch (option) {
            case 'h': /* handle help */ break;
            case 'v': /* handle version */ break;
            case 't': testDir = optarg; break;
            case 'V': verbose = true; break;
        }
    }
    
    // ... rest of logic
}
```

### **Plugin System**
```cpp
class Plugin {
public:
    virtual ~Plugin() = default;
    virtual std::string getName() const = 0;
    virtual int handleCommand(int argc, char *argv[]) = 0;
};

class PluginManager {
private:
    std::map<std::string, std::unique_ptr<Plugin>> plugins;
    
public:
    void loadPlugin(const std::string &path) {
        // Load dynamic library
        // Register plugin commands
    }
    
    int executePlugin(const std::string &name, int argc, char *argv[]) {
        if (plugins.find(name) != plugins.end()) {
            return plugins[name]->handleCommand(argc, argv);
        }
        return -1;
    }
};
```

## Debugging and Troubleshooting

### **Argument Debugging**
```cpp
static void debugArguments(int argc, char *argv[]) {
    std::cout << "Argument count: " << argc << "\n";
    for (int i = 0; i < argc; ++i) {
        std::cout << "argv[" << i << "]: " << argv[i] << "\n";
    }
}

int main(int argc, char *argv[]) {
    if (getenv("QUANTUM_DEBUG")) {
        debugArguments(argc, argv);
    }
    // ... rest of main
}
```

### **Error Recovery**
```cpp
int main(int argc, char *argv[]) {
    try {
        // ... main logic
    }
    catch (const std::exception &e) {
        std::cerr << "Fatal error: " << e.what() << "\n";
        std::cerr << "Use --help for usage information\n";
        return 1;
    }
    catch (...) {
        std::cerr << "Unknown fatal error\n";
        return 1;
    }
}
```

## Why This Design Works

### **Simplicity**
- **Clear Logic**: Straightforward argument handling
- **Minimal Complexity**: Easy to understand and maintain
- **Direct Mapping**: Commands map directly to functions
- **No Over-Engineering**: Appropriate for current needs

### **User Experience**
- **Intuitive Interface**: Standard command-line conventions
- **Comprehensive Coverage**: All common use cases addressed
- **Help Availability**: Easy access to usage information
- **Error Prevention**: Clear command structure reduces mistakes

### **Flexibility**
- **Multiple Modes**: REPL, file execution, testing, help
- **Optional Arguments**: Sensible defaults for optional parameters
- **Extensible**: Easy to add new commands and options
- **Cross-Platform**: Works on all major operating systems

### **Maintainability**
- **Modular Design**: Each command handled by dedicated function
- **Single Responsibility**: main() only handles dispatch
- **Clear Structure**: Easy to follow execution flow
- **Testable Components**: Individual functions can be unit tested

The `main()` function demonstrates excellent application design principles, providing a clean, intuitive, and extensible command-line interface that serves as the perfect entry point for the Quantum Language interpreter while maintaining professional standards for CLI applications.
