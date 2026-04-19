// main.cpp
#include "lexer.h"
#include "parser.h"
#include "interpreter.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

using namespace std;

//============================================================================
// Структура для хранения результатов тестирования
//============================================================================

struct TestResult {
    string name;
    bool success;
    string error;
    int steps;
    double duration;
};

//============================================================================
// Вспомогательные функции
//============================================================================

string readFile(const string& filename) {
    ifstream file(filename);
    if (!file.is_open()) {
        cerr << "Не удалось открыть файл: " << filename << endl;
        return "";
    }
    stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

void writeFile(const string& filename, const string& content) {
    ofstream file(filename);
    if (!file.is_open()) {
        cerr << "Не удалось записать файл: " << filename << endl;
        return;
    }
    file << content;
}

void printHelp() {
    cout << "=== Интерпретатор модельного языка ===" << endl;
    cout << "Использование:" << endl;
    cout << "  interpreter <file>            - выполнить программу из файла" << endl;
    cout << "  interpreter -d <file>         - выполнить в режиме отладки" << endl;
    cout << "  interpreter -h                - показать эту справку" << endl;
    cout << "  interpreter --test            - запустить встроенные тесты" << endl;
    cout << endl;
    cout << "Вариант: I.1 (if без else), II.3 (for step until), III.1 (goto)," << endl;
    cout << "        IV.2 (real), V.1 (унарный минус), VI.2 (полное вычисление)" << endl;
}

//============================================================================
// Встроенные тесты
//============================================================================

bool runTest(const string& name, const string& source, const string& expectedOutput) {
    cout << "  Тест: " << name << " ... ";
    cout.flush();
    
    Parser parser(source);
    
    if (!parser.parse()) {
        cout << "ОШИБКА СИНТАКСИСА" << endl;
        for (const auto& err : parser.getErrors()) {
            cout << "    " << err << endl;
        }
        return false;
    }
    
    // Перенаправляем вывод для перехвата
    stringstream output;
    auto oldCout = cout.rdbuf(output.rdbuf());
    
    Interpreter interpreter(parser.getPoliz(), parser.getSymbolTable());
    
    bool success = true;
    try {
        interpreter.run();
    } catch (const exception& e) {
        success = false;
        output << "[ОШИБКА: " << e.what() << "]";
    }
    
    cout.rdbuf(oldCout);
    string actualOutput = output.str();
    
    // Сравниваем с ожидаемым (упрощённо: ищем подстроку)
    if (success && actualOutput.find(expectedOutput) != string::npos) {
        cout << "OK" << endl;
        return true;
    } else {
        cout << "НЕ УСПЕШНО" << endl;
        cout << "  Ожидалось: \"" << expectedOutput << "\"" << endl;
        cout << "  Получено:  \"" << actualOutput << "\"" << endl;
        return false;
    }
}

void runAllTests() {
    cout << "\n=== Запуск встроенных тестов ===\n" << endl;
    
    int passed = 0;
    int total = 0;
    
    // А-тест 1: Целочисленная арифметика
    total++;
    if (runTest("A1: Целочисленная арифметика",
        "program\n"
        "    int a = 5, b = 10, c;\n"
        "    if (a < b) c = a + b;\n"
        "    write(c);\n",
        "15")) passed++;
    
    // А-тест 2: Строковая конкатенация
    total++;
    if (runTest("A2: Строковая конкатенация",
        "program\n"
        "    string s1 = \"Hello\", s2 = \"World\", s3;\n"
        "    s3 = s1 + \" \" + s2;\n"
        "    write(s3);\n",
        "Hello World")) passed++;
    
    // А-тест 3: Вещественный тип и унарный минус
    total++;
    if (runTest("A3: Вещественный тип",
        "program\n"
        "    real x = 3.5, y;\n"
        "    y = -x * 2.0;\n"
        "    write(y);\n",
        "-7")) passed++;
    
    // А-тест 4: Цикл for step/until
    total++;
    if (runTest("A4: Цикл for step/until",
        "program\n"
        "    int i, sum = 0;\n"
        "    for i = 1 step 2 until 10 do\n"
        "        sum = sum + i;\n"
        "    write(sum);\n",
        "25")) passed++;
    
    // А-тест 5: goto и метка
    total++;
    if (runTest("A5: goto и метка",
        "program\n"
        "    int x = 0;\n"
        "    start:\n"
        "        x = x + 1;\n"
        "        if (x < 3) goto start;\n"
        "    write(x);\n",
        "3")) passed++;
    
    // А-тест 6: Смешанные int/real операции
    total++;
    if (runTest("A6: Смешанные int/real",
        "program\n"
        "    int a = 5;\n"
        "    real b = 2.0;\n"
        "    real c;\n"
        "    c = a / b;\n"
        "    write(c);\n",
        "2.5")) passed++;
    
    // В-тест 1: Необъявленная переменная (должна выдать ошибку)
    total++;
    cout << "  Тест: B1: Необъявленная переменная ... ";
    {
        string source = "program\n    int x = 5;\n    y = x + 1;\n    write(x);\n";
        Parser parser(source);
        if (!parser.parse()) {
            bool found = false;
            for (const auto& err : parser.getErrors()) {
                if (err.find("не объявлена") != string::npos) {
                    found = true;
                    break;
                }
            }
            if (found) {
                cout << "OK (ошибка обнаружена)" << endl;
                passed++;
            } else {
                cout << "НЕ УСПЕШНО (ошибка не найдена)" << endl;
            }
        } else {
            cout << "НЕ УСПЕШНО (синтаксический анализ успешен, а должен был выдать ошибку)" << endl;
        }
    }
    
    // В-тест 2: Несоответствие типов при инициализации
    total++;
    cout << "  Тест: B2: Несоответствие типов ... ";
    {
        string source = "program\n    int a = \"hello\";\n    write(a);\n";
        Parser parser(source);
        if (!parser.parse()) {
            bool found = false;
            for (const auto& err : parser.getErrors()) {
                if (err.find("типов") != string::npos) {
                    found = true;
                    break;
                }
            }
            if (found) {
                cout << "OK (ошибка обнаружена)" << endl;
                passed++;
            } else {
                cout << "НЕ УСПЕШНО (ошибка не найдена)" << endl;
            }
        } else {
            cout << "НЕ УСПЕШНО (синтаксический анализ успешен)" << endl;
        }
    }
    
    cout << "\n=== Результаты тестов ===" << endl;
    cout << "Пройдено: " << passed << " из " << total << endl;
    if (passed == total) {
        cout << "Все тесты успешно пройдены!" << endl;
    } else {
        cout << "Некоторые тесты не пройдены." << endl;
    }
}

//============================================================================
// Основная функция
//============================================================================

int main(int argc, char* argv[]) {
    // Разбор аргументов командной строки
    bool debugMode = false;
    bool runTests = false;
    string filename;
    
    for (int i = 1; i < argc; i++) {
        string arg = argv[i];
        if (arg == "-h" || arg == "--help") {
            printHelp();
            return 0;
        } else if (arg == "-d" || arg == "--debug") {
            debugMode = true;
        } else if (arg == "--test") {
            runTests = true;
        } else if (arg[0] != '-') {
            filename = arg;
        }
    }
    
    if (runTests) {
        runAllTests();
        return 0;
    }
    
    if (filename.empty()) {
        cerr << "Ошибка: не указан файл с программой." << endl;
        printHelp();
        return 1;
    }
    
    // Чтение исходного кода
    string source = readFile(filename);
    if (source.empty()) {
        return 1;
    }
    
    cout << "=== Исходная программа ===" << endl;
    cout << source << endl;
    
    // Парсинг и генерация ПОЛИЗа
    cout << "\n=== Синтаксический анализ ===" << endl;
    Parser parser(source);
    
    if (!parser.parse()) {
        cout << "\n=== Обнаружены ошибки ===" << endl;
        for (const auto& err : parser.getErrors()) {
            cout << err << endl;
        }
        return 1;
    }
    
    cout << "Синтаксический анализ успешен!" << endl;
    
    // Вывод ПОЛИЗа
    parser.getPoliz().print();
    
    // Вывод таблицы символов
    parser.getSymbolTable().print();
    
    // Интерпретация
    cout << "\n=== Выполнение программы ===" << endl;
    Interpreter interpreter(parser.getPoliz(), parser.getSymbolTable());
    
    if (debugMode) {
        interpreter.setDebugMode(true);
        interpreter.setMaxSteps(500);
    }
    
    try {
        interpreter.run();
        cout << "\n\n=== Выполнение завершено ===" << endl;
    } catch (const exception& e) {
        cerr << "\nОшибка выполнения: " << e.what() << endl;
        return 1;
    }
    
    return 0;
}