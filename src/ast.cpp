// ast.cpp — Implements astToString / programToString for the --ast flag.

#include "ast.h"
#include "lexer.h" // for tokenTypeName

#include <sstream>

namespace cvm {

static std::string indentStr(int n) { return std::string(static_cast<size_t>(n) * 2, ' '); }

namespace {

class ExprPrinter : public ExpressionVisitor {
public:
    std::ostringstream out;
    int indent;
    explicit ExprPrinter(int i) : indent(i) {}

    void visit(const BinaryExpression& e) override {
        out << indentStr(indent) << "Binary(" << tokenTypeName(e.op) << ")\n";
        out << astToString(*e.left,  indent + 1);
        out << astToString(*e.right, indent + 1);
    }
    void visit(const UnaryExpression& e) override {
        out << indentStr(indent) << "Unary(" << tokenTypeName(e.op) << ")\n";
        out << astToString(*e.operand, indent + 1);
    }
    void visit(const LiteralExpression& e) override {
        out << indentStr(indent) << "Literal(";
        if (e.kind == LiteralKind::INT) out << e.intValue;
        else                            out << (e.boolValue ? "true" : "false");
        out << ")\n";
    }
    void visit(const VariableExpression& e) override {
        out << indentStr(indent) << "Variable(" << e.name << ")\n";
    }
    void visit(const AssignmentExpression& e) override {
        out << indentStr(indent) << "Assign(" << e.name << ")\n";
        out << astToString(*e.value, indent + 1);
    }
};

class StmtPrinter : public StatementVisitor {
public:
    std::ostringstream out;
    int indent;
    explicit StmtPrinter(int i) : indent(i) {}

    void visit(const ExpressionStatement& s) override {
        out << indentStr(indent) << "ExprStmt\n";
        out << astToString(*s.expr, indent + 1);
    }
    void visit(const PrintStatement& s) override {
        out << indentStr(indent) << "Print\n";
        out << astToString(*s.expr, indent + 1);
    }
    void visit(const InputStatement& s) override {
        out << indentStr(indent) << "Input(" << s.variable << ")\n";
    }
    void visit(const VariableDeclaration& s) override {
        out << indentStr(indent) << "VarDecl(" << s.name << ")\n";
        if (s.initializer) out << astToString(*s.initializer, indent + 1);
    }
    void visit(const BlockStatement& s) override {
        out << indentStr(indent) << "Block\n";
        for (const auto& st : s.statements) out << astToString(*st, indent + 1);
    }
    void visit(const IfStatement& s) override {
        out << indentStr(indent) << "If\n";
        out << indentStr(indent + 1) << "cond:\n";
        out << astToString(*s.condition,  indent + 2);
        out << indentStr(indent + 1) << "then:\n";
        out << astToString(*s.thenBranch, indent + 2);
        if (s.elseBranch) {
            out << indentStr(indent + 1) << "else:\n";
            out << astToString(*s.elseBranch, indent + 2);
        }
    }
    void visit(const WhileStatement& s) override {
        out << indentStr(indent) << "While\n";
        out << indentStr(indent + 1) << "cond:\n";
        out << astToString(*s.condition, indent + 2);
        out << indentStr(indent + 1) << "body:\n";
        out << astToString(*s.body,      indent + 2);
    }
};

} // anonymous namespace

std::string astToString(const Expression& expr, int indent) {
    ExprPrinter p(indent);
    expr.accept(p);
    return p.out.str();
}

std::string astToString(const Statement& stmt, int indent) {
    StmtPrinter p(indent);
    stmt.accept(p);
    return p.out.str();
}

std::string programToString(const std::vector<StatementPtr>& statements) {
    std::ostringstream oss;
    for (const auto& s : statements) oss << astToString(*s, 0);
    return oss.str();
}

} // namespace cvm
