// repl.cpp — Glue layer between Lexer / Parser / Compiler / VM.

#include "repl.h"

#include "ast.h"
#include "bytecode.h"
#include "compiler.h"
#include "lexer.h"
#include "parser.h"
#include "token.h"
#include "vm.h"

#include <cctype>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

namespace cvm {

// Run a full source string through the pipeline.
bool Repl::runSource(const std::string& source,
                     std::istream& in,
                     std::ostream& out,
                     std::ostream& err) {
    try {
        Lexer lexer(source);
        auto tokens = lexer.tokenize();

        if (flags_.tokens) {
            err << "--- tokens ---\n";
            for (const auto& tok : tokens) {
                err << "  " << tokenTypeName(tok.type)
                    << " \"" << tok.lexeme << "\""
                    << " (line " << tok.line << ", col " << tok.column << ")\n";
            }
        }

        Parser parser(std::move(tokens));
        auto program = parser.parseProgram();

        if (flags_.ast) {
            err << "--- ast ---\n";
            err << programToString(program);
        }

        Compiler compiler;
        Chunk chunk = compiler.compile(program);

        if (flags_.bytecode) {
            err << "--- bytecode ---\n";
            disassembleChunk(chunk, err);
        }

        if (flags_.tokens || flags_.ast || flags_.bytecode || flags_.trace) {
            err << "--- output ---\n";
        }

        VM vm;
        vm.execute(chunk, out, in, err, flags_.trace);
        return true;
    } catch (const ParseError& e) {
        err << "parse error (line " << e.line
            << ", col " << e.column << "): " << e.what() << "\n";
        return false;
    } catch (const CompileError& e) {
        err << "compile error (line " << e.line() << "): " << e.what() << "\n";
        return false;
    } catch (const RuntimeError& e) {
        err << "runtime error (ip " << e.ip() << "): " << e.what() << "\n";
        return false;
    } catch (const std::exception& e) {
        err << "error: " << e.what() << "\n";
        return false;
    }
}

int Repl::runFile(const std::string& path,
                  std::istream& in,
                  std::ostream& out,
                  std::ostream& err) {
    std::ifstream f(path);
    if (!f) {
        err << "error: cannot open file '" << path << "'\n";
        return 1;
    }
    std::ostringstream ss;
    ss << f.rdbuf();
    return runSource(ss.str(), in, out, err) ? 0 : 1;
}

// Heuristic check: does this buffer look like a complete statement?
// We require all braces matched and (the last non-whitespace char is '}' or ';').
static bool isComplete(const std::string& buf) {
    int depth = 0;
    char lastSig = '\0';
    for (size_t i = 0; i < buf.size(); ++i) {
        char c = buf[i];
        if (c == '{') ++depth;
        else if (c == '}') --depth;
        if (!std::isspace(static_cast<unsigned char>(c))) lastSig = c;
    }
    if (depth != 0) return false;
    if (lastSig == '\0') return false;
    return lastSig == ';' || lastSig == '}';
}

int Repl::run(std::istream& in, std::ostream& out, std::ostream& err) {
    out << "CVM++ interactive shell. Type 'exit' to quit, 'run <file>' to load a script.\n";

    std::string buffer;
    while (true) {
        if (buffer.empty()) out << ">>> ";
        else                out << "... ";
        out.flush();

        std::string line;
        if (!std::getline(in, line)) {
            out << "\n";
            return 0;
        }

        std::string trimmed = line;
        while (!trimmed.empty() &&
               std::isspace(static_cast<unsigned char>(trimmed.back()))) {
            trimmed.pop_back();
        }
        size_t start = 0;
        while (start < trimmed.size() &&
               std::isspace(static_cast<unsigned char>(trimmed[start]))) {
            ++start;
        }
        trimmed = trimmed.substr(start);

        // Top-level meta commands only when no partial input is buffered.
        if (buffer.empty()) {
            if (trimmed == "exit" || trimmed == "quit") {
                return 0;
            }
            if (trimmed.rfind("run ", 0) == 0) {
                std::string path = trimmed.substr(4);
                runFile(path, in, out, err);
                continue;
            }
            if (trimmed.empty()) continue;
        }

        if (!buffer.empty()) buffer += "\n";
        buffer += line;

        if (isComplete(buffer)) {
            runSource(buffer, in, out, err);
            buffer.clear();
        }
    }
}

} // namespace cvm
