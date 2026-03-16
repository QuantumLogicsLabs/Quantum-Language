cmake_minimum_required(VERSION 3.16)
project(Quantum CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
    add_compile_options(/O2 /W3)
else()
    add_compile_options(-O2 -Wall -Wextra -Wno-unused-parameter)
endif()

include_directories(${CMAKE_SOURCE_DIR}/include)

# Collect every .cpp under src/ automatically.
# This means dropping new files into src/ is enough — no CMakeLists edit needed.
file(GLOB_RECURSE QUANTUM_SOURCES CONFIGURE_DEPENDS "src/*.cpp")

# Exclude the old tree-walk entry point if it still exists alongside the new one.
# (Both main.cpp and main_vm.cpp define main(); only one can be in the build.)
list(FILTER QUANTUM_SOURCES EXCLUDE REGEX ".*main_vm\\.cpp$")
list(FILTER QUANTUM_SOURCES EXCLUDE REGEX ".*Interpreter\\.cpp$")

add_executable(quantum ${QUANTUM_SOURCES})

if(WIN32)
    target_link_libraries(quantum advapi32)
endif()

message(STATUS "Quantum v2.0.0 — Bytecode VM")
message(STATUS "  Sources : ${QUANTUM_SOURCES}")
message(STATUS "  Binary  : quantum")
message(STATUS "  Usage   : quantum <file.sa>")