// parser_tests.cpp — exercises the recursive descent parser.

#include "ast.h"
#include "lexer.h"
#include "parser.h"
#include "token.h"

#include <iostream>
#include <string>
#include <vector>

namespace {

int failures = 0;

#define CHECK(cond, msg)                                              \
    do {                                                              \
        if (!(cond)) {                                                \
            std::cerr << "FAIL " << __FILE__ << ":" << __LINE__       \
                      << " — " << (msg) << "\n";                      \
            ++failures;                                               \
        }                                                             \
    } while (0)

std::vector<cvm::StatementPtr> parse(const std::string& src) {
    cvm::Lexer  lexer(src);
    cvm::Parser parser(lexer.tokenize());
    return parser.parseProgram();
}

void testVariableDeclaration() {
    auto prog = parse("let x = 1 + 2;");
    CHECK(prog.size() == 1, "single statement");
    auto* decl = dynamic_cast<cvm::VariableDeclaration*>(prog[0].get());
    CHECK(decl != nullptr,          "stmt is VariableDeclaration");
    CHECK(decl && decl->name == "x", "var name x");
    CHECK(decl && decl->initializer != nullptr, "initializer present");
}

void testIfElse() {
    auto prog = parse("if (1 < 2) { print(1); } else { print(2); }");
    CHECK(prog.size() == 1, "one statement");
    auto* ifs = dynamic_cast<cvm::IfStatement*>(prog[0].get());
    CHECK(ifs != nullptr,            "stmt is IfStatement");
    CHECK(ifs && ifs->thenBranch,    "then branch present");
    CHECK(ifs && ifs->elseBranch,    "else branch present");
}

void testWhile() {
    auto prog = parse("while (x > 0) { x = x - 1; }");
    CHECK(prog.size() == 1, "one statement");
    auto* ws = dynamic_cast<cvm::WhileStatement*>(prog[0].get());
    CHECK(ws != nullptr, "stmt is WhileStatement");
}

void testPrecedence() {
    // 1 + 2 * 3 should parse as 1 + (2 * 3), so the outer op is PLUS.
    auto prog = parse("let r = 1 + 2 * 3;");
    auto* decl = dynamic_cast<cvm::VariableDeclaration*>(prog[0].get());
    CHECK(decl && decl->initializer, "has initializer");
    auto* outer = dynamic_cast<cvm::BinaryExpression*>(decl->initializer.get());
    CHECK(outer != nullptr,                       "initializer is BinaryExpression");
    CHECK(outer && outer->op == cvm::TokenType::PLUS, "outer op is PLUS");
    auto* right = outer ? dynamic_cast<cvm::BinaryExpression*>(outer->right.get())
                        : nullptr;
    CHECK(right && right->op == cvm::TokenType::STAR, "right child is STAR");
}

void testParseError() {
    bool threw = false;
    try {
        parse("let = 5;"); // missing identifier
    } catch (const cvm::ParseError&) {
        threw = true;
    }
    CHECK(threw, "parse error on malformed input");
}

void testAssignmentExpression() {
    auto prog = parse("let x = 0; x = 5;");
    CHECK(prog.size() == 2, "two statements");
    auto* exprStmt = dynamic_cast<cvm::ExpressionStatement*>(prog[1].get());
    CHECK(exprStmt != nullptr, "second is ExpressionStatement");
    auto* assign = exprStmt ? dynamic_cast<cvm::AssignmentExpression*>(exprStmt->expr.get())
                            : nullptr;
    CHECK(assign != nullptr,            "expression is AssignmentExpression");
    CHECK(assign && assign->name == "x", "assignment target is x");
}

} // namespace

int main() {
    testVariableDeclaration();
    testIfElse();
    testWhile();
    testPrecedence();
    testParseError();
    testAssignmentExpression();

    if (failures == 0) {
        std::cout << "parser_tests: OK\n";
        return 0;
    }
    std::cerr << "parser_tests: " << failures << " failure(s)\n";
    return 1;
}
