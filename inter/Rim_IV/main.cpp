#include "lexer.h"
#include "parser.h"
#include "interpreter.h"
#include "debug.h"
#include <iostream>
#include <fstream>
#include <sstream>

int main(int argc, char* argv[]) {
    std::string src;
    std::string filename;

    bool debugMode = false;
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "-d" || arg == "--debug") {
            debugMode = true;
        } else if (arg[0] != '-') {
            filename = arg;
        }
    }
    if (filename.empty()) {
        std::cerr << "Usage: " << argv[0] << " [-d] <file>" << std::endl;
        std::cerr << "  -d, --debug  - enable debug mode (logs to file_name.log)" << std::endl;
        return 1;
    }

    // Read source code
    std::ifstream f(filename);
    if (!f) {
        std::cerr << "Error: could not open file " << filename << std::endl;
        return 1;
    }
    std::ostringstream ss;
    ss << f.rdbuf();
    src = ss.str();
    
    // Initialize debugger
    Debugger dbg;
    if (debugMode) {
        dbg.init(filename);
        std::cout << "Debug mode enabled. Log file: " << filename.substr(0, filename.find_last_of('.')) << ".log" << std::endl;
    }

    try {
        // Phase 1: lexical analysis
        Lexer lexer(src);
        std::vector<Token> tokens = lexer.tokenize();

        if (debugMode) dbg.logTokens(tokens);

        // Phase 2: syntax + semantic analysis + POLIZ generation
        SymTable sym;
        std::vector<PolizOp> code;
        Parser parser(tokens, sym, code);
        parser.parse();

        if (debugMode) {
            dbg.logPoliz(code);
            dbg.logSymbolTable(sym);
        }

        // Phase 3: POLIZ interpretation
        if (debugMode) dbg.logExecutionStart();

        Interpreter interp(code, sym, &dbg);
        interp.run();

        if (debugMode) dbg.logExecutionEnd();

    } catch (const std::exception& e) {
        if (debugMode) dbg.logError(e.what(), "Execution");
        std::cerr << std::endl << "[ERROR] " << e.what() << std::endl;
        return 1;
    }

    dbg.close();
    return 0;
}