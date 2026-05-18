// repl.h — Interactive shell and the pipeline orchestrator.
//
// The Repl class owns the end-to-end pipeline: source text → Lexer → Parser →
// Compiler → VM. Debug flags control which intermediate representations get
// dumped before execution.
//
// Interactive mode uses brace + semicolon balancing so multi-line if/while
// blocks can be typed at the prompt. Each accepted input is compiled and run
// as an independent program — there is currently no cross-input state in the
// REPL (a future extension; see README).

#pragma once

#include <iosfwd>
#include <string>

namespace cvm {

struct DebugFlags {
    bool tokens   = false;
    bool ast      = false;
    bool bytecode = false;
    bool trace    = false;
};

class Repl {
public:
    explicit Repl(const DebugFlags& flags) : flags_(flags) {}

    // Interactive prompt loop. Returns the exit code.
    int run(std::istream& in, std::ostream& out, std::ostream& err);

    // Run a source string once. Returns true on success.
    bool runSource(const std::string& source,
                   std::istream& in,
                   std::ostream& out,
                   std::ostream& err);

    // Read a file from disk and run it. Returns the exit code.
    int runFile(const std::string& path,
                std::istream& in,
                std::ostream& out,
                std::ostream& err);

private:
    DebugFlags flags_;
};

} // namespace cvm
