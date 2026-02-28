# The `explicit` Keyword in C++

## Overview

The `explicit` keyword in C++ is used to prevent implicit conversions and copy initialization. It's particularly important for constructors that can be called with a single argument.

## Purpose

The `explicit` keyword prevents:
1. **Implicit conversions** from single-argument constructors
2. **Copy initialization** using the `=` syntax
3. **Function argument conversion** when passing arguments

## Syntax

```cpp
class MyClass {
public:
    explicit MyClass(int value);  // Prevents implicit conversion
    MyClass(double value);        // Allows implicit conversion
};
```

## Examples

### Without `explicit` (Implicit Conversion Allowed)

```cpp
class Number {
public:
    Number(int value) : value_(value) {}
    
private:
    int value_;
};

void process(Number n) {}

int main() {
    Number n = 42;        // OK: Implicit conversion from int
    process(123);         // OK: Implicit conversion from int
    return 0;
}
```

### With `explicit` (Implicit Conversion Prevented)

```cpp
class Number {
public:
    explicit Number(int value) : value_(value) {}
    
private:
    int value_;
};

void process(Number n) {}

int main() {
    Number n = 42;        // ERROR: No implicit conversion
    Number n(42);        // OK: Direct initialization
    process(123);         // ERROR: No implicit conversion
    process(Number(123)); // OK: Explicit conversion
    return 0;
}
```

## Benefits

1. **Prevents Unexpected Conversions**: Avoids bugs from unintended type conversions
2. **Clearer Code**: Makes the programmer's intent explicit
3. **Better Error Messages**: Compilation errors are more descriptive
4. **Type Safety**: Enforces strict type checking

## When to Use `explicit`

### Use `explicit` when:
- Constructor has a single parameter
- The parameter represents a fundamental type conversion
- The conversion could be ambiguous or lossy
- You want to prevent accidental conversions

### Don't use `explicit` when:
- Constructor has multiple parameters
- The conversion is natural and expected
- You want to support implicit conversion for convenience

## Real-World Example

```cpp
class Money {
public:
    explicit Money(double amount) : cents_(static_cast<long>(amount * 100)) {}
    
    // Allow explicit construction from string
    explicit Money(const std::string& amount);
    
private:
    long cents_;
};

void processPayment(Money payment) {}

int main() {
    Money m1 = 10.50;        // ERROR: No implicit conversion
    Money m2(10.50);          // OK: Explicit construction
    Money m3 = Money(10.50);  // OK: Explicit construction
    
    processPayment(25.0);     // ERROR: No implicit conversion
    processPayment(Money(25.0)); // OK: Explicit conversion
    
    return 0;
}
```

## In Quantum Language

In the Quantum Language compiler, `explicit` is used in constructors like:

```cpp
class Token {
public:
    explicit Token(TokenType t, std::string v, int ln, int c);
    // Prevents: Token t = TokenType::NUMBER;
    // Requires: Token t(TokenType::NUMBER, "42", 1, 1);
};
```

## C++11 and Later

Since C++11, `explicit` can also be used with conversion operators:

```cpp
class MyString {
public:
    explicit operator bool() const {
        return !empty();
    }
};
```

## Best Practices

1. **Default to explicit**: Make single-argument constructors explicit by default
2. **Document exceptions**: If you allow implicit conversion, document why
3. **Consider move semantics**: `explicit` works with move constructors too
4. **Test conversions**: Write tests to verify conversion behavior

## Summary

The `explicit` keyword is a powerful tool for preventing bugs and making code more maintainable. It forces programmers to be explicit about type conversions, leading to safer and more understandable code.
