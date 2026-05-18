// parser.h — Recursive descent parser. Produces an AST from a Token stream.

#pragma once

#include "ast.h"
#include "token.h"

#include <initializer_list>
#include <stdexcept>
#include <vector>

namespace cvm {

class ParseError : public std::runtime_error {
public:
    ParseError(const std::string& msg, int line, int column)
        : std::runtime_error(msg), line(line), column(column) {}
    int line;
    int column;
};

class Parser {
public:
    explicit Parser(std::vector<Token> tokens);

    // Parse the full token stream into a list of top-level statements.
    std::vector<StatementPtr> parseProgram();

private:
    std::vector<Token> tokens_;
    size_t pos_ = 0;

    // Token cursor helpers.
    const Token& peek(size_t offset = 0) const;
    const Token& previous() const;
    const Token& advance();
    bool isAtEnd() const;
    bool check(TokenType type) const;
    bool match(std::initializer_list<TokenType> types);
    const Token& consume(TokenType type, const std::string& message);
    [[noreturn]] void error(const Token& t, const std::string& message) const;

    // Statement grammar.
    StatementPtr declaration();
    StatementPtr statement();
    StatementPtr varDeclaration();
    StatementPtr printStatement();
    StatementPtr inputStatement();
    StatementPtr ifStatement();
    StatementPtr whileStatement();
    StatementPtr block();
    StatementPtr expressionStatement();

    // Expression grammar (precedence-climbing recursive descent).
    ExpressionPtr expression();
    ExpressionPtr assignment();
    ExpressionPtr equality();
    ExpressionPtr comparison();
    ExpressionPtr term();
    ExpressionPtr factor();
    ExpressionPtr unary();
    ExpressionPtr primary();
};

} // namespace cvm
