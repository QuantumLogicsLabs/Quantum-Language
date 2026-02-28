<div align="center">
<code>
```
  ██████╗ ██╗   ██╗ █████╗ ███╗   ██╗████████╗██╗   ██╗███╗   ███╗
 ██╔═══██╗██║   ██║██╔══██╗████╗  ██║╚══██╔══╝██║   ██║████╗ ████║
 ██║   ██║██║   ██║███████║██╔██╗ ██║   ██║   ██║   ██║██╔████╔██║
 ██║▄▄ ██║██║   ██║██╔══██║██║╚██╗██║   ██║   ██║   ██║██║╚██╔╝██║
 ╚██████╔╝╚██████╔╝██║  ██║██║ ╚████║   ██║   ╚██████╔╝██║ ╚═╝ ██║
  ╚══▀▀═╝  ╚═════╝ ╚═╝  ╚═╝╚═╝  ╚═══╝   ╚═╝    ╚═════╝ ╚═╝     ╚═╝
```
</code>

**The Multi-Paradigm Scripting Language Built for Cybersecurity**

[![Version](https://img.shields.io/badge/version-1.0.0-cyan)](https://github.com/SENODROOM/Quantum-Language/releases/tag/v1.0.0)
[![Language](https://img.shields.io/badge/built%20with-C%2B%2B17-blue)](https://en.cppreference.com/w/cpp/17)
[![Platform](https://img.shields.io/badge/platform-Windows%20%7C%20Linux%20%7C%20macOS-lightgrey)](https://github.com/SENODROOM/Quantum-Language)
[![License](https://img.shields.io/badge/license-MIT-green)](LICENSE)
[![Release Date](https://img.shields.io/badge/released-Feb%202026-orange)](https://github.com/SENODROOM/Quantum-Language/releases)

</div>

---

## What is Quantum?

**Quantum** is a dynamically-typed, tree-walk interpreted scripting language built from scratch in C++17. It is designed to feel familiar to developers from any background — you can write it like Python, JavaScript, or C/C++ and it will just work.

Quantum files use the **`.sa`** extension and run with a single command:

```bash
quantum script.sa
```

Quantum is purpose-built for **cybersecurity tooling**, featuring built-in functions for XOR operations, Base64 encoding, hex manipulation, and rot13 — with more cryptographic and network features planned.

---

## ✨ Key Features

| Feature | Description |
|---|---|
| 🔀 **Multi-syntax** | Python, JavaScript, and C/C++ syntax work side by side |
| 🧠 **Dynamic Typing** | No type declarations — variables infer types at runtime |
| 🌳 **Tree-walk Interpreter** | Clean, hackable execution engine in C++17 |
| 💻 **REPL Mode** | Interactive shell for quick experimentation |
| 🔐 **Cybersecurity Builtins** | `xor_bytes`, `base64_encode`, `to_hex`, `from_hex`, `rot13` |
| 📦 **Rich Standard Library** | Math, string, array, dict, I/O, time, random, encoding |
| 🎯 **First-class Functions** | Closures, lambdas, arrow functions, anonymous functions |
| 🏛️ **OOP** | Classes with inheritance and instance methods |
| ⚙️ **Bitwise Operations** | Full `&`, `\|`, `^`, `~`, `<<`, `>>` support |

---

## Quick Start

```bash
# Clone and build
git clone https://github.com/SENODROOM/Quantum-Language.git
cd Quantum-Language
mkdir build && cd build
cmake ..
cmake --build . --config Release
```

Then run your first script:

```python
# hello.sa
print("Hello from Quantum!")
```

```bash
quantum hello.sa
# → Hello from Quantum!
```

> See [SETUP.md](SETUP.md) for full installation instructions on all platforms.

---

## Code Samples

### XOR Encryption
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

### Higher-Order Functions (JavaScript style)
```javascript
const numbers = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10]

const evens   = numbers.filter(x => x % 2 == 0)
const squares = evens.map(x => x * x)
const total   = squares.reduce((acc, x) => acc + x, 0)

console.log("Sum of squares of evens:", total)
```

### Classes and Inheritance (Python style)
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

---

## Project Structure

```
Quantum-Language/
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
├── docs/                 # Documentation
├── design/               # Language design notes
├── extensions/           # Editor/IDE extensions
├── CMakeLists.txt
├── build.bat             # Windows build script
├── quantum.bat           # Windows runner script
├── SETUP.md
├── SYNTAX.md
└── README.md
```

---

## CLI Reference

```
quantum <file.sa>          Run a Quantum script
quantum                    Start interactive REPL
quantum --check <file.sa>  Check syntax without executing
quantum --version          Show version information
quantum --help             Show usage help
quantum --aura             Show project achievement board
```

---

## Cybersecurity Roadmap

The following keywords are **reserved** for upcoming features:

| Keyword | Planned Purpose |
|---|---|
| `scan` | Network port/host scanning |
| `payload` | Exploit payload construction |
| `encrypt` | Full cryptographic encryption (AES, RSA) |
| `decrypt` | Cryptographic decryption |
| `hash` | Hashing algorithms (MD5, SHA-256, SHA-512) |

---

## Documentation

- 📖 [SETUP.md](SETUP.md) — Installation guide for all platforms
- 📝 [SYNTAX.md](SYNTAX.md) — Complete language syntax reference
- 💡 [examples/](examples/) — Example programs in all syntax styles

---

## Contributing

Contributions are welcome! Please read [CONTRIBUTING.md](CONTRIBUTING.md) before submitting pull requests.

---

## License

This project is licensed under the **MIT License** — see [LICENSE](LICENSE) for details.

---

## Built By

**Muhammad Saad Amin** — Quantum Language v1.0.0  
A cybersecurity-ready scripting language built from scratch in C++17.

<div align="center">
<sub>⚡ Quantum — Write once, run anywhere, hack everything.</sub>
</div>