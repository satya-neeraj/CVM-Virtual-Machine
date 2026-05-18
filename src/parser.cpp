// parser.cpp — Recursive descent parser implementation.
//
// Grammar (simplified):
//   program     -> declaration* EOF ;
//   declaration -> varDecl | statement ;
//   varDecl     -> "let" IDENT ( "=" expression )? ";" ;
//   statement   -> printStmt | inputStmt | ifStmt | whileStmt | block | exprStmt ;
//   block       -> "{" declaration* "}" ;
//   ifStmt      -> "if" "(" expression ")" statement ( "else" statement )? ;
//   whileStmt   -> "while" "(" expression ")" statement ;
//   printStmt   -> "print" "(" expression ")" ";" ;
//   inputStmt   -> "input" "(" IDENT ")" ";" ;
//   exprStmt    -> expression ";" ;
//   expression  -> assignment ;
//   assignment  -> IDENT "=" assignment | equality ;
//   equality    -> comparison ( ( "==" | "!=" ) comparison )* ;
//   comparison  -> term ( ( "<" | ">" | "<=" | ">=" ) term )* ;
//   term        -> factor ( ( "+" | "-" ) factor )* ;
//   factor      -> unary ( ( "*" | "/" ) unary )* ;
//   unary       -> ( "-" | "!" ) unary | primary ;
//   primary     -> NUMBER | "true" | "false" | IDENT | "(" expression ")" ;

#include "parser.h"

#include <sstream>
#include <utility>

namespace cvm {

Parser::Parser(std::vector<Token> tokens) : tokens_(std::move(tokens)) {}

// ---------------- Cursor helpers ----------------

const Token& Parser::peek(size_t offset) const {
    if (pos_ + offset >= tokens_.size()) return tokens_.back();
    return tokens_[pos_ + offset];
}

const Token& Parser::previous() const { return tokens_[pos_ - 1]; }

const Token& Parser::advance() {
    if (!isAtEnd()) pos_++;
    return previous();
}

bool Parser::isAtEnd() const { return peek().type == TokenType::END_OF_FILE; }

bool Parser::check(TokenType type) const {
    if (isAtEnd()) return type == TokenType::END_OF_FILE;
    return peek().type == type;
}

bool Parser::match(std::initializer_list<TokenType> types) {
    for (TokenType t : types) {
        if (check(t)) { advance(); return true; }
    }
    return false;
}

const Token& Parser::consume(TokenType type, const std::string& message) {
    if (check(type)) return advance();
    error(peek(), message);
}

void Parser::error(const Token& t, const std::string& message) const {
    std::ostringstream oss;
    oss << "Parse error at line " << t.line << ", column " << t.column
        << " (token '" << t.lexeme << "'): " << message;
    throw ParseError(oss.str(), t.line, t.column);
}

// ---------------- Statements ----------------

std::vector<StatementPtr> Parser::parseProgram() {
    std::vector<StatementPtr> stmts;
    while (!isAtEnd()) stmts.push_back(declaration());
    return stmts;
}

StatementPtr Parser::declaration() {
    if (match({TokenType::LET})) return varDeclaration();
    return statement();
}

StatementPtr Parser::varDeclaration() {
    const Token& name = consume(TokenType::IDENTIFIER, "Expected variable name after 'let'.");
    ExpressionPtr init;
    if (match({TokenType::ASSIGN})) init = expression();
    consume(TokenType::SEMICOLON, "Expected ';' after variable declaration.");

    auto node = std::make_unique<VariableDeclaration>();
    node->name        = name.lexeme;
    node->initializer = std::move(init);
    node->line        = name.line;
    return node;
}

StatementPtr Parser::statement() {
    if (match({TokenType::PRINT}))  return printStatement();
    if (match({TokenType::INPUT}))  return inputStatement();
    if (match({TokenType::IF}))     return ifStatement();
    if (match({TokenType::WHILE}))  return whileStatement();
    if (match({TokenType::LBRACE})) return block();
    return expressionStatement();
}

StatementPtr Parser::printStatement() {
    int line = previous().line;
    consume(TokenType::LPAREN, "Expected '(' after 'print'.");
    auto expr = expression();
    consume(TokenType::RPAREN, "Expected ')' after print expression.");
    consume(TokenType::SEMICOLON, "Expected ';' after print statement.");

    auto stmt = std::make_unique<PrintStatement>();
    stmt->expr = std::move(expr);
    stmt->line = line;
    return stmt;
}

StatementPtr Parser::inputStatement() {
    int line = previous().line;
    consume(TokenType::LPAREN, "Expected '(' after 'input'.");
    const Token& name = consume(TokenType::IDENTIFIER, "Expected variable name in input().");
    consume(TokenType::RPAREN, "Expected ')' after input variable.");
    consume(TokenType::SEMICOLON, "Expected ';' after input statement.");

    auto stmt = std::make_unique<InputStatement>();
    stmt->variable = name.lexeme;
    stmt->line     = line;
    return stmt;
}

StatementPtr Parser::ifStatement() {
    int line = previous().line;
    consume(TokenType::LPAREN, "Expected '(' after 'if'.");
    auto cond = expression();
    consume(TokenType::RPAREN, "Expected ')' after if condition.");

    auto thenBranch = statement();
    StatementPtr elseBranch;
    if (match({TokenType::ELSE})) elseBranch = statement();

    auto stmt = std::make_unique<IfStatement>();
    stmt->condition  = std::move(cond);
    stmt->thenBranch = std::move(thenBranch);
    stmt->elseBranch = std::move(elseBranch);
    stmt->line       = line;
    return stmt;
}

StatementPtr Parser::whileStatement() {
    int line = previous().line;
    consume(TokenType::LPAREN, "Expected '(' after 'while'.");
    auto cond = expression();
    consume(TokenType::RPAREN, "Expected ')' after while condition.");
    auto body = statement();

    auto stmt = std::make_unique<WhileStatement>();
    stmt->condition = std::move(cond);
    stmt->body      = std::move(body);
    stmt->line      = line;
    return stmt;
}

StatementPtr Parser::block() {
    auto blk = std::make_unique<BlockStatement>();
    while (!check(TokenType::RBRACE) && !isAtEnd()) {
        blk->statements.push_back(declaration());
    }
    consume(TokenType::RBRACE, "Expected '}' at end of block.");
    return blk;
}

StatementPtr Parser::expressionStatement() {
    auto expr = expression();
    consume(TokenType::SEMICOLON, "Expected ';' after expression.");
    auto stmt = std::make_unique<ExpressionStatement>();
    stmt->expr = std::move(expr);
    return stmt;
}

// ---------------- Expressions ----------------

ExpressionPtr Parser::expression() { return assignment(); }

ExpressionPtr Parser::assignment() {
    auto expr = equality();
    if (match({TokenType::ASSIGN})) {
        const Token& eq = previous();
        auto value      = assignment(); // right-associative
        auto* var       = dynamic_cast<VariableExpression*>(expr.get());
        if (!var) error(eq, "Invalid assignment target.");

        auto a = std::make_unique<AssignmentExpression>();
        a->name  = var->name;
        a->value = std::move(value);
        a->line  = eq.line;
        return a;
    }
    return expr;
}

static ExpressionPtr makeBinary(ExpressionPtr left, TokenType op, ExpressionPtr right, int line) {
    auto bin   = std::make_unique<BinaryExpression>();
    bin->op    = op;
    bin->left  = std::move(left);
    bin->right = std::move(right);
    bin->line  = line;
    return bin;
}

ExpressionPtr Parser::equality() {
    auto expr = comparison();
    while (match({TokenType::EQ, TokenType::NEQ})) {
        TokenType op = previous().type;
        int line     = previous().line;
        auto right   = comparison();
        expr = makeBinary(std::move(expr), op, std::move(right), line);
    }
    return expr;
}

ExpressionPtr Parser::comparison() {
    auto expr = term();
    while (match({TokenType::LT, TokenType::GT, TokenType::LE, TokenType::GE})) {
        TokenType op = previous().type;
        int line     = previous().line;
        auto right   = term();
        expr = makeBinary(std::move(expr), op, std::move(right), line);
    }
    return expr;
}

ExpressionPtr Parser::term() {
    auto expr = factor();
    while (match({TokenType::PLUS, TokenType::MINUS})) {
        TokenType op = previous().type;
        int line     = previous().line;
        auto right   = factor();
        expr = makeBinary(std::move(expr), op, std::move(right), line);
    }
    return expr;
}

ExpressionPtr Parser::factor() {
    auto expr = unary();
    while (match({TokenType::STAR, TokenType::SLASH})) {
        TokenType op = previous().type;
        int line     = previous().line;
        auto right   = unary();
        expr = makeBinary(std::move(expr), op, std::move(right), line);
    }
    return expr;
}

ExpressionPtr Parser::unary() {
    if (match({TokenType::MINUS, TokenType::BANG})) {
        TokenType op  = previous().type;
        int       ln  = previous().line;
        auto      opd = unary();
        auto u = std::make_unique<UnaryExpression>();
        u->op      = op;
        u->operand = std::move(opd);
        u->line    = ln;
        return u;
    }
    return primary();
}

ExpressionPtr Parser::primary() {
    if (match({TokenType::NUMBER})) {
        auto lit = std::make_unique<LiteralExpression>();
        lit->kind     = LiteralKind::INT;
        lit->intValue = previous().intValue;
        lit->line     = previous().line;
        return lit;
    }
    if (match({TokenType::TRUE_KW})) {
        auto lit = std::make_unique<LiteralExpression>();
        lit->kind      = LiteralKind::BOOL;
        lit->boolValue = true;
        lit->line      = previous().line;
        return lit;
    }
    if (match({TokenType::FALSE_KW})) {
        auto lit = std::make_unique<LiteralExpression>();
        lit->kind      = LiteralKind::BOOL;
        lit->boolValue = false;
        lit->line      = previous().line;
        return lit;
    }
    if (match({TokenType::IDENTIFIER})) {
        auto var = std::make_unique<VariableExpression>();
        var->name = previous().lexeme;
        var->line = previous().line;
        return var;
    }
    if (match({TokenType::LPAREN})) {
        auto expr = expression();
        consume(TokenType::RPAREN, "Expected ')' after expression.");
        return expr;
    }
    error(peek(), "Expected expression.");
}

} // namespace cvm
