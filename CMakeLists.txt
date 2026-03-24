cmake_minimum_required(VERSION 3.16)
project(Quantum CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

if(MSVC)
    add_compile_options(/O2 /W3)
else()
    add_compile_options(-O2 -Wall -Wextra -Wno-unused-parameter)
endif()

include_directories(${CMAKE_SOURCE_DIR}/include)

# ── Collect sources (exclude old main_vm.cpp if it still exists) ───────────────
file(GLOB_RECURSE QUANTUM_SOURCES CONFIGURE_DEPENDS "src/*.cpp")
list(FILTER QUANTUM_SOURCES EXCLUDE REGEX ".*main_vm\\.cpp$")

# ── quantum  — compiler + bundler  (quantum hello.sa → hello.exe) ─────────────
#    Compiled with QUANTUM_MODE_COMPILER=1.
#    Uses quantum_stub.exe (below) as the template when bundling .exe files.
add_executable(quantum ${QUANTUM_SOURCES})
target_compile_definitions(quantum PRIVATE QUANTUM_MODE_COMPILER=1)
if(WIN32)
    target_link_libraries(quantum advapi32)
endif()

# ── qrun  — pure interpreter  (qrun hello.sa → runs in-place, no .exe) ────────
#    Compiled with QRUN_MODE=1.
add_executable(qrun ${QUANTUM_SOURCES})
target_compile_definitions(qrun PRIVATE QRUN_MODE=1)
if(WIN32)
    target_link_libraries(qrun advapi32)
endif()

# ── quantum_stub  — standalone runtime template ────────────────────────────────
#    Compiled WITHOUT any mode defines.
#    When quantum.exe bundles hello.sa it copies THIS binary as hello.exe,
#    then appends the bytecode payload.  Because quantum_stub.exe has no
#    QUANTUM_MODE_COMPILER define, it enters the embedded-bytecode branch on
#    startup and executes the payload — exactly what hello.exe must do.
add_executable(quantum_stub ${QUANTUM_SOURCES})
# deliberately no target_compile_definitions here
if(WIN32)
    target_link_libraries(quantum_stub advapi32)
endif()

message(STATUS "Quantum v2.0.0 — Bytecode VM")
message(STATUS "  Sources  : ${QUANTUM_SOURCES}")
message(STATUS "  Binaries : quantum  qrun  quantum_stub")
message(STATUS "  Compile  : quantum hello.sa   ->  hello.exe  (uses quantum_stub as template)")
message(STATUS "  Interpret: qrun    hello.sa   ->  runs directly")