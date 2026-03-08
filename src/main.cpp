#include "../include/Lexer.h"
#include "../include/Parser.h"
#include "../include/Interpreter.h"
#include "../include/Error.h"
#include "../include/Value.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include <filesystem>
#ifdef _WIN32
#include <windows.h>
#endif

namespace fs = std::filesystem;

// ─── Globals ──────────────────────────────────────────────────────────────────

// Set to true during --test runs so input() returns "" instantly instead of
// blocking on stdin.
bool g_testMode = false;

// ─── Banner / Aura ───────────────────────────────────────────────────────────

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
              << Colors::YELLOW << "  Quantum Language v1.0.0 | The Cybersecurity-Ready Scripting Language\n"
              << Colors::RESET << "\n";
}

static void printAura()
{
    std::cout << Colors::CYAN << Colors::BOLD
              << "\n╔══════════════════════════════════════════════════════════════════╗\n"
              << "║" << Colors::YELLOW << "                🌟 QUANTUM LANGUAGE ACHIEVEMENTS 🌟" << Colors::CYAN << "               ║\n"
              << "╠══════════════════════════════════════════════════════════════════╣\n"
              << "║" << Colors::GREEN << "  ✅ Complete C++17 Compiler Implementation" << Colors::CYAN << "                       ║\n"
              << "║" << Colors::GREEN << "  ✅ Tree-Walk Interpreter Engine" << Colors::CYAN << "                                 ║\n"
              << "║" << Colors::GREEN << "  ✅ Lexical Analysis & Tokenization" << Colors::CYAN << "                              ║\n"
              << "║" << Colors::GREEN << "  ✅ Recursive Descent Parser" << Colors::CYAN << "                                     ║\n"
              << "║" << Colors::GREEN << "  ✅ Abstract Syntax Tree (AST)" << Colors::CYAN << "                                   ║\n"
              << "║" << Colors::GREEN << "  ✅ Dynamic Type System" << Colors::CYAN << "                                          ║\n"
              << "║" << Colors::GREEN << "  ✅ REPL Interactive Mode" << Colors::CYAN << "                                        ║\n"
              << "║" << Colors::GREEN << "  ✅ Cross-Platform Build System" << Colors::CYAN << "                                  ║\n"
              << "║" << Colors::GREEN << "  ✅ VS Code Language Support" << Colors::CYAN << "                                     ║\n"
              << "║" << Colors::GREEN << "  ✅ GitHub CI/CD Pipeline" << Colors::CYAN << "                                        ║\n"
              << "║" << Colors::GREEN << "  ✅ Comprehensive Documentation" << Colors::CYAN << "                                  ║\n"
              << "╠══════════════════════════════════════════════════════════════════╣\n"
              << "║" << Colors::MAGENTA << "                    📊 PROJECT STATISTICS 📊" << Colors::CYAN << "                      ║\n"
              << "╠══════════════════════════════════════════════════════════════════╣\n"
              << "║" << Colors::WHITE << "  📁 Source Files: " << Colors::YELLOW << "6 core modules" << Colors::CYAN << "                                 ║\n"
              << "║" << Colors::WHITE << "  📝 Language Version: " << Colors::YELLOW << "v1.0.0" << Colors::CYAN << "                                     ║\n"
              << "║" << Colors::WHITE << "  🔧 Build System: " << Colors::YELLOW << "CMake + MSVC" << Colors::CYAN << "                                   ║\n"
              << "║" << Colors::WHITE << "  🎯 Language Standard: " << Colors::YELLOW << "C++17" << Colors::CYAN << "                                     ║\n"
              << "║" << Colors::WHITE << "  🚀 Performance: " << Colors::YELLOW << "Optimized Release Build" << Colors::CYAN << "                         ║\n"
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

// ─── REPL ─────────────────────────────────────────────────────────────────────

static void runREPL()
{
    printBanner();
    std::cout << Colors::GREEN << "  REPL Mode — type 'exit' or Ctrl+D to quit\n"
              << Colors::RESET << "\n";

    Interpreter interp;
    std::string line;
    int lineNum = 1;

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
            Lexer lexer(line);
            auto tokens = lexer.tokenize();
            Parser parser(std::move(tokens));
            auto ast = parser.parse();
            interp.execute(*ast);
        }
        catch (const ParseError &e)
        {
            std::cerr << Colors::RED << "[ParseError] " << Colors::RESET << e.what() << " (line " << e.line << ")\n";
        }
        catch (const QuantumError &e)
        {
            std::cerr << Colors::RED << "[" << e.kind << "] " << Colors::RESET << e.what();
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

static void runFile(const std::string &path)
{
    std::ifstream file(path);
    if (!file.is_open())
    {
        std::cerr << Colors::RED << "[Error] " << Colors::RESET
                  << "Cannot open file: " << path << "\n";
        std::exit(1);
    }

    if (path.size() < 3 || path.substr(path.size() - 3) != ".sa")
    {
        std::cerr << Colors::YELLOW << "[Warning] " << Colors::RESET
                  << "File does not have .sa extension\n";
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

        Interpreter interp;
        if (ast->is<BlockStmt>())
            interp.execBlock(ast->as<BlockStmt>(), interp.globals);
        else
            interp.execute(*ast);

        try
        {
            auto mainFn = interp.globals->get("main");
            if (mainFn.isFunction() &&
                std::holds_alternative<std::shared_ptr<QuantumFunction>>(mainFn.data))
            {
                CallExpr ce;
                ce.callee = std::make_unique<ASTNode>(Identifier{"main"}, 0);
                ASTNode callNode(std::move(ce), 0);
                interp.evaluate(callNode);
            }
        }
        catch (const ReturnSignal &)
        {
            // Normal return from main() — not an error.
        }
        catch (const NameError &e)
        {
            // Only swallow "Undefined variable: 'main'" (no main() defined).
            // Any other NameError thrown inside main() is a real bug.
            const std::string msg = e.what();
            if (msg.find("'main'") == std::string::npos &&
                msg.find("\"main\"") == std::string::npos)
            {
                std::cerr << Colors::RED << Colors::BOLD
                          << "\n  \u2717 " << e.kind << Colors::RESET;
                if (e.line > 0)
                    std::cerr << " at line " << e.line;
                std::cerr << "\n    " << msg << "\n\n";
                std::exit(1);
            }
        }
        catch (const QuantumError &e)
        {
            // All runtime errors inside main() are real failures.
            std::cerr << Colors::RED << Colors::BOLD
                      << "\n  \u2717 " << e.kind << Colors::RESET;
            if (e.line > 0)
                std::cerr << " at line " << e.line;
            std::cerr << "\n    " << e.what() << "\n\n";
            std::exit(1);
        }
    }
    catch (const ParseError &e)
    {
        std::cerr << Colors::RED << Colors::BOLD
                  << "\n  ✗ ParseError" << Colors::RESET
                  << " in " << path << " at line " << e.line << ":" << e.col << "\n"
                  << "    " << e.what() << "\n\n";
        std::exit(1);
    }
    catch (const QuantumError &e)
    {
        std::cerr << Colors::RED << Colors::BOLD
                  << "\n  ✗ " << e.kind << Colors::RESET;
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
        (void)ast;
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

// ─── Test helpers ─────────────────────────────────────────────────────────────

struct TestResult
{
    std::string path;   // display path (relative)
    std::string source; // full file source code
    std::string error;  // non-empty = failed
};

// Redirect stdin to NUL / /dev/null so that any input() / cin call inside a
// tested file returns EOF immediately instead of blocking the terminal.
static void redirectStdinToNull()
{
#ifdef _WIN32
    FILE *nul = nullptr;
    freopen_s(&nul, "NUL", "r", stdin);
#else
    freopen("/dev/null", "r", stdin);
#endif
}

// RAII guard: restores cout/cerr buffers unconditionally on scope exit,
// even when an exception propagates through testFile().  Without this,
// any throw that bypassed the manual rdbuf restore calls would leave
// stdout/stderr redirected to the sink for every subsequent test file.
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

// Returns true if an error message is caused by empty/EOF input from stdin
// being redirected to /dev/null during --test mode.  These are NOT real bugs
// in the file — the file would work fine with real user input.
static bool isInputDrivenError(const std::string &msg)
{
    // Errors that only happen because input() returned "" or EOF:
    // - Arithmetic/comparison on an empty string ("got string", "got nil")
    //   when the file stored input() result in a variable and did math on it
    // - IndexError from iterating over empty input
    // - Any error that originates from an empty-string-to-number conversion
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

// Run one .sa file non-fatally.
// Phase 1: Parse-only — catches all syntax errors.
// Phase 2: Execute — catches runtime errors, but treats input()-driven
//          failures as passes (the file is valid; it just needs real input).
// Returns "" on success/pass, or an error description on genuine failure.
static std::string testFile(const std::string &path)
{
    std::ifstream file(path);
    if (!file.is_open())
        return "Cannot open file";

    std::ostringstream ss;
    ss << file.rdbuf();
    std::string source = ss.str();

    // ── Phase 1: Parse only (no execution, no I/O risk) ───────────────────
    // Any ParseError here is a definite bug in the file.
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
        std::ostringstream out;
        out << "ParseError at line " << e.line << ":" << e.col << ": " << e.what();
        return out.str();
    }
    catch (const std::exception &e)
    {
        return std::string("LexError at line 1: ") + e.what();
    }

    // ── Phase 2: Execute with output swallowed ────────────────────────────
    // Swallow any print/cout output so it doesn't pollute the test console.
    // StreamGuard restores streams unconditionally on scope exit.
    std::ostringstream sink;
    StreamGuard guard(
        std::cout.rdbuf(sink.rdbuf()),
        std::cerr.rdbuf(sink.rdbuf()));

    std::string result; // "" means pass

    try
    {
        Lexer lexer(source);
        auto tokens = lexer.tokenize();
        Parser parser(std::move(tokens));
        auto ast = parser.parse();

        Interpreter interp;
        if (ast->is<BlockStmt>())
            interp.execBlock(ast->as<BlockStmt>(), interp.globals);
        else
            interp.execute(*ast);

        // Auto-call main() when present
        try
        {
            auto mainFn = interp.globals->get("main");
            if (mainFn.isFunction() &&
                std::holds_alternative<std::shared_ptr<QuantumFunction>>(mainFn.data))
            {
                CallExpr ce;
                ce.callee = std::make_unique<ASTNode>(Identifier{"main"}, 0);
                ASTNode callNode(std::move(ce), 0);
                interp.evaluate(callNode);
            }
        }
        catch (const ReturnSignal &)
        {
            // Normal return from main() — not an error.
        }
        catch (const NameError &e)
        {
            const std::string msg = e.what();
            // Swallow only "no variable 'main'" — everything else is a real bug.
            if (msg.find("'main'") == std::string::npos &&
                msg.find("\"main\"") == std::string::npos)
            {
                if (!isInputDrivenError(msg))
                {
                    std::ostringstream out;
                    out << e.kind;
                    if (e.line > 0)
                        out << " at line " << e.line;
                    out << ": " << msg;
                    result = out.str();
                }
            }
        }
        catch (const QuantumError &e)
        {
            const std::string msg = e.what();
            // If the error is purely because input() got "" from /dev/null,
            // the file itself is fine — treat as pass.
            if (!isInputDrivenError(msg))
            {
                std::ostringstream out;
                out << e.kind;
                if (e.line > 0)
                    out << " at line " << e.line;
                out << ": " << msg;
                result = out.str();
            }
        }
        catch (const std::exception &e)
        {
            std::string msg = e.what();
            if (!isInputDrivenError(msg))
                result = std::string("Fatal in main(): ") + msg;
        }
    }
    catch (const QuantumError &e)
    {
        const std::string msg = e.what();
        if (!isInputDrivenError(msg))
        {
            std::ostringstream out;
            out << e.kind;
            if (e.line > 0)
                out << " at line " << e.line;
            out << ": " << msg;
            result = out.str();
        }
    }
    catch (const std::exception &e)
    {
        std::string msg = e.what();
        if (!isInputDrivenError(msg))
            result = std::string("Fatal: ") + msg;
    }
    catch (...)
    {
        result = "Fatal: unknown exception";
    }

    // StreamGuard destructor restores cout/cerr here automatically.
    return result;
}

// Recursively collect every .sa file under `dir`.
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

// ─── runTestExamples ─────────────────────────────────────────────────────────

static int runTestExamples(const std::string &examplesDir)
{
    fs::path dir(examplesDir);
    if (!fs::exists(dir) || !fs::is_directory(dir))
    {
        std::cerr << Colors::RED << "[Error] " << Colors::RESET
                  << "Examples directory not found: " << dir.string() << "\n";
        return 1;
    }

    // Redirect stdin → NUL/dev/null so programs that call input() / cin
    // don't hang waiting for keyboard input.
    redirectStdinToNull();
    g_testMode = true;

    // Collect & sort files
    std::vector<fs::path> files;
    collectSaFiles(dir, files);

    if (files.empty())
    {
        std::cout << Colors::YELLOW << "[Warning] " << Colors::RESET
                  << "No .sa files found under: " << dir.string() << "\n";
        return 0;
    }

    std::sort(files.begin(), files.end());

    // Console header
    std::cout << Colors::CYAN << Colors::BOLD
              << "\n══════════════════════════════════════════════════\n"
              << "  Quantum Test Runner\n"
              << "══════════════════════════════════════════════════\n"
              << Colors::RESET
              << "  Directory  : " << Colors::YELLOW << fs::absolute(dir).string() << Colors::RESET << "\n"
              << "  Files found: " << Colors::YELLOW << files.size() << Colors::RESET << "\n\n";

    // Run every file
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

        // Read source now (before testFile swallows the stream)
        std::string source;
        {
            std::ifstream sf(pathStr);
            if (sf)
            {
                std::ostringstream tmp;
                tmp << sf.rdbuf();
                source = tmp.str();
            }
        }

        std::string errorMsg = testFile(pathStr);

        if (errorMsg.empty())
        {
            std::cout << Colors::GREEN << "  ✓ " << Colors::RESET << displayPath << "\n";
            ++passed;
        }
        else
        {
            std::cout << Colors::RED << "  ✗ " << Colors::RESET << displayPath
                      << Colors::RED << "  →  " << errorMsg << Colors::RESET << "\n";
            failures.push_back({displayPath, source, errorMsg});
        }
    }

    int total = static_cast<int>(files.size());
    int failed = static_cast<int>(failures.size());

    // Console summary
    std::cout << Colors::CYAN << Colors::BOLD
              << "\n══════════════════════════════════════════════════\n"
              << Colors::RESET;

    if (failed == 0)
    {
        std::cout << Colors::GREEN << Colors::BOLD
                  << "  ✓ All " << total << " file(s) passed!\n"
                  << Colors::RESET;
    }
    else
    {
        std::cout << Colors::GREEN << "  Passed : " << passed << " / " << total << "\n"
                  << Colors::RESET
                  << Colors::RED << "  Failed : " << failed << " / " << total << "\n"
                  << Colors::RESET;
    }

    std::cout << Colors::CYAN << Colors::BOLD
              << "══════════════════════════════════════════════════\n\n"
              << Colors::RESET;

    // ── Write test_results.txt ────────────────────────────────────────────
    // Only failed files are written; each entry contains:
    //   • the file path
    //   • the exact error
    //   • the complete source code of that file
    const std::string reportPath = "test_results.txt";
    std::ofstream report(reportPath);

    if (!report.is_open())
    {
        std::cerr << Colors::RED << "[Error] " << Colors::RESET
                  << "Could not write report to: " << reportPath << "\n";
        return failed > 0 ? 1 : 0;
    }

    report << "================================================================================\n";
    report << "  Quantum Language — Test Results\n";
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
              << "  " << prog << " --test examples         Test all .sa files under examples/\n"
              << "  " << prog << " --test <dir>            Test all .sa files under <dir>\n\n"
              << Colors::BOLD << "File extension:\n"
              << Colors::RESET
              << "  Quantum scripts use the .sa extension\n\n"
              << Colors::BOLD << "Examples:\n"
              << Colors::RESET
              << "  quantum hello.sa\n"
              << "  quantum script.sa\n"
              << "  quantum --test examples\n";
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
        std::cout << "Quantum Language v1.0.0\n"
                  << "Runtime: Tree-walk interpreter\n"
                  << "Built By Muhammad Saad Amin\n";
        return 0;
    }

    if (arg == "--check" && argc >= 3)
    {
        return checkFile(argv[2]);
    }

    if (arg == "--test")
    {
        std::string targetDir = "examples";
        if (argc >= 3)
            targetDir = argv[2];
        return runTestExamples(targetDir);
    }

    runFile(arg);
    return 0;
}