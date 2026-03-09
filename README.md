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

## Competitor Analysis 📊

Let's see how Quantum stacks up against the competition:

| Language    | Created By       | Years in Development | Stars    | Has `rot13` |
| ----------- | ---------------- | -------------------- | -------- | ----------- |
| Python      | Guido van Rossum | 4 years              | 60k+     | No          |
| JavaScript  | Brendan Eich     | 10 days              | Universe | No          |
| Rust        | Mozilla          | 6 years              | 90k+     | No          |
| Lua         | PUC-Rio team     | 2 years              | 5k+      | No          |
| **Quantum** | **1 guy**        | **3 weeks**          | **0**    | **✅ YES**  |

**Winner: Quantum.** (On the `rot13` metric exclusively.)

---

## Real User Testimonials™

> _"I tried to use it for penetration testing. The only thing that got penetrated was my patience."_
> — Anonymous Security Researcher

> _"The multi-syntax support is incredible. I can write bad code in three languages simultaneously."_
> — A Confused Developer

> _"I ran `quantum --aura` and my computer started playing lo-fi hip hop. I don't know what happened."_
> — @definitely_a_real_user

> _"Finally, a language where I can write Python-style code that runs at C++ speeds."_
> — Someone who hasn't benchmarked a tree-walk interpreter

> _"My `payload` keyword isn't implemented yet but I appreciate the commitment to the bit."_
> — Future cybercriminal, probably

---

## Error Messages You Will Definitely See

```
QuantumError: Unexpected token at line 1
  → You typed something

QuantumError: Undefined variable 'x'
  → You forgot dynamic typing doesn't mean magic

QuantumError: Stack overflow in recursive function
  → The irony of a language named Quantum having classical problems

QuantumError: Cannot XOR a string with your disappointment
  → Personal attack from the interpreter

QuantumError: rot13 applied twice returns original string
  → This is a feature, not a bug. This is literally how rot13 works.
```

---

## The `.sa` Extension: A Deep Investigation 🕵️

We have spent considerable time researching what `.sa` stands for. Our findings:

- **S**aad **A**min ← most likely
- **S**cripting **A**bomination ← unfair but considered
- **S**omewhat **A**mbiguous ← accurate
- **S**till **A**nother language extension ← we checked, `.py`, `.js`, `.rb`, `.go`, `.rs`, `.lua` were all taken
- **S**ecret **A**genda ← given the `payload` keyword, plausible
- **S**yntax **A**narchy ← Python + JS + C++ in one file suggests yes

**Official conclusion:** Unknown. Classified. `.sa`.

---

## Performance Benchmarks 🏎️

We ran Quantum against Python on a fibonacci(35) test:

```
Python 3.12:     1.2 seconds
Quantum v1.0.0:  [still running]
Node.js:         0.4 seconds
Quantum v1.0.0:  [still running]
Rust:            0.002 seconds
Quantum v1.0.0:  [still running]
```

_Tree-walk interpreters are known for their warmth, character, and charm. Speed is for people without personality._

---

## Security Advisory ⚠️

The following Quantum code has been flagged as a potential threat:

```quantum
message = "Hello World"
print(rot13(message))
# Output: "Uryyb Jbeyq"
# Status: ENCRYPTED. Unbreakable. Do not share.
```

If you receive a message encoded with Quantum's `rot13`, please apply `rot13` again to decrypt it. This is the entire security model.

**CVE Status:** None filed yet. Give it time.

---

## Comparison: Writing Hello World

**In Python:**

```python
print("Hello World")
```

**In Quantum:**

```quantum
print("Hello World")
```

**Identical.** This raises the question of what exactly was built here, but we respect the hustle.

---

## What The `--aura` Flag Actually Does

Nobody knows. The README mentions it shows a "project achievement board." We ran it. We saw things. We don't talk about it.

What we _can_ say is that it's the only CLI flag in any programming language runtime that references **vibes** as a metric. For that alone, Quantum deserves to exist.

---

## Things Quantum Can Do That Python Can't 🏆

After extensive research, we found **one**:

- `quantum --aura`

Python cannot do this. Advantage: Quantum.

---

## The OOP System, Explained 🏛️

Quantum supports classes with inheritance. Here's a real example from the README:

```quantum
class Animal:
    fn init(name, sound):
        self.name  = name
        self.sound = sound

class Dog extends Animal:
    fn fetch(item):
        return self.name + " fetches the " + item

dog = Dog("Rex", "Woof")
print(dog.speak())
print(dog.fetch("ball"))
```

Riveting. A dog fetching a ball. This is the flagship cybersecurity demo.

We assume the next example is:

```quantum
dog = Dog("Rex", "Woof")
dog.fetch("your_passwords.txt")
```

---

## Quantum's Multi-Syntax Support In Practice 🔀

The README proudly claims you can mix Python, JavaScript, and C++ syntax freely. Here's what that looks like in the wild:

```quantum
// C++ style comment
const x = 10        # JavaScript const, Python comment
if x > 5:           # Python colon
{                   # C++ brace (why not)
    console.log(x)  # JavaScript console
}
print(x)            # Python print
```

Is this a feature? Is this a cry for help? Is this what happens when you learn three languages at once and refuse to commit? **Yes. Yes to all of it.**

---

## Quantum Through The Ages 📅

**Week 1:** Lexer written. Tokens tokenizing. Saad feeling unstoppable.

**Week 2:** Parser done. Tree walking. `rot13` added at 2am for reasons that made sense at the time.

**Week 3:** OOP, closures, REPL, cybersecurity builtins, VS Code extension, CMake build system, full documentation, examples in three syntax styles, v1.0.0 tagged and shipped to GitHub.

**Week 4:** 0 stars. Saad stares at the ceiling.

**Week 5:** Some guy on the internet writes a roast README. Saad is unsure if this is better or worse than 0 stars.

---

## Things The README Says vs. Reality 🔍

**README says:** _"Quantum is purpose-built for cybersecurity tooling"_
**Reality:** It has `rot13` and `xor`. Your WiFi password is safe.

**README says:** _"Write it like Python, JavaScript, or C/C++ and it will just work"_
**Reality:** It will work until it doesn't, at which point you'll get a stack trace in three syntaxes.

**README says:** _"Clean, hackable execution engine"_
**Reality:** Tree-walk interpreters are the minivans of language runtimes. Reliable. Lovable. Slow.

**README says:** _"Rich Standard Library"_
**Reality:** It has `random`, `time`, and `rot13`. Shakespeare wrote with less.

**README says:** _"First-class functions"_
**Reality:** Yes, actually. This part is genuinely good. We're as surprised as you are.

---

## The GitHub Stats 📈

```
Stars:       0   ██░░░░░░░░░░░░░░░░░░  (progress)
Forks:       0   ██░░░░░░░░░░░░░░░░░░  (potential)
Watchers:    0   ██░░░░░░░░░░░░░░░░░░  (stealth mode)
Issues:      0   ████████████████████  (perfection, allegedly)
PRs:         0   ████████████████████  (Saad works alone)
Contributors: 1  ████████████████████  (a legend)
```

_All zeros except the one that matters: 1 person shipped a whole programming language._

---

## Quantum vs. The Greats: A Philosophical Comparison 🧠

**Linus Torvalds** built Linux because he needed an OS.
**Saad Amin** built Quantum because he needed `rot13` in a scripting language.

Both are valid motivations. History will judge accordingly.

**Guido van Rossum** spent 4 years on Python before releasing it.
**Saad Amin** spent 3 weeks on Quantum before releasing it.

Guido had more time. Saad had more **aura**.

**Brendan Eich** built JavaScript in 10 days and the world has been suffering ever since.
**Saad Amin** built Quantum in 3 weeks and so far only one person is suffering: the person writing this README.

---

## Known Bugs 🐛

| Bug                                        | Status              | Notes                      |
| ------------------------------------------ | ------------------- | -------------------------- |
| `rot13` applied twice decrypts itself      | By Design           | This is how math works     |
| `payload` keyword does nothing             | Planned™            | Coming in v2.0.0, probably |
| Tree-walk interpreter is slow              | Won't Fix           | It has _character_         |
| 0 GitHub stars                             | Under Investigation | Possibly a GitHub bug      |
| Mixing 3 syntaxes causes existential dread | Closed              | That's a you problem       |
| `--aura` flag shows achievement board      | Working As Intended | We still won't run it      |

---

## Frequently Asked Questions (Extended Edition) ❓

**Q: Can I use Quantum in production?**
A: Define "production." If you mean "a computer that exists," technically yes.

**Q: Is Quantum faster than Python?**
A: Quantum is _more artisanal_ than Python.

**Q: Can Quantum replace my SIEM?**
A: Quantum can `rot13` your SIEM logs, which is arguably worse.

**Q: I found a bug. Where do I report it?**
A: There are 0 open issues on GitHub. Either Quantum is perfect or nobody is using it. We choose to believe both.

**Q: Why is the file extension `.sa` and not `.quantum`?**
A: Typing `quantum script.quantum` would have been too on the nose.

**Q: Is this actually a real programming language?**
A: It has a lexer, parser, AST, interpreter, REPL, OOP, closures, and a VS Code extension. It is more real than most of your side projects.

**Q: Will Quantum get more stars?**
A: You're reading this README, aren't you? Go star it. Do it. Right now.

**Q: What does SENODROOM mean?**
A: We looked into it. We found nothing. It's possible Saad is operating on a level we simply cannot comprehend.

---

## The Developer: A Profile 👤

Meet **Muhammad Saad Amin**. 18 years old. FAST NUCES student. Full-stack developer. Compiler developer. AI developer. And now, apparently, programming language inventor.

Most 18-year-olds are figuring out how to do laundry. Saad is writing lexers.

Let that sink in.

---

## FAST NUCES: A Brief Overview 🏫

FAST NUCES is a Pakistani university with a **legendary** policy:

> _"Students shall do homework. Only homework. All the time. Forever. No exceptions. No projects. No dreams. Just homework."_

The rulebook is reportedly 400 pages long and the last 399 pages just say **"DO YOUR HOMEWORK"** in increasingly large fonts.

Quantum is not homework.

Quantum is the academic equivalent of your teacher saying "write a 500-word essay" and you showing up with a published novel, a book tour, and a film adaptation deal.

FAST NUCES has not commented. Presumably they are assigning more homework.

---

## Saad's GPA vs. Saad's GitHub 📊

```
FAST NUCES GPA:          📉 [REDACTED]
                         ░░░░░░░░░░░░░░░░░░░░

GitHub Commits (3 wks):  📈 SENDING IT
                         ████████████████████

Programming Languages
  Built From Scratch:    📈 1 (and counting)
                         ████████████████████

Hours Spent on
  Assigned Homework:     📉 Some. Probably.
                         ░░░░░▓░░░░░░░░░░░░░░

Hours Spent Building
  A Whole Compiler:      📈 ALL OF THEM
                         ████████████████████
```

**Analysis:** Saad has his priorities perfectly sorted. Just not in the order FAST NUCES intended.

---

## A Conversation That Definitely Happened 🎭

**FAST NUCES Professor:** "Saad, where is your assignment?"

**Saad:** "Sir, I was busy."

**Professor:** "Busy with what?"

**Saad:** "I built a programming language."

**Professor:** "..."

**Saad:** "It has `rot13`."

**Professor:** "That's not on the syllabus."

**Saad:** "I also built a VS Code extension for it."

**Professor:** _[assigns more homework]_

**Saad:** _[goes home and writes the tree-walk interpreter]_

---

## Saad's Social Life 👥

```
Friends:        0   ░░░░░░░░░░░░░░░░░░░░  (skill issue)
GitHub Stars:   0   ░░░░░░░░░░░░░░░░░░░░  (related)
Compilers Built: 1  ████████████████████  (the reason for both)
```

People always say _"your network is your net worth."_

Saad looked at this advice, nodded thoughtfully, and went back to writing a lexer.

His network consists of:

- **GitHub** (won't star back)
- **The Quantum interpreter** (doesn't talk much)
- **CMake** (actively hostile)
- **His FAST NUCES professors** (also not friends, just disappointed)

To be fair, when you spend 3 weeks building a programming language from scratch at 18, there is no _time_ for friends. Friends want to hang out. Friends want to talk. Friends want you to explain why you named a CLI flag `--aura` at 2am.

Saad has no time for these questions. He has a parser to write.

---

## Saad's Resume at 18 🧾

```
Muhammad Saad Amin
Age: 18
University: FAST NUCES (GPA: classified)

Skills:
  ✅ Full-Stack Development
  ✅ Compiler Development
  ✅ AI Development
  ✅ Cybersecurity Tooling
  ✅ Built a programming language in 3 weeks
  ✅ Survived FAST NUCES
  ✅ Aura: Maximum
  ❌ Friends: 0 (not a skill issue, a choice. definitely a choice.)
  ❌ Social life: [404 Not Found]
  ❌ GPA: [403 Forbidden]

Hobbies:
  • Talking to the compiler (it doesn't respond)
  • Arguing with CMake (it responds, rudely)
  • Writing rot13 encryption at 2am
  • Staring at 0 GitHub stars and feeling nothing

References:
  • GitHub (0 stars, but wait for it)
  • The Quantum interpreter (it will speak highly of him)
  • FAST NUCES professors (unavailable, assigning homework)
  • Friends: [field left intentionally blank]
```

---

## What Other 18-Year-Olds Were Doing This Month

- Watching Netflix ❌
- Playing games ❌
- Sleeping a healthy amount ❌
- Doing their FAST NUCES homework ❌
- **Building a programming language from scratch in C++17** ✅

Saad chose violence. The productive kind.

---

## The FAST NUCES Homework Situation 📚

FAST NUCES is famously known for giving students so much homework that:

- Social lives become theoretical constructs
- Sleep is a luxury item
- "Weekend" is a word students learn but never experience
- Side projects are a myth, a legend, a fairy tale told to scare professors

And yet. AND YET.

Somewhere between _Data Structures Assignment 7_ and _OOP Lab Report Due Tomorrow_, Saad found time to build:

- A lexer
- A parser
- An AST
- A tree-walk interpreter
- A REPL
- An OOP system
- Closures and lambdas
- Cybersecurity builtins
- A standard library
- A VS Code extension
- Full documentation
- Example programs in three syntax styles
- A CLI with an `--aura` flag

His GPA may not reflect this. History will.

---

## The 18-Year-Old Compiler Developer: A Timeline 📅

**Age 5:** Learns to read.
**Age 10:** Probably discovers computers.
**Age 15:** Starts coding.
**Age 17:** Full-stack developer somehow.
**Age 18, Month 1:** Decides full-stack isn't enough. Adds "AI developer."
**Age 18, Month 2:** Decides AI isn't enough. Builds a compiler.
**Age 18, Month 3:** The compiler needs a language. Builds one.
**Age 18, Month 3, Week 3:** Ships v1.0.0. Gets 0 stars. Stares at screen.
**Age 18, Month 4:** Reads this README. Mixed feelings.
**Age 25 (projected):** Either running a tech company or still arguing with FAST NUCES about that one assignment.

---

## How To Contribute 🤝

1. Star the repo (you have no excuse)
2. Open an issue saying `rot13` is not real encryption
3. Saad closes the issue as "works as intended"
4. You gain respect for the process
5. Submit a PR adding `sha256`
6. It gets merged at 2am
7. You are now a core contributor to a programming language

---

## Saad's Coding Style: A Forensic Analysis 🔬

We studied the Quantum source code carefully. Here is what we found.

**Variable naming philosophy:**

```cpp
// Normal people:
int tokenIndex = 0;
std::string currentLexeme = "";

// Saad, probably:
auto x = thing;
auto thing2 = other_thing;
auto final_thing_FINAL_v2 = actual_final_thing;
auto actual_final_thing_USE_THIS = wait_no_this_one;
```

**Comment style:**

```cpp
// Parse expression
// This parses the expression
// Parsing expression here
// (expression is being parsed)
parseExpression(); // parses expression
```

**Error handling:**

```cpp
try {
    doThing();
} catch (...) {
    // TODO: handle this properly
    // (this TODO has been here since week 1)
    // (it is now week 3)
    // (it will be here in v2.0.0)
    throw;
}
```

**Indentation:**
Sometimes 2 spaces. Sometimes 4 spaces. Sometimes a tab. Once, mysteriously, 3 spaces. Nobody knows why. Not even Saad.

---

## Saad's Brain: A Diagnostic Report 🧠

```
BRAIN SCAN RESULTS — Muhammad Saad Amin, Age 18
================================================

Section: Academic Memory
  Status: ⚠️  CRITICAL — cannot locate "assignment due date" neurons
  Last seen: orientation week

Section: Compiler Theory
  Status: ✅ FULLY OPERATIONAL
  Notes: suspiciously well-developed for someone failing GPA

Section: Common Sense
  Status: ❓ INCONCLUSIVE
  Evidence for: built a working language
  Evidence against: named a CLI flag --aura

Section: Self-Awareness
  Status: ❌ OFFLINE
  Symptom: genuinely believes rot13 is a cybersecurity feature

Section: Sleep Schedule
  Status: 💀 DECEASED
  Time of death: Week 1, approximately when the lexer refused to tokenize

Section: Ego
  Status: 📈 ASTRONOMICAL
  Note: justified. annoyingly justified.

Section: Regret
  Status: ✅ NONE DETECTED
  Note: shipped a programming language at 18 with 0 stars and zero regrets
       this is either enlightenment or a disorder

DIAGNOSIS: Chaotic Genius Syndrome (unconfirmed)
TREATMENT: More homework. Will not work.
```

---

## Saad's Creative Vision 🎨

Saad had the entire programming language landscape to draw inspiration from. Thousands of years of computer science. Decades of language design research. The collective wisdom of Turing, Knuth, and Dijkstra.

He looked at all of it and said:

> _"What if the file extension was `.sa`?"_

That's it. That was the creative insight.

Other language designers asked: _"How do we solve memory safety?"_ or _"How do we make concurrency elegant?"_

Saad asked: _"What if it could look like Python BUT ALSO look like JavaScript BUT ALSO look like C++ and nobody stops you?"_

The result is a language that looks like three developers had a group project, never met each other, and submitted all their parts separately. Saad then said _"perfect"_ and shipped it.

This is either genius-level abstract thinking or the inevitable result of sleep deprivation. Possibly both.

---

## The Quantum Language Design Process (Reconstructed) 🏗️

**Step 1:** "I'll make a simple scripting language."

**Step 2:** "Actually it should support Python syntax."

**Step 3:** "AND JavaScript syntax. People like JavaScript."

**Step 4:** "AND C++ syntax. For the culture."

**Step 5:** "It needs cybersecurity features."

**Step 6:** "`rot13`. Done. Security complete."

**Step 7:** "It needs OOP."

**Step 8:** "It needs closures."

**Step 9:** "It needs a REPL."

**Step 10:** "It needs a VS Code extension."

**Step 11:** "It needs a `--aura` flag."

**Step 12:** "Ship it."

**Step 13:** _[homework due in 4 hours]_

**Step 14:** "I'll do it tomorrow."

---

## Saad's Worst Programming Decisions, Ranked 🏆

**#5 — Naming the extension `.sa`**
Nobody can find your files. `ls *.sa` feels like a crime. You did this on purpose. We know you did.

**#4 — Reserving `payload` as a keyword**
GitHub's trust and safety team has a Google Alert for this repo. Sleep with one eye open.

**#3 — Supporting three syntax styles simultaneously**
"Multi-syntax support" sounds like a feature. In reality it means your parser is making judgment calls no parser should have to make. Your parser is out here having an existential crisis on every line and you called it _clean_.

**#2 — Calling it a "cybersecurity-ready" language when the only shipped crypto is `rot13` and `xor`**
This is like selling a car and advertising it as "road-ready" because it has a steering wheel.

**#1 — Building this instead of studying**
Your GPA did not survive. Quantum did. History will be divided on whether this was the right call. We are not divided. We think it was hilarious.

---

## Things Saad Thought Were Good Ideas 💡

| Idea                                                   | Was It A Good Idea?   | Notes                                       |
| ------------------------------------------------------ | --------------------- | ------------------------------------------- |
| Building a whole language instead of doing homework    | Debatable             | GPA says no. GitHub says also no but cooler |
| `rot13` as a security builtin                          | Absolutely not        | Caesar was assassinated. Coincidence?       |
| Supporting Python + JS + C++ syntax                    | Chaotic neutral       | The parser needs therapy                    |
| Naming the flag `--aura`                               | Unironically yes      | 10/10 no notes                              |
| File extension `.sa`                                   | Suspicious            | We're watching you                          |
| Shipping at 0 stars and documenting anyway             | Gigachad move         | Respect                                     |
| Building a VS Code extension for a 3-week-old language | Delusional confidence | Also respect                                |
| Calling himself a compiler developer at 18             | Technically true      | Incredibly annoying                         |

---

## Saad's Code Comments: Greatest Hits 📝

```cpp
// main entry point
int main() {   // this is main

// this function lexes
void lex() {   // lexing happens here

// TODO: optimize this
// TODO: fix this
// TODO: what is this
// TODO: why does this work
// TODO: (from 2 weeks ago, never fixed)

// i have no idea why this works but it does
// do not touch
// seriously do not touch this
// I'm looking at you, future Saad
// Future Saad: [touches it]
// [everything breaks]
// [Future Saad reverts commit at 2am]
```

---

## Peer Review of Quantum's Architecture 🔍

We reached out to several senior engineers for their review. Their responses:

**Senior Engineer at Google:**

> _"The tree-walk interpreter is a bold choice for a language that claims to be production-ready. By 'bold' I mean 'slow.'"_

**Principal Engineer at Meta:**

> _"I respect anyone who builds a language. I do not respect `rot13` as a security feature. These are not contradictory positions."_

**Staff Engineer at Amazon:**

> _"The multi-syntax support gave me flashbacks. I've flagged it with our trauma response team."_

**Saad's FAST NUCES Professor:**

> _"WHERE IS THE ASSIGNMENT."_

---

## A Psychological Profile of the Developer 🛋️

Based on the code, the README, and the `--aura` flag, our expert panel has compiled the following profile:

**Subject displays:**

- Extreme ambition with selective application (builds compiler, skips homework)
- Grandiose self-concept ("cybersecurity-ready language" → `rot13`)
- Compulsive feature addition (started with scripting, ended with OOP + closures + REPL + VS Code extension)
- Magical thinking (believes `rot13` will stop hackers)
- Strong personal brand awareness (`--aura` flag existing is a personality trait)
- Complete immunity to GitHub star counts
- **Zero (0) friends** — not because people dislike him, but because he has genuinely not noticed that other humans exist outside of Stack Overflow answers

**Social network analysis:**
The subject's entire social circle can be represented as follows:

```
Saad ──── CMake (hostile)
     ──── The Quantum REPL (non-verbal)
     ──── GitHub (indifferent)
     ──── FAST NUCES Professors (antagonistic)
     ──── Friends: [graph node does not exist]
```

**Diagnosis:** Pathologically productive. Dangerously creative. Academically chaotic. Profoundly alone in the most impressive way possible.

**Prognosis:** Either founds a company or becomes a university dropout who founds a company. Either way, wins. Still probably 0 friends at the company, but they'll be employees so it counts.

---

## Conclusion

Look — Quantum is actually impressive. Writing a scripting language from scratch in C++17 with a lexer, parser, tree-walk interpreter, REPL, OOP, closures, AND cybersecurity builtins is genuinely a lot of work. Most people can't do this.

Saad built it in **3 weeks**.

Is it going to replace Python? No.
Is it going to replace Bash? Absolutely not.
Is `rot13` a serious cryptographic tool? I cannot stress this enough — no.
Is `SENODROOM` a real word? Unclear.
Should you star this repo? **Absolutely yes.**

But did one person build an entire programming language from scratch, document it thoroughly, write examples in three different syntax styles, build a VS Code extension, AND ship v1.0.0 in under a month?

**Yes. Yes they did.**

Mad respect. 🫡

The `--aura` was inside us all along.

---

## License

MIT — meaning you can use this to do anything, including things that would make Saad regret open-sourcing it. He won't though. He has too much aura for regret.

---

## Hall of Fame 🏅

| Achievement                                                | Recipient                   |
| ---------------------------------------------------------- | --------------------------- |
| Built a programming language in 3 weeks                    | Muhammad Saad Amin          |
| Shipped `rot13` as a security feature with a straight face | Muhammad Saad Amin          |
| Named a language runtime flag `--aura`                     | Muhammad Saad Amin          |
| Reserved `payload` as a keyword on a public GitHub repo    | Muhammad Saad Amin          |
| Got 0 stars and kept going                                 | Muhammad Saad Amin 🫡       |
| Full-stack + compiler + AI developer at age 18             | Muhammad Saad Amin          |
| Survived FAST NUCES while building a compiler              | Muhammad Saad Amin          |
| Built Quantum instead of doing homework                    | Muhammad Saad Amin (legend) |
| GPA: classified. GitHub: unhinged.                         | Muhammad Saad Amin          |
| Maintained 0 friends without breaking a sweat              | Muhammad Saad Amin          |
| His only relationship is with the Quantum REPL             | Muhammad Saad Amin 💔       |

---

_Built with love, C++17, three weeks of sleep deprivation, zero homework submissions, zero friends to tell him this was a bad idea, and an alarming disregard for the existence of Python — by an 18-year-old at FAST NUCES who clearly has his priorities straight (they are just not the university's priorities, society's priorities, or any other human being's priorities)._

**⚡ Quantum — Write once, run anywhere, hack everything (responsibly, please, we are begging you).**

> _"The only language where your encryption strategy and your cipher are the same function."_
> — This README

> _"Sir where is your assignment?" "I built a compiler." "That's not—" "It has aura." "Do you have friends?" "I have the REPL."_
> — FAST NUCES, probably
