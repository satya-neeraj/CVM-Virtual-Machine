// vm_tests.cpp — end-to-end execution tests.
//
// Each test runs source through the full Lexer → Parser → Compiler → VM
// pipeline using stringstreams for I/O, then compares stdout.

#include "ast.h"
#include "bytecode.h"
#include "compiler.h"
#include "lexer.h"
#include "parser.h"
#include "vm.h"

#include <iostream>
#include <sstream>
#include <string>

namespace {

int failures = 0;

#define CHECK_EQ(actual, expected, msg)                               \
    do {                                                              \
        if (!((actual) == (expected))) {                              \
            std::cerr << "FAIL " << __FILE__ << ":" << __LINE__       \
                      << " — " << (msg) << "\n"                       \
                      << "       got:      " << (actual)              \
                      << "       expected: " << (expected) << "\n";   \
            ++failures;                                               \
        }                                                             \
    } while (0)

#define CHECK(cond, msg)                                              \
    do {                                                              \
        if (!(cond)) {                                                \
            std::cerr << "FAIL " << __FILE__ << ":" << __LINE__       \
                      << " — " << (msg) << "\n";                      \
            ++failures;                                               \
        }                                                             \
    } while (0)

std::string run(const std::string& src, const std::string& stdinData = "") {
    cvm::Lexer    lexer(src);
    cvm::Parser   parser(lexer.tokenize());
    auto          program = parser.parseProgram();
    cvm::Compiler compiler;
    cvm::Chunk    chunk = compiler.compile(program);

    std::istringstream in(stdinData);
    std::ostringstream out;
    std::ostringstream err;
    cvm::VM vm;
    vm.execute(chunk, out, in, err, false);
    return out.str();
}

void testArithmetic() {
    CHECK_EQ(run("print(1 + 2 * 3 - 4);"), std::string("3\n"), "1+2*3-4 == 3");
    CHECK_EQ(run("print((1 + 2) * 3);"),   std::string("9\n"), "(1+2)*3 == 9");
    CHECK_EQ(run("print(-5 + 8);"),        std::string("3\n"), "unary minus");
}

void testVariables() {
    CHECK_EQ(run("let x = 5; let y = x * 2; print(y);"),
             std::string("10\n"),
             "var arithmetic");
}

void testIfElse() {
    CHECK_EQ(run("if (1 < 2) { print(1); } else { print(2); }"),
             std::string("1\n"),
             "true branch");
    CHECK_EQ(run("if (1 > 2) { print(1); } else { print(2); }"),
             std::string("2\n"),
             "false branch");
}

void testWhile() {
    // countdown 3, 2, 1
    CHECK_EQ(run("let n = 3; while (n > 0) { print(n); n = n - 1; }"),
             std::string("3\n2\n1\n"),
             "while countdown");
}

void testBooleans() {
    CHECK_EQ(run("print(true);"),    std::string("true\n"),  "print true");
    CHECK_EQ(run("print(!false);"),  std::string("true\n"),  "print !false");
    CHECK_EQ(run("print(1 == 1);"),  std::string("true\n"),  "1 == 1");
    CHECK_EQ(run("print(1 != 1);"),  std::string("false\n"), "1 != 1");
}

void testInputAndCompute() {
    CHECK_EQ(run("let x = 0; input(x); print(x * 2);", "21\n"),
             std::string("42\n"),
             "input then multiply");
}

void testIntegerDivision() {
    CHECK_EQ(run("print(20 / 3);"), std::string("6\n"), "integer div truncates");
}

void testNestedLoop() {
    // sum 1..5 = 15
    std::string src =
        "let i = 1; let s = 0;"
        "while (i <= 5) { s = s + i; i = i + 1; }"
        "print(s);";
    CHECK_EQ(run(src), std::string("15\n"), "sum 1..5");
}

void testDivisionByZero() {
    bool threw = false;
    try {
        run("let x = 1 / 0;");
    } catch (const cvm::RuntimeError&) {
        threw = true;
    }
    CHECK(threw, "division by zero throws");
}

} // namespace

int main() {
    testArithmetic();
    testVariables();
    testIfElse();
    testWhile();
    testBooleans();
    testInputAndCompute();
    testIntegerDivision();
    testNestedLoop();
    testDivisionByZero();

    if (failures == 0) {
        std::cout << "vm_tests: OK\n";
        return 0;
    }
    std::cerr << "vm_tests: " << failures << " failure(s)\n";
    return 1;
}
