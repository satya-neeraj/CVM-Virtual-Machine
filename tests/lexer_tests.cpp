// lexer_tests.cpp — exercises the tokenizer.
//
// We use a tiny hand-rolled assertion macro instead of a framework so the
// test binary has zero external dependencies.

#include "lexer.h"
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

std::vector<cvm::Token> tokenize(const std::string& src) {
    cvm::Lexer lex(src);
    return lex.tokenize();
}

void testBasicTokens() {
    auto t = tokenize("123 abc");
    CHECK(t.size() == 3, "expected 3 tokens incl. EOF");
    CHECK(t[0].type == cvm::TokenType::NUMBER,     "first token is NUMBER");
    CHECK(t[0].intValue == 123,                    "number value 123");
    CHECK(t[1].type == cvm::TokenType::IDENTIFIER, "second token is IDENTIFIER");
    CHECK(t[1].lexeme == "abc",                    "ident lexeme abc");
    CHECK(t[2].type == cvm::TokenType::END_OF_FILE,"last token is EOF");
}

void testOperators() {
    auto t = tokenize("+ - * / == != < > <= >= = !");
    using TT = cvm::TokenType;
    TT expected[] = {
        TT::PLUS, TT::MINUS, TT::STAR, TT::SLASH,
        TT::EQ,   TT::NEQ,
        TT::LT,   TT::GT,    TT::LE,   TT::GE,
        TT::ASSIGN, TT::BANG,
    };
    CHECK(t.size() == sizeof(expected)/sizeof(expected[0]) + 1, "operator count");
    for (size_t i = 0; i < sizeof(expected)/sizeof(expected[0]); ++i) {
        CHECK(t[i].type == expected[i], "operator order");
    }
}

void testKeywords() {
    auto t = tokenize("let if else while print input true false");
    using TT = cvm::TokenType;
    TT expected[] = {
        TT::LET, TT::IF, TT::ELSE, TT::WHILE, TT::PRINT, TT::INPUT,
        TT::TRUE_KW, TT::FALSE_KW,
    };
    CHECK(t.size() == sizeof(expected)/sizeof(expected[0]) + 1, "keyword count");
    for (size_t i = 0; i < sizeof(expected)/sizeof(expected[0]); ++i) {
        CHECK(t[i].type == expected[i], "keyword recognized");
    }
}

void testComments() {
    auto t = tokenize("// header comment\n let /* block */ x = 1;");
    using TT = cvm::TokenType;
    TT expected[] = {
        TT::LET, TT::IDENTIFIER, TT::ASSIGN, TT::NUMBER, TT::SEMICOLON,
    };
    CHECK(t.size() == sizeof(expected)/sizeof(expected[0]) + 1, "comments skipped");
    for (size_t i = 0; i < sizeof(expected)/sizeof(expected[0]); ++i) {
        CHECK(t[i].type == expected[i], "post-comment token order");
    }
}

void testLineTracking() {
    auto t = tokenize("let\nx = 1;");
    CHECK(t[0].line == 1, "let on line 1");
    CHECK(t[1].line == 2, "x on line 2");
}

} // namespace

int main() {
    testBasicTokens();
    testOperators();
    testKeywords();
    testComments();
    testLineTracking();

    if (failures == 0) {
        std::cout << "lexer_tests: OK\n";
        return 0;
    }
    std::cerr << "lexer_tests: " << failures << " failure(s)\n";
    return 1;
}
