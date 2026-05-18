// vm.h — Stack-based virtual machine that executes a Chunk.
//
// The VM has three pieces of state: an operand stack, an indexed variable
// table parallel to Chunk::variableNames, and an instruction pointer (byte
// offset into Chunk::code). The execute() loop is a classic fetch-decode-
// execute switch over OpCode.
//
// I/O is parameterized so tests can drive the VM with std::stringstream
// instead of std::cin/std::cout.

#pragma once

#include "bytecode.h"

#include <cstdint>
#include <iosfwd>
#include <stdexcept>
#include <string>
#include <vector>

namespace cvm {

class RuntimeError : public std::runtime_error {
public:
    RuntimeError(const std::string& msg, size_t ip)
        : std::runtime_error(msg), ip_(ip) {}
    size_t ip() const { return ip_; }
private:
    size_t ip_;
};

class VM {
public:
    // Execute `chunk`. Writes program output to `out`, reads `input(x)` lines
    // from `in`. When `trace` is true, prints a one-line execution log to
    // `err` for every instruction. Returns the final stack size (should be 0
    // for well-formed programs).
    size_t execute(const Chunk& chunk,
                   std::ostream& out,
                   std::istream& in,
                   std::ostream& err,
                   bool trace = false);

private:
    std::vector<Value> stack_;
    std::vector<Value> variables_;
    size_t             ip_ = 0;

    void  push(const Value& v);
    Value pop();
    Value& peek();

    // Read a 16-bit little-endian operand starting at `ip_`, advancing ip_.
    uint16_t readU16(const Chunk& chunk);

    // Throws RuntimeError if the operand isn't an int / bool.
    int64_t requireInt(const Value& v);
    bool    requireBool(const Value& v);
};

} // namespace cvm
