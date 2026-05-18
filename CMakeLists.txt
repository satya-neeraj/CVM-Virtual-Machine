cmake_minimum_required(VERSION 3.10)
project(CVMPlusPlus CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/W4 /permissive-)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

include_directories(${CMAKE_SOURCE_DIR}/include)

# Core library — everything except main.cpp goes here so tests can link it.
add_library(cvm_core STATIC
    src/lexer.cpp
    src/parser.cpp
    src/ast.cpp
    src/bytecode.cpp
    src/compiler.cpp
    src/vm.cpp
    src/repl.cpp
)

# The CLI executable.
add_executable(cvm src/main.cpp)
target_link_libraries(cvm PRIVATE cvm_core)

# Tests.
enable_testing()

add_executable(lexer_tests  tests/lexer_tests.cpp)
target_link_libraries(lexer_tests PRIVATE cvm_core)
add_test(NAME lexer_tests COMMAND lexer_tests)

add_executable(parser_tests tests/parser_tests.cpp)
target_link_libraries(parser_tests PRIVATE cvm_core)
add_test(NAME parser_tests COMMAND parser_tests)

add_executable(vm_tests     tests/vm_tests.cpp)
target_link_libraries(vm_tests PRIVATE cvm_core)
add_test(NAME vm_tests COMMAND vm_tests)
