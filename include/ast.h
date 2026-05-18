// ast.h — Abstract Syntax Tree node hierarchy with the Visitor pattern.
//
// We use polymorphic inheritance + unique_ptr ownership for nodes. Two
// abstract visitor interfaces (ExpressionVisitor and StatementVisitor) let the
// Compiler and the AST pretty-printer dispatch on concrete node types without
// switch statements or RTTI gymnastics outside of one place (Parser::assignment).

#pragma once

#include "token.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace cvm {

// Forward declarations for the visitors below.
struct BinaryExpression;
struct UnaryExpression;
struct LiteralExpression;
struct VariableExpression;
struct AssignmentExpression;

struct ExpressionStatement;
struct PrintStatement;
struct InputStatement;
struct VariableDeclaration;
struct BlockStatement;
struct IfStatement;
struct WhileStatement;

class ExpressionVisitor {
public:
    virtual ~ExpressionVisitor() = default;
    virtual void visit(const BinaryExpression&)     = 0;
    virtual void visit(const UnaryExpression&)      = 0;
    virtual void visit(const LiteralExpression&)    = 0;
    virtual void visit(const VariableExpression&)   = 0;
    virtual void visit(const AssignmentExpression&) = 0;
};

class StatementVisitor {
public:
    virtual ~StatementVisitor() = default;
    virtual void visit(const ExpressionStatement&) = 0;
    virtual void visit(const PrintStatement&)      = 0;
    virtual void visit(const InputStatement&)      = 0;
    virtual void visit(const VariableDeclaration&) = 0;
    virtual void visit(const BlockStatement&)      = 0;
    virtual void visit(const IfStatement&)         = 0;
    virtual void visit(const WhileStatement&)      = 0;
};

struct Expression {
    virtual ~Expression() = default;
    virtual void accept(ExpressionVisitor& v) const = 0;
};

struct Statement {
    virtual ~Statement() = default;
    virtual void accept(StatementVisitor& v) const = 0;
};

using ExpressionPtr = std::unique_ptr<Expression>;
using StatementPtr  = std::unique_ptr<Statement>;

// ----------------------- Expression nodes -----------------------

enum class LiteralKind { INT, BOOL };

struct LiteralExpression : Expression {
    LiteralKind kind = LiteralKind::INT;
    int64_t     intValue  = 0;
    bool        boolValue = false;
    int         line      = 0;
    void accept(ExpressionVisitor& v) const override { v.visit(*this); }
};

struct VariableExpression : Expression {
    std::string name;
    int         line = 0;
    void accept(ExpressionVisitor& v) const override { v.visit(*this); }
};

struct UnaryExpression : Expression {
    TokenType     op = TokenType::MINUS; // MINUS or BANG
    ExpressionPtr operand;
    int           line = 0;
    void accept(ExpressionVisitor& v) const override { v.visit(*this); }
};

struct BinaryExpression : Expression {
    TokenType     op = TokenType::PLUS;
    ExpressionPtr left;
    ExpressionPtr right;
    int           line = 0;
    void accept(ExpressionVisitor& v) const override { v.visit(*this); }
};

struct AssignmentExpression : Expression {
    std::string   name;
    ExpressionPtr value;
    int           line = 0;
    void accept(ExpressionVisitor& v) const override { v.visit(*this); }
};

// ----------------------- Statement nodes ------------------------

struct ExpressionStatement : Statement {
    ExpressionPtr expr;
    void accept(StatementVisitor& v) const override { v.visit(*this); }
};

struct PrintStatement : Statement {
    ExpressionPtr expr;
    int           line = 0;
    void accept(StatementVisitor& v) const override { v.visit(*this); }
};

struct InputStatement : Statement {
    std::string variable;
    int         line = 0;
    void accept(StatementVisitor& v) const override { v.visit(*this); }
};

struct VariableDeclaration : Statement {
    std::string   name;
    ExpressionPtr initializer; // may be null
    int           line = 0;
    void accept(StatementVisitor& v) const override { v.visit(*this); }
};

struct BlockStatement : Statement {
    std::vector<StatementPtr> statements;
    void accept(StatementVisitor& v) const override { v.visit(*this); }
};

struct IfStatement : Statement {
    ExpressionPtr condition;
    StatementPtr  thenBranch;
    StatementPtr  elseBranch; // may be null
    int           line = 0;
    void accept(StatementVisitor& v) const override { v.visit(*this); }
};

struct WhileStatement : Statement {
    ExpressionPtr condition;
    StatementPtr  body;
    int           line = 0;
    void accept(StatementVisitor& v) const override { v.visit(*this); }
};

// Debug printers (implemented in ast.cpp).
std::string astToString(const Expression& expr, int indent = 0);
std::string astToString(const Statement&  stmt, int indent = 0);
std::string programToString(const std::vector<StatementPtr>& statements);

} // namespace cvm
