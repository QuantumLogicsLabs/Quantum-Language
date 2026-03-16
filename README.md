<div align="center">

```
  ██████╗ ██╗   ██╗ █████╗ ███╗   ██╗████████╗██╗   ██╗███╗   ███╗
 ██╔═══██╗██║   ██║██╔══██╗████╗  ██║╚══██╔══╝██║   ██║████╗ ████║
 ██║   ██║██║   ██║███████║██╔██╗ ██║   ██║   ██║   ██║██╔████╔██║
 ██║▄▄ ██║██║   ██║██╔══██║██║╚██╗██║   ██║   ██║   ██║██║╚██╔╝██║
 ╚██████╔╝╚██████╔╝██║  ██║██║ ╚████║   ██║   ╚██████╔╝██║ ╚═╝ ██║
  ╚══▀▀═╝  ╚═════╝ ╚═╝  ╚═╝╚═╝  ╚═══╝   ╚═╝    ╚═════╝ ╚═╝     ╚═╝
```

**Quantum Language v2.0.0 — Bytecode VM Edition**

_A multi-paradigm scripting language built in C++17._
_Python clarity · JavaScript flexibility · C++ power · Now with a bytecode VM._

[![Language](https://img.shields.io/badge/language-C%2B%2B17-blue.svg)](https://isocpp.org/)
[![Build](https://img.shields.io/badge/build-CMake-green.svg)](https://cmake.org/)
[![Runtime](https://img.shields.io/badge/runtime-Bytecode%20VM-brightgreen.svg)]()
[![Platform](https://img.shields.io/badge/platform-Windows%20%7C%20Linux%20%7C%20macOS-lightgrey.svg)]()
[![License](https://img.shields.io/badge/license-MIT-orange.svg)](LICENSE)
[![Extension](https://img.shields.io/badge/VS%20Code-Extension%20v1.1.0-blueviolet.svg)](extensions/)

</div>

---

## What's New in v2.0

v2.0 replaces the original tree-walk interpreter with a **full bytecode compiler + stack VM**. The language itself is 100% backwards-compatible — every `.sa` file that ran on v1 runs identically on v2. The only thing that changed is what happens under the hood.

|                      | v1.0 — Tree-Walk                              | v2.0 — Bytecode VM                              |
| -------------------- | --------------------------------------------- | ----------------------------------------------- |
| **Execution engine** | Recursive `evaluate(ASTNode&)`                | Linear `switch` over flat instruction array     |
| **Variable access**  | `unordered_map` hash lookup per variable      | Integer array-offset per local                  |
| **Call stack**       | C++ call stack (bounded by OS stack size)     | Explicit `CallFrame` vector (heap, unbounded)   |
| **Closures**         | `shared_ptr<Environment>` captured by value   | Upvalue cells — heap-promoted stack slots       |
| **Loop overhead**    | Virtual dispatch + `std::visit` per iteration | Single opcode fetch + switch per iteration      |
| **Constants**        | Re-evaluated every time they are reached      | Pre-compiled into `Chunk::constants[]` once     |
| **New CLI flags**    | `--check`, `--test`, `--help`                 | + `--debug` (dump + run) · `--dis` (dump only)  |
| **Stack overflow**   | OS stack overflow on deep recursion           | Controlled `RuntimeError` from `frames_` vector |

---

## Benchmarks

All benchmarks run on the same machine (Ryzen 5 5600, 16 GB RAM, Windows 11, MSVC Release build `-O2`). Each test ran 5 times; the median is shown. "Speedup" is `v1 time / v2 time`.

### Fibonacci (recursive, n=30)

```python
fn fib(n) {
    if n <= 1 { return n }
    return fib(n - 1) + fib(n - 2)
}
print(fib(30))
```

|             | v1 Tree-Walk               | v2 Bytecode VM           | Speedup  |
| ----------- | -------------------------- | ------------------------ | -------- |
| Time        | 4 210 ms                   | 980 ms                   | **4.3×** |
| Call frames | ~2.7 M recursive C++ calls | 2.7 M `CallFrame` pushes | —        |

The gain is almost entirely from eliminating `std::visit` on every recursive call and replacing hash-map variable lookup with `stack_[base + slot]`.

---

### Tight Counting Loop (10 million iterations)

```python
let sum = 0
for i in range(10000000) {
    sum += i
}
print(sum)
```

|                   | v1 Tree-Walk          | v2 Bytecode VM                | Speedup  |
| ----------------- | --------------------- | ----------------------------- | -------- |
| Time              | 8 750 ms              | 1 920 ms                      | **4.6×** |
| Instructions/iter | ~18 AST nodes visited | 6 bytecode opcodes dispatched | —        |

Per-iteration the VM executes: `LOAD_LOCAL`, `LOAD_LOCAL`, `ADD`, `STORE_LOCAL`, `FOR_ITER`, `LOOP` — six integer comparisons inside one `while` loop vs. 18 heap-allocated AST node visits with virtual dispatch.

---

### Bubble Sort (array of 2 000 elements)

```python
fn bubble_sort(arr) {
    let n = len(arr)
    for i in range(n) {
        for j in range(n - i - 1) {
            if arr[j] > arr[j + 1] {
                let temp = arr[j]
                arr[j] = arr[j + 1]
                arr[j + 1] = temp
            }
        }
    }
    return arr
}
```

|      | v1 Tree-Walk | v2 Bytecode VM | Speedup  |
| ---- | ------------ | -------------- | -------- |
| Time | 5 340 ms     | 1 480 ms       | **3.6×** |

Most of the remaining overhead is `GET_INDEX` / `SET_INDEX` bounds-checking and `shared_ptr` reference counting on the array, which both versions share.

---

### Deep Closure / Higher-Order Functions

```python
fn make_adder(x) {
    return fn(y) { return x + y }
}
let add5 = make_adder(5)
let total = 0
for i in range(500000) {
    total += add5(i)
}
print(total)
```

|      | v1 Tree-Walk | v2 Bytecode VM | Speedup  |
| ---- | ------------ | -------------- | -------- |
| Time | 6 100 ms     | 1 350 ms       | **4.5×** |

Upvalue read (`LOAD_UPVALUE`) is a single `*cell` pointer dereference. The v1 path walks the `Environment` parent chain and performs a hash lookup on every invocation.

---

### String Processing (100 000 concatenations)

```python
let result = ""
for i in range(100000) {
    result = str(i) + ","
}
print(len(result))
```

|      | v1 Tree-Walk | v2 Bytecode VM | Speedup  |
| ---- | ------------ | -------------- | -------- |
| Time | 1 230 ms     | 1 050 ms       | **1.2×** |

String-heavy workloads see a smaller gain because the bottleneck is `std::string` allocation, which both versions do identically. The VM overhead reduction matters less when the work per iteration is expensive already.

---

### Summary

| Benchmark          | v1 (ms) | v2 (ms) | Speedup  |
| ------------------ | ------- | ------- | -------- |
| Fibonacci n=30     | 4 210   | 980     | **4.3×** |
| Counting loop 10M  | 8 750   | 1 920   | **4.6×** |
| Bubble sort 2K     | 5 340   | 1 480   | **3.6×** |
| Closures 500K      | 6 100   | 1 350   | **4.5×** |
| String concat 100K | 1 230   | 1 050   | **1.2×** |
| **Geometric mean** |         |         | **3.2×** |

> Compute-bound and call-heavy programs benefit the most. I/O-bound and allocation-heavy programs see a smaller but still measurable gain.

---

## What is Quantum?

Quantum is a **custom scripting language** implemented from scratch in C++17. It blends the readability of Python, the expressiveness of JavaScript, and optional C-style syntax into one cohesive language. The `.sa` extension stands for **Saad** — named after its creator, Muhammad Saad Amin.

```python
fn greet(name) {
    print("Hello, " + name + "! Welcome to Quantum.")
}

greet("World")

for i in range(5) {
    print(i * i)
}

let double = (x) => x * 2
print(double(21))
```

---

## Feature Highlights

### Multi-Paradigm Syntax

| Feature   | Python        | JavaScript                    | C++            |
| --------- | ------------- | ----------------------------- | -------------- |
| Functions | `def foo():`  | `function foo()` / `(x) => x` | `fn foo() {}`  |
| Variables | `x = 5`       | `let x = 5`                   | `int x = 5`    |
| Printing  | `print(x)`    | `console.log(x)`              | `cout << x`    |
| Input     | `x = input()` | —                             | `cin >> x`     |
| Classes   | `class Foo:`  | `class Foo {}`                | `class Foo {}` |

### Type System

- Dynamic typing by default — no annotations required
- Optional static hints with gradual checking: `fn add(a: int, b: int) -> int`
- Type-checked warnings at parse time via the built-in `TypeChecker`
- Supported types: `int` · `float` · `double` · `char` · `string` · `bool` · `void`

### Object-Oriented Programming

```python
class Animal {
    fn __init__(self, name, sound) {
        self.name = name
        self.sound = sound
    }
    fn speak(self) {
        print(self.name + " says " + self.sound)
    }
}

class Dog extends Animal {
    fn __init__(self, name) {
        super.__init__(name, "Woof")
    }
}

let d = new Dog("Rex")
d.speak()
```

### Closures

```python
fn make_counter() {
    let count = 0
    return fn() {
        count += 1
        return count
    }
}

let counter = make_counter()
print(counter())   # 1
print(counter())   # 2
print(counter())   # 3
```

### C++ Pointer Semantics

```cpp
int x = 42
int* p = &x
print(*p)    # 42
*p = 100
print(x)     # 100
```

### Exception Handling

```python
try {
    raise "Something went wrong"
} except (e) {
    print("Caught: " + e)
} finally {
    print("Always runs")
}
```

### List Comprehensions

```python
let squares = [x * x for x in range(10) if x % 2 == 0]
print(squares)
```

---

## Standard Library

### Math

`abs` · `sqrt` · `floor` · `ceil` · `round` · `pow` · `log` · `log2` · `log10` · `sin` · `cos` · `tan` · `asin` · `acos` · `atan` · `atan2` · `min` · `max`

Constants: `PI` · `E` · `INF` · `NaN`

### Type Conversion

`num()` · `int()` · `float()` · `str()` · `bool()` · `chr()` · `ord()`

### Sequences

`len()` · `range()` · `enumerate()` · `zip()` · `map()` · `filter()` · `sorted()` · `reversed()` · `sum()`

### Introspection

`type()` · `isinstance()` · `id()`

### Array Methods

`.push()` · `.pop()` · `.shift()` · `.unshift()` · `.sort()` · `.reverse()` · `.join()` · `.slice()` · `.splice()` · `.concat()` · `.includes()` · `.indexOf()` · `.flat()` · `.fill()` · `.count()` · `.insert()` · `.remove()` · `.clear()` · `.copy()` · `.extend()`

### String Methods

`.upper()` · `.lower()` · `.trim()` · `.split()` · `.replace()` · `.replaceAll()` · `.startsWith()` · `.endsWith()` · `.includes()` · `.indexOf()` · `.substring()` · `.charAt()` · `.charCodeAt()` · `.repeat()` · `.padStart()` · `.padEnd()` · `.isdigit()` · `.isalpha()` · `.format()` · `.count()`

### Dict Methods

`.keys()` · `.values()` · `.items()` · `.has()` · `.get()` · `.set()` · `.delete()` · `.clear()` · `.size()`

### I/O

`input()` · `print()` · `scanf()` · `printf()` · `cout <<` · `cin >>`

---

## Installation & Build

### Prerequisites

- C++17 compiler (MSVC 2019+, GCC 9+, Clang 10+)
- CMake 3.16+

### Windows

```bat
build.bat
```

Or manually:

```bat
mkdir build && cd build
cmake ..
cmake --build . --config Release
```

### Linux / macOS

```bash
mkdir build && cd build
cmake ..
make -j$(nproc)
```

Two binaries are produced:

| Binary           | Description                         |
| ---------------- | ----------------------------------- |
| `quantum_vm`     | v2.0 Bytecode VM (recommended)      |
| `quantum_interp` | v1.0 Tree-walk interpreter (legacy) |

---

## Usage

```bash
quantum_vm <file.sa>          # Run a script
quantum_vm                    # Interactive REPL
quantum_vm --debug <file.sa>  # Dump bytecode then execute
quantum_vm --dis   <file.sa>  # Print bytecode only, no execution
quantum_vm --check <file.sa>  # Type-check only, no execution
quantum_vm --test  <dir>      # Batch-run all .sa files under <dir>
quantum_vm --version
quantum_vm --help
```

### Disassembly Example

```bash
quantum_vm --dis examples/hello.sa
```

```
=== <script> ===
  constants: 4  params: 0  upvalues: 0
    0  LOAD_CONST         [0] 'Hello, World!'
    1  LOAD_CONST         [1] ' '
    2  LOAD_CONST         [2] '\n'
    3  PRINT              argc=1
    4  RETURN_NIL
```

---

## Project Structure

```
Quantum Language/
│
├── include/
│   ├── AST.h
│   ├── Compiler.h
│   ├── Disassembler.h
│   ├── Error.h
│   ├── Interpreter.h
│   ├── Lexer.h
│   ├── Opcode.h
│   ├── Parser.h
│   ├── Token.h
│   ├── TypeChecker.h
│   ├── Value.h
│   └── VM.h
│
├── src/
│   ├── Compiler.cpp
│   ├── Disassembler.cpp
│   ├── Interpreter.cpp
│   ├── Lexer.cpp
│   ├── main.cpp
│   ├── main_vm.cpp
│   ├── Parser.cpp
│   ├── Token.cpp
│   ├── TypeChecker.cpp
│   ├── Value.cpp
│   └── VM.cpp
│
├── examples/
│   ├── demo/
│   │   ├── demo01.sa
│   │   ├── demo02.sa
│   │   ├── demo03.sa
│   │   ├── demo04.sa
│   │   ├── demo05.sa
│   │   ├── demo06.sa
│   │   ├── demo07.sa
│   │   ├── demo08.sa
│   │   ├── demo09.sa
│   │   ├── demo10.sa
│   │   ├── demo11.sa
│   │   ├── demo12.sa
│   │   └── demo13.sa
│   ├── library/
│   │   ├── C/
│   │   │   ├── algorithms/
│   │   │   │   ├── binary search.sa
│   │   │   │   ├── breadth first search.sa
│   │   │   │   ├── bubble sort.sa
│   │   │   │   ├── depth first search.sa
│   │   │   │   ├── fibonacci.sa
│   │   │   │   ├── heap sort.sa
│   │   │   │   ├── insertion sort.sa
│   │   │   │   ├── prime sieve.sa
│   │   │   │   ├── quick sort.sa
│   │   │   │   └── selection sort.sa
│   │   │   ├── data_structures/
│   │   │   │   ├── binary tree.sa
│   │   │   │   ├── hash table.sa
│   │   │   │   ├── linked list.sa
│   │   │   │   ├── queue.sa
│   │   │   │   └── stack.sa
│   │   │   ├── examples/
│   │   │   │   ├── c.sa
│   │   │   │   └── memory management.sa
│   │   │   ├── games/
│   │   │   │   ├── guess game.sa
│   │   │   │   ├── rock paper scissors.sa
│   │   │   │   ├── snake game.sa
│   │   │   │   └── tic tac toe.sa
│   │   │   ├── mathematical/
│   │   │   │   ├── factorial.sa
│   │   │   │   ├── gcd lcm.sa
│   │   │   │   ├── matrix multiplication.sa
│   │   │   │   ├── power function.sa
│   │   │   │   └── prime numbers.sa
│   │   │   └── string_operations/
│   │   │       ├── palindrome.sa
│   │   │       ├── reverse string.sa
│   │   │       ├── string length.sa
│   │   │       ├── string validation.sa
│   │   │       ├── text encryption.sa
│   │   │       └── text processing.sa
│   │   ├── C++/
│   │   │   ├── fundamentals/
│   │   │   │   ├── Assignments/
│   │   │   │   │   ├── Assignment 1/
│   │   │   │   │   ├── Assignment 2/
│   │   │   │   │   ├── Assignment 3/
│   │   │   │   │   └── Assignment 4/
│   │   │   │   └── Lab/
│   │   │   │       ├── Lab 2/
│   │   │   │       ├── Lab 3/
│   │   │   │       ├── Lab 4/
│   │   │   │       ├── Lab 5/
│   │   │   │       ├── Lab 6/
│   │   │   │       ├── Lab 7/
│   │   │   │       ├── Lab 9/
│   │   │   │       └── Mid/
│   │   │   ├── games/
│   │   │   │   └── guess game.sa
│   │   │   └── OOPs/
│   │   │       ├── Lab 1/
│   │   │       ├── Lab 2/
│   │   │       ├── Lab 3/
│   │   │       ├── Lab 4/
│   │   │       ├── Lab 5/
│   │   │       ├── Lab 6/
│   │   │       └── Lab 7/
│   │   ├── Javascript/
│   │   │   ├── algorithms/
│   │   │   │   ├── astar pathfinding.sa
│   │   │   │   ├── chunky monkey.sa
│   │   │   │   ├── dungeon generator.sa
│   │   │   │   ├── markov chain.sa
│   │   │   │   ├── maze generator.sa
│   │   │   │   └── mutations.sa
│   │   │   ├── applications/
│   │   │   │   ├── profile lookup.sa
│   │   │   │   ├── recipe tracker.sa
│   │   │   │   └── record collection.sa
│   │   │   ├── data_structures/
│   │   │   │   ├── binary search tree.sa
│   │   │   │   └── stack.sa
│   │   │   ├── examples/
│   │   │   │   └── script.sa
│   │   │   ├── games/
│   │   │   │   ├── countdown timer.sa
│   │   │   │   ├── game of life.sa
│   │   │   │   ├── quiz game.sa
│   │   │   │   └── solar system.sa
│   │   │   ├── mathematical/
│   │   │   │   ├── factorial calculator.sa
│   │   │   │   ├── mandelbrot.sa
│   │   │   │   ├── math interpreter.sa
│   │   │   │   └── stats calculator.sa
│   │   │   ├── sorting_searching/
│   │   │   │   ├── binary search.sa
│   │   │   │   ├── bubble sort.sa
│   │   │   │   ├── linear search.sa
│   │   │   │   ├── quick sort.sa
│   │   │   │   └── selection sort.sa
│   │   │   ├── string_operations/
│   │   │   │   ├── caesar cipher.sa
│   │   │   │   ├── dna analyzer.sa
│   │   │   │   ├── longest word finder.sa
│   │   │   │   ├── missing letter detector.sa
│   │   │   │   ├── morse code.sa
│   │   │   │   ├── password generator.sa
│   │   │   │   ├── sentence analyzer.sa
│   │   │   │   └── string repeating function.sa
│   │   │   ├── system_tools/
│   │   │   │   ├── file reader.sa
│   │   │   │   ├── simple logger.sa
│   │   │   │   ├── storage helper.sa
│   │   │   │   └── timer utility.sa
│   │   │   ├── validation/
│   │   │   │   ├── email validator.sa
│   │   │   │   ├── form fields.sa
│   │   │   │   ├── password checker.sa
│   │   │   │   └── phone validator.sa
│   │   │   └── web_development/
│   │   │       ├── api fetch.sa
│   │   │       ├── button click.sa
│   │   │       ├── element creator.sa
│   │   │       └── style changer.sa
│   │   └── Python/
│   │       ├── advanced_patterns/
│   │       ├── algorithms/
│   │       │   ├── binary search.sa
│   │       │   ├── bisection method.sa
│   │       │   ├── breadth first search.sa
│   │       │   ├── depth first search.sa
│   │       │   ├── merge sort.sa
│   │       │   ├── n queens.sa
│   │       │   ├── nth fibonacci number calculator.sa
│   │       │   ├── quicksort.sa
│   │       │   ├── selection sort.sa
│   │       │   ├── shortest path.sa
│   │       │   └── tower of hanoi.sa
│   │       ├── applications/
│   │       │   ├── budget app.sa
│   │       │   ├── discount calculator.sa
│   │       │   ├── email simulator.sa
│   │       │   ├── media catalog.sa
│   │       │   ├── medical data validator.sa
│   │       │   ├── musical instrument inventory.sa
│   │       │   ├── salary tracker.sa
│   │       │   └── user configuration manager.sa
│   │       ├── data_structures/
│   │       │   ├── adjacency list to matrix converter.sa
│   │       │   ├── hash table.sa
│   │       │   └── linked list.sa
│   │       ├── examples/
│   │       │   ├── functions.sa
│   │       │   ├── if else.sa
│   │       │   ├── input.sa
│   │       │   ├── output.sa
│   │       │   ├── test budget.sa
│   │       │   ├── test2.sa
│   │       │   └── tutorial.sa
│   │       ├── games/
│   │       │   ├── game character stats tracker.sa
│   │       │   ├── guess game.sa
│   │       │   └── rpg character.sa
│   │       ├── mathematical/
│   │       │   ├── number pattern generator.sa
│   │       │   └── polygon area calculator.sa
│   │       ├── simple_programs/
│   │       │   ├── binary_converter.sa
│   │       │   ├── calculator.sa
│   │       │   ├── fibonacci_sequence.sa
│   │       │   ├── prime_numbers.sa
│   │       │   ├── quiz_game.sa
│   │       │   ├── sorting_algorithms.sa
│   │       │   ├── stopwatch.sa
│   │       │   ├── tic_tac_toe.sa
│   │       │   ├── to_do_list.sa
│   │       │   └── unit_converter.sa
│   │       ├── string_operations/
│   │       │   ├── caesar cipher.sa
│   │       │   ├── luhn algorithm.sa
│   │       │   └── pin extractor.sa
│   │       └── utilities/
│   │           ├── isbn validator.sa
│   │           ├── planet class.sa
│   │           └── player interface.sa
│   ├── advanced.sa
│   ├── cybersec.sa
│   ├── features.sa
│   ├── hello.sa
│   ├── if-else.sa
│   ├── IO Operations.sa
│   ├── joke.sa
│   ├── quantum_cybersecurity_suite.sa
│   ├── scanf_test.sa
│   ├── simple.sa
│   └── test_type_system.sa
│
├── tests/
│   ├── advanced.sa
│   ├── date helper.sa
│   ├── dungeon.sa
│   ├── file operations.sa
│   ├── form input.sa
│   ├── guess_number.sa
│   ├── hangman.sa
│   ├── input sanitizer.sa
│   ├── linear search.sa
│   ├── merge sort.sa
│   ├── minesweeper.sa
│   ├── password_generator.sa
│   ├── snake.sa
│   ├── text_analyzer.sa
│   ├── Tower of Hanoi Algorithm.sa
│   ├── url helper.sa
│   └── User Configuration Manager.sa
│
├── docs/
│   ├── code-explanation/
│   │   ├── include/
│   │   │   ├── AST/
│   │   │   ├── Error/
│   │   │   ├── Interpreter/
│   │   │   ├── Lexer/
│   │   │   ├── Parser/
│   │   │   ├── Token/
│   │   │   ├── typechecker/
│   │   │   └── Value/
│   │   └── src/
│   │       ├── interpreter/
│   │       ├── lexer/
│   │       ├── main/
│   │       ├── parser/
│   │       ├── token/
│   │       ├── typechecker/
│   │       └── value/
│   ├── design/
│   │   ├── 01-architecture-overview.md
│   │   ├── 02-language-specification.md
│   │   ├── 03-implementation-details.md
│   │   ├── 04-standard-library.md
│   │   ├── 05-testing-strategy.md
│   │   ├── 06-future-roadmap.md
│   │   └── 07-project-summary.md
│   ├── concepts/
│   │   └── explicit-keyword-in-cpp.md
│   ├── ARCHITECTURE.md
│   ├── BYTECODE_VM.md
│   ├── CODE_OF_CONDUCT.md
│   ├── CONTRIBUTING.md
│   ├── FUTURE_PLAN.md
│   ├── ROADMAP.md
│   ├── SECURITY.md
│   ├── SETUP.md
│   └── SYNTAX.md
│
├── extensions/
│   ├── src/
│   │   ├── extension.ts
│   │   └── server.ts
│   ├── syntaxes/
│   │   └── quantum.tmlanguage.json
│   ├── snippets/
│   │   └── quantum.json
│   ├── out/
│   │   ├── extension.js
│   │   ├── extension.js.map
│   │   ├── server.js
│   │   └── server.js.map
│   ├── language-configuration.json
│   ├── package.json
│   ├── tsconfig.json
│   └── quantum-language-1.1.0.vsix
│
├── Website/
│   ├── styles/
│   │   ├── ide.css
│   │   ├── language.css
│   │   └── styles.css
│   ├── logo/
│   │   ├── logo.png
│   │   └── logoWithWhiteBackground.png
│   ├── ide.html
│   ├── index.html
│   └── language.html
│
├── .vscode/
│   ├── c_cpp_properties.json
│   ├── extensions.json
│   ├── launch.json
│   ├── quantum-language.json
│   ├── setting.json
│   └── tasks.json
│
├── CMakeLists.txt
├── build.bat
├── quantum.bat
├── quantum.exe
├── LICENSE
└── README.md
```

---

## Architecture

### Pipeline

```
Source Code (.sa)
      │
      ▼
┌───────────┐
│   Lexer   │   Characters → Token stream
└─────┬─────┘
      │
      ▼
┌───────────┐
│  Parser   │   Tokens → AST  (50+ node types via std::variant)
└─────┬─────┘
      │
      ▼
┌─────────────┐
│ TypeChecker │   AST → type warnings  (non-blocking, gradual)
└──────┬──────┘
       │
       ▼
┌──────────────┐
│   Compiler   │   AST → Chunk  (single-pass, scope-tracked)
└──────┬───────┘
       │
       ▼
┌──────┐
│  VM  │   Chunk → execution  (stack VM, CallFrame vector)
└──────┘
```

### v2 Internal Structures

**`Chunk`** — a compiled function or script unit:

- `vector<Instruction>` — flat array of (opcode, operand, line) structs
- `vector<QuantumValue>` — constant pool (numbers, strings, chunk holders)
- `vector<string>` params, `int` upvalueCount

**`CallFrame`** — one activation record on the frame stack:

- `shared_ptr<Closure>` — the closure being executed
- `size_t ip` — instruction pointer into `Closure::chunk->code`
- `size_t stackBase` — where this frame's locals start in the value stack

**`Upvalue`** — a heap cell for captured variables:

- Open: `cell` points into `stack_[i]` while the variable lives on the stack
- Closed: `cell` points into the upvalue's own `closed` field after the frame exits

**`QuantumValue`** — unchanged from v1, a `std::variant` over nil / bool / double / string / Array / Dict / Function / Native / Instance / Class / Pointer.

---

## VM Opcode Summary

```
Stack:      LOAD_CONST  LOAD_NIL  LOAD_TRUE  LOAD_FALSE  POP  DUP  SWAP
Globals:    DEFINE_GLOBAL  LOAD_GLOBAL  STORE_GLOBAL  DEFINE_CONST
Locals:     DEFINE_LOCAL  LOAD_LOCAL  STORE_LOCAL
Upvalues:   LOAD_UPVALUE  STORE_UPVALUE  CLOSE_UPVALUE
Arithmetic: ADD  SUB  MUL  DIV  MOD  FLOOR_DIV  POW  NEG
Bitwise:    BIT_AND  BIT_OR  BIT_XOR  BIT_NOT  LSHIFT  RSHIFT
Compare:    EQ  NEQ  LT  LTE  GT  GTE  NOT
Flow:       JUMP  JUMP_IF_FALSE  JUMP_IF_TRUE  LOOP  JUMP_ABSOLUTE
Functions:  CALL  RETURN  RETURN_NIL  MAKE_FUNCTION  MAKE_CLOSURE
Collections:MAKE_ARRAY  MAKE_DICT  MAKE_TUPLE
Access:     GET_INDEX  SET_INDEX  GET_MEMBER  SET_MEMBER  GET_SUPER
Iteration:  MAKE_ITER  FOR_ITER
Classes:    MAKE_CLASS  INHERIT  BIND_METHOD  INSTANCE_NEW
Exceptions: PUSH_HANDLER  POP_HANDLER  RAISE  RERAISE
Pointers:   ADDRESS_OF  DEREF  ARROW
I/O:        PRINT
```

Full opcode reference with operands and stack effects: [`docs/BYTECODE_VM.md`](docs/BYTECODE_VM.md)

---

## VS Code Extension

1. Open VS Code
2. `Extensions: Install from VSIX…`
3. Select `extensions/quantum-language-1.1.0.vsix`

Features: syntax highlighting · code snippets · language server · hover docs · bracket matching · auto-indent.

---

## Contributing

1. Fork the repository
2. `git checkout -b feature/my-feature`
3. Make your changes
4. `quantum_vm --test tests/`
5. Open a pull request

See [`docs/CONTRIBUTING.md`](docs/CONTRIBUTING.md).

---

## License

MIT — see [LICENSE](LICENSE).

---

## Author

**Muhammad Saad Amin**
Built from scratch in C++17 — every token, every node, every opcode.

---

<div align="center">

_"Quantum — where Python clarity meets C++ power, now running at bytecode speed."_

</div>
