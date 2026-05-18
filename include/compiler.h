// compiler.h — Translates the AST into a Chunk of bytecode.
//
// The Compiler implements both visitor interfaces. Each visit() method emits
// the bytes that implement the visited node. Jumps are emitted with a 0xFFFF
// placeholder operand which is later patched once the target offset is known.
//
// Variables are kept in a flat indexed table (Chunk::variableNames). The
// Compiler maintains a name->index map so that each `let` allocates a slot and
// every reference looks up that slot. There is no lexical scoping yet — all
// variables are effectively global to the chunk. This is intentional for a
// first-cut language; lexical scopes can be added by saving/restoring the map.

#pragma once

#include "ast.h"
#include "bytecode.h"

#include <cstddef>
#include <stdexcept>
#include <string>
#include <unordered_map>

namespace cvm {

class CompileError : public std::runtime_error {
public:
    CompileError(const std::string& msg, int line)
        : std::runtime_error(msg), line_(line) {}
    int line() const { return line_; }
private:
    int line_;
};

class Compiler : public ExpressionVisitor, public StatementVisitor {
public:
    // Top-level entry point: walks the program and returns the produced chunk.
    Chunk compile(const std::vector<StatementPtr>& program);

    // Expression visitors.
    void visit(const BinaryExpression&)     override;
    void visit(const UnaryExpression&)      override;
    void visit(const LiteralExpression&)    override;
    void visit(const VariableExpression&)   override;
    void visit(const AssignmentExpression&) override;

    // Statement visitors.
    void visit(const ExpressionStatement&) override;
    void visit(const PrintStatement&)      override;
    void visit(const InputStatement&)      override;
    void visit(const VariableDeclaration&) override;
    void visit(const BlockStatement&)      override;
    void visit(const IfStatement&)         override;
    void visit(const WhileStatement&)      override;

private:
    Chunk* chunk_ = nullptr;
    std::unordered_map<std::string, size_t> variableSlots_;

    // Emission helpers.
    void   emitByte(uint8_t b);
    void   emitOp(OpCode op);
    void   emitU16(size_t value, int line);

    // Emits `op` followed by a 0xFFFF placeholder; returns the offset of the
    // placeholder so it can be patched later with patchJump().
    size_t emitJump(OpCode op);

    // Writes `currentOffset()` into the two bytes at `placeholderOffset`.
    void   patchJump(size_t placeholderOffset, int line);

    // Writes an explicit `target` into the two placeholder bytes.
    void   patchJumpTo(size_t placeholderOffset, size_t target, int line);

    size_t currentOffset() const;

    size_t resolveVariable(const std::string& name, int line);
    size_t declareVariable(const std::string& name);

    void compileExpression(const Expression& expr);
    void compileStatement (const Statement&  stmt);
};

} // namespace cvm
