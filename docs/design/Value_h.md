# Value.h - Dynamic Type System and Runtime Environment

## Overview
`Value.h` defines the core dynamic type system for Quantum Language. It contains the `QuantumValue` class that can hold values of any type, the environment system for lexical scoping, and the object-oriented programming structures for classes and instances.

## Architecture Overview

The value system uses modern C++ features to implement:
- **Dynamic Typing**: Single type that can hold any value type
- **Memory Safety**: Smart pointers for automatic memory management
- **Object-Oriented Programming**: Classes, instances, inheritance, and methods
- **Functional Programming**: First-class functions with closures
- **Collection Types**: Arrays and dictionaries with comprehensive operations

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

### Value Type Definitions (Lines 14-35)

#### QuantumNil Structure (Line 16)
```cpp
struct QuantumNil {};
```
**Nil Type Features:**
- **Empty Struct**: Minimal memory footprint
- **Type Safety**: Distinct from other types
- **Semantic Clarity**: Represents absence of value

#### QuantumFunction Structure (Lines 18-23)
```cpp
struct QuantumFunction {
    std::string name;
    std::vector<std::string> params;
    ASTNode* body;                          // non-owning ptr
    std::shared_ptr<Environment> closure;
};
```

**Function Components:**
- **`name`**: Function identifier for debugging and recursion
- **`params`**: Parameter names for arity checking and binding
- **`body`**: Non-owning pointer to AST (owned by parser/AST)
- **`closure`**: Captured environment for lexical scoping

#### QuantumNative Function Type (Line 28)
```cpp
using QuantumNativeFunc = std::function<struct QuantumValue(std::vector<struct QuantumValue>)>;
```

**Native Function Features:**
- **Type Alias**: Cleaner syntax for function signatures
- **std::function**: Type-erased function wrapper
- **Vector Arguments**: Variable number of arguments support
- **QuantumValue Return**: Consistent return type system

#### QuantumNative Structure (Lines 30-33)
```cpp
struct QuantumNative {
    std::string name;
    QuantumNativeFunc fn;
};
```

**Native Function Wrapper:**
- **Name**: Function identifier for debugging and method calls
- **Function**: Actual C++ implementation
- **Bridge**: Connects Quantum Language to C++ functionality

#### Type Aliases (Lines 37-38)
```cpp
using Array   = std::vector<QuantumValue>;
using Dict    = std::unordered_map<std::string, QuantumValue>;
```

**Collection Type Definitions:**
- **Array**: Dynamic array with random access
- **Dict**: Hash map for key-value storage
- **QuantumValue**: Recursive type definitions enable nested structures

### QuantumValue Class (Lines 40-95)

#### Data Variant Definition (Lines 41-52)
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
        std::shared_ptr<QuantumClass>
    >;

    Data data;
```

**Variant Type Analysis:**
1. **Primitive Types**: `QuantumNil`, `bool`, `double`, `std::string`
2. **Collection Types**: `Array`, `Dict` (as shared pointers)
3. **Callable Types**: `QuantumFunction`, `QuantumNative`
4. **Object Types**: `QuantumInstance`, `QuantumClass`

**Design Benefits:**
- **Type Safety**: Compile-time checking of variant operations
- **Memory Efficiency**: Only stores active type, no overhead for unused types
- **Extensibility**: Easy to add new types to the variant
- **Pattern Matching**: Clean dispatch with `std::visit`

#### Constructors (Lines 56-67)
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
```

**Constructor Features:**
- **Default Constructor**: Creates nil value
- **Explicit Constructors**: Prevent implicit conversions
- **Move Semantics**: Efficient transfer of ownership
- **Overloaded**: Separate constructor for each type

#### Type Check Methods (Lines 69-79)
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
```

**Type Check Features:**
- **Const Methods**: Can be called on const values
- **Efficient**: `std::holds_alternative` is O(1) operation
- **Comprehensive**: Covers all variant types
- **Combined Checks**: `isFunction()` checks multiple callable types

#### Accessor Methods (Lines 81-90)
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
```

**Accessor Features:**
- **Type Safety**: `std::get` throws if wrong type accessed
- **Efficient**: Direct access without type checking overhead
- **Const Correctness**: Preserves const semantics
- **Smart Pointers**: Returns shared pointers for reference counting

#### Utility Methods (Lines 92-94)
```cpp
    bool isTruthy() const;
    std::string toString() const;
    std::string typeName() const;
```

**Utility Functions:**
- **`isTruthy()`**: Truth value evaluation for conditionals
- **`toString()`**: String representation for output
- **`typeName()`**: Type name for error messages

### Environment Class (Lines 97-113)

#### Class Definition (Lines 99-113)
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

#### QuantumClass Structure (Lines 117-123)
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

#### QuantumInstance Structure (Lines 125-132)
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

### Control Flow Signals (Lines 136-143)

#### Signal Structures (Lines 136-143)
```cpp
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

## Design Patterns and Architecture

### Variant Pattern
```cpp
using Data = std::variant<QuantumNil, bool, double, std::string, /* ... */>;
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
std::shared_ptr<QuantumFunction>
```

**Benefits:**
- **Automatic Memory Management**: Reference counting handles cleanup
- **Shared Ownership**: Multiple references to same objects
- **Circular Reference Handling**: Potential for weak_ptr usage
- **Exception Safety**: Automatic cleanup on exceptions

## Performance Optimizations

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

### Environment Safety
```cpp
QuantumValue get(const std::string& name) const {
    auto it = vars.find(name);
    if (it != vars.end()) return it->second;
    if (parent) return parent->get(name);
    throw NameError("Undefined variable: '" + name + "'");
}
```

**Error Handling:**
- **Graceful Failure**: Clear error messages for undefined variables
- **Chain Traversal**: Searches entire parent chain
- **Exception Throwing**: Uses custom exception types

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

// Method resolution
auto method = instance->getField("methodName");
```

**OOP Integration:**
- **Closures**: Functions capture lexical environment
- **Method Resolution**: Instance and class method lookup
- **Inheritance**: Base class method access

## Why This Design Works

### Modern C++ Features
- **Type-safe Variants**: Eliminates runtime type errors
- **Smart Pointers**: Automatic memory management
- **Move Semantics**: Performance optimization
- **Template Metaprogramming**: Compile-time optimizations

### Language Design Goals
- **Dynamic Typing**: Flexible, script-like behavior
- **Object-Oriented**: Modern programming paradigms
- **Functional**: First-class functions and closures
- **Performance**: Efficient implementation for production use

### Maintainability and Extensibility
- **Clear Separation**: Each component has single responsibility
- **Extensible**: Easy to add new types and operations
- **Testable**: Clean interfaces for unit testing
- **Documented**: Self-documenting code with clear patterns

The value system provides a robust foundation for Quantum Language's runtime, combining the flexibility of dynamic typing with the performance and safety of modern C++ implementation.
