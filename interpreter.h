// interpreter.h
#ifndef INTERPRETER_H
#define INTERPRETER_H

#include "poliz.h"
#include "symbol_table.h"
#include <vector>
#include <stack>
#include <iostream>
#include <functional>
#include <map>

//============================================================================
// Значение на стеке виртуальной машины (вариантный тип)
//============================================================================

class StackValue {
public:
    DataType type;
    int intValue;
    double realValue;
    std::string stringValue;
    
    // Конструкторы
    StackValue();
    StackValue(int v);
    StackValue(double v);
    StackValue(const std::string& v);
    
    // Копирование
    StackValue(const StackValue& other);
    StackValue& operator=(const StackValue& other);
    
    // Преобразования
    bool toBool() const;
    std::string toString() const;
    
    // Вывод
    void print() const;
};

//============================================================================
// Интерпретатор ПОЛИЗа (виртуальная машина)
//============================================================================

class Interpreter {
private:
    //-----------------------------------------------------------------------
    // Состояние виртуальной машины
    //-----------------------------------------------------------------------
    
    const Poliz& poliz;             // выполняемая программа
    SymbolTable& symTable;          // таблица символов (для переменных)
    
    std::vector<StackValue> stack;  // стек значений
    size_t ip;                       // instruction pointer (счётчик команд)
    bool running;                    // флаг выполнения
    
    //-----------------------------------------------------------------------
    // Отладка
    //-----------------------------------------------------------------------
    
    bool debugMode;                  // режим отладки
    int maxSteps;                    // максимальное количество шагов (для защиты от зацикливания)
    int currentStep;                 // текущий шаг
    
    //-----------------------------------------------------------------------
    // Приватные методы
    //-----------------------------------------------------------------------
    
    // Работа со стеком
    void push(const StackValue& v);
    StackValue pop();
    StackValue peek() const;
    size_t stackSize() const { return stack.size(); }
    void clearStack();
    
    // Выполнение одной инструкции
    void executeInstruction(const PolizInstruction& instr);
    
    // Бинарные и унарные операции
    StackValue applyBinaryOp(PolizCmd op, const StackValue& left, const StackValue& right);
    StackValue applyUnaryOp(PolizCmd op, const StackValue& operand);
    
    // Проверка типов для операций (Таблица №2)
    bool isBinaryOpCompatible(PolizCmd op, DataType left, DataType right);
    DataType getBinaryOpResultType(PolizCmd op, DataType left, DataType right);
    
    // Отладочный вывод
    void printState() const;
    
public:
    //-----------------------------------------------------------------------
    // Конструкторы
    //-----------------------------------------------------------------------
    
    Interpreter(const Poliz& p, SymbolTable& st);
    ~Interpreter() = default;
    
    //-----------------------------------------------------------------------
    // Управление выполнением
    //-----------------------------------------------------------------------
    
    // Запуск интерпретации
    void run(std::istream& input = std::cin);
    
    // Выполнение одного шага (для отладки)
    bool step();
    
    // Сброс состояния (без очистки программы)
    void reset();
    
    // Останов
    void stop() { running = false; }
    
    //-----------------------------------------------------------------------
    // Настройка
    //-----------------------------------------------------------------------
    
    void setDebugMode(bool enable) { debugMode = enable; }
    void setMaxSteps(int steps) { maxSteps = steps; }
    
    //-----------------------------------------------------------------------
    // Получение состояния
    //-----------------------------------------------------------------------
    
    bool isRunning() const { return running; }
    size_t getIP() const { return ip; }
    size_t getStackSize() const { return stack.size(); }
    
    //-----------------------------------------------------------------------
    // Ввод/вывод (можно переопределить)
    //-----------------------------------------------------------------------
    
    std::function<void(const std::string&)> outputCallback;
    std::function<std::string()> inputCallback;
    
    // Установка стандартных колбэков
    void setStandardIO();
};

#endif // INTERPRETER_H