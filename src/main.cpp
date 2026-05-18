// main.cpp — Command-line entry point.
//
// Usage:
//   cvm                 - launch interactive REPL
//   cvm script.cvm      - execute a script file
//
// Flags (may appear anywhere before / mixed with the script path):
//   --tokens     dump the token stream to stderr before running
//   --ast        dump the parsed AST
//   --bytecode   disassemble the produced chunk
//   --trace      log each VM instruction as it executes
//   -h, --help   show this help

#include "repl.h"

#include <iostream>
#include <string>

namespace {

void printHelp(std::ostream& os, const char* exe) {
    os << "CVM++ - tiny scripting language with a stack-based VM\n"
       << "\n"
       << "Usage:\n"
       << "  " << exe << "                 launch interactive REPL\n"
       << "  " << exe << " script.cvm      run a script file\n"
       << "\n"
       << "Flags:\n"
       << "  --tokens     dump token stream\n"
       << "  --ast        dump parsed AST\n"
       << "  --bytecode   disassemble bytecode\n"
       << "  --trace      log each executed instruction\n"
       << "  -h, --help   show this message\n";
}

} // namespace

int main(int argc, char** argv) {
    cvm::DebugFlags flags;
    std::string scriptPath;

    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if      (a == "--tokens")   flags.tokens   = true;
        else if (a == "--ast")      flags.ast      = true;
        else if (a == "--bytecode") flags.bytecode = true;
        else if (a == "--trace")    flags.trace    = true;
        else if (a == "-h" || a == "--help") {
            printHelp(std::cout, argv[0]);
            return 0;
        } else if (!a.empty() && a[0] == '-') {
            std::cerr << "unknown flag: " << a << "\n";
            printHelp(std::cerr, argv[0]);
            return 2;
        } else {
            if (!scriptPath.empty()) {
                std::cerr << "error: multiple script paths given\n";
                return 2;
            }
            scriptPath = a;
        }
    }

    cvm::Repl repl(flags);
    if (!scriptPath.empty()) {
        return repl.runFile(scriptPath, std::cin, std::cout, std::cerr);
    }
    return repl.run(std::cin, std::cout, std::cerr);
}
