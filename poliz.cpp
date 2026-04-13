// poliz.cpp
#include "poliz.h"
#include <sstream>
#include <iomanip>
#include <algorithm>

using namespace std;

//============================================================================
// Преобразование PolizCmd в строку
//============================================================================

string polizCmdToString(PolizCmd cmd) {
    switch (cmd) {
        case PolizCmd::PUSH_INT:        return "PUSH_INT";
        case PolizCmd::PUSH_REAL:       return "PUSH_REAL";
        case PolizCmd::PUSH_STRING:     return "PUSH_STRING";
        case PolizCmd::PUSH_VAR:        return "PUSH_VAR";
        case PolizCmd::POP_VAR:         return "POP_VAR";
        case PolizCmd::ADD:             return "ADD";
        case PolizCmd::SUB:             return "SUB";
        case PolizCmd::MUL:             return "MUL";
        case PolizCmd::DIV:             return "DIV";
        case PolizCmd::MOD:             return "MOD";
        case PolizCmd::NEG:             return "NEG";
        case PolizCmd::AND:             return "AND";
        case PolizCmd::OR:              return "OR";
        case PolizCmd::NOT:             return "NOT";
        case PolizCmd::LESS:            return "LESS";
        case PolizCmd::GREATER:         return "GREATER";
        case PolizCmd::LE:              return "LE";
        case PolizCmd::GE:              return "GE";
        case PolizCmd::EQ:              return "EQ";
        case PolizCmd::NE:              return "NE";
        case PolizCmd::LABEL:           return "LABEL";
        case PolizCmd::GOTO:            return "GOTO";
        case PolizCmd::GOTO_IF_FALSE:   return "GOTO_IF_FALSE";
        case PolizCmd::GOTO_IF_TRUE:    return "GOTO_IF_TRUE";
        case PolizCmd::READ:            return "READ";
        case PolizCmd::WRITE:           return "WRITE";
        case PolizCmd::HALT:            return "HALT";
        case PolizCmd::NOP:             return "NOP";
        default:                        return "UNKNOWN";
    }
}

//============================================================================
// Реализация PolizInstruction
//============================================================================

PolizInstruction::PolizInstruction()
    : cmd(PolizCmd::NOP), intValue(0), realValue(0.0), strValue(""), address(-1) {}

PolizInstruction::PolizInstruction(PolizCmd c)
    : cmd(c), intValue(0), realValue(0.0), strValue(""), address(-1) {}

PolizInstruction::PolizInstruction(PolizCmd c, int v)
    : cmd(c), intValue(v), realValue(0.0), strValue(""), address(-1) {}

PolizInstruction::PolizInstruction(PolizCmd c, double v)
    : cmd(c), intValue(0), realValue(v), strValue(""), address(-1) {}

PolizInstruction::PolizInstruction(PolizCmd c, const string& v)
    : cmd(c), intValue(0), realValue(0.0), strValue(v), address(-1) {}

PolizInstruction::PolizInstruction(PolizCmd c, const string& v, int addr)
    : cmd(c), intValue(0), realValue(0.0), strValue(v), address(addr) {}

string PolizInstruction::toString() const {
    ostringstream oss;
    oss << polizCmdToString(cmd);
    
    switch (cmd) {
        case PolizCmd::PUSH_INT:
            oss << " " << intValue;
            break;
        case PolizCmd::PUSH_REAL:
            oss << " " << realValue;
            break;
        case PolizCmd::PUSH_STRING:
            oss << " \"" << strValue << "\"";
            break;
        case PolizCmd::PUSH_VAR:
        case PolizCmd::POP_VAR:
            oss << " addr=" << address;
            if (!strValue.empty()) oss << " (" << strValue << ")";
            break;
        case PolizCmd::LABEL:
            oss << " " << strValue;
            break;
        case PolizCmd::GOTO:
        case PolizCmd::GOTO_IF_FALSE:
        case PolizCmd::GOTO_IF_TRUE:
            oss << " " << intValue;
            break;
        default:
            break;
    }
    
    return oss.str();
}

void PolizInstruction::print() const {
    cout << toString() << endl;
}

//============================================================================
// Реализация Poliz
//============================================================================

Poliz::Poliz() {}

Poliz::Poliz(size_t reservedSize) {
    code.reserve(reservedSize);
}

//============================================================================
// Добавление инструкций
//============================================================================

void Poliz::emit(PolizCmd cmd) {
    code.emplace_back(cmd);
}

void Poliz::emit(PolizCmd cmd, int v) {
    code.emplace_back(cmd, v);
}

void Poliz::emit(PolizCmd cmd, double v) {
    code.emplace_back(cmd, v);
}

void Poliz::emit(PolizCmd cmd, const string& v) {
    code.emplace_back(cmd, v);
}

void Poliz::emit(PolizCmd cmd, const string& v, int addr) {
    code.emplace_back(cmd, v, addr);
}

void Poliz::emit(const Token& token) {
    switch (token.type) {
        case TokenType::INT_CONST:
            emit(PolizCmd::PUSH_INT, token.intValue);
            break;
        case TokenType::REAL_CONST:
            emit(PolizCmd::PUSH_REAL, token.realValue);
            break;
        case TokenType::STRING_CONST:
            emit(PolizCmd::PUSH_STRING, token.stringValue);
            break;
        case TokenType::IDENTIFIER:
            // Для идентификатора нужен адрес, который пока неизвестен
            // Будет установлен позже через SymbolTable
            emit(PolizCmd::PUSH_VAR, token.stringValue, -1);
            break;
        case TokenType::OP_PLUS:    emit(PolizCmd::ADD); break;
        case TokenType::OP_MINUS:   emit(PolizCmd::SUB); break;
        case TokenType::OP_MUL:     emit(PolizCmd::MUL); break;
        case TokenType::OP_DIV:     emit(PolizCmd::DIV); break;
        case TokenType::OP_MOD:     emit(PolizCmd::MOD); break;
        case TokenType::OP_LT:      emit(PolizCmd::LESS); break;
        case TokenType::OP_GT:      emit(PolizCmd::GREATER); break;
        case TokenType::OP_LE:      emit(PolizCmd::LE); break;
        case TokenType::OP_GE:      emit(PolizCmd::GE); break;
        case TokenType::OP_EQ:      emit(PolizCmd::EQ); break;
        case TokenType::OP_NE:      emit(PolizCmd::NE); break;
        case TokenType::KW_AND:     emit(PolizCmd::AND); break;
        case TokenType::KW_OR:      emit(PolizCmd::OR); break;
        case TokenType::KW_NOT:     emit(PolizCmd::NOT); break;
        default:
            // Игнорируем остальные
            break;
    }
}

//============================================================================
// Работа с метками
//============================================================================

void Poliz::label(const string& name) {
    int address = code.size();
    labelMap[name] = address;
    emit(PolizCmd::LABEL, name);
}

int Poliz::reserve() {
    int position = code.size();
    emit(PolizCmd::NOP);  // заглушка, будет заменена
    reservedSpots.push_back(position);
    return position;
}

void Poliz::patch(int position, int target) {
    if (position >= 0 && position < (int)code.size()) {
        code[position].cmd = PolizCmd::GOTO;
        code[position].intValue = target;
    }
}

void Poliz::patch(int position, const string& labelName) {
    if (position >= 0 && position < (int)code.size()) {
        code[position].cmd = PolizCmd::GOTO;
        code[position].strValue = labelName;
        code[position].intValue = -1;  // временно, будет заменено при линковке
    }
}

//============================================================================
// Линковка
//============================================================================

void Poliz::collectLabels() {
    labelMap.clear();
    for (size_t i = 0; i < code.size(); ++i) {
        if (code[i].cmd == PolizCmd::LABEL) {
            labelMap[code[i].strValue] = i;
        }
    }
}

void Poliz::linkLabels() {
    for (size_t i = 0; i < code.size(); ++i) {
        PolizInstruction& instr = code[i];
        
        // Замена ссылок на метки в инструкциях перехода
        if (instr.cmd == PolizCmd::GOTO || 
            instr.cmd == PolizCmd::GOTO_IF_FALSE ||
            instr.cmd == PolizCmd::GOTO_IF_TRUE) {
            
            // Если intValue == -1, значит это символическая ссылка
            if (instr.intValue == -1 && !instr.strValue.empty()) {
                auto it = labelMap.find(instr.strValue);
                if (it != labelMap.end()) {
                    instr.intValue = it->second;
                } else {
                    // Метка не найдена — ошибка
                    cerr << "Ошибка линковки: метка '" << instr.strValue 
                         << "' не найдена" << endl;
                    instr.intValue = 0;
                }
            }
        }
        
        // Замена адресов переменных (если есть)
        if (instr.cmd == PolizCmd::PUSH_VAR || instr.cmd == PolizCmd::POP_VAR) {
            // Адрес переменной должен быть установлен через SymbolTable
            // Здесь мы его не меняем, он должен быть правильным с самого начала
        }
    }
}

void Poliz::link() {
    collectLabels();
    linkLabels();
}

//============================================================================
// Доступ к инструкциям
//============================================================================

const PolizInstruction& Poliz::operator[](size_t index) const {
    if (index >= code.size()) {
        throw out_of_range("Poliz index out of range");
    }
    return code[index];
}

PolizInstruction& Poliz::operator[](size_t index) {
    if (index >= code.size()) {
        throw out_of_range("Poliz index out of range");
    }
    return code[index];
}

PolizInstruction& Poliz::back() {
    return code.back();
}

const PolizInstruction& Poliz::back() const {
    return code.back();
}

//============================================================================
// Очистка
//============================================================================

void Poliz::clear() {
    code.clear();
    labelMap.clear();
    reservedSpots.clear();
}

//============================================================================
// Вывод и отладка
//============================================================================

void Poliz::print() const {
    cout << "\n=== ПОЛИЗ ===" << endl;
    for (size_t i = 0; i < code.size(); ++i) {
        cout << i << ": " << code[i].toString() << endl;
    }
    cout << "=============\n" << endl;
}

void Poliz::printDisassembly() const {
    cout << "\n=== Дизассемблированный код ===" << endl;
    
    for (size_t i = 0; i < code.size(); ++i) {
        const PolizInstruction& instr = code[i];
        
        // Пропускаем LABEL при выводе (они не исполняются)
        if (instr.cmd == PolizCmd::LABEL) {
            cout << "\n" << instr.strValue << ":" << endl;
            continue;
        }
        
        cout << "  " << i << ": ";
        
        switch (instr.cmd) {
            case PolizCmd::PUSH_INT:
                cout << "push " << instr.intValue;
                break;
            case PolizCmd::PUSH_REAL:
                cout << "push " << instr.realValue;
                break;
            case PolizCmd::PUSH_STRING:
                cout << "push \"" << instr.strValue << "\"";
                break;
            case PolizCmd::PUSH_VAR:
                cout << "push var[" << instr.address << "]";
                break;
            case PolizCmd::POP_VAR:
                cout << "pop var[" << instr.address << "]";
                break;
            case PolizCmd::ADD: cout << "add"; break;
            case PolizCmd::SUB: cout << "sub"; break;
            case PolizCmd::MUL: cout << "mul"; break;
            case PolizCmd::DIV: cout << "div"; break;
            case PolizCmd::MOD: cout << "mod"; break;
            case PolizCmd::NEG: cout << "neg"; break;
            case PolizCmd::AND: cout << "and"; break;
            case PolizCmd::OR:  cout << "or"; break;
            case PolizCmd::NOT: cout << "not"; break;
            case PolizCmd::LESS: cout << "less"; break;
            case PolizCmd::GREATER: cout << "greater"; break;
            case PolizCmd::LE: cout << "le"; break;
            case PolizCmd::GE: cout << "ge"; break;
            case PolizCmd::EQ: cout << "eq"; break;
            case PolizCmd::NE: cout << "ne"; break;
            case PolizCmd::GOTO: cout << "goto " << instr.intValue; break;
            case PolizCmd::GOTO_IF_FALSE: cout << "goto_if_false " << instr.intValue; break;
            case PolizCmd::GOTO_IF_TRUE: cout << "goto_if_true " << instr.intValue; break;
            case PolizCmd::READ: cout << "read"; break;
            case PolizCmd::WRITE: cout << "write"; break;
            case PolizCmd::HALT: cout << "halt"; break;
            default: cout << "nop"; break;
        }
        cout << endl;
    }
    cout << "========================\n" << endl;
}

//============================================================================
// Сериализация (для сохранения между запусками)
//============================================================================

vector<char> Poliz::serialize() const {
    // Упрощённая сериализация
    vector<char> data;
    // В реальном проекте здесь должна быть полная сериализация
    return data;
}

void Poliz::deserialize(const vector<char>& data) {
    // Упрощённая десериализация
    // В реальном проекте здесь должно быть восстановление из данных
}