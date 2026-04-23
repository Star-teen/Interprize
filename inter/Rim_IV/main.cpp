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
        std::cerr << "Использование: " << argv[0] << " [-d] <файл>" << std::endl;
        std::cerr << "  -d, --debug  - включить отладку (лог в файл имя_файла.log)" << std::endl;
        return 1;
    }

    // Чтение исходного кода
    std::ifstream f(filename);
    if (!f) {
        std::cerr << "Ошибка: не удалось открыть файл " << filename << std::endl;
        return 1;
    }
    std::ostringstream ss;
    ss << f.rdbuf();
    src = ss.str();
    
    // Инициализация отладчика
    Debugger dbg;
    if (debugMode) {
        dbg.init(filename);
        std::cout << "Отладка включена. Лог-файл: " << filename.substr(0, filename.find_last_of('.')) << ".log" << std::endl;
    }

    try {
        // Фаза 1: лексический анализ
        Lexer lexer(src);
        std::vector<Token> tokens = lexer.tokenize();

        if (debugMode) dbg.logTokens(tokens);

        // Фаза 2: синтаксический + семантический анализ + генерация ПОЛИЗ
        SymTable sym;
        std::vector<PolizOp> code;
        Parser parser(tokens, sym, code);
        parser.parse();

        if (debugMode) {
            dbg.logPoliz(code);
            dbg.logSymbolTable(sym);
        }

        // Фаза 3: интерпретация ПОЛИЗ
        if (debugMode) dbg.logExecutionStart();

        Interpreter interp(code, sym, &dbg);
        interp.run();

        if (debugMode) dbg.logExecutionEnd();

    } catch (const std::exception& e) {
        if (debugMode) dbg.logError(e.what(), "Выполнение");
        std::cerr << std::endl << "[ОШИБКА] " << e.what() << std::endl;
        return 1;
    }

    dbg.close();
    return 0;
}
