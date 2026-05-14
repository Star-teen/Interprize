#include "lexer.h"
#include "parser.h"
#include "interpreter.h"
#include <iostream>
#include <fstream>
#include <sstream>

int main(int argc, char* argv[]) {
    std::string src;
    std::string filename;

    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <file>" << std::endl;
        return 1;
    }
    
    filename = argv[1];

    // Read source code
    std::ifstream f(filename);
    if (!f) {
        std::cerr << "Error: could not open file " << filename << std::endl;
        return 1;
    }
    std::ostringstream ss;
    ss << f.rdbuf();
    src = ss.str();

    try {
        // Phase 1: lexical analysis
        Lexer lexer(src);
        std::vector<Token> tokens = lexer.tokenize();

        // Phase 2: syntax + semantic analysis + POLIZ generation
        SymTable sym;
        std::vector<PolizOp> code;
        Parser parser(tokens, sym, code);
        parser.parse();

        // Phase 3: POLIZ interpretation
        Interpreter interp(code, sym);
        interp.run();

    } catch (const std::exception& e) {
        std::cerr << std::endl << "[ERROR] " << e.what() << std::endl;
        return 1;
    }

    return 0;
}