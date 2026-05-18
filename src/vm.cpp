// vm.cpp — Stack-based bytecode interpreter.

#include "vm.h"

#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

namespace cvm {

// ---------------------------- helpers -------------------------------

void VM::push(const Value& v) { stack_.push_back(v); }

Value VM::pop() {
    if (stack_.empty()) {
        throw RuntimeError("stack underflow", ip_);
    }
    Value v = stack_.back();
    stack_.pop_back();
    return v;
}

Value& VM::peek() {
    if (stack_.empty()) {
        throw RuntimeError("stack underflow on peek", ip_);
    }
    return stack_.back();
}

uint16_t VM::readU16(const Chunk& chunk) {
    if (ip_ + 1 >= chunk.code.size()) {
        throw RuntimeError("truncated 16-bit operand", ip_);
    }
    uint16_t lo = chunk.code[ip_];
    uint16_t hi = chunk.code[ip_ + 1];
    ip_ += 2;
    return static_cast<uint16_t>(lo | (hi << 8));
}

int64_t VM::requireInt(const Value& v) {
    if (v.type != Value::Type::INT) {
        throw RuntimeError("type error: expected integer", ip_);
    }
    return v.i;
}

bool VM::requireBool(const Value& v) {
    if (v.type != Value::Type::BOOL) {
        throw RuntimeError("type error: expected boolean", ip_);
    }
    return v.b;
}

// ------------------------- the big switch ---------------------------

size_t VM::execute(const Chunk& chunk,
                   std::ostream& out,
                   std::istream& in,
                   std::ostream& err,
                   bool trace) {
    stack_.clear();
    variables_.assign(chunk.variableNames.size(), Value::makeInt(0));
    ip_ = 0;

    auto traceLine = [&](size_t opAddr, OpCode op, const std::string& operandInfo) {
        if (!trace) return;
        err << "[trace] " << opAddr << ": " << opcodeName(op);
        if (!operandInfo.empty()) err << " " << operandInfo;
        err << "  stack=[";
        for (size_t i = 0; i < stack_.size(); ++i) {
            if (i) err << ", ";
            err << valueToString(stack_[i]);
        }
        err << "]\n";
    };

    while (ip_ < chunk.code.size()) {
        size_t opAddr = ip_;
        OpCode op = static_cast<OpCode>(chunk.code[ip_++]);

        switch (op) {
            case OpCode::HALT:
                traceLine(opAddr, op, "");
                return stack_.size();

            case OpCode::LOAD_CONST: {
                uint16_t idx = readU16(chunk);
                if (idx >= chunk.constants.size()) {
                    throw RuntimeError("constant index out of range", opAddr);
                }
                push(chunk.constants[idx]);
                traceLine(opAddr, op,
                          "#" + std::to_string(idx) + " (" +
                          valueToString(chunk.constants[idx]) + ")");
                break;
            }

            case OpCode::LOAD_VAR: {
                uint16_t idx = readU16(chunk);
                if (idx >= variables_.size()) {
                    throw RuntimeError("variable index out of range", opAddr);
                }
                push(variables_[idx]);
                traceLine(opAddr, op,
                          "@" + std::to_string(idx) + " (" +
                          chunk.variableNames[idx] + ")");
                break;
            }

            case OpCode::STORE_VAR: {
                uint16_t idx = readU16(chunk);
                if (idx >= variables_.size()) {
                    throw RuntimeError("variable index out of range", opAddr);
                }
                variables_[idx] = pop();
                traceLine(opAddr, op,
                          "@" + std::to_string(idx) + " (" +
                          chunk.variableNames[idx] + ")");
                break;
            }

            case OpCode::POP: {
                pop();
                traceLine(opAddr, op, "");
                break;
            }

            case OpCode::ADD: {
                Value b = pop(); Value a = pop();
                push(Value::makeInt(requireInt(a) + requireInt(b)));
                traceLine(opAddr, op, "");
                break;
            }
            case OpCode::SUB: {
                Value b = pop(); Value a = pop();
                push(Value::makeInt(requireInt(a) - requireInt(b)));
                traceLine(opAddr, op, "");
                break;
            }
            case OpCode::MUL: {
                Value b = pop(); Value a = pop();
                push(Value::makeInt(requireInt(a) * requireInt(b)));
                traceLine(opAddr, op, "");
                break;
            }
            case OpCode::DIV: {
                Value b = pop(); Value a = pop();
                int64_t bi = requireInt(b);
                if (bi == 0) throw RuntimeError("division by zero", opAddr);
                push(Value::makeInt(requireInt(a) / bi));
                traceLine(opAddr, op, "");
                break;
            }
            case OpCode::NEGATE: {
                Value a = pop();
                push(Value::makeInt(-requireInt(a)));
                traceLine(opAddr, op, "");
                break;
            }
            case OpCode::NOT: {
                Value a = pop();
                push(Value::makeBool(!requireBool(a)));
                traceLine(opAddr, op, "");
                break;
            }

            case OpCode::CMP_EQ: {
                Value b = pop(); Value a = pop();
                bool eq = false;
                if (a.type == b.type) {
                    eq = (a.type == Value::Type::INT) ? (a.i == b.i) : (a.b == b.b);
                }
                push(Value::makeBool(eq));
                traceLine(opAddr, op, "");
                break;
            }
            case OpCode::CMP_NEQ: {
                Value b = pop(); Value a = pop();
                bool eq = false;
                if (a.type == b.type) {
                    eq = (a.type == Value::Type::INT) ? (a.i == b.i) : (a.b == b.b);
                }
                push(Value::makeBool(!eq));
                traceLine(opAddr, op, "");
                break;
            }
            case OpCode::CMP_LT: {
                Value b = pop(); Value a = pop();
                push(Value::makeBool(requireInt(a) <  requireInt(b)));
                traceLine(opAddr, op, "");
                break;
            }
            case OpCode::CMP_GT: {
                Value b = pop(); Value a = pop();
                push(Value::makeBool(requireInt(a) >  requireInt(b)));
                traceLine(opAddr, op, "");
                break;
            }
            case OpCode::CMP_LE: {
                Value b = pop(); Value a = pop();
                push(Value::makeBool(requireInt(a) <= requireInt(b)));
                traceLine(opAddr, op, "");
                break;
            }
            case OpCode::CMP_GE: {
                Value b = pop(); Value a = pop();
                push(Value::makeBool(requireInt(a) >= requireInt(b)));
                traceLine(opAddr, op, "");
                break;
            }

            case OpCode::JUMP: {
                uint16_t target = readU16(chunk);
                if (target > chunk.code.size()) {
                    throw RuntimeError("jump target out of bounds", opAddr);
                }
                ip_ = target;
                traceLine(opAddr, op, "-> " + std::to_string(target));
                break;
            }
            case OpCode::JUMP_IF_FALSE: {
                uint16_t target = readU16(chunk);
                Value cond = pop();
                bool take = !requireBool(cond);
                if (take) {
                    if (target > chunk.code.size()) {
                        throw RuntimeError("jump target out of bounds", opAddr);
                    }
                    ip_ = target;
                }
                traceLine(opAddr, op,
                          "-> " + std::to_string(target) +
                          (take ? " (taken)" : " (skipped)"));
                break;
            }

            case OpCode::PRINT: {
                Value v = pop();
                out << valueToString(v) << "\n";
                traceLine(opAddr, op, "");
                break;
            }

            case OpCode::INPUT: {
                uint16_t idx = readU16(chunk);
                if (idx >= variables_.size()) {
                    throw RuntimeError("variable index out of range", opAddr);
                }
                std::string line;
                if (!std::getline(in, line)) {
                    throw RuntimeError("input stream closed before INPUT", opAddr);
                }
                try {
                    size_t parsedTo = 0;
                    long long val = std::stoll(line, &parsedTo);
                    // Allow trailing whitespace but nothing else useful.
                    while (parsedTo < line.size() &&
                           std::isspace(static_cast<unsigned char>(line[parsedTo]))) {
                        ++parsedTo;
                    }
                    if (parsedTo != line.size()) {
                        throw std::invalid_argument("trailing chars");
                    }
                    variables_[idx] = Value::makeInt(static_cast<int64_t>(val));
                } catch (const std::exception&) {
                    throw RuntimeError("invalid integer input: '" + line + "'", opAddr);
                }
                traceLine(opAddr, op,
                          "@" + std::to_string(idx) + " (" +
                          chunk.variableNames[idx] + ")");
                break;
            }

            default:
                throw RuntimeError(
                    "unknown opcode " +
                        std::to_string(static_cast<int>(op)),
                    opAddr);
        }
    }

    // Fell off the end without HALT — treat as implicit halt.
    return stack_.size();
}

} // namespace cvm
