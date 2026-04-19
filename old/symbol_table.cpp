// symbol_table.cpp
#include "symbol_table.h"
#include <algorithm>
#include <sstream>
#include <iomanip>

using namespace std;

//============================================================================
// Преобразование DataType в строку
//============================================================================

string dataTypeToString(DataType type) {
    switch (type) {
        case DataType::INT:     return "int";
        case DataType::REAL:    return "real";
        case DataType::STRING:  return "string";
        case DataType::UNDEFINED: return "undefined";
        default:                return "unknown";
    }
}

//============================================================================
// Реализация VariableSymbol
//============================================================================

VariableSymbol::VariableSymbol() 
    : name(""), type(DataType::UNDEFINED), declared(false), initialized(false),
      intValue(0), realValue(0.0), stringValue(""), address(-1) {}

VariableSymbol::VariableSymbol(const string& n, DataType t, int addr)
    : name(n), type(t), declared(true), initialized(false),
      intValue(0), realValue(0.0), stringValue(""), address(addr) {}

void VariableSymbol::setValue(int v) {
    if (type == DataType::INT) {
        intValue = v;
        initialized = true;
    } else if (type == DataType::REAL) {
        realValue = static_cast<double>(v);
        initialized = true;
    }
    // Для STRING игнорируем
}

void VariableSymbol::setValue(double v) {
    if (type == DataType::REAL) {
        realValue = v;
        initialized = true;
    } else if (type == DataType::INT) {
        intValue = static_cast<int>(v);
        initialized = true;
    }
    // Для STRING игнорируем
}

void VariableSymbol::setValue(const string& v) {
    if (type == DataType::STRING) {
        stringValue = v;
        initialized = true;
    }
    // Для INT/REAL игнорируем
}

string VariableSymbol::getValueAsString() const {
    if (!initialized) return "uninitialized";
    
    switch (type) {
        case DataType::INT:
            return to_string(intValue);
        case DataType::REAL: {
            ostringstream oss;
            oss << fixed << setprecision(6) << realValue;
            string s = oss.str();
            // Убираем лишние нули в конце
            while (s.length() > 1 && s.back() == '0') s.pop_back();
            if (s.back() == '.') s.pop_back();
            return s;
        }
        case DataType::STRING:
            return "\"" + stringValue + "\"";
        default:
            return "?";
    }
}

void VariableSymbol::print() const {
    cout << "  Var{name='" << name << "', type=" << dataTypeToString(type)
         << ", addr=" << address << ", init=" << (initialized ? "yes" : "no")
         << ", value=" << getValueAsString() << "}" << endl;
}

//============================================================================
// Реализация LabelSymbol
//============================================================================

LabelSymbol::LabelSymbol() 
    : name(""), address(-1), defined(false) {}

LabelSymbol::LabelSymbol(const string& n)
    : name(n), address(-1), defined(false) {}

void LabelSymbol::addFixup(int pos) {
    fixups.push_back(pos);
}

void LabelSymbol::print() const {
    cout << "  Label{name='" << name << "', address=" << address 
         << ", defined=" << (defined ? "yes" : "no")
         << ", fixups=" << fixups.size() << "}" << endl;
    for (int pos : fixups) {
        cout << "    -> fixup at " << pos << endl;
    }
}

//============================================================================
// Реализация SymbolTable
//============================================================================

SymbolTable::SymbolTable() 
    : nextVariableAddress(0) {}

//============================================================================
// Работа с переменными
//============================================================================

bool SymbolTable::declareVariable(const string& name, DataType type) {
    // Проверка на повторное объявление
    if (variables.find(name) != variables.end()) {
        return false;
    }
    
    int address = nextVariableAddress++;
    VariableSymbol sym(name, type, address);
    variables[name] = sym;
    variablesByAddress.push_back(&variables[name]);
    
    return true;
}

bool SymbolTable::isVariableDeclared(const string& name) const {
    return variables.find(name) != variables.end();
}

DataType SymbolTable::getVariableType(const string& name) const {
    auto it = variables.find(name);
    if (it != variables.end()) {
        return it->second.type;
    }
    return DataType::UNDEFINED;
}

int SymbolTable::getVariableAddress(const string& name) const {
    auto it = variables.find(name);
    if (it != variables.end()) {
        return it->second.address;
    }
    return -1;
}

bool SymbolTable::isVariableInitialized(const string& name) const {
    auto it = variables.find(name);
    if (it != variables.end()) {
        return it->second.initialized;
    }
    return false;
}

void SymbolTable::setVariableValue(const string& name, int value) {
    auto it = variables.find(name);
    if (it != variables.end()) {
        it->second.setValue(value);
    }
}

void SymbolTable::setVariableValue(const string& name, double value) {
    auto it = variables.find(name);
    if (it != variables.end()) {
        it->second.setValue(value);
    }
}

void SymbolTable::setVariableValue(const string& name, const string& value) {
    auto it = variables.find(name);
    if (it != variables.end()) {
        it->second.setValue(value);
    }
}

int SymbolTable::getIntValue(const string& name) const {
    auto it = variables.find(name);
    if (it != variables.end() && it->second.type == DataType::INT) {
        return it->second.intValue;
    }
    return 0;
}

double SymbolTable::getRealValue(const string& name) const {
    auto it = variables.find(name);
    if (it != variables.end() && it->second.type == DataType::REAL) {
        return it->second.realValue;
    }
    return 0.0;
}

string SymbolTable::getStringValue(const string& name) const {
    auto it = variables.find(name);
    if (it != variables.end() && it->second.type == DataType::STRING) {
        return it->second.stringValue;
    }
    return "";
}

int SymbolTable::getIntValueByAddress(int address) const {
    if (address >= 0 && address < (int)variablesByAddress.size()) {
        VariableSymbol* sym = variablesByAddress[address];
        if (sym && sym->type == DataType::INT) {
            return sym->intValue;
        }
    }
    return 0;
}

double SymbolTable::getRealValueByAddress(int address) const {
    if (address >= 0 && address < (int)variablesByAddress.size()) {
        VariableSymbol* sym = variablesByAddress[address];
        if (sym && sym->type == DataType::REAL) {
            return sym->realValue;
        }
    }
    return 0.0;
}

string SymbolTable::getStringValueByAddress(int address) const {
    if (address >= 0 && address < (int)variablesByAddress.size()) {
        VariableSymbol* sym = variablesByAddress[address];
        if (sym && sym->type == DataType::STRING) {
            return sym->stringValue;
        }
    }
    return "";
}

DataType SymbolTable::getTypeByAddress(int address) const {
    if (address >= 0 && address < (int)variablesByAddress.size()) {
        VariableSymbol* sym = variablesByAddress[address];
        if (sym) {
            return sym->type;
        }
    }
    return DataType::UNDEFINED;
}

//============================================================================
// Работа со стеком переменных для описаний
//============================================================================

void SymbolTable::pushPendingVariable(const string& name) {
    pendingVariables.push_back(name);
}

void SymbolTable::clearPendingVariables() {
    pendingVariables.clear();
}

void SymbolTable::declarePendingVariables(DataType type) {
    for (const string& name : pendingVariables) {
        if (!declareVariable(name, type)) {
            // Переменная уже объявлена — это ошибка, но мы её зафиксируем
            // (ошибка будет обработана на более высоком уровне)
            // Здесь просто пропускаем повторное объявление
        }
    }
    pendingVariables.clear();
}

//============================================================================
// Работа с метками
//============================================================================

bool SymbolTable::declareLabel(const string& name) {
    if (labels.find(name) != labels.end()) {
        return false;  // метка уже существует
    }
    labels[name] = LabelSymbol(name);
    return true;
}

bool SymbolTable::isLabelDeclared(const string& name) const {
    return labels.find(name) != labels.end();
}

void SymbolTable::defineLabel(const string& name, int address) {
    auto it = labels.find(name);
    if (it != labels.end()) {
        it->second.address = address;
        it->second.defined = true;
    } else {
        // Если метка не была объявлена, создаём её
        LabelSymbol sym(name);
        sym.address = address;
        sym.defined = true;
        labels[name] = sym;
    }
}

int SymbolTable::getLabelAddress(const string& name) const {
    auto it = labels.find(name);
    if (it != labels.end()) {
        return it->second.address;
    }
    return -1;
}

void SymbolTable::addLabelFixup(const string& name, int position) {
    auto it = labels.find(name);
    if (it != labels.end()) {
        it->second.addFixup(position);
    } else {
        // Если метка ещё не объявлена, создаём запись с fixup'ом
        LabelSymbol sym(name);
        sym.addFixup(position);
        labels[name] = sym;
    }
}

vector<int> SymbolTable::getLabelFixups(const string& name) {
    auto it = labels.find(name);
    if (it != labels.end()) {
        return it->second.fixups;
    }
    return {};
}

//============================================================================
// Вспомогательные методы
//============================================================================

vector<VariableSymbol> SymbolTable::getAllVariables() const {
    vector<VariableSymbol> result;
    for (const auto& pair : variables) {
        result.push_back(pair.second);
    }
    return result;
}

vector<LabelSymbol> SymbolTable::getAllLabels() const {
    vector<LabelSymbol> result;
    for (const auto& pair : labels) {
        result.push_back(pair.second);
    }
    return result;
}

//============================================================================
// Вывод и отладка
//============================================================================

void SymbolTable::printVariables() const {
    cout << "=== Variables (" << variables.size() << ") ===" << endl;
    for (const auto& pair : variables) {
        pair.second.print();
    }
}

void SymbolTable::printLabels() const {
    cout << "=== Labels (" << labels.size() << ") ===" << endl;
    for (const auto& pair : labels) {
        pair.second.print();
    }
}

void SymbolTable::print() const {
    cout << "\n========================================" << endl;
    cout << "Symbol Table" << endl;
    cout << "========================================" << endl;
    printVariables();
    cout << endl;
    printLabels();
    cout << "========================================\n" << endl;
}

//============================================================================
// Сброс
//============================================================================

void SymbolTable::reset() {
    variables.clear();
    labels.clear();
    variablesByAddress.clear();
    pendingVariables.clear();
    nextVariableAddress = 0;
}