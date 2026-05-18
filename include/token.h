// token.h — Token type enum, Token struct and helper for diagnostics.
// All other compiler stages consume Tokens produced by the Lexer.

#pragma once

#include <cstdint>
#include <string>

namespace cvm {

enum class TokenType {
    // Literals
    NUMBER,
    IDENTIFIER,

    // Keywords
    LET,
    IF,
    ELSE,
    WHILE,
    PRINT,
    INPUT,
    TRUE_KW,
    FALSE_KW,

    // Operators
    PLUS,       // +
    MINUS,      // -
    STAR,       // *
    SLASH,      // /
    ASSIGN,     // =
    EQ,         // ==
    BANG,       // !
    NEQ,        // !=
    LT,         // <
    GT,         // >
    LE,         // <=
    GE,         // >=

    // Punctuation
    LPAREN,     // (
    RPAREN,     // )
    LBRACE,     // {
    RBRACE,     // }
    SEMICOLON,  // ;

    // Special
    END_OF_FILE
};

struct Token {
    TokenType   type   = TokenType::END_OF_FILE;
    std::string lexeme;          // original source text for the token
    int64_t     intValue = 0;    // populated for NUMBER tokens
    int         line     = 1;    // 1-based line in source
    int         column   = 1;    // 1-based column in source
};

// Convert a TokenType to a human-readable name (used for --tokens and errors).
std::string tokenTypeName(TokenType type);

} // namespace cvm
