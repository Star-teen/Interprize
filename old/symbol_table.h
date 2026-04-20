// symbol_table.h
#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <variant>
#include <iostream>

//============================================================================
// Типы данных, поддерживаемые языком
// Согласно варианту IV.2: int, real, string
// (boolean отсутствует)
//============================================================================

enum class DataType {
    INT,        // целочисленный тип
    REAL,       // вещественный тип (вариант IV.2)
    STRING,     // строковый тип
    UNDEFINED   // тип не определён (для ошибок)
};

// Преобразование DataType в строку (для отладки)
std::string dataTypeToString(DataType type);

//============================================================================
// Запись в таблице символов для переменной
//============================================================================

struct VariableSymbol {
    std::string name;       // имя переменной
    DataType type;          // тип переменной
    bool declared;          // объявлена ли (всегда true для записей в таблице)
    bool initialized;       // инициализирована ли (получила ли значение)
    
    // Значения (активно только одно, в зависимости от type)
    int intValue;
    double realValue;
    std::string stringValue;
    
    // Позиция в ПОЛИЗе (для быстрого доступа)
    int address;            // индекс в массиве переменных
    
    // Конструкторы
    VariableSymbol();
    VariableSymbol(const std::string& n, DataType t, int addr);
    
    // Установка значения в зависимости от типа
    void setValue(int v);
    void setValue(double v);
    void setValue(const std::string& v);
    
    // Получение значения в виде строки
    std::string getValueAsString() const;
    
    // Вывод для отладки
    void print() const;
};

//============================================================================
// Запись в таблице символов для метки
//============================================================================

struct LabelSymbol {
    std::string name;       // имя метки
    int address;            // адрес в ПОЛИЗе (после линковки)
    bool defined;           // определена ли метка
    std::vector<int> fixups; // список мест, которые нужно заполнить адресом
    
    LabelSymbol();
    LabelSymbol(const std::string& n);
    
    void addFixup(int pos);
    void print() const;
};

//============================================================================
// Таблица символов (главный класс)
//============================================================================

class SymbolTable {
private:
    //-----------------------------------------------------------------------
    // Основные хранилища
    //-----------------------------------------------------------------------
    
    // Таблица переменных: имя -> запись
    std::unordered_map<std::string, VariableSymbol> variables;
    
    // Таблица меток: имя -> запись
    std::unordered_map<std::string, LabelSymbol> labels;
    
    //-----------------------------------------------------------------------
    // Вспомогательные структуры для быстрого доступа по адресу
    //-----------------------------------------------------------------------
    
    // Вектор переменных по адресу (адрес = индекс в векторе)
    std::vector<VariableSymbol*> variablesByAddress;
    
    // Следующий свободный адрес для переменной
    int nextVariableAddress;
    
    //-----------------------------------------------------------------------
    // Стеки для семантического анализа (используются при разборе описаний)
    //-----------------------------------------------------------------------
    
    // Стек имён переменных, ожидающих тип
    std::vector<std::string> pendingVariables;
    
public:
    //========================================================================
    // Конструкторы / деструктор
    //========================================================================
    
    SymbolTable();
    ~SymbolTable() = default;
    
    //========================================================================
    // Работа с переменными
    //========================================================================
    
    // Объявление новой переменной
    // Возвращает true, если объявление успешно, false если переменная уже существует
    bool declareVariable(const std::string& name, DataType type);
    
    // Проверка, объявлена ли переменная
    bool isVariableDeclared(const std::string& name) const;
    
    // Получение типа переменной
    DataType getVariableType(const std::string& name) const;
    
    // Получение адреса переменной
    int getVariableAddress(const std::string& name) const;
    
    // Проверка, инициализирована ли переменная
    bool isVariableInitialized(const std::string& name) const;
    
    // Установка значения переменной
    void setVariableValue(const std::string& name, int value);
    void setVariableValue(const std::string& name, double value);
    void setVariableValue(const std::string& name, const std::string& value);
    
    // Получение значения переменной
    int getIntValue(const std::string& name) const;
    double getRealValue(const std::string& name) const;
    std::string getStringValue(const std::string& name) const;
    
    // Получение значения по адресу (для интерпретатора)
    int getIntValueByAddress(int address) const;
    double getRealValueByAddress(int address) const;
    std::string getStringValueByAddress(int address) const;
    DataType getTypeByAddress(int address) const;
    
    //========================================================================
    // Работа со стеком переменных для описаний
    //========================================================================
    
    // Добавление переменной в стек (при разборе списка идентификаторов)
    void pushPendingVariable(const std::string& name);
    
    // Очистка стека ожидающих переменных
    void clearPendingVariables();
    
    // Присвоение типа всем переменным в стеке и добавление их в таблицу
    void declarePendingVariables(DataType type);
    
    // Проверка, есть ли переменные в стеке
    bool hasPendingVariables() const { return !pendingVariables.empty(); }
    
    //========================================================================
    // Работа с метками
    //========================================================================
    
    // Объявление метки (встретили определение метки: label:)
    bool declareLabel(const std::string& name);
    
    // Проверка, объявлена ли метка
    bool isLabelDeclared(const std::string& name) const;
    
    // Определение адреса метки (после генерации ПОЛИЗа)
    void defineLabel(const std::string& name, int address);
    
    // Получение адреса метки
    int getLabelAddress(const std::string& name) const;
    
    // Добавление отложенной ссылки на метку (для последующего заполнения)
    void addLabelFixup(const std::string& name, int position);
    
    // Получение списка fixup'ов для метки (для backpatching)
    std::vector<int> getLabelFixups(const std::string& name);
    
    //========================================================================
    // Вспомогательные методы
    //========================================================================
    
    // Количество переменных
    size_t getVariableCount() const { return variables.size(); }
    
    // Количество меток
    size_t getLabelCount() const { return labels.size(); }
    
    // Получение всех переменных (для отладки)
    std::vector<VariableSymbol> getAllVariables() const;
    
    // Получение всех меток (для отладки)
    std::vector<LabelSymbol> getAllLabels() const;

    // Добавить в класс SymbolTable (после существующих методов)

    // Установка значения по адресу
    void setIntValueByAddress(int address, int value);
    void setRealValueByAddress(int address, double value);
    void setStringValueByAddress(int address, const std::string& value);

    // Получение значения по адресу (уже есть, но проверим)
    int getIntValueByAddress(int address) const;
    double getRealValueByAddress(int address) const;
    std::string getStringValueByAddress(int address) const;
    DataType getTypeByAddress(int address) const;
    
    //========================================================================
    // Вывод и отладка
    //========================================================================
    
    void printVariables() const;
    void printLabels() const;
    void print() const;
    
    //========================================================================
    // Сброс (для повторного использования)
    //========================================================================
    
    void reset();
};

#endif // SYMBOL_TABLE_H