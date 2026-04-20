// poliz.h
#ifndef POLIZ_H
#define POLIZ_H

#include "token.h"
#include "symbol_table.h"
#include <vector>
#include <map>
#include <string>
#include <iostream>

//============================================================================
// Команды ПОЛИЗа (внутреннего языка)
//============================================================================

enum class PolizCmd {
    // ----- Константы и переменные -----
    PUSH_INT,       // положить целую константу на стек
    PUSH_REAL,      // положить вещественную константу на стек
    PUSH_STRING,    // положить строковую константу на стек
    PUSH_VAR,       // положить значение переменной на стек (по адресу)
    POP_VAR,        // снять значение со стека и записать в переменную
    
    // ----- Арифметические операции -----
    ADD,            // сложение: a b -> a+b
    SUB,            // вычитание: a b -> a-b
    MUL,            // умножение: a b -> a*b
    DIV,            // деление: a b -> a/b
    NEG,            // унарный минус: a -> -a
    
    // ----- Логические операции (полное вычисление) -----
    AND,            // логическое И: a b -> a && b
    OR,             // логическое ИЛИ: a b -> a || b
    NOT,            // логическое НЕ: a -> !a
    
    // ----- Операции отношения -----
    LESS,           // <
    GREATER,        // >
    LE,             // <=
    GE,             // >=
    EQ,             // ==
    NE,             // !=
    
    // ----- Управление потоком -----
    LABEL,          // метка (не исполняется, нужна для адресации)
    GOTO,           // безусловный переход
    GOTO_IF_FALSE,  // переход, если на вершине стека false (0)
    GOTO_IF_TRUE,   // переход, если на вершине стека true (≠0)
    
    // ----- Ввод/вывод -----
    READ,           // чтение значения в переменную
    WRITE,          // вывод значения со стека
    
    // ----- Прочие -----
    HALT,           // останов программы
    NOP             // нет операции (заглушка)
};

// Преобразование PolizCmd в строку (для отладки)
std::string polizCmdToString(PolizCmd cmd);

//============================================================================
// Инструкция ПОЛИЗа
//============================================================================

struct PolizInstruction {
    PolizCmd cmd;           // команда
    int intValue;           // для PUSH_INT, GOTO, и т.д.
    double realValue;       // для PUSH_REAL
    std::string strValue;   // для PUSH_STRING, LABEL
    int address;            // для переменных (PUSH_VAR, POP_VAR)
    
    // Конструкторы
    PolizInstruction();
    explicit PolizInstruction(PolizCmd c);
    PolizInstruction(PolizCmd c, int v);
    PolizInstruction(PolizCmd c, double v);
    PolizInstruction(PolizCmd c, const std::string& v);
    PolizInstruction(PolizCmd c, const std::string& v, int addr);
    
    // Проверка, является ли инструкция меткой
    bool isLabel() const { return cmd == PolizCmd::LABEL; }
    
    // Получение строкового представления
    std::string toString() const;
    
    // Вывод
    void print() const;
};

//============================================================================
// Класс ПОЛИЗ — внутреннее представление программы
//============================================================================

class Poliz {
private:
    std::vector<PolizInstruction> code;     // массив инструкций
    std::map<std::string, int> labelMap;    // метка -> индекс (после линковки)
    std::vector<int> reservedSpots;         // зарезервированные места для патчинга
    
public:
    //========================================================================
    // Конструкторы
    //========================================================================
    
    Poliz();
    explicit Poliz(size_t reservedSize);
    ~Poliz() = default;
    
    //========================================================================
    // Добавление инструкций
    //========================================================================
    
    // Добавление в конец
    void emit(PolizCmd cmd);
    void emit(PolizCmd cmd, int v);
    void emit(PolizCmd cmd, double v);
    void emit(PolizCmd cmd, const std::string& v);
    void emit(PolizCmd cmd, const std::string& v, int addr);
    
    // Добавление лексемы (преобразует Token в PolizInstruction)
    void emit(const Token& token);
    
    //========================================================================
    // Работа с метками
    //========================================================================
    
    // Объявление метки в текущей позиции
    void label(const std::string& name);
    
    // Получение адреса метки (после линковки)
    int getLabelAddress(const std::string& name) const;
    
    // Резервирование места для будущей метки (возвращает индекс)
    int reserve();
    
    // Заполнение зарезервированного места адресом
    void patch(int position, int target);
    
    // Заполнение зарезервированного места адресом метки
    void patch(int position, const std::string& labelName);
    
    //========================================================================
    // Линковка (связывание меток с адресами)
    //========================================================================
    
    // Первый проход: сбор всех меток
    void collectLabels();
    
    // Второй проход: замена ссылок на метки адресами
    void linkLabels();
    
    // Полная линковка (collectLabels + linkLabels)
    void link();
    
    //========================================================================
    // Доступ к инструкциям
    //========================================================================
    
    size_t size() const { return code.size(); }
    bool empty() const { return code.empty(); }
    
    const PolizInstruction& operator[](size_t index) const;
    PolizInstruction& operator[](size_t index);
    
    // Получение последней инструкции
    PolizInstruction& back();
    const PolizInstruction& back() const;
    
    //========================================================================
    // Очистка
    //========================================================================
    
    void clear();
    
    //========================================================================
    // Вывод и отладка
    //========================================================================
    
    void print() const;
    void printDisassembly() const;
    
    //========================================================================
    // Сохранение и загрузка (для многократной интерпретации)
    //========================================================================
    
    std::vector<char> serialize() const;
    void deserialize(const std::vector<char>& data);
};

#endif // POLIZ_H