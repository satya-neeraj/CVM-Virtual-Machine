// lexer.cpp — Hand-written single-pass tokenizer.

#include "lexer.h"

#include <cctype>
#include <sstream>
#include <stdexcept>
#include <unordered_map>

namespace cvm {

static const std::unordered_map<std::string, TokenType>& keywords() {
    static const std::unordered_map<std::string, TokenType> table = {
        {"let",   TokenType::LET},
        {"if",    TokenType::IF},
        {"else",  TokenType::ELSE},
        {"while", TokenType::WHILE},
        {"print", TokenType::PRINT},
        {"input", TokenType::INPUT},
        {"true",  TokenType::TRUE_KW},
        {"false", TokenType::FALSE_KW},
    };
    return table;
}

std::string tokenTypeName(TokenType type) {
    switch (type) {
        case TokenType::NUMBER:      return "NUMBER";
        case TokenType::IDENTIFIER:  return "IDENTIFIER";
        case TokenType::LET:         return "LET";
        case TokenType::IF:          return "IF";
        case TokenType::ELSE:        return "ELSE";
        case TokenType::WHILE:       return "WHILE";
        case TokenType::PRINT:       return "PRINT";
        case TokenType::INPUT:       return "INPUT";
        case TokenType::TRUE_KW:     return "TRUE";
        case TokenType::FALSE_KW:    return "FALSE";
        case TokenType::PLUS:        return "PLUS";
        case TokenType::MINUS:       return "MINUS";
        case TokenType::STAR:        return "STAR";
        case TokenType::SLASH:       return "SLASH";
        case TokenType::ASSIGN:      return "ASSIGN";
        case TokenType::EQ:          return "EQ";
        case TokenType::BANG:        return "BANG";
        case TokenType::NEQ:         return "NEQ";
        case TokenType::LT:          return "LT";
        case TokenType::GT:          return "GT";
        case TokenType::LE:          return "LE";
        case TokenType::GE:          return "GE";
        case TokenType::LPAREN:      return "LPAREN";
        case TokenType::RPAREN:      return "RPAREN";
        case TokenType::LBRACE:      return "LBRACE";
        case TokenType::RBRACE:      return "RBRACE";
        case TokenType::SEMICOLON:   return "SEMICOLON";
        case TokenType::END_OF_FILE: return "EOF";
    }
    return "UNKNOWN";
}

Lexer::Lexer(const std::string& source) : source_(source) {}

bool Lexer::isAtEnd() const { return pos_ >= source_.size(); }

char Lexer::peek(size_t offset) const {
    if (pos_ + offset >= source_.size()) return '\0';
    return source_[pos_ + offset];
}

char Lexer::advance() {
    char c = source_[pos_++];
    if (c == '\n') {
        line_++;
        column_ = 1;
    } else {
        column_++;
    }
    return c;
}

bool Lexer::match(char expected) {
    if (isAtEnd() || source_[pos_] != expected) return false;
    advance();
    return true;
}

void Lexer::skipWhitespaceAndComments() {
    while (!isAtEnd()) {
        char c = peek();
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
            advance();
        } else if (c == '/' && peek(1) == '/') {
            // line comment: consume to end-of-line
            while (!isAtEnd() && peek() != '\n') advance();
        } else if (c == '/' && peek(1) == '*') {
            // block comment: consume to matching */
            advance(); advance();
            while (!isAtEnd() && !(peek() == '*' && peek(1) == '/')) advance();
            if (!isAtEnd()) { advance(); advance(); }
        } else {
            break;
        }
    }
}

Token Lexer::number() {
    int startLine   = line_;
    int startColumn = column_;
    size_t start    = pos_;
    while (!isAtEnd() && std::isdigit(static_cast<unsigned char>(peek()))) advance();
    std::string lex = source_.substr(start, pos_ - start);
    Token t;
    t.type     = TokenType::NUMBER;
    t.lexeme   = lex;
    t.intValue = std::stoll(lex);
    t.line     = startLine;
    t.column   = startColumn;
    return t;
}

Token Lexer::identifierOrKeyword() {
    int startLine   = line_;
    int startColumn = column_;
    size_t start    = pos_;
    while (!isAtEnd() &&
           (std::isalnum(static_cast<unsigned char>(peek())) || peek() == '_')) {
        advance();
    }
    std::string lex = source_.substr(start, pos_ - start);
    Token t;
    t.lexeme = lex;
    t.line   = startLine;
    t.column = startColumn;
    const auto& kw = keywords();
    auto it = kw.find(lex);
    t.type = (it != kw.end()) ? it->second : TokenType::IDENTIFIER;
    return t;
}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;
    while (true) {
        skipWhitespaceAndComments();
        if (isAtEnd()) break;

        int startLine   = line_;
        int startColumn = column_;
        char c          = peek();

        if (std::isdigit(static_cast<unsigned char>(c))) {
            tokens.push_back(number());
            continue;
        }
        if (std::isalpha(static_cast<unsigned char>(c)) || c == '_') {
            tokens.push_back(identifierOrKeyword());
            continue;
        }

        advance();
        Token t;
        t.line   = startLine;
        t.column = startColumn;
        t.lexeme = std::string(1, c);

        switch (c) {
            case '+': t.type = TokenType::PLUS;      break;
            case '-': t.type = TokenType::MINUS;     break;
            case '*': t.type = TokenType::STAR;      break;
            case '/': t.type = TokenType::SLASH;     break;
            case '(': t.type = TokenType::LPAREN;    break;
            case ')': t.type = TokenType::RPAREN;    break;
            case '{': t.type = TokenType::LBRACE;    break;
            case '}': t.type = TokenType::RBRACE;    break;
            case ';': t.type = TokenType::SEMICOLON; break;
            case '=':
                if (match('=')) { t.type = TokenType::EQ;     t.lexeme = "=="; }
                else            { t.type = TokenType::ASSIGN; }
                break;
            case '!':
                if (match('=')) { t.type = TokenType::NEQ;    t.lexeme = "!="; }
                else            { t.type = TokenType::BANG;   }
                break;
            case '<':
                if (match('=')) { t.type = TokenType::LE;     t.lexeme = "<="; }
                else            { t.type = TokenType::LT;     }
                break;
            case '>':
                if (match('=')) { t.type = TokenType::GE;     t.lexeme = ">="; }
                else            { t.type = TokenType::GT;     }
                break;
            default: {
                std::ostringstream oss;
                oss << "Unexpected character '" << c
                    << "' at line " << startLine << ", column " << startColumn;
                throw std::runtime_error(oss.str());
            }
        }
        tokens.push_back(t);
    }

    Token eofTok;
    eofTok.type   = TokenType::END_OF_FILE;
    eofTok.line   = line_;
    eofTok.column = column_;
    tokens.push_back(eofTok);
    return tokens;
}

} // namespace cvm
