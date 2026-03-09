# Value.h - Dynamic Type System and Runtime Environment

## Overview
`Value.h` defines the core dynamic type system for Quantum Language. It contains the `QuantumValue` class that can hold values of any type, the environment system for lexical scoping, and the object-oriented programming structures for classes and instances. The current version includes enhanced features like reference parameters and pointer types.

## Architecture Overview

The value system uses modern C++ features to implement:
- **Dynamic Typing**: Single type that can hold any value type
- **Memory Safety**: Smart pointers for automatic memory management
- **Object-Oriented Programming**: Classes, instances, inheritance, and methods
- **Functional Programming**: First-class functions with closures
- **Collection Types**: Arrays and dictionaries with comprehensive operations
- **NEW**: Reference Parameters and Pointer Types for advanced programming

## Line-by-Line Analysis

### Header Guard and Dependencies (Lines 1-8)
```cpp
#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <functional>
#include <variant>
#include <stdexcept>
```

**Dependency Analysis:**
- **`string`**: String value storage and manipulation
- **`vector`**: Array implementation and parameter lists
- **`unordered_map`**: Dictionary implementation and environment storage
- **`memory`**: Smart pointers for automatic memory management
- **`functional`**: Native function implementation
- **`variant`**: Type-safe union for dynamic typing
- **`stdexcept`**: Exception hierarchy base classes

### Forward Declarations (Lines 10-12)
```cpp
class Environment;
struct ASTNode;
class Interpreter;
```

**Forward Declaration Benefits:**
- **Break Circular Dependencies**: Prevents include loops
- **Compilation Efficiency**: Reduces compilation dependencies
- **Interface Separation**: Headers only need what they use

### Value Type Definitions (Lines 14-51)

#### QuantumNil Structure (Lines 16-18)
```cpp
struct QuantumNil
{
};
```
**Nil Type Features:**
- **Empty Struct**: Minimal memory footprint
- **Type Safety**: Distinct from other types
- **Semantic Clarity**: Represents absence of value

#### QuantumFunction Structure (Lines 20-27)
```cpp
struct QuantumFunction
{
    std::string name;
    std::vector<std::string> params;
    std::vector<bool> paramIsRef; // NEW: true = pass-by-reference (int& r)
    ASTNode *body;                // non-owning ptr
    std::shared_ptr<Environment> closure;
};
```

**Enhanced Function Components:**
- **`name`**: Function identifier for debugging and recursion
- **`params`**: Parameter names for arity checking and binding
- **`paramIsRef`**: **NEW** Vector tracking which parameters are references
- **`body`**: Non-owning pointer to AST (owned by parser/AST)
- **`closure`**: Captured environment for lexical scoping

#### QuantumClass and QuantumInstance (Lines 29-30)
```cpp
struct QuantumClass;
struct QuantumInstance;
```

**Forward Declarations:**
- **Circular References**: Classes and instances reference each other
- **Type Safety**: Complete type definitions later in file

#### QuantumNative Function Type (Line 32)
```cpp
using QuantumNativeFunc = std::function<struct QuantumValue(std::vector<struct QuantumValue>)>;
```

**Native Function Features:**
- **Type Alias**: Cleaner syntax for function signatures
- **std::function**: Type-erased function wrapper
- **Vector Arguments**: Variable number of arguments support
- **QuantumValue Return**: Consistent return type system

#### QuantumNative Structure (Lines 34-38)
```cpp
struct QuantumNative
{
    std::string name;
    QuantumNativeFunc fn;
};
```

**Native Function Wrapper:**
- **Name**: Function identifier for debugging and method calls
- **Function**: Actual C++ implementation
- **Bridge**: Connects Quantum Language to C++ functionality

#### Type Aliases (Lines 42-43)
```cpp
using Array = std::vector<QuantumValue>;
using Dict = std::unordered_map<std::string, QuantumValue>;
```

**Collection Type Definitions:**
- **Array**: Dynamic array with random access
- **Dict**: Hash map for key-value storage
- **QuantumValue**: Recursive type definitions enable nested structures

#### Pointer Type Definition (Lines 45-51)
```cpp
// ─── Pointer Type ─────────────────────────────────────────────────────────────

struct QuantumPointer
{
    std::shared_ptr<QuantumValue> cell; // live reference to variable storage
    std::string varName;                // for display/debug

    QuantumPointer(std::shared_ptr<QuantumValue> cell, const std::string& name);
    bool isNull() const;
    QuantumValue deref() const;
    void assign(QuantumValue value);
};
```

**NEW Pointer Features:**
- **Reference Semantics**: Provides pointer-like behavior for references
- **Live Reference**: Points to actual variable storage location
- **Variable Name Tracking**: Enhanced debugging with variable names
- **Null Safety**: Comprehensive null checking operations
- **Assignment Support**: Can modify the pointed-to value

### QuantumValue Class (Lines 53-95)

#### Data Variant Definition (Lines 55-69)
```cpp
struct QuantumValue {
    using Data = std::variant<
        QuantumNil,
        bool,
        double,
        std::string,
        std::shared_ptr<Array>,
        std::shared_ptr<Dict>,
        std::shared_ptr<QuantumFunction>,
        std::shared_ptr<QuantumNative>,
        std::shared_ptr<QuantumInstance>,
        std::shared_ptr<QuantumClass>,
        std::shared_ptr<QuantumPointer>  // NEW
    >;

    Data data;
```

**Enhanced Variant Type Analysis:**
1. **Primitive Types**: `QuantumNil`, `bool`, `double`, `std::string`
2. **Collection Types**: `Array`, `Dict` (as shared pointers)
3. **Callable Types**: `QuantumFunction`, `QuantumNative`
4. **Object Types**: `QuantumInstance`, `QuantumClass`
5. **NEW**: `QuantumPointer` for reference parameter support

**Design Benefits:**
- **Type Safety**: Compile-time checking of variant operations
- **Memory Efficiency**: Only stores active type, no overhead for unused types
- **Extensibility**: Easy to add new types to the variant
- **Pattern Matching**: Clean dispatch with `std::visit`

#### Constructors (Lines 71-82)
```cpp
    // Constructors
    QuantumValue() : data(QuantumNil{}) {}
    explicit QuantumValue(bool b)  : data(b) {}
    explicit QuantumValue(double d): data(d) {}
    explicit QuantumValue(const std::string& s) : data(s) {}
    explicit QuantumValue(std::string&& s)       : data(std::move(s)) {}
    explicit QuantumValue(std::shared_ptr<Array> a) : data(std::move(a)) {}
    explicit QuantumValue(std::shared_ptr<Dict>  d) : data(std::move(d)) {}
    explicit QuantumValue(std::shared_ptr<QuantumFunction> f) : data(std::move(f)) {}
    explicit QuantumValue(std::shared_ptr<QuantumNative>   n) : data(std::move(n)) {}
    explicit QuantumValue(std::shared_ptr<QuantumInstance> i) : data(std::move(i)) {}
    explicit QuantumValue(std::shared_ptr<QuantumClass>    c) : data(std::move(c)) {}
    explicit QuantumValue(std::shared_ptr<QuantumPointer>   p) : data(std::move(p)) {}  // NEW
```

**Enhanced Constructor Features:**
- **Default Constructor**: Creates nil value
- **Explicit Constructors**: Prevent implicit conversions
- **Move Semantics**: Efficient transfer of ownership
- **NEW**: Pointer constructor for reference parameter support

#### Type Check Methods (Lines 84-94)
```cpp
    // Type checks
    bool isNil()      const { return std::holds_alternative<QuantumNil>(data); }
    bool isBool()     const { return std::holds_alternative<bool>(data); }
    bool isNumber()   const { return std::holds_alternative<double>(data); }
    bool isString()   const { return std::holds_alternative<std::string>(data); }
    bool isArray()    const { return std::holds_alternative<std::shared_ptr<Array>>(data); }
    bool isDict()     const { return std::holds_alternative<std::shared_ptr<Dict>>(data); }
    bool isFunction() const { return std::holds_alternative<std::shared_ptr<QuantumFunction>>(data)
                                  || std::holds_alternative<std::shared_ptr<QuantumNative>>(data); }
    bool isInstance() const { return std::holds_alternative<std::shared_ptr<QuantumInstance>>(data); }
    bool isClass()    const { return std::holds_alternative<std::shared_ptr<QuantumClass>>(data); }
    bool isPointer()  const { return std::holds_alternative<std::shared_ptr<QuantumPointer>>(data); }  // NEW
```

**Enhanced Type Check Features:**
- **Const Methods**: Can be called on const values
- **Efficient**: `std::holds_alternative` is O(1) operation
- **Comprehensive**: Covers all variant types
- **Combined Checks**: `isFunction()` checks multiple callable types
- **NEW**: `isPointer()` for reference parameter detection

#### Accessor Methods (Lines 96-106)
```cpp
    // Accessors
    bool        asBool()   const { return std::get<bool>(data); }
    double      asNumber() const { return std::get<double>(data); }
    std::string asString() const { return std::get<std::string>(data); }
    std::shared_ptr<Array>    asArray()    const { return std::get<std::shared_ptr<Array>>(data); }
    std::shared_ptr<Dict>     asDict()     const { return std::get<std::shared_ptr<Dict>>(data); }
    std::shared_ptr<QuantumFunction> asFunction() const { return std::get<std::shared_ptr<QuantumFunction>>(data); }
    std::shared_ptr<QuantumNative>   asNative()   const { return std::get<std::shared_ptr<QuantumNative>>(data); }
    std::shared_ptr<QuantumInstance> asInstance() const { return std::get<std::shared_ptr<QuantumInstance>>(data); }
    std::shared_ptr<QuantumClass>    asClass()    const { return std::get<std::shared_ptr<QuantumClass>>(data); }
    std::shared_ptr<QuantumPointer>   asPointer()  const { return std::get<std::shared_ptr<QuantumPointer>>(data); }  // NEW
```

**Enhanced Accessor Features:**
- **Type Safety**: `std::get` throws if wrong type accessed
- **Efficient**: Direct access without type checking overhead
- **Const Correctness**: Preserves const semantics
- **Smart Pointers**: Returns shared pointers for reference counting
- **NEW**: Pointer accessor for reference parameter operations

#### Utility Methods (Lines 108-110)
```cpp
    bool isTruthy() const;
    std::string toString() const;
    std::string typeName() const;
```

**Utility Functions:**
- **`isTruthy()`**: Truth value evaluation for conditionals (enhanced with pointer support)
- **`toString()`**: String representation for output
- **`typeName()`**: Type name for error messages

### Environment Class (Lines 112-124)

#### Class Definition (Lines 114-124)
```cpp
class Environment : public std::enable_shared_from_this<Environment> {
public:
    explicit Environment(std::shared_ptr<Environment> parent = nullptr);

    void define(const std::string& name, QuantumValue val, bool isConst = false);
    QuantumValue get(const std::string& name) const;
    void set(const std::string& name, QuantumValue val);
    bool has(const std::string& name) const;

    std::shared_ptr<Environment> parent;

private:
    std::unordered_map<std::string, QuantumValue> vars;
    std::unordered_map<std::string, bool> constants;
};
```

**Environment Features:**
- **Inheritance**: `std::enable_shared_from_this` for shared_from_this()
- **Parent Chain**: Lexical scoping through parent pointers
- **Variable Storage**: Hash map for O(1) lookup
- **Const Tracking**: Separate map for constant variables
- **Public Interface**: Clean API for variable management

### Object-Oriented Programming Structures

#### QuantumClass Structure (Lines 126-132)
```cpp
struct QuantumClass {
    std::string name;
    std::shared_ptr<QuantumClass> base;
    std::unordered_map<std::string, std::shared_ptr<QuantumFunction>> methods;
    std::unordered_map<std::string, std::shared_ptr<QuantumFunction>> staticMethods;
    std::unordered_map<std::string, QuantumValue> staticFields;
};
```

**Class Features:**
- **Inheritance**: Single inheritance through base class pointer
- **Instance Methods**: Regular methods operating on instances
- **Static Methods**: Class-level methods not requiring instances
- **Static Fields**: Class-level variables shared across instances

#### QuantumInstance Structure (Lines 134-140)
```cpp
struct QuantumInstance {
    std::shared_ptr<QuantumClass> klass;
    std::unordered_map<std::string, QuantumValue> fields;
    std::shared_ptr<Environment> env;

    QuantumValue getField(const std::string& name) const;
    void setField(const std::string& name, QuantumValue val);
};
```

**Instance Features:**
- **Class Reference**: Pointer to class definition
- **Instance Fields**: Per-instance state storage
- **Environment**: Closure for method execution
- **Field Access**: Methods for getting and setting fields

### Control Flow Signals (Lines 142-148)

#### Signal Structures (Lines 142-148)
```cpp
// ─── Control Flow Signals ────────────────────────────────────────────────────

struct ReturnSignal {
    QuantumValue value;
    explicit ReturnSignal(QuantumValue v) : value(std::move(v)) {}
};

struct BreakSignal  {};
struct ContinueSignal {};
```

**Control Flow Features:**
- **Exception-based Control**: Uses exceptions for non-local control flow
- **Return Values**: `ReturnSignal` carries return values
- **Simple Signals**: `BreakSignal` and `ContinueSignal` are empty
- **Efficient**: Zero-cost when no exceptions thrown

## New Features in Current Version

### Reference Parameter Support
```cpp
std::vector<bool> paramIsRef; // true = pass-by-reference (int& r)
std::shared_ptr<QuantumPointer> cell; // live reference to variable storage
```

**Reference Parameter Features:**
- **Parameter Tracking**: `paramIsRef` vector identifies reference parameters
- **Pointer Types**: `QuantumPointer` provides reference-like semantics
- **Variable Name Tracking**: Enhanced debugging with variable names
- **Null Safety**: Comprehensive checking for pointer operations

### Enhanced Type System
```cpp
std::shared_ptr<QuantumPointer>  // NEW in variant
bool isPointer() const;           // NEW type check
std::shared_ptr<QuantumPointer> asPointer() const;  // NEW accessor
```

**Type System Enhancements:**
- **Pointer Integration**: Full integration with existing type system
- **Truthiness Support**: Pointers evaluate to false if null or pointing to null
- **String Representation**: Proper string formatting for pointer types
- **Error Handling**: Enhanced error messages with variable name context

### Improved Function Support
```cpp
struct QuantumFunction {
    std::vector<bool> paramIsRef; // NEW: Reference parameter tracking
    // ...
};
```

**Function Enhancements:**
- **Reference Parameters**: Support for `int& ref` syntax
- **Parameter Metadata**: Enhanced parameter information
- **Closure Support**: Maintains existing closure functionality
- **Type Safety**: Runtime checking of reference parameter usage

## Design Patterns and Architecture

### Variant Pattern
```cpp
using Data = std::variant<QuantumNil, bool, double, std::string, /* ... */, std::shared_ptr<QuantumPointer>>;
```

**Benefits:**
- **Type Safety**: Compile-time checking of all operations
- **Memory Efficiency**: No overhead for unused types
- **Pattern Matching**: Clean dispatch with `std::visit`
- **Extensibility**: Easy to add new types

### Environment Chain Pattern
```
Global Environment
├── Function Closure
│   └── Local Scope
└── Another Function
    └── Block Scope
```

**Features:**
- **Lexical Scoping**: Variables follow static scope rules
- **Closures**: Functions capture their defining environment
- **Memory Management**: Shared pointers prevent dangling references

### Smart Pointer Pattern
```cpp
std::shared_ptr<Array>
std::shared_ptr<Dict>
std::shared_ptr<QuantumPointer>
```

**Benefits:**
- **Automatic Memory Management**: Reference counting handles cleanup
- **Shared Ownership**: Multiple references to same objects
- **Reference Semantics**: Pointer types provide reference-like behavior

## Performance Considerations

### Small Object Optimization
```cpp
std::variant<QuantumNil, bool, double, std::string, /* ... */>
```

**Optimizations:**
- **Stack Storage**: Small types stored inline in variant
- **No Heap Allocation**: Primitive types don't require heap allocation
- **Cache Friendly**: Contiguous memory layout for small types

### Move Semantics
```cpp
explicit QuantumValue(std::string&& s) : data(std::move(s)) {}
```

**Benefits:**
- **Reduced Allocations**: Avoids copying large objects
- **Efficient Transfers**: Moves ownership instead of copying
- **RAII**: Automatic resource management

### Hash Table Lookup
```cpp
std::unordered_map<std::string, QuantumValue> vars;
```

**Performance Features:**
- **O(1) Average Lookup**: Constant-time variable access
- **Good Cache Locality**: Efficient memory access patterns
- **Dynamic Sizing**: Automatic resizing as needed

## Type Safety and Error Handling

### Variant Type Safety
```cpp
bool isNumber() const { return std::holds_alternative<double>(data); }
double asNumber() const { return std::get<double>(data); }
```

**Safety Features:**
- **Compile-time Checking**: Variant operations are type-checked
- **Runtime Validation**: `std::get` throws on type mismatch
- **Clear Semantics**: Each type has distinct behavior

### Pointer Safety
```cpp
bool isNull() const;
QuantumValue deref() const;
void assign(QuantumValue value);
```

**Safety Features:**
- **Null Checking**: Prevents dereferencing null pointers
- **Error Messages**: Clear errors for invalid operations
- **Variable Name Context**: Enhanced debugging information

## Integration with Interpreter

### Value Creation and Management
```cpp
// In Interpreter.cpp
QuantumValue result = evaluate(node);
env->define("x", result);
```

**Integration Features:**
- **Unified Type System**: All values use QuantumValue
- **Automatic Memory Management**: Smart pointers handle cleanup
- **Type Conversions**: Safe conversion between types

### Function and Method Calls
```cpp
// Function closure capture
auto fn = std::make_shared<QuantumFunction>();
fn->closure = currentEnv;

// Reference parameter support
if (paramIsRef[i]) {
    auto ptr = std::make_shared<QuantumPointer>(variableCell, paramName);
    args[i] = QuantumValue(ptr);
}
```

**OOP Integration:**
- **Closures**: Functions capture lexical environment
- **Method Resolution**: Instance and class method lookup
- **Reference Parameters**: Enhanced function call semantics

## Why This Design Works

### Modern C++ Features
- **Type-safe Variants**: Eliminates runtime type errors
- **Smart Pointers**: Automatic memory management
- **Move Semantics**: Performance optimization
- **Template Metaprogramming**: Compile-time optimizations

### Language Design Goals
- **Dynamic Typing**: Flexible, script-like behavior
- **Reference Semantics**: Advanced parameter passing capabilities
- **Object-Oriented**: Modern programming paradigms
- **Performance**: Efficient implementation for production use

### Enhanced Features
- **Reference Parameters**: Support for pass-by-reference syntax
- **Pointer Types**: Reference-like behavior with null safety
- **Variable Name Tracking**: Better debugging and error messages
- **Type Extensions**: Easy to add new types to the variant system

The value system provides a robust foundation for Quantum Language's runtime, combining the flexibility of dynamic typing with the performance and safety of modern C++ implementation, while adding new features that enhance the programming experience with advanced parameter passing capabilities.
