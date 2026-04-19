// ============================================================
// main.cpp — точка входа интерпретатора
//
// Связывает все фазы:
//   Фаза 1: Lexer        → vector<Token>
//   Фаза 2: Parser       → vector<PolizOp> + SymTable (заполненная)
//   Фаза 3: Interpreter  → выполнение программы
//
// Сборка:  g++ -std=c++17 -Wall -o interp main.cpp
// Запуск:  ./interp program.ml
//          ./interp program.ml < input.txt
// ============================================================
#include "lexer.h"
#include "parser.h"
#include "interpreter.h"
#include <iostream>
#include <fstream>
#include <sstream>

int main(int argc, char* argv[]) {
    std::string src;

    if (argc >= 2) {
        // Читаем программу из файла переданного аргументом
        std::ifstream f(argv[1]);
        if (!f) {
            std::cerr << "Ошибка: не удалось открыть файл '"
                      << argv[1] << "'" << std::endl;
            return 1;
        }
        std::ostringstream ss; ss << f.rdbuf();
        src = ss.str();
    } else {
        // Читаем со стандартного ввода (удобно для тестов через echo или pipe)
        std::ostringstream ss; ss << std::cin.rdbuf();
        src = ss.str();
    }

    try {
        // Фаза 1: лексический анализ
        Lexer lexer(src);
        std::vector<Token> tokens = lexer.tokenize();

        // Фаза 2: синтаксический + семантический анализ + генерация ПОЛИЗ
        SymTable             sym;
        std::vector<PolizOp> code;
        Parser parser(tokens, sym, code);
        parser.parse();

        // Фаза 3: интерпретация ПОЛИЗ
        Interpreter interp(code, sym);
        interp.run();

    } catch (const std::exception& e) {
        std::cerr << std::endl << "[ОШИБКА] " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
