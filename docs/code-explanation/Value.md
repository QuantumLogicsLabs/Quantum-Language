# Value.cpp - Dynamic Type System and Runtime Environment

## Overview
`Value.cpp` implements the core dynamic type system for Quantum Language. This file contains the `QuantumValue` class implementation, environment management for lexical scoping, and object-oriented programming support with classes and instances. The current version includes enhanced features like reference parameters and pointer types.

## Architecture Overview

The value system uses modern C++ `std::variant` to implement a dynamic type system that can hold:
- Primitive types (nil, bool, number, string)
- Collection types (arrays, dictionaries)
- Callable types (functions, native functions)
- Object-oriented types (classes, instances)
- **NEW**: Pointer types for reference-like behavior

## Line-by-Line Analysis

### Includes and Dependencies (Lines 1-6)
```cpp
#include "../include/Value.h"
#include "../include/Error.h"
#include <sstream>
#include <cmath>
#include <iomanip>
#include <cstdint>
```
- **Line 1**: Core Value system definitions
- **Line 2**: Exception hierarchy for type errors
- **Lines 3-6**: Standard libraries for string manipulation, math operations, formatting, and integer types

### QuantumValue::isTruthy() (Lines 10-22)
```cpp
bool QuantumValue::isTruthy() const
{
    return std::visit([](const auto &v) -> bool
                      {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, QuantumNil>)    return false;
        if constexpr (std::is_same_v<T, bool>)          return v;
        if constexpr (std::is_same_v<T, double>)        return v != 0.0;
        if constexpr (std::is_same_v<T, std::string>)   return !v.empty();
        if constexpr (std::is_same_v<T, std::shared_ptr<Array>>) return !v->empty();
        if constexpr (std::is_same_v<T, std::shared_ptr<QuantumPointer>>) return v && !v->isNull();
        return true; }, data);
}
```

**Enhanced Truthiness Logic:**
- **Pattern Matching**: Uses `std::visit` with generic lambda for type-safe dispatch
- **Truthiness Rules**: Follows language-specific rules:
  - `nil` → false
  - `bool` → actual boolean value
  - `number` → false only if exactly 0.0
  - `string` → false only if empty
  - `array` → false only if empty
  - **NEW**: `pointer` → false if null or points to null value
  - **Default**: All other types (functions, classes, instances) are truthy
- **Compile-time Dispatch**: `if constexpr` ensures only relevant branches are compiled

### QuantumValue::toString() (Lines 24-90)
```cpp
std::string QuantumValue::toString() const
{
    return std::visit([](const auto &v) -> std::string
                      {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, QuantumNil>)  return "nil";
        if constexpr (std::is_same_v<T, bool>)        return v ? "true" : "false";
        if constexpr (std::is_same_v<T, double>) {
            if (std::floor(v) == v && std::abs(v) < 1e15)
                return std::to_string((long long)v);
            std::ostringstream oss;
            oss << std::setprecision(10) << v;
            return oss.str();
        }
        if constexpr (std::is_same_v<T, std::string>) return v;
        if constexpr (std::is_same_v<T, std::shared_ptr<Array>>) {
            std::string s = "[";
            for (size_t i = 0; i < v->size(); i++) {
                if (i) s += ", ";
                if ((*v)[i].isString()) s += "\"" + (*v)[i].toString() + "\"";
                else s += (*v)[i].toString();
            }
            return s + "]";
        }
        // ... continues for all types including pointer support ...
    }, data);
}
```

**Key Features:**
- **Smart Number Formatting**: Integers display without decimal points, large numbers with scientific notation
- **Collection Formatting**: Arrays and dictionaries with proper nesting and quotes
- **Object Representation**: Functions show as `<fn:name>`, instances as `<instance:Class>`
- **String Quoting**: Strings in arrays are properly quoted for readability

### Environment Class Implementation (Lines 95-125)

#### Constructor (Lines 95-95)
```cpp
Environment::Environment(std::shared_ptr<Environment> p) : parent(std::move(p)) {}
```
- **Parent Chain**: Establishes lexical scoping through parent pointers
- **Shared Ownership**: Uses `shared_ptr` for automatic memory management

#### define() Method (Lines 97-100)
```cpp
void Environment::define(const std::string& name, QuantumValue val, bool isConst) {
    vars[name] = std::move(val);
    if (isConst) constants[name] = true;
}
```
- **Variable Storage**: Stores name-value pairs in unordered_map
- **Const Tracking**: Separate map tracks constant variables
- **Move Semantics**: Efficient value transfer with `std::move`

#### get() Method (Lines 102-107)
```cpp
QuantumValue Environment::get(const std::string& name) const {
    auto it = vars.find(name);
    if (it != vars.end()) return it->second;
    if (parent) return parent->get(name);
    throw NameError("Undefined variable: '" + name + "'");
}
```
- **Lexical Scoping**: Searches current scope first, then parent chain
- **Error Handling**: Throws `NameError` if variable not found in any scope

#### set() Method (Lines 109-119)
```cpp
void Environment::set(const std::string& name, QuantumValue val) {
    auto it = vars.find(name);
    if (it != vars.end()) {
        if (constants.count(name))
            throw RuntimeError("Cannot reassign constant '" + name + "'");
        it->second = std::move(val);
        return;
    }
    if (parent) { parent->set(name, std::move(val)); return; }
    throw NameError("Undefined variable: '" + name + "'");
}
```
- **Const Protection**: Prevents modification of constant variables
- **Scope Resolution**: Finds the correct scope where variable is defined
- **Assignment Semantics**: Only modifies existing variables, doesn't create new ones

### QuantumInstance Implementation (Lines 127-146)

#### getField() Method (Lines 129-141)
```cpp
QuantumValue QuantumInstance::getField(const std::string& name) const {
    auto it = fields.find(name);
    if (it != fields.end()) return it->second;
    // Check methods
    auto k = klass.get();
    while (k) {
        auto mit = k->methods.find(name);
        if (mit != k->methods.end())
            return QuantumValue(mit->second);
        k = k->base.get();
    }
    throw NameError("No field/method '" + name + "' on instance of " + klass->name);
}
```

**Field Resolution Order:**
1. **Instance Fields**: First check instance-specific fields
2. **Instance Methods**: Check class methods (including inherited)
3. **Inheritance Chain**: Walk up base class hierarchy
4. **Error**: Throw if not found anywhere

#### setField() Method (Lines 143-145)
```cpp
void QuantumInstance::setField(const std::string& name, QuantumValue val) {
    fields[name] = std::move(val);
}
```
- **Simple Assignment**: Direct field storage on instance
- **No Method Override**: Cannot override methods with fields (by design)

### QuantumPointer Implementation (Lines 148-170)
```cpp
QuantumPointer::QuantumPointer(std::shared_ptr<QuantumValue> cell, const std::string& name)
    : cell(std::move(cell)), varName(name) {}

bool QuantumPointer::isNull() const {
    return !cell || cell->isNil();
}

QuantumValue QuantumPointer::deref() const {
    if (!cell) {
        throw RuntimeError("Dereferencing null pointer");
    }
    return *cell;
}

void QuantumPointer::assign(QuantumValue value) {
    if (!cell) {
        throw RuntimeError("Assignment to null pointer");
    }
    *cell = std::move(value);
}
```

**NEW Pointer Features:**
- **Reference Semantics**: `QuantumPointer` provides reference-like behavior
- **Null Safety**: Checks for null pointer before dereferencing
- **Variable Name Tracking**: Stores variable name for debugging
- **Assignment Support**: Can modify the pointed-to value

## New Features in Current Version

### Reference Parameters Support
The value system now supports pass-by-reference parameters through:
- **`paramIsRef` vector**: Tracks which parameters are references
- **`QuantumPointer` type**: Provides pointer-like semantics
- **Enhanced truthiness**: Pointers evaluate to false if null or pointing to null

### Enhanced Type System
- **Pointer Types**: First-class pointer values with dereferencing
- **Reference Tracking**: Variable names stored for better error messages
- **Null Safety**: Comprehensive null checking for pointer operations

### Improved Error Handling
- **Better Context**: Error messages include variable names
- **Null Pointer Detection**: Specific errors for dereferencing null pointers
- **Type Safety**: Enhanced runtime type checking

## Design Patterns and Architecture

### Variant-Based Type System
```cpp
using Data = std::variant<
    QuantumNil, bool, double, std::string,
    std::shared_ptr<Array>, std::shared_ptr<Dict>,
    std::shared_ptr<QuantumFunction>, std::shared_ptr<QuantumNative>,
    std::shared_ptr<QuantumInstance>, std::shared_ptr<QuantumClass>,
    std::shared_ptr<QuantumPointer>  // NEW
>;
```

**Benefits:**
- **Type Safety**: Compile-time checking of variant operations
- **Memory Efficiency**: Only stores active type, no overhead for unused types
- **Extensibility**: Easy to add new types to the variant
- **Pattern Matching**: Clean dispatch with `std::visit`

### Environment Chain Pattern
```
Global Environment
    └── Function Scope
        └── Block Scope
            └── Current Scope
```

**Features:**
- **Lexical Scoping**: Variables follow static scope rules
- **Closures**: Functions capture their defining environment
- **Memory Safety**: Shared pointers prevent dangling references

### Object-Oriented System
```cpp
struct QuantumClass {
    std::string name;
    std::shared_ptr<QuantumClass> base;           // Inheritance
    std::unordered_map<std::string, std::shared_ptr<QuantumFunction>> methods;
    std::unordered_map<std::string, std::shared_ptr<QuantumFunction>> staticMethods;
    std::unordered_map<std::string, QuantumValue> staticFields;
};

struct QuantumInstance {
    std::shared_ptr<QuantumClass> klass;
    std::unordered_map<std::string, QuantumValue> fields;  // Instance state
    std::shared_ptr<Environment> env;                     // Method closure
};
```

**OOP Features:**
- **Inheritance**: Single inheritance with base class pointer
- **Methods**: Instance and static method support
- **Fields**: Per-instance state storage
- **Closures**: Methods have access to defining environment

## Performance Optimizations

### Smart Pointer Usage
- **Shared Ownership**: Multiple references to same objects
- **Reference Counting**: Automatic memory management
- **Weak References**: Potential for breaking cycles (not implemented yet)

### String Operations
- **Move Semantics**: Efficient string transfers
- **String Streams**: Optimized formatting for complex types
- **Reserve Optimization**: Pre-allocation for known sizes

### Collection Operations
- **Unordered Maps**: O(1) average lookup for fields and variables
- **Vector Storage**: Contiguous memory for arrays
- **Reserve Capacity**: Pre-allocation for expected sizes

## Error Handling Strategy

### Type Safety
```cpp
static double toNum(const QuantumValue &v, const std::string &ctx) {
    if (v.isNumber())
        return v.asNumber();
    throw TypeError("Expected number in " + ctx + ", got " + v.typeName());
}
```

**Features:**
- **Context Information**: Error messages include operation context
- **Type Information**: Shows actual vs expected types
- **Graceful Degradation**: Clear error messages for debugging

### Pointer Safety
```cpp
QuantumValue QuantumPointer::deref() const {
    if (!cell) {
        throw RuntimeError("Dereferencing null pointer");
    }
    return *cell;
}
```

**Safety Features:**
- **Null Checking**: Prevents dereferencing null pointers
- **Clear Errors**: Specific error messages for pointer operations
- **Variable Names**: Enhanced debugging with variable name tracking

## Integration with Interpreter

### Value Creation and Management
```cpp
// In Interpreter.cpp
QuantumValue result = evaluate(node);
env->define("x", result);
```

### Function and Method Calls
```cpp
// Function closure capture
auto fn = std::make_shared<QuantumFunction>();
fn->closure = currentEnv;

// Method resolution
auto method = instance->getField("methodName");
```

### Reference Parameter Support
```cpp
// Pass-by-reference in function calls
if (paramIsRef[i]) {
    // Create pointer to variable
    auto ptr = std::make_shared<QuantumValue>(argValue);
    args[i] = QuantumValue(std::make_shared<QuantumPointer>(ptr, paramName));
}
```

## Why This Design Works

### Modern C++ Features
- **Type-safe Variants**: Eliminates runtime type errors
- **Smart Pointers**: Automatic memory management
- **Move Semantics**: Performance optimization
- **Template Metaprogramming**: Compile-time optimizations

### Language Design Goals
- **Dynamic Typing**: Flexible, script-like behavior
- **Reference Semantics**: Support for pass-by-reference parameters
- **Object-Oriented**: Modern programming paradigms
- **Performance**: Efficient implementation for production use

### Enhanced Features
- **Pointer Types**: Reference-like behavior for advanced programming
- **Null Safety**: Comprehensive error checking for pointer operations
- **Better Debugging**: Variable name tracking in error messages
- **Type Extensions**: Easy to add new types to the variant system

The Value system forms the foundation of Quantum Language's runtime, providing a robust, efficient, and extensible platform for dynamic language features while maintaining the performance characteristics expected from a C++ implementation, with new features that enhance the programming experience.
