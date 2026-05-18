// compiler.cpp — AST traversal that produces a Chunk of bytecode.

#include "compiler.h"

#include <limits>
#include <sstream>

namespace cvm {

// ---------------------------- public API ----------------------------

Chunk Compiler::compile(const std::vector<StatementPtr>& program) {
    Chunk chunk;
    chunk_ = &chunk;
    variableSlots_.clear();

    for (const auto& stmt : program) {
        compileStatement(*stmt);
    }
    emitOp(OpCode::HALT);

    chunk_ = nullptr;
    return chunk;
}

// ---------------------------- emission ------------------------------

void Compiler::emitByte(uint8_t b) {
    chunk_->code.push_back(b);
}

void Compiler::emitOp(OpCode op) {
    emitByte(static_cast<uint8_t>(op));
}

void Compiler::emitU16(size_t value, int line) {
    if (value > std::numeric_limits<uint16_t>::max()) {
        throw CompileError("16-bit operand overflow (chunk too large)", line);
    }
    emitByte(static_cast<uint8_t>(value & 0xFF));
    emitByte(static_cast<uint8_t>((value >> 8) & 0xFF));
}

size_t Compiler::emitJump(OpCode op) {
    emitOp(op);
    size_t placeholder = chunk_->code.size();
    emitByte(0xFF);
    emitByte(0xFF);
    return placeholder;
}

void Compiler::patchJump(size_t placeholderOffset, int line) {
    patchJumpTo(placeholderOffset, currentOffset(), line);
}

void Compiler::patchJumpTo(size_t placeholderOffset, size_t target, int line) {
    if (target > std::numeric_limits<uint16_t>::max()) {
        throw CompileError("jump target overflows 16 bits", line);
    }
    chunk_->code[placeholderOffset]     = static_cast<uint8_t>(target & 0xFF);
    chunk_->code[placeholderOffset + 1] = static_cast<uint8_t>((target >> 8) & 0xFF);
}

size_t Compiler::currentOffset() const {
    return chunk_->code.size();
}

// --------------------------- symbol table ---------------------------

size_t Compiler::declareVariable(const std::string& name) {
    auto it = variableSlots_.find(name);
    if (it != variableSlots_.end()) {
        return it->second; // redeclaration just reuses the slot
    }
    size_t idx = chunk_->declareVariable(name);
    variableSlots_[name] = idx;
    return idx;
}

size_t Compiler::resolveVariable(const std::string& name, int line) {
    auto it = variableSlots_.find(name);
    if (it == variableSlots_.end()) {
        std::ostringstream os;
        os << "undeclared variable '" << name << "'";
        throw CompileError(os.str(), line);
    }
    return it->second;
}

// --------------------------- dispatch -------------------------------

void Compiler::compileExpression(const Expression& expr) { expr.accept(*this); }
void Compiler::compileStatement (const Statement&  stmt) { stmt.accept(*this); }

// ----------------------- expression visitors ------------------------

void Compiler::visit(const LiteralExpression& e) {
    Value v = (e.kind == LiteralKind::INT)
                ? Value::makeInt(e.intValue)
                : Value::makeBool(e.boolValue);
    size_t idx = chunk_->addConstant(v);
    emitOp(OpCode::LOAD_CONST);
    emitU16(idx, e.line);
}

void Compiler::visit(const VariableExpression& e) {
    size_t idx = resolveVariable(e.name, e.line);
    emitOp(OpCode::LOAD_VAR);
    emitU16(idx, e.line);
}

void Compiler::visit(const UnaryExpression& e) {
    compileExpression(*e.operand);
    switch (e.op) {
        case TokenType::MINUS: emitOp(OpCode::NEGATE); break;
        case TokenType::BANG:  emitOp(OpCode::NOT);    break;
        default:
            throw CompileError("unknown unary operator", e.line);
    }
}

void Compiler::visit(const BinaryExpression& e) {
    compileExpression(*e.left);
    compileExpression(*e.right);
    switch (e.op) {
        case TokenType::PLUS:  emitOp(OpCode::ADD);     break;
        case TokenType::MINUS: emitOp(OpCode::SUB);     break;
        case TokenType::STAR:  emitOp(OpCode::MUL);     break;
        case TokenType::SLASH: emitOp(OpCode::DIV);     break;
        case TokenType::EQ:    emitOp(OpCode::CMP_EQ);  break;
        case TokenType::NEQ:   emitOp(OpCode::CMP_NEQ); break;
        case TokenType::LT:    emitOp(OpCode::CMP_LT);  break;
        case TokenType::GT:    emitOp(OpCode::CMP_GT);  break;
        case TokenType::LE:    emitOp(OpCode::CMP_LE);  break;
        case TokenType::GE:    emitOp(OpCode::CMP_GE);  break;
        default:
            throw CompileError("unknown binary operator", e.line);
    }
}

void Compiler::visit(const AssignmentExpression& e) {
    size_t idx = resolveVariable(e.name, e.line);
    compileExpression(*e.value);
    // STORE_VAR pops the value, but assignment-as-expression should yield it.
    // Emit STORE then LOAD so the result remains on the stack.
    emitOp(OpCode::STORE_VAR);
    emitU16(idx, e.line);
    emitOp(OpCode::LOAD_VAR);
    emitU16(idx, e.line);
}

// ----------------------- statement visitors -------------------------

void Compiler::visit(const ExpressionStatement& s) {
    compileExpression(*s.expr);
    emitOp(OpCode::POP); // drop the produced value, statements don't keep it
}

void Compiler::visit(const PrintStatement& s) {
    compileExpression(*s.expr);
    emitOp(OpCode::PRINT);
}

void Compiler::visit(const InputStatement& s) {
    size_t idx = resolveVariable(s.variable, s.line);
    emitOp(OpCode::INPUT);
    emitU16(idx, s.line);
}

void Compiler::visit(const VariableDeclaration& s) {
    size_t idx = declareVariable(s.name);
    if (s.initializer) {
        compileExpression(*s.initializer);
    } else {
        size_t zero = chunk_->addConstant(Value::makeInt(0));
        emitOp(OpCode::LOAD_CONST);
        emitU16(zero, s.line);
    }
    emitOp(OpCode::STORE_VAR);
    emitU16(idx, s.line);
}

void Compiler::visit(const BlockStatement& s) {
    for (const auto& inner : s.statements) {
        compileStatement(*inner);
    }
}

void Compiler::visit(const IfStatement& s) {
    compileExpression(*s.condition);
    size_t jumpToElse = emitJump(OpCode::JUMP_IF_FALSE);

    compileStatement(*s.thenBranch);

    if (s.elseBranch) {
        size_t jumpOverElse = emitJump(OpCode::JUMP);
        patchJump(jumpToElse, s.line);
        compileStatement(*s.elseBranch);
        patchJump(jumpOverElse, s.line);
    } else {
        patchJump(jumpToElse, s.line);
    }
}

void Compiler::visit(const WhileStatement& s) {
    size_t loopStart = currentOffset();
    compileExpression(*s.condition);
    size_t exitJump = emitJump(OpCode::JUMP_IF_FALSE);

    compileStatement(*s.body);

    // Unconditional jump back to the start of the loop.
    emitOp(OpCode::JUMP);
    emitU16(loopStart, s.line);

    patchJump(exitJump, s.line);
}

} // namespace cvm
