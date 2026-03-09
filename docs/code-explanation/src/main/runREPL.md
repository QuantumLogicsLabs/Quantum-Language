# runREPL() Function - Interactive Read-Eval-Print Loop

## Overview
The `runREPL()` function implements Quantum Language's interactive development environment. It provides a Read-Eval-Print Loop that allows users to type Quantum Language code line by line, get immediate feedback, and interactively develop programs with full compiler pipeline access.

## Complete Source Code

```cpp
static void runREPL()
{
    printBanner();
    std::cout << Colors::GREEN << "  REPL Mode — type 'exit' or Ctrl+D to quit\n"
              << Colors::RESET << "\n";

    Interpreter interp;
    std::string line;
    int lineNum = 1;

    while (true)
    {
        std::cout << Colors::CYAN << "quantum[" << lineNum++ << "]> " << Colors::RESET;
        if (!std::getline(std::cin, line))
            break;
        if (line == "exit" || line == "quit")
            break;
        if (line.empty())
            continue;

        try
        {
            Lexer lexer(line);
            auto tokens = lexer.tokenize();
            Parser parser(std::move(tokens));
            auto ast = parser.parse();
            interp.execute(*ast);
        }
        catch (const ParseError &e)
        {
            std::cerr << Colors::RED << "[ParseError] " << Colors::RESET << e.what() << " (line " << e.line << ")\n";
        }
        catch (const QuantumError &e)
        {
            std::cerr << Colors::RED << "[" << e.kind << "] " << Colors::RESET << e.what();
            if (e.line > 0)
                std::cerr << " (line " << e.line << ")";
            std::cerr << "\n";
        }
        catch (const std::exception &e)
        {
            std::cerr << Colors::RED << "[Error] " << Colors::RESET << e.what() << "\n";
        }
    }

    std::cout << Colors::YELLOW << "\n  Goodbye! 👋\n"
              << Colors::RESET;
}
```

## Line-by-Line Analysis

### **Function Signature (Line 1)**
```cpp
static void runREPL()
```
- **`static`**: Internal linkage - function only visible within this translation unit
- **`void`**: No return value - function produces side effects only
- **`runREPL`**: Descriptive name indicating Read-Eval-Print Loop functionality
- **No Parameters**: Self-contained function that manages its own state

### **Initial Setup (Lines 3-6)**
```cpp
printBanner();
std::cout << Colors::GREEN << "  REPL Mode — type 'exit' or Ctrl+D to quit\n"
          << Colors::RESET << "\n";

Interpreter interp;
std::string line;
int lineNum = 1;
```

**Setup Analysis:**

#### **Banner Display (Line 3):**
```cpp
printBanner();
```
- **Purpose**: Shows Quantum Language logo and version information
- **Visual Impact**: Creates professional first impression
- **Context Setting**: Establishes interactive session beginning

#### **Mode Announcement (Lines 4-6):**
```cpp
std::cout << Colors::GREEN << "  REPL Mode — type 'exit' or Ctrl+D to quit\n"
          << Colors::RESET << "\n";
```
- **`Colors::GREEN`**: Green color for mode indication
- **Clear Instructions**: Explicit exit commands and keyboard shortcut
- **User Guidance**: Helps new users understand how to exit
- **`Colors::RESET`**: Clears formatting for subsequent output

#### **State Initialization (Lines 7-9):**
```cpp
Interpreter interp;
std::string line;
int lineNum = 1;
```
- **`Interpreter interp`**: Single interpreter instance for entire session
- **`std::string line`**: Buffer for user input storage
- **`int lineNum = 1`**: Line counter starting from 1 for user-friendly numbering

### **Main REPL Loop (Lines 11-47)**
```cpp
while (true)
{
    // Prompt, input, processing, error handling
}
```

**Loop Structure Analysis:**

#### **Infinite Loop Design (Line 11):**
```cpp
while (true)
```
- **Purpose**: Continuous execution until explicit break
- **Control Flow**: Exit conditions handled inside loop
- **Session Management**: Maintains interactive state until user quits

#### **Prompt Display (Line 13):**
```cpp
std::cout << Colors::CYAN << "quantum[" << lineNum++ << "]> " << Colors::RESET;
```
- **`Colors::CYAN`**: Cyan color for prompt visibility
- **`quantum[`**: Language name prefix for identification
- **`lineNum++`**: Incrementing line counter for unique numbering
- **`]> `**: Standard prompt syntax with closing bracket and greater-than
- **`Colors::RESET`**: Clear formatting after prompt

**Prompt Features:**
- **Line Numbering**: Each prompt shows current line number
- **Language Branding**: "quantum" prefix for identity
- **Visual Distinction**: Cyan color separates prompts from output
- **Standard Format**: Familiar shell-like prompt syntax

#### **Input Handling (Lines 14-18):**
```cpp
if (!std::getline(std::cin, line))
    break;
if (line == "exit" || line == "quit")
    break;
if (line.empty())
    continue;
```

**Input Analysis:**

##### **EOF Detection (Lines 14-15):**
```cpp
if (!std::getline(std::cin, line))
    break;
```
- **`std::getline`**: Reads complete line including spaces
- **EOF Handling**: Breaks loop on Ctrl+D (Unix) or Ctrl+Z (Windows)
- **Stream State**: Checks input stream failure condition
- **Graceful Exit**: Clean termination without error messages

##### **Exit Commands (Lines 16-17):**
```cpp
if (line == "exit" || line == "quit")
    break;
```
- **Multiple Commands**: Supports both "exit" and "quit"
- **Case Sensitivity**: Exact string matching
- **User Friendly**: Common exit commands in many REPLs
- **Clean Break**: Exits loop without additional processing

##### **Empty Line Handling (Lines 18):**
```cpp
if (line.empty())
    continue;
```
- **Skip Processing**: Ignores empty input lines
- **User Experience**: Allows pressing Enter without errors
- **Efficiency**: Avoids unnecessary compilation of empty input
- **Loop Continuation**: Continues to next iteration

### **Compilation Pipeline (Lines 20-26)**
```cpp
try
{
    Lexer lexer(line);
    auto tokens = lexer.tokenize();
    Parser parser(std::move(tokens));
    auto ast = parser.parse();
    interp.execute(*ast);
}
```

**Pipeline Analysis:**

#### **Lexical Analysis (Line 22):**
```cpp
Lexer lexer(line);
```
- **Input**: Single line of user code
- **Output**: Token stream for parser
- **Line-based**: Each REPL line processed independently
- **Error Handling**: Lexer errors caught by outer try-catch

#### **Tokenization (Line 23):**
```cpp
auto tokens = lexer.tokenize();
```
- **Method Call**: Executes lexer's tokenization
- **Token Stream**: Produces vector of tokens
- **Move Semantics**: Tokens can be efficiently moved to parser
- **Intermediate Step**: Converts text to structured tokens

#### **Parsing (Lines 24-25):**
```cpp
Parser parser(std::move(tokens));
auto ast = parser.parse();
```
- **Token Transfer**: `std::move` efficiently transfers token ownership
- **Parser Creation**: Parser instance with token stream
- **AST Generation**: Creates Abstract Syntax Tree from tokens
- **Structured Output**: Tree representation of code structure

#### **Execution (Line 26):**
```cpp
interp.execute(*ast);
```
- **Interpreter Execution**: Executes AST using same interpreter instance
- **State Preservation**: Maintains variables and functions across lines
- **Pointer Dereference**: `*ast` accesses AST node content
- **Immediate Results**: Produces output or side effects

### **Error Handling (Lines 27-45)**
```cpp
catch (const ParseError &e)
{
    std::cerr << Colors::RED << "[ParseError] " << Colors::RESET << e.what() << " (line " << e.line << ")\n";
}
catch (const QuantumError &e)
{
    std::cerr << Colors::RED << "[" << e.kind << "] " << Colors::RESET << e.what();
    if (e.line > 0)
        std::cerr << " (line " << e.line << ")";
    std::cerr << "\n";
}
catch (const std::exception &e)
{
    std::cerr << Colors::RED << "[Error] " << Colors::RESET << e.what() << "\n";
}
```

**Error Handling Analysis:**

#### **ParseError Handling (Lines 28-30):**
```cpp
catch (const ParseError &e)
{
    std::cerr << Colors::RED << "[ParseError] " << Colors::RESET << e.what() << " (line " << e.line << ")\n";
}
```
- **Specific Exception**: Catches syntax and parsing errors
- **Error Context**: Includes line number from parser
- **Color Coding**: Red for error visibility
- **Clear Format**: `[ParseError]` prefix for easy identification

#### **QuantumError Handling (Lines 31-37):**
```cpp
catch (const QuantumError &e)
{
    std::cerr << Colors::RED << "[" << e.kind << "] " << Colors::RESET << e.what();
    if (e.line > 0)
        std::cerr << " (line " << e.line << ")";
    std::cerr << "\n";
}
```
- **Runtime Errors**: Catches execution-time errors
- **Error Kind**: Shows specific error type (TypeError, NameError, etc.)
- **Conditional Line**: Only shows line number if available
- **Consistent Format**: Brackets and color coding

#### **Generic Exception Handling (Lines 38-40):**
```cpp
catch (const std::exception &e)
{
    std::cerr << Colors::RED << "[Error] " << Colors::RESET << e.what() << "\n";
}
```
- **Fallback Handler**: Catches any other standard exceptions
- **Safety Net**: Prevents uncaught exceptions from crashing REPL
- **Simple Format**: Generic `[Error]` prefix
- **Complete Coverage**: Ensures all exceptions are handled

### **Session Termination (Lines 47-49)**
```cpp
std::cout << Colors::YELLOW << "\n  Goodbye! 👋\n"
          << Colors::RESET;
```

**Termination Analysis:**
- **Farewell Message**: Friendly goodbye with emoji
- **`Colors::YELLOW`**: Warm color for departure
- **Leading Newline**: Spacing from last prompt
- **Color Reset**: Clean state for subsequent output
- **User Experience**: Positive session closure

## Design Patterns and Architecture

### **REPL Pattern**
```
Initialize → Loop {
    Display Prompt → Read Input → Validate Input
    → Compile → Execute → Handle Errors
} → Cleanup → Exit
```
- **Interactive Loop**: Continuous user interaction
- **Immediate Feedback**: Each line processed immediately
- **State Preservation**: Interpreter maintains state across iterations
- **Error Recovery**: Errors don't terminate session

### **Compilation Pipeline Pattern**
```
Source Line → Lexer → Tokens → Parser → AST → Interpreter → Results
```
- **Linear Pipeline**: Each stage processes output of previous
- **Error Propagation**: Errors bubble up through pipeline
- **Resource Management**: Move semantics optimize transfers
- **State Sharing**: Single interpreter instance maintains context

### **Exception Handling Pattern**
```
try {
    // Compilation and execution
} catch (SpecificError) {
    // Targeted error handling
} catch (GeneralError) {
    // Fallback error handling
}
```
- **Specificity**: Different handlers for different error types
- **Context Preservation**: Error information maintained through catch blocks
- **User Experience**: Clear, actionable error messages
- **Robustness**: Multiple catch blocks ensure complete coverage

## Technical Considerations

### **Performance**
- **Single Interpreter**: Reuse across all REPL lines
- **Move Semantics**: Efficient token transfer to parser
- **Line-based Processing**: Minimal memory overhead per iteration
- **Exception Safety**: RAII ensures resource cleanup

### **Memory Management**
- **Stack Allocation**: Most objects created on stack
- **Smart Pointers**: Interpreter manages its own memory
- **Automatic Cleanup**: Exception safety prevents leaks
- **State Preservation**: Variables and functions persist across lines

### **User Experience**
- **Line Numbering**: Clear reference for error reporting
- **Color Coding**: Visual distinction between prompts, output, errors
- **Multiple Exit Methods**: Both commands and keyboard shortcuts
- **Empty Line Handling**: Natural behavior for interactive use

## Integration with Compiler Components

### **Lexer Integration**
```cpp
Lexer lexer(line);
auto tokens = lexer.tokenize();
```
- **Line-based Input**: Each REPL line processed independently
- **Token Stream**: Structured representation for parser
- **Error Handling**: Lexer errors caught by REPL exception handler
- **Stateless**: Fresh lexer for each line

### **Parser Integration**
```cpp
Parser parser(std::move(tokens));
auto ast = parser.parse();
```
- **Token Transfer**: Efficient move semantics
- **AST Generation**: Tree structure for interpreter
- **Error Context**: Line numbers preserved from parsing
- **Clean Interface**: Standard parser usage pattern

### **Interpreter Integration**
```cpp
Interpreter interp;
interp.execute(*ast);
```
- **Persistent State**: Single instance across REPL session
- **AST Execution**: Tree-walk interpretation
- **Variable Retention**: Definitions persist between lines
- **Function Definitions**: Available across session

## Why This Design Works

### **Interactive Excellence**
- **Immediate Feedback**: Users see results instantly
- **Error Context**: Clear error messages with line numbers
- **State Preservation**: Variables and functions persist across lines
- **Professional Presentation**: Colored prompts and formatted output

### **Robust Error Handling**
- **Specific Exceptions**: Different handlers for different error types
- **Graceful Recovery**: Errors don't terminate REPL session
- **User-Friendly Messages**: Clear, actionable error descriptions
- **Complete Coverage**: Multiple catch blocks prevent crashes

### **Efficient Resource Usage**
- **Single Interpreter**: Reuse across session reduces overhead
- **Move Semantics**: Efficient token transfer without copying
- **Stack Allocation**: Most objects use automatic storage
- **RAII Compliance**: Automatic resource cleanup

### **Developer Experience**
- **Familiar Shell Interface**: Standard REPL prompt format
- **Line Numbering**: Easy reference for debugging
- **Multiple Exit Methods**: Flexible session termination
- **Professional Appearance**: Color-coded output for clarity

The `runREPL()` function creates an excellent interactive development environment that combines the full power of the Quantum Language compiler with user-friendly interaction patterns, making it ideal for learning, experimentation, and rapid prototyping.
