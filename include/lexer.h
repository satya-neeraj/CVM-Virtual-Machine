// lexer.h — Scans raw source text and produces a stream of Tokens.
// The lexer is the first stage of the pipeline: source -> tokens.

#pragma once

#include "token.h"

#include <string>
#include <vector>

namespace cvm {

class Lexer {
public:
    explicit Lexer(const std::string& source);

    // Tokenize the entire source. Always terminates the result with an
    // END_OF_FILE token. Throws std::runtime_error on an unexpected character.
    std::vector<Token> tokenize();

private:
    const std::string& source_;
    size_t pos_    = 0;
    int    line_   = 1;
    int    column_ = 1;

    bool isAtEnd() const;
    char peek(size_t offset = 0) const;
    char advance();
    bool match(char expected);

    void  skipWhitespaceAndComments();
    Token number();
    Token identifierOrKeyword();
};

} // namespace cvm
