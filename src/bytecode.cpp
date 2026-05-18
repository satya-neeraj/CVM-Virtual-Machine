// bytecode.cpp — Helpers for Value, opcode names, and the disassembler.

#include "bytecode.h"

#include <iomanip>

namespace cvm {

std::string valueToString(const Value& v) {
    switch (v.type) {
        case Value::Type::INT:  return std::to_string(v.i);
        case Value::Type::BOOL: return v.b ? "true" : "false";
    }
    return "?";
}

std::string opcodeName(OpCode op) {
    switch (op) {
        case OpCode::HALT:          return "HALT";
        case OpCode::LOAD_CONST:    return "LOAD_CONST";
        case OpCode::LOAD_VAR:      return "LOAD_VAR";
        case OpCode::STORE_VAR:     return "STORE_VAR";
        case OpCode::POP:           return "POP";
        case OpCode::ADD:           return "ADD";
        case OpCode::SUB:           return "SUB";
        case OpCode::MUL:           return "MUL";
        case OpCode::DIV:           return "DIV";
        case OpCode::NEGATE:        return "NEGATE";
        case OpCode::NOT:           return "NOT";
        case OpCode::CMP_EQ:        return "CMP_EQ";
        case OpCode::CMP_NEQ:       return "CMP_NEQ";
        case OpCode::CMP_LT:        return "CMP_LT";
        case OpCode::CMP_GT:        return "CMP_GT";
        case OpCode::CMP_LE:        return "CMP_LE";
        case OpCode::CMP_GE:        return "CMP_GE";
        case OpCode::JUMP:          return "JUMP";
        case OpCode::JUMP_IF_FALSE: return "JUMP_IF_FALSE";
        case OpCode::PRINT:         return "PRINT";
        case OpCode::INPUT:         return "INPUT";
    }
    return "UNKNOWN";
}

size_t Chunk::addConstant(const Value& v) {
    constants.push_back(v);
    return constants.size() - 1;
}

size_t Chunk::declareVariable(const std::string& name) {
    variableNames.push_back(name);
    return variableNames.size() - 1;
}

static uint16_t readU16(const std::vector<uint8_t>& code, size_t pos) {
    return static_cast<uint16_t>(code[pos])
         | (static_cast<uint16_t>(code[pos + 1]) << 8);
}

void disassembleChunk(const Chunk& chunk, std::ostream& os) {
    os << "== Bytecode (" << chunk.code.size() << " bytes) ==\n";
    os << "Constants:\n";
    for (size_t i = 0; i < chunk.constants.size(); ++i) {
        os << "  [" << i << "] " << valueToString(chunk.constants[i]) << "\n";
    }
    os << "Variables:\n";
    for (size_t i = 0; i < chunk.variableNames.size(); ++i) {
        os << "  [" << i << "] " << chunk.variableNames[i] << "\n";
    }
    os << "Code:\n";

    size_t i = 0;
    while (i < chunk.code.size()) {
        os << "  " << std::setw(4) << std::setfill('0') << i << "  ";
        OpCode op = static_cast<OpCode>(chunk.code[i]);
        os << opcodeName(op);
        switch (op) {
            case OpCode::LOAD_CONST:
            case OpCode::LOAD_VAR:
            case OpCode::STORE_VAR:
            case OpCode::INPUT:
            case OpCode::JUMP:
            case OpCode::JUMP_IF_FALSE: {
                uint16_t arg = readU16(chunk.code, i + 1);
                os << " " << arg;
                if (op == OpCode::LOAD_CONST && arg < chunk.constants.size()) {
                    os << "  ; " << valueToString(chunk.constants[arg]);
                } else if ((op == OpCode::LOAD_VAR ||
                            op == OpCode::STORE_VAR ||
                            op == OpCode::INPUT) &&
                           arg < chunk.variableNames.size()) {
                    os << "  ; " << chunk.variableNames[arg];
                }
                i += 3;
                break;
            }
            default:
                i += 1;
                break;
        }
        os << "\n";
    }
    os << "===========================\n";
}

} // namespace cvm
