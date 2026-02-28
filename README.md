<div align="center">

```
  ██████╗ ██╗   ██╗ █████╗ ███╗   ██╗████████╗██╗   ██╗███╗   ███╗
 ██╔═══██╗██║   ██║██╔══██╗████╗  ██║╚══██╔══╝██║   ██║████╗ ████║
 ██║   ██║██║   ██║███████║██╔██╗ ██║   ██║   ██║   ██║██╔████╔██║
 ██║▄▄ ██║██║   ██║██╔══██║██║╚██╗██║   ██║   ██║   ██║██║╚██╔╝██║
 ╚██████╔╝╚██████╔╝██║  ██║██║ ╚████║   ██║   ╚██████╔╝██║ ╚═╝ ██║
  ╚══▀▀═╝  ╚═════╝ ╚═╝  ╚═╝╚═╝  ╚═══╝   ╚═╝    ╚═════╝ ╚═╝     ╚═╝
```

**The Multi-Paradigm Scripting Language Built for Cybersecurity**

![Version](https://img.shields.io/badge/version-1.0.0-cyan)
![Language](https://img.shields.io/badge/built%20with-C%2B%2B17-blue)
![Platform](https://img.shields.io/badge/platform-Windows%20%7C%20Linux%20%7C%20macOS-lightgrey)
![License](https://img.shields.io/badge/license-MIT-green)

</div>

---

## What is Quantum?

**Quantum** is a dynamically-typed, tree-walk interpreted scripting language built from scratch in C++17. It is designed to feel familiar to developers from any background — you can write it like Python, JavaScript, or C/C++ and it will just work. Quantum files use the `.sa` extension.

Quantum is built with cybersecurity tooling in mind, with reserved keywords and built-in functions for encoding, XOR operations, hex manipulation, and more planned features around scanning, payloads, and cryptography.

---

## Features

- **Multi-syntax support** — Python, JavaScript, and C/C++ syntax all work in the same file
- **Dynamic typing** — no type declarations required; variables infer their type at runtime
- **Tree-walk interpreter** — clean and hackable execution engine written in C++17
- **REPL mode** — interactive shell for quick experimentation
- **Rich standard library** — math, string, array, dict, I/O, time, randomness, encoding
- **Cybersecurity builtins** — `xor_bytes`, `base64_encode`, `to_hex`, `from_hex`, `rot13`
- **First-class functions** — closures, lambdas, arrow functions, anonymous functions
- **Classes and instances** — OOP with inheritance
- **Bitwise operations** — full support for `&`, `|`, `^`, `~`, `<<`, `>>`

---

## Installation

### Prerequisites

- C++17 compatible compiler (MSVC, GCC, Clang)
- CMake 3.15+

### Build from Source

```bash
git clone https://github.com/yourusername/quantum-lang.git
cd quantum-lang
mkdir build && cd build
cmake ..
cmake --build . --config Release
```

The compiled binary will be at `build/Release/quantum.exe` (Windows) or `build/quantum` (Linux/macOS).

### Add to PATH (Windows)

```powershell
$env:PATH += ";C:\path\to\quantum\build\Release"
```

---

## Usage

```bash
# Run a script
quantum script.sa

# Start interactive REPL
quantum

# Check syntax without running
quantum --check script.sa

# Show version
quantum --version

# Show help
quantum --help
```

---

## Language Guide

### Variables

Quantum supports four declaration styles — use whichever feels natural:

```python
# Python-style (no keyword needed)
name = "Alice"
score = 100
active = true

# Quantum-style
let x = 42
const MAX = 1000

# C-style (with type coercion)
int count = 0
float pi = 3.14
double big = 1e15
char initial = "A"
string message = "hello"
bool flag = false
```

### Functions

All five function styles are supported:

```python
# Quantum style
fn add(a, b) {
    return a + b
}

# Python style
def greet(name):
    return "Hello, " + name + "!"

def factorial(n):
    if n <= 1:
        return 1
    return n * factorial(n - 1)

# JavaScript style
function multiply(a, b) {
    return a * b
}

# Arrow function — expression body (implicit return)
double = (x) => x * 2
add    = (a, b) => a + b
triple = x => x * 3

# Arrow function — block body
clamp = (x, min, max) => {
    if x < min: return min
    if x > max: return max
    return x
}

# Anonymous function assigned to variable
square = fn(n) { return n * n }
square = function(n) { return n * n }
```

### Control Flow

```python
# Python style (colon + indentation)
if score > 90:
    print("A grade")
elif score > 75:
    print("B grade")
else:
    print("Try harder")

# Brace style
if score > 90 {
    print("A grade")
} else if score > 75 {
    print("B grade")
} else {
    print("Try harder")
}

# Single-line (no braces)
if x > 0 print("positive")
```

### Loops

```python
# While loop
i = 0
while i < 10:
    print(i)
    i++

# For-in loop
for item in ["a", "b", "c"]:
    print(item)

# Range-based
for n in range(1, 6):
    print(n * n)

# C-style post/pre increment
count = 0
count++
++count
count--
```

### Output & Input

Quantum supports three I/O styles interchangeably:

```python
# Python / Quantum style
print("Hello, World!")
print("x =", x, "| y =", y)

# C-style printf / scanf
printf("Score: %d out of %d\n", score, total)
scanf("%d", &value)

# C++ style cout / cin
cout << "Enter your name: "
cin >> name
cout << "Hello, " << name << endl
cout << "Value: " << x + y << "\n"
```

### Arrays

```python
nums = [10, 20, 30, 40, 50]

nums.push(60)            # append
nums.pop()               # remove last
nums.slice(1, 4)         # [20, 30, 40]
len(nums)                # 5

doubled = nums.map(fn(n) { return n * 2 })
evens   = nums.filter(fn(n) { return n % 2 == 0 })
total   = nums.reduce(fn(acc, n) { return acc + n }, 0)
nums.sort()
nums.reverse()
nums.includes(30)        # true
nums.index_of(30)        # 2
```

### Dictionaries

```python
user = {
    "name":  "Alice",
    "role":  "pentester",
    "level": 99
}

print(user["name"])           # Alice
user["tools"] = ["nmap", "burp"]
print(user.get("tools"))
print(user.keys())
print(user.values())
user.remove("level")
```

### Strings

```python
msg = "  Hello Quantum World  "

msg.trim()                    # "Hello Quantum World"
msg.upper()                   # "  HELLO QUANTUM WORLD  "
msg.lower()
msg.split(" ")                # array of words
msg.replace("World", "Earth")
msg.contains("Quantum")       # true
msg.starts_with("  Hello")    # true
msg.ends_with("World  ")      # true
len(msg)                      # 23
```

### Closures

```python
fn make_counter(start):
    count = start
    return fn():
        count += 1
        return count

counter = make_counter(0)
print(counter())   # 1
print(counter())   # 2
print(counter())   # 3
```

### Classes

```python
class Animal:
    fn init(name, sound):
        self.name = name
        self.sound = sound

    fn speak():
        return self.name + " says " + self.sound

class Dog extends Animal:
    fn fetch(item):
        return self.name + " fetches the " + item

dog = Dog("Rex", "Woof")
print(dog.speak())
print(dog.fetch("ball"))
```

### Bitwise Operations

Essential for cybersecurity and low-level work:

```python
a = 0xFF
b = 0x0F

print(a & b)        # AND  → 15
print(a | b)        # OR   → 255
print(a ^ b)        # XOR  → 240
print(~a)           # NOT  → -256
print(1 << 8)       # SHL  → 256
print(256 >> 4)     # SHR  → 16
print(hex(a & b))   # 0xF
```

---

## Standard Library

### Math
| Function | Description |
|----------|-------------|
| `abs(x)` | Absolute value |
| `sqrt(x)` | Square root |
| `pow(x, y)` | x to the power y |
| `floor(x)` | Round down |
| `ceil(x)` | Round up |
| `round(x)` | Round to nearest |
| `log(x)` | Natural logarithm |
| `log2(x)` | Base-2 logarithm |
| `sin(x)` / `cos(x)` / `tan(x)` | Trig functions |
| `min(...)` / `max(...)` | Min/max of values |
| `PI` / `E` / `INF` | Constants |

### Type Conversion
| Function | Description |
|----------|-------------|
| `num(x)` | Convert to number |
| `str(x)` | Convert to string |
| `bool(x)` | Convert to boolean |
| `type(x)` | Get type name |
| `chr(n)` | Number to character |
| `ord(c)` | Character to number |

### Encoding & Cybersecurity
| Function | Description |
|----------|-------------|
| `hex(n)` | Number to hex string |
| `bin(n)` | Number to binary string |
| `to_hex(s)` | String bytes to hex |
| `from_hex(s)` | Hex string to bytes |
| `xor_bytes(a, b)` | XOR two byte strings |
| `base64_encode(s)` | Base64 encode |
| `rot13(s)` | ROT13 cipher |

### Utility
| Function | Description |
|----------|-------------|
| `len(x)` | Length of string/array/dict |
| `range(n)` / `range(a, b)` | Generate number range |
| `rand()` / `rand(a, b)` | Random float |
| `rand_int(a, b)` | Random integer |
| `time()` | Unix timestamp (seconds) |
| `sleep(s)` | Sleep for s seconds |
| `assert(cond, msg)` | Assert condition |
| `exit(code)` | Exit program |
| `format(fmt, ...)` | Format string (sprintf-style) |
| `keys(dict)` / `values(dict)` | Dict keys/values |

---

## Format Strings

Quantum supports C-style `printf` format strings:

```
%d  %i   → integer
%f  %e   → float
%s       → string
%c       → character
%x  %X   → hex (lower/upper)
%o       → octal
%b       → binary
%%       → literal %
```

Width, precision, and flags are supported:

```python
printf("%-10s %6.2f\n", "Price:", 3.14159)
printf("%08X\n", 255)
s = format("Hello, %s! You scored %d%%", name, score)
```

---

## Reserved Cybersecurity Keywords

The following keywords are reserved for upcoming cybersecurity features:

| Keyword | Planned Purpose |
|---------|----------------|
| `scan` | Network scanning |
| `payload` | Exploit payload construction |
| `encrypt` | Cryptographic encryption |
| `decrypt` | Cryptographic decryption |
| `hash` | Hashing algorithms (MD5, SHA) |

---

## CLI Reference

```
quantum <file.sa>          Run a Quantum script
quantum                    Start interactive REPL
quantum --check <file.sa>  Check syntax only (no execution)
quantum --version          Show version information
quantum --help             Show usage help
quantum --aura             Show project achievement board
```

---

## Project Structure

```
quantum-lang/
├── src/
│   ├── main.cpp          # Entry point, REPL, file runner
│   ├── Lexer.cpp         # Tokenizer with INDENT/DEDENT support
│   ├── Parser.cpp        # Recursive descent parser
│   ├── Interpreter.cpp   # Tree-walk interpreter + native functions
│   ├── Value.cpp         # Value types and Environment
│   └── Token.cpp         # Token utilities
├── include/
│   ├── AST.h             # AST node variant definitions
│   ├── Error.h           # Error types + ANSI color codes
│   ├── Interpreter.h
│   ├── Lexer.h
│   ├── Parser.h
│   ├── Token.h
│   └── Value.h
├── examples/
│   ├── Python/           # Python-syntax examples
│   ├── C/                # C-syntax examples
│   └── C++/              # C++-syntax examples
└── CMakeLists.txt
```

---

## Example Programs

### Fibonacci (Python style)
```python
def fib(n):
    if n <= 1:
        return n
    return fib(n - 1) + fib(n - 2)

for i in range(10):
    print(fib(i))
```

### XOR Encryption (Cybersecurity)
```python
fn xor_encrypt(text, key):
    return xor_bytes(text, key)

message   = "Hello, Quantum!"
key       = "secret"
encrypted = xor_encrypt(message, key)
decrypted = xor_encrypt(encrypted, key)

print("Encrypted:", to_hex(encrypted))
print("Decrypted:", decrypted)
```

### FizzBuzz (C style)
```c
int i = 1
while(i <= 30) {
    if(i % 15 == 0)      printf("FizzBuzz\n")
    else if(i % 3 == 0)  printf("Fizz\n")
    else if(i % 5 == 0)  printf("Buzz\n")
    else                 printf("%d\n", i)
    i++
}
```

### Higher-Order Functions (JavaScript style)
```javascript
const numbers = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10]

const evens   = numbers.filter(x => x % 2 == 0)
const squares = evens.map(x => x * x)
const total   = squares.reduce((acc, x) => acc + x, 0)

console.log("Sum of squares of evens:", total)
```

---

## Built By

**Muhammad Saad Amin**

Quantum Language v1.0.0 — A cybersecurity-ready scripting language built from scratch in C++17.