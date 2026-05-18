// bytecode.h — Bytecode instruction set and the Chunk container.
//
// Instruction encoding:
//   Every opcode is 1 byte. Some opcodes are followed by a 16-bit
//   little-endian operand (constant index, variable index, or jump target).
//   Jumps are absolute byte offsets into Chunk::code.

#pragma once

#include <cstdint>
#include <ostream>
#include <string>
#include <vector>

namespace cvm {

enum class OpCode : uint8_t {
    HALT = 0,        //                       stop execution
    LOAD_CONST,      // u16 const_index     stack += constants[idx]
    LOAD_VAR,        // u16 var_index       stack += variables[idx]
    STORE_VAR,       // u16 var_index       variables[idx] = pop()
    POP,             //                       pop()
    ADD,             //                       a, b -> a + b  (int)
    SUB,             //                       a, b -> a - b  (int)
    MUL,             //                       a, b -> a * b  (int)
    DIV,             //                       a, b -> a / b  (int, integer division)
    NEGATE,          //                       a -> -a        (int)
    NOT,             //                       a -> !a        (bool)
    CMP_EQ,          //                       a, b -> bool
    CMP_NEQ,         //                       a, b -> bool
    CMP_LT,          //                       a, b -> bool
    CMP_GT,          //                       a, b -> bool
    CMP_LE,          //                       a, b -> bool
    CMP_GE,          //                       a, b -> bool
    JUMP,            // u16 target          ip = target
    JUMP_IF_FALSE,   // u16 target          if !pop() ip = target
    PRINT,           //                       prints pop() to stdout
    INPUT            // u16 var_index       reads stdin line, stores int in vars[idx]
};

// Tagged value type used on the VM stack and in variable storage.
struct Value {
    enum class Type { INT, BOOL } type = Type::INT;
    int64_t i = 0;
    bool    b = false;

    static Value makeInt(int64_t v)  { Value x; x.type = Type::INT;  x.i = v; return x; }
    static Value makeBool(bool v)    { Value x; x.type = Type::BOOL; x.b = v; return x; }
};

std::string valueToString(const Value& v);
std::string opcodeName(OpCode op);

// A Chunk is the compiled artifact: byte-coded instructions + constants pool +
// table of declared variable names (indexed). The VM executes one Chunk.
struct Chunk {
    std::vector<uint8_t>     code;
    std::vector<Value>       constants;
    std::vector<std::string> variableNames;

    size_t addConstant(const Value& v);
    size_t declareVariable(const std::string& name);
};

// Print a human-readable disassembly of a Chunk (used by --bytecode flag).
void disassembleChunk(const Chunk& chunk, std::ostream& os);

} // namespace cvm
