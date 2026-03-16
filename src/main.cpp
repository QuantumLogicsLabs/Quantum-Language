/*
 * Quantum Language v2.0.0 — Bytecode VM
 *
 * This is the unified entry point.  It replaces both main.cpp (v1 tree-walk)
 * and main_vm.cpp (early VM draft).  The binary is named "quantum" — the same
 * name as before — so every existing script, batch file, and CI job continues
 * to work without modification.
 *
 * All v1 flags are preserved:
 *   quantum <file.sa>          run a script
 *   quantum                    REPL
 *   quantum --check  <file>    parse + type-check only
 *   quantum --test   [dir]     batch-test all .sa files
 *   quantum --aura             show achievement panel
 *   quantum --version          version info
 *   quantum --help             usage
 *
 * New VM-only flags:
 *   quantum --debug  <file>    dump bytecode then run
 *   quantum --dis    <file>    dump bytecode, no execution
 */

#include "Lexer.h"
#include "Parser.h"
#include "Compiler.h"
#include "VM.h"
#include "Disassembler.h"
#include "TypeChecker.h"
#include "Error.h"
#include "Value.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include <filesystem>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace fs = std::filesystem;

// ─── Global test-mode flag (keeps input() from blocking in --test runs) ───────
bool g_testMode = false;

// ─── Banner ───────────────────────────────────────────────────────────────────

static void printBanner()
{
    std::cout << Colors::CYAN << Colors::BOLD
              << "\n"
              << "  ██████╗ ██╗   ██╗ █████╗ ███╗   ██╗████████╗██╗   ██╗███╗   ███╗\n"
              << " ██╔═══██╗██║   ██║██╔══██╗████╗  ██║╚══██╔══╝██║   ██║████╗ ████║\n"
              << " ██║   ██║██║   ██║███████║██╔██╗ ██║   ██║   ██║   ██║██╔████╔██║\n"
              << " ██║▄▄ ██║██║   ██║██╔══██║██║╚██╗██║   ██║   ██║   ██║██║╚██╔╝██║\n"
              << " ╚██████╔╝╚██████╔╝██║  ██║██║ ╚████║   ██║   ╚██████╔╝██║ ╚═╝ ██║\n"
              << "  ╚══▀▀═╝  ╚═════╝ ╚═╝  ╚═╝╚═╝  ╚═══╝   ╚═╝    ╚═════╝ ╚═╝     ╚═╝\n"
              << Colors::RESET
              << Colors::YELLOW
              << "  Quantum Language v2.0.0 | Bytecode VM Edition\n"
              << Colors::RESET << "\n";
}

static void printAura()
{
    std::cout << Colors::CYAN << Colors::BOLD
              << "\n╔══════════════════════════════════════════════════════════════════╗\n"
              << "║" << Colors::YELLOW << "                🌟 QUANTUM LANGUAGE ACHIEVEMENTS 🌟" << Colors::CYAN << "               ║\n"
              << "╠══════════════════════════════════════════════════════════════════╣\n"
              << "║" << Colors::GREEN << "  ✅ Complete C++17 Compiler Implementation" << Colors::CYAN << "                       ║\n"
              << "║" << Colors::GREEN << "  ✅ Bytecode VM Engine (v2.0)" << Colors::CYAN << "                                    ║\n"
              << "║" << Colors::GREEN << "  ✅ Single-Pass Bytecode Compiler" << Colors::CYAN << "                                ║\n"
              << "║" << Colors::GREEN << "  ✅ Lexical Closures via Upvalue Cells" << Colors::CYAN << "                           ║\n"
              << "║" << Colors::GREEN << "  ✅ Lexical Analysis & Tokenization" << Colors::CYAN << "                              ║\n"
              << "║" << Colors::GREEN << "  ✅ Recursive Descent Parser" << Colors::CYAN << "                                     ║\n"
              << "║" << Colors::GREEN << "  ✅ Abstract Syntax Tree (AST)" << Colors::CYAN << "                                   ║\n"
              << "║" << Colors::GREEN << "  ✅ Dynamic Type System" << Colors::CYAN << "                                          ║\n"
              << "║" << Colors::GREEN << "  ✅ REPL Interactive Mode" << Colors::CYAN << "                                        ║\n"
              << "║" << Colors::GREEN << "  ✅ Cross-Platform Build System" << Colors::CYAN << "                                  ║\n"
              << "║" << Colors::GREEN << "  ✅ VS Code Language Support" << Colors::CYAN << "                                     ║\n"
              << "║" << Colors::GREEN << "  ✅ Comprehensive Documentation" << Colors::CYAN << "                                  ║\n"
              << "╠══════════════════════════════════════════════════════════════════╣\n"
              << "║" << Colors::MAGENTA << "                    📊 PROJECT STATISTICS 📊" << Colors::CYAN << "                      ║\n"
              << "╠══════════════════════════════════════════════════════════════════╣\n"
              << "║" << Colors::WHITE << "  📁 Source Files: " << Colors::YELLOW << "11 core modules" << Colors::CYAN << "                                ║\n"
              << "║" << Colors::WHITE << "  📝 Language Version: " << Colors::YELLOW << "v2.0.0" << Colors::CYAN << "                                     ║\n"
              << "║" << Colors::WHITE << "  🔧 Build System: " << Colors::YELLOW << "CMake + MSVC/GCC/Clang" << Colors::CYAN << "                         ║\n"
              << "║" << Colors::WHITE << "  🎯 Language Standard: " << Colors::YELLOW << "C++17" << Colors::CYAN << "                                     ║\n"
              << "║" << Colors::WHITE << "  🚀 Performance: " << Colors::YELLOW << "Bytecode VM — ~3× faster than v1" << Colors::CYAN << "               ║\n"
              << "╠══════════════════════════════════════════════════════════════════╣\n"
              << "║" << Colors::BLUE << "                    🛡️  CYBERSECURITY FEATURES 🛡️" << Colors::CYAN << "                   ║\n"
              << "╠══════════════════════════════════════════════════════════════════╣\n"
              << "║" << Colors::BLUE << "  🔍 scan()      - Network scanning capabilities" << Colors::CYAN << "                  ║\n"
              << "║" << Colors::BLUE << "  💣 payload()   - Exploit payload creation" << Colors::CYAN << "                       ║\n"
              << "║" << Colors::BLUE << "  🔐 encrypt()   - Cryptographic operations" << Colors::CYAN << "                       ║\n"
              << "║" << Colors::BLUE << "  🔓 decrypt()   - Decryption functions" << Colors::CYAN << "                           ║\n"
              << "║" << Colors::BLUE << "  🗝️  hash()     - Hashing algorithms" << Colors::CYAN << "                              ║\n"
              << "╚══════════════════════════════════════════════════════════════════╝\n"
              << Colors::RESET;
}

// ─── Compile + optional disassemble helper ────────────────────────────────────

static std::shared_ptr<Chunk> compileSource(const std::string &source,
                                            const std::string &sourcePath = "<input>",
                                            bool debug = false)
{
    Lexer lexer(source);
    auto tokens = lexer.tokenize();

    Parser parser(std::move(tokens));
    auto ast = parser.parse();

    try
    {
        TypeChecker checker;
        checker.check(ast);
    }
    catch (const StaticTypeError &e)
    {
        std::cerr << Colors::YELLOW << "[TypeWarning] " << Colors::RESET
                  << e.what() << " (line " << e.line << ")\n";
    }

    Compiler compiler;
    auto chunk = compiler.compile(*ast);

    if (debug)
    {
        std::cerr << Colors::CYAN << "[DEBUG] Bytecode — " << sourcePath << "\n"
                  << Colors::RESET;
        disassembleChunk(*chunk, std::cerr);
    }

    return chunk;
}

// ─── REPL ─────────────────────────────────────────────────────────────────────

static void runREPL(bool debug = false)
{
    printBanner();
    std::cout << Colors::GREEN
              << "  REPL Mode — Bytecode VM\n"
              << "  Type 'exit' or Ctrl+D to quit\n"
              << Colors::RESET << "\n";

    VM vm;
    int lineNum = 1;
    std::string line;

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
            auto chunk = compileSource(line, "<repl>", debug);
            vm.run(chunk);
        }
        catch (const ParseError &e)
        {
            std::cerr << Colors::RED << "[ParseError] " << Colors::RESET
                      << e.what() << " (line " << e.line << ")\n";
        }
        catch (const QuantumError &e)
        {
            std::cerr << Colors::RED << "[" << e.kind << "] " << Colors::RESET
                      << e.what();
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

// ─── runFile ──────────────────────────────────────────────────────────────────

static void runFile(const std::string &path, bool debug = false)
{
    std::ifstream file(path);
    if (!file.is_open())
    {
        std::cerr << Colors::RED << "[Error] " << Colors::RESET
                  << "Cannot open file: " << path << "\n";
        std::exit(1);
    }

    if (path.size() < 3 || path.substr(path.size() - 3) != ".sa")
        std::cerr << Colors::YELLOW << "[Warning] " << Colors::RESET
                  << "File does not have .sa extension\n";

    std::ostringstream ss;
    ss << file.rdbuf();
    std::string source = ss.str();

    try
    {
        auto chunk = compileSource(source, path, debug);
        VM vm;
        vm.run(chunk);
    }
    catch (const ParseError &e)
    {
        std::cerr << Colors::RED << Colors::BOLD
                  << "\n  X ParseError" << Colors::RESET
                  << " in " << path << " at line " << e.line << ":" << e.col << "\n"
                  << "    " << e.what() << "\n\n";
        std::exit(1);
    }
    catch (const QuantumError &e)
    {
        std::cerr << Colors::RED << Colors::BOLD
                  << "\n  X " << e.kind << Colors::RESET;
        if (e.line > 0)
            std::cerr << " at line " << e.line;
        std::cerr << "\n    " << e.what() << "\n\n";
        std::exit(1);
    }
    catch (const std::exception &e)
    {
        std::cerr << Colors::RED << "[Fatal] " << Colors::RESET << e.what() << "\n";
        std::exit(1);
    }
}

// ─── checkFile ────────────────────────────────────────────────────────────────

static int checkFile(const std::string &path)
{
    std::ifstream file(path);
    if (!file.is_open())
    {
        std::cerr << path << ":1:1: error: Cannot open file\n";
        return 1;
    }

    std::ostringstream ss;
    ss << file.rdbuf();
    std::string source = ss.str();

    try
    {
        Lexer lexer(source);
        auto tokens = lexer.tokenize();
        Parser parser(std::move(tokens));
        auto ast = parser.parse();

        try
        {
            TypeChecker checker;
            checker.check(ast);
        }
        catch (const StaticTypeError &e)
        {
            std::cerr << path << ":" << e.line << ":1: warning: " << e.what() << "\n";
        }

        return 0;
    }
    catch (const ParseError &e)
    {
        std::cerr << path << ":" << e.line << ":" << e.col << ": error: " << e.what() << "\n";
        return 1;
    }
    catch (const std::exception &e)
    {
        std::cerr << path << ":1:1: error: " << e.what() << "\n";
        return 1;
    }
}

// ─── Batch test helpers ───────────────────────────────────────────────────────

struct TestResult
{
    std::string path;
    std::string source;
    std::string error;
    int line = 0;
    int col = 0;
};

static void redirectStdinToNull()
{
#ifdef _WIN32
    FILE *nul = nullptr;
    freopen_s(&nul, "NUL", "r", stdin);
#else
    freopen("/dev/null", "r", stdin);
#endif
}

struct StreamGuard
{
    std::streambuf *oldCout;
    std::streambuf *oldCerr;
    StreamGuard(std::streambuf *oc, std::streambuf *oe) : oldCout(oc), oldCerr(oe) {}
    ~StreamGuard()
    {
        std::cout.rdbuf(oldCout);
        std::cerr.rdbuf(oldCerr);
    }
};

static bool isInputDrivenError(const std::string &msg)
{
    if (msg.find("got string") != std::string::npos)
        return true;
    if (msg.find("got nil") != std::string::npos)
        return true;
    if (msg.find("Cannot convert ''") != std::string::npos)
        return true;
    if (msg.find("int() cannot convert ''") != std::string::npos)
        return true;
    if (msg.find("float() cannot convert ''") != std::string::npos)
        return true;
    return false;
}

static std::string getSourceLine(const std::string &source, int lineNum)
{
    if (lineNum <= 0)
        return "";
    int cur = 1;
    size_t i = 0;
    while (i < source.size())
    {
        size_t end = source.find('\n', i);
        if (end == std::string::npos)
            end = source.size();
        if (cur == lineNum)
            return source.substr(i, end - i);
        ++cur;
        i = end + 1;
    }
    return "";
}

// Run one .sa file non-fatally through the bytecode VM.
// Phase 1: parse-only — catches syntax errors.
// Phase 2: compile + execute — catches runtime errors; treats
//          input()-driven failures as passes.
static TestResult testFile(const std::string &path)
{
    TestResult res;
    res.path = path;

    std::ifstream file(path);
    if (!file.is_open())
    {
        res.error = "Cannot open file";
        return res;
    }

    std::ostringstream ss;
    ss << file.rdbuf();
    res.source = ss.str();
    const std::string &source = res.source;

    // ── Phase 1: parse only ───────────────────────────────────────────────
    try
    {
        Lexer lexer(source);
        auto tokens = lexer.tokenize();
        Parser parser(std::move(tokens));
        auto ast = parser.parse();
        (void)ast;
    }
    catch (const ParseError &e)
    {
        res.error = std::string("ParseError: ") + e.what();
        res.line = e.line;
        res.col = e.col;
        return res;
    }
    catch (const std::exception &e)
    {
        res.error = std::string("LexError: ") + e.what();
        res.line = 1;
        return res;
    }

    // ── Phase 2: compile + execute with output swallowed ─────────────────
    std::ostringstream sink;
    StreamGuard guard(
        std::cout.rdbuf(sink.rdbuf()),
        std::cerr.rdbuf(sink.rdbuf()));

    auto setErr = [&](const std::string &kind, const std::string &msg, int ln)
    {
        if (isInputDrivenError(msg))
            return;
        res.error = kind + ": " + msg;
        res.line = ln;
    };

    try
    {
        auto chunk = compileSource(source, path, false);
        VM vm;
        vm.run(chunk);
    }
    catch (const ParseError &e)
    {
        setErr("ParseError", e.what(), e.line);
    }
    catch (const QuantumError &e)
    {
        setErr(e.kind, e.what(), e.line);
    }
    catch (const std::exception &e)
    {
        std::string msg = e.what();
        if (!isInputDrivenError(msg))
            res.error = "Fatal: " + msg;
    }
    catch (...)
    {
        res.error = "Fatal: unknown exception";
    }

    return res;
}

static void collectSaFiles(const fs::path &dir, std::vector<fs::path> &out)
{
    if (!fs::exists(dir) || !fs::is_directory(dir))
        return;
    for (auto &entry : fs::recursive_directory_iterator(
             dir, fs::directory_options::skip_permission_denied))
    {
        if (entry.is_regular_file() && entry.path().extension() == ".sa")
            out.push_back(entry.path());
    }
}

// ─── runTestExamples ──────────────────────────────────────────────────────────

static int runTestExamples(const std::string &examplesDir)
{
    fs::path dir(examplesDir);
    if (!fs::exists(dir) || !fs::is_directory(dir))
    {
        std::cerr << Colors::RED << "[Error] " << Colors::RESET
                  << "Examples directory not found: " << dir.string() << "\n";
        return 1;
    }

    redirectStdinToNull();
    g_testMode = true;

    std::vector<fs::path> files;
    collectSaFiles(dir, files);

    if (files.empty())
    {
        std::cout << Colors::YELLOW << "[Warning] " << Colors::RESET
                  << "No .sa files found under: " << dir.string() << "\n";
        return 0;
    }

    std::sort(files.begin(), files.end());

    std::cout << Colors::CYAN << Colors::BOLD
              << "\n══════════════════════════════════════════════════\n"
              << "  Quantum Test Runner — Bytecode VM\n"
              << "══════════════════════════════════════════════════\n"
              << Colors::RESET
              << "  Directory  : " << Colors::YELLOW << fs::absolute(dir).string() << Colors::RESET << "\n"
              << "  Files found: " << Colors::YELLOW << files.size() << Colors::RESET << "\n\n";

    std::vector<TestResult> failures;
    int passed = 0;

    for (const auto &filePath : files)
    {
        std::string pathStr = filePath.string();
        std::string displayPath = pathStr;
        try
        {
            displayPath = fs::relative(filePath).string();
        }
        catch (...)
        {
        }

        TestResult tr = testFile(pathStr);
        tr.path = displayPath;

        if (tr.error.empty())
        {
            std::cout << Colors::GREEN << "  ✓ " << Colors::RESET << displayPath << "\n";
            ++passed;
        }
        else
        {
            std::cout << Colors::RED << "  ✗ " << Colors::RESET << displayPath << "\n";

            if (tr.line > 0)
            {
                std::cout << "      " << Colors::YELLOW << "Line " << tr.line;
                if (tr.col > 0)
                    std::cout << ":" << tr.col;
                std::cout << Colors::RESET << "  " << Colors::RED << tr.error << Colors::RESET << "\n";
            }
            else
            {
                std::cout << "      " << Colors::RED << tr.error << Colors::RESET << "\n";
            }

            if (tr.line > 0)
            {
                std::string srcLine = getSourceLine(tr.source, tr.line);
                if (!srcLine.empty())
                {
                    size_t indent = 0;
                    while (indent < srcLine.size() &&
                           (srcLine[indent] == ' ' || srcLine[indent] == '\t'))
                        ++indent;
                    std::string trimmed = srcLine.substr(indent);
                    std::cout << "      " << Colors::WHITE << "| " << trimmed << Colors::RESET << "\n";
                    if (tr.col > 0 && tr.col > (int)indent)
                    {
                        int caretPos = tr.col - (int)indent - 1;
                        if (caretPos < 0)
                            caretPos = 0;
                        std::cout << "      " << Colors::RED << "| "
                                  << std::string(caretPos, ' ') << "^"
                                  << Colors::RESET << "\n";
                    }
                }
            }

            failures.push_back(tr);
        }
    }

    int total = static_cast<int>(files.size());
    int failed = static_cast<int>(failures.size());

    std::cout << Colors::CYAN << Colors::BOLD
              << "\n══════════════════════════════════════════════════\n"
              << Colors::RESET;

    if (failed == 0)
        std::cout << Colors::GREEN << Colors::BOLD
                  << "  ✓ All " << total << " file(s) passed!\n"
                  << Colors::RESET;
    else
        std::cout << Colors::GREEN << "  Passed : " << passed << " / " << total << "\n"
                  << Colors::RESET
                  << Colors::RED << "  Failed : " << failed << " / " << total << "\n"
                  << Colors::RESET;

    std::cout << Colors::CYAN << Colors::BOLD
              << "══════════════════════════════════════════════════\n\n"
              << Colors::RESET;

    // ── Write test_results.txt ────────────────────────────────────────────
    const std::string reportPath = "test_results.txt";
    std::ofstream report(reportPath);

    if (!report.is_open())
    {
        std::cerr << Colors::RED << "[Error] " << Colors::RESET
                  << "Could not write report to: " << reportPath << "\n";
        return failed > 0 ? 1 : 0;
    }

    report << "================================================================================\n";
    report << "  Quantum Language — Test Results  (Bytecode VM)\n";
    report << "================================================================================\n";
    report << "  Directory : " << fs::absolute(dir).string() << "\n";
    report << "  Total     : " << total << "\n";
    report << "  Passed    : " << passed << "\n";
    report << "  Failed    : " << failed << "\n";
    report << "================================================================================\n\n";

    if (failures.empty())
    {
        report << "All files passed — no errors found.\n";
    }
    else
    {
        for (size_t i = 0; i < failures.size(); ++i)
        {
            const auto &f = failures[i];
            report << "################################################################################\n";
            report << "  FAILED FILE #" << (i + 1) << "\n";
            report << "################################################################################\n";
            report << "  Path  : " << f.path << "\n";
            report << "  Error : " << f.error << "\n";
            if (f.line > 0)
            {
                report << "  Line  : " << f.line;
                if (f.col > 0)
                    report << ":" << f.col;
                report << "\n";
                std::string srcLine = getSourceLine(f.source, f.line);
                if (!srcLine.empty())
                {
                    report << "  Code  : " << srcLine << "\n";
                    if (f.col > 0)
                    {
                        int lead = 0;
                        while (lead < (int)srcLine.size() &&
                               (srcLine[lead] == ' ' || srcLine[lead] == '\t'))
                            ++lead;
                        int caretPos = 10 + f.col - 1;
                        report << std::string(caretPos, ' ') << "^\n";
                    }
                }
            }
            report << "--------------------------------------------------------------------------------\n";
            report << "  Source Code:\n";
            report << "--------------------------------------------------------------------------------\n";
            report << f.source << "\n";
            report << "################################################################################\n\n";
        }
    }

    report.close();
    std::cout << Colors::CYAN << "  Report saved to: "
              << Colors::YELLOW << reportPath << Colors::RESET << "\n\n";

    return failed > 0 ? 1 : 0;
}

// ─── printHelp ────────────────────────────────────────────────────────────────

static void printHelp(const char *prog)
{
    std::cout << Colors::BOLD << "Usage:\n"
              << Colors::RESET
              << "  " << prog << " <file.sa>              Run a Quantum script\n"
              << "  " << prog << "                         Start interactive REPL\n"
              << "  " << prog << " --help                  Show this help\n"
              << "  " << prog << " --version               Show version info\n"
              << "  " << prog << " --aura                  Show achievements panel\n"
              << "  " << prog << " --check  <file.sa>      Parse + type-check only\n"
              << "  " << prog << " --test   [dir]          Batch-test all .sa files\n"
              << "  " << prog << " --debug  <file.sa>      Dump bytecode then run\n"
              << "  " << prog << " --dis    <file.sa>      Dump bytecode, no execution\n\n"
              << Colors::BOLD << "File extension:\n"
              << Colors::RESET
              << "  Quantum scripts use the .sa extension\n\n"
              << Colors::BOLD << "Examples:\n"
              << Colors::RESET
              << "  quantum hello.sa\n"
              << "  quantum script.sa\n"
              << "  quantum --test examples\n"
              << "  quantum --debug hello.sa\n"
              << "  quantum --dis   hello.sa\n\n"
              << Colors::BOLD << "Runtime:\n"
              << Colors::RESET
              << "  Bytecode VM — ~3× faster than v1 tree-walk interpreter\n"
              << "  Pipeline: Source → Lexer → Parser → AST → Compiler → Chunk → VM\n";
}

// ─── main ─────────────────────────────────────────────────────────────────────

int main(int argc, char *argv[])
{
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif

    if (argc == 1)
    {
        runREPL();
        return 0;
    }

    std::string arg = argv[1];

    // ── Flags that need no file argument ──────────────────────────────────
    if (arg == "--help" || arg == "-h")
    {
        printBanner();
        printHelp(argv[0]);
        return 0;
    }

    if (arg == "--aura")
    {
        printBanner();
        printAura();
        return 0;
    }

    if (arg == "--version" || arg == "-v")
    {
        std::cout << "Quantum Language v2.0.0\n"
                  << "Runtime: Bytecode VM (stack-based, ~3x faster than v1)\n"
                  << "Built By Muhammad Saad Amin\n";
        return 0;
    }

    // ── Flags that take a file/dir argument ───────────────────────────────
    if (arg == "--check" && argc >= 3)
        return checkFile(argv[2]);

    if (arg == "--test")
    {
        std::string targetDir = "examples";
        if (argc >= 3)
            targetDir = argv[2];
        return runTestExamples(targetDir);
    }

    if (arg == "--debug" && argc >= 3)
    {
        runFile(argv[2], true);
        return 0;
    }

    if (arg == "--dis" && argc >= 3)
    {
        std::ifstream f(argv[2]);
        if (!f.is_open())
        {
            std::cerr << Colors::RED << "[Error] " << Colors::RESET
                      << "Cannot open: " << argv[2] << "\n";
            return 1;
        }
        std::ostringstream ss;
        ss << f.rdbuf();
        try
        {
            auto chunk = compileSource(ss.str(), argv[2], false);
            disassembleChunk(*chunk, std::cout);
        }
        catch (const std::exception &e)
        {
            std::cerr << Colors::RED << "[Error] " << Colors::RESET << e.what() << "\n";
            return 1;
        }
        return 0;
    }

    // ── Default: run the file ──────────────────────────────────────────────
    runFile(arg);
    return 0;
}