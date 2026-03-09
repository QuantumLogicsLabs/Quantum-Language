# QUANTUM LANGUAGE 🔐⚡

### _Because Python was too easy and C++ wasn't painful enough_

---

> **"Write once, run anywhere, hack everything."**
> — Muhammad Saad Amin, definitely while wearing a hoodie in a dark room

---

## What Is This?

Quantum is a **dynamically-typed scripting language** built from scratch in C++17 by someone who clearly had too much free time and not enough therapy.

It uses the `.sa` file extension. We don't know what `.sa` stands for. **Saad Amin**? **Slightly Ambitious**? **Suspiciously Ambitious**? The world may never know.

You run it like this:

```
quantum script.sa
```

One command. Very cool. Very hacker. Much wow.

---

## Why Does This Exist?

Excellent question. Python exists. JavaScript exists. Bash exists. Rust exists. But apparently none of them were _cybersecurity-ready_ enough, so Saad said:

> _"You know what the world needs? Another scripting language. But this one has `rot13`."_

And thus, Quantum was born.

---

## ✨ "Features"

| Feature                   | What They Said                                 | What It Means                                                    |
| ------------------------- | ---------------------------------------------- | ---------------------------------------------------------------- |
| 🔀 Multi-syntax           | "Python, JS, and C++ syntax work side by side" | The parser is just vibing                                        |
| 🧠 Dynamic Typing         | "No type declarations needed"                  | Types are just suggestions anyway                                |
| 🌳 Tree-walk Interpreter  | "Clean, hackable engine"                       | It's slow but it's _artisanal_                                   |
| 💻 REPL Mode              | "Interactive shell"                            | For when you want to feel like a hacker but only have 10 minutes |
| 🔐 Cybersecurity Builtins | "`xor_bytes`, `rot13`"                         | ROT13. In 2026. Peak security.                                   |
| 🎯 First-class Functions  | "Closures, lambdas, arrow functions"           | Basically just stole JS's homework                               |
| 🏛️ OOP                    | "Classes with inheritance"                     | Because of course it has classes                                 |

---

## The Cybersecurity "Roadmap" 🗺️

The README lists these as **reserved keywords for upcoming features**:

- `scan` — _Network scanning_ (totally legal, we promise)
- `payload` — _"Exploit payload construction"_ (sir this is a GitHub repo)
- `encrypt` / `decrypt` — AES, RSA (coming soon™)
- `hash` — MD5, SHA-256 (MD5 in 2026 👀)

To be fair, `rot13` is already implemented, so the NSA is shaking.

---

## Quick Start

```bash
# Step 1: Question your life choices
git clone https://github.com/SENODROOM/Quantum-Language.git

# Step 2: Build it (pray to CMake gods)
mkdir build && cd build
cmake ..
cmake --build . --config Release

# Step 3: Write your first script
echo 'print("Hello from Quantum!")' > hello.sa

# Step 4: Run it and feel like a hacker
quantum hello.sa
# → Hello from Quantum!

# Step 5: Realize Python would've taken 3 seconds
```

---

## Project Structure

```
Quantum-Language/
├── src/
│   ├── main.cpp          # Where dreams begin
│   ├── Lexer.cpp         # Where strings go to die
│   ├── Parser.cpp        # Where grammar goes to be questioned
│   ├── Interpreter.cpp   # Where bugs are born
│   └── Value.cpp         # Where types go to not exist
├── examples/
│   ├── Python/           # "See, it's just like Python!"
│   ├── C/                # "See, it's just like C!"
│   └── C++/              # "See, it's just like C++!" (It is not)
└── README.md             # This document, but less funny
```

---

## Sample Code: XOR Encryption

```quantum
fn xor_encrypt(text, key):
    return xor_bytes(text, key)

message   = "Hello, Quantum!"
key       = "secret"
encrypted = xor_encrypt(message, key)
decrypted = xor_encrypt(encrypted, key)

print("Encrypted:", to_hex(encrypted))
print("Decrypted:", decrypted)
```

> XOR encryption. The cryptographic equivalent of a bike lock. But hey, it's _built-in_, so that's something.

---

## Frequently Asked Questions

**Q: Is this production-ready?**
A: It has 0 stars on GitHub. You decide.

**Q: Should I use this for real cybersecurity work?**
A: Please don't.

**Q: Why `.sa` extension?**
A: We asked. Saad didn't respond. We assume it's classified.

**Q: Is `rot13` really a cybersecurity feature?**
A: Julius Caesar used it. He was eventually stabbed by his friends, but that's unrelated.

**Q: Will `payload` keyword actually help me build exploits?**
A: It will help GitHub flag your repo.

---

## CLI Reference

```
quantum <file.sa>          Run a Quantum script (and feel powerful)
quantum                    Start REPL (pretend you're in Mr. Robot)
quantum --check <file.sa>  Check syntax (it won't pass, but nice try)
quantum --version          Confirm you installed this
quantum --help             Admit you're lost
quantum --aura             ??? (We're scared to run this)
```

---

## Conclusion

Look — Quantum is actually impressive. Writing a scripting language from scratch in C++17 with a lexer, parser, tree-walk interpreter, REPL, OOP, closures, AND cybersecurity builtins is genuinely a lot of work. Most people can't do this.

Saad built it anyway.

Is it going to replace Python? No.
Is it going to replace Bash? Absolutely not.
Is `rot13` a serious cryptographic tool? I cannot stress this enough — no.

But did one person build an entire programming language from scratch and ship v1.0.0?

**Yes. Yes they did.**

Mad respect. 🫡

---

## License

MIT — meaning you can use this to do anything, including things that would make Saad regret open-sourcing it.

---

_Built with love, C++17, and an alarming disregard for the existence of Python._

**⚡ Quantum — Write once, run anywhere, hack everything (responsibly, please).**
