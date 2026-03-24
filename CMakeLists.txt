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
#    Also runs hello.exe when it detects embedded bytecode in itself.
add_executable(quantum ${QUANTUM_SOURCES})
target_compile_definitions(quantum PRIVATE QUANTUM_MODE_COMPILER=1)
if(WIN32)
    target_link_libraries(quantum advapi32)
endif()

# ── qrun  — pure interpreter  (qrun hello.sa → runs in-place, no .exe) ────────
#    Same source tree; only the QRUN_MODE define changes behaviour in main.cpp.
add_executable(qrun ${QUANTUM_SOURCES})
target_compile_definitions(qrun PRIVATE QRUN_MODE=1)
if(WIN32)
    target_link_libraries(qrun advapi32)
endif()

message(STATUS "Quantum v2.0.0 — Bytecode VM")
message(STATUS "  Sources  : ${QUANTUM_SOURCES}")
message(STATUS "  Binaries : quantum  qrun")
message(STATUS "  Compile  : quantum hello.sa   ->  hello.exe")
message(STATUS "  Interpret: qrun    hello.sa   ->  runs directly")