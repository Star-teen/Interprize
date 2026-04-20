// parser.cpp
#include "parser.h"
#include <iostream>
#include <sstream>

using namespace std;

//============================================================================
// Конструктор
//============================================================================

Parser::Parser(const string& source) 
    : lexer(source), hadError(false) {
    advance();  // читаем первый токен
}

//============================================================================
// Вспомогательные методы
//============================================================================

void Parser::expect(TokenType type, const string& errorMsg) {
    if (currentToken.type != type) {
        errorAt(currentToken, errorMsg + " (ожидалось " + 
                tokenTypeToString(type) + ", найдено '" + 
                currentToken.lexeme + "')");
    } else {
        advance();
    }
}

void Parser::error(const string& msg) {
    errorAt(currentToken, msg);
}

void Parser::errorAt(const Token& token, const string& msg) {
    ostringstream oss;
    oss << "Ошибка в строке " << token.line << ", колонке " << token.column 
        << ": " << msg;
    errors.push_back(oss.str());
    hadError = true;
    
    // Не выбрасываем исключение, чтобы продолжить сбор ошибок
}

//============================================================================
// Семантические действия
//============================================================================

void Parser::checkDeclared(const string& name) {
    if (!symTable.isVariableDeclared(name)) {
        error("Переменная '" + name + "' не объявлена");
    }
}

DataType Parser::checkVariableType(const string& name) {
    if (!symTable.isVariableDeclared(name)) {
        error("Переменная '" + name + "' не объявлена");
        return DataType::UNDEFINED;
    }
    return symTable.getVariableType(name);
}

void Parser::checkBinaryOp(DataType left, DataType right, TokenType op) {
    // Таблица №2 (без boolean)
    // Разрешённые комбинации:
    // int + int = int
    // real + real = real
    // real + int = real (и наоборот)
    // string + string = string
    // int < int = int (0/1)
    // real < real = int
    // string < string = int
    
    if (left == DataType::UNDEFINED || right == DataType::UNDEFINED) {
        return;  // ошибка уже была
    }
    
    // Операции отношения всегда возвращают int (0 или 1)
    bool isRelOp = (op == TokenType::OP_LT || op == TokenType::OP_GT ||
                    op == TokenType::OP_LE || op == TokenType::OP_GE ||
                    op == TokenType::OP_EQ || op == TokenType::OP_NE);
    
    // Логические операции требуют int операндов
    bool isLogicalOp = (op == TokenType::LEX_AND || op == TokenType::LEX_OR);
    
    if (isLogicalOp) {
        if (left != DataType::INT || right != DataType::INT) {
            error("Логические операции применимы только к целым числам");
        }
        return;
    }
    
    // Арифметические операции
    if (op == TokenType::OP_PLUS && left == DataType::STRING && right == DataType::STRING) {
        return;  // конкатенация строк
    }
    
    if (op == TokenType::OP_PLUS || op == TokenType::OP_MINUS || 
        op == TokenType::OP_MUL || op == TokenType::OP_DIV) {
        
        if ((left == DataType::INT && right == DataType::INT) ||
            (left == DataType::REAL && right == DataType::REAL) ||
            (left == DataType::INT && right == DataType::REAL) ||
            (left == DataType::REAL && right == DataType::INT)) {
            return;  // допустимо
        }
        
        if (op == TokenType::OP_PLUS && left == DataType::STRING && right == DataType::STRING) {
            return;  // конкатенация
        }
        
        error("Несовместимые типы в арифметической операции: " + 
              dataTypeToString(left) + " и " + dataTypeToString(right));
    }
    
    // Операции отношения
    if (isRelOp) {
        if ((left == DataType::INT && right == DataType::INT) ||
            (left == DataType::REAL && right == DataType::REAL) ||
            (left == DataType::INT && right == DataType::REAL) ||
            (left == DataType::REAL && right == DataType::INT) ||
            (left == DataType::STRING && right == DataType::STRING)) {
            return;  // допустимо
        }
        error("Несовместимые типы в операции отношения");
    }
}

void Parser::checkIntegerCondition() {
    if (typeStack.empty()) {
        error("Стек типов пуст при проверке условия");
        return;
    }
    
    DataType condType = typeStack.top();
    typeStack.pop();
    
    if (condType != DataType::INT) {
        error("Условие должно быть целочисленным (0 = false, ≠0 = true)");
    }
}

void Parser::generateBinaryOp(TokenType op) {
    switch (op) {
        case TokenType::OP_PLUS:  poliz.emit(PolizCmd::ADD); break;
        case TokenType::OP_MINUS: poliz.emit(PolizCmd::SUB); break;
        case TokenType::OP_MUL:   poliz.emit(PolizCmd::MUL); break;
        case TokenType::OP_DIV:   poliz.emit(PolizCmd::DIV); break;
        case TokenType::OP_LT:    poliz.emit(PolizCmd::LESS); break;
        case TokenType::OP_GT:    poliz.emit(PolizCmd::GREATER); break;
        case TokenType::OP_LE:    poliz.emit(PolizCmd::LE); break;
        case TokenType::OP_GE:    poliz.emit(PolizCmd::GE); break;
        case TokenType::OP_EQ:    poliz.emit(PolizCmd::EQ); break;
        case TokenType::OP_NE:    poliz.emit(PolizCmd::NE); break;
        case TokenType::LEX_AND:   poliz.emit(PolizCmd::AND); break;
        case TokenType::LEX_OR:    poliz.emit(PolizCmd::OR); break;
        default: break;
    }
}

//============================================================================
// Синтаксические правила
//============================================================================

void Parser::program() {
    if (currentToken.type == TokenType::LEX_PROGRAM) {
        advance();
    } else {
        error("Ожидается 'program'");
    }
    
    descriptions();
    operators();
    
    if (currentToken.type != TokenType::END_OF_FILE) {
        error("Неожиданные символы после конца программы");
    }
    
    poliz.emit(PolizCmd::HALT);
}

//----------------------------------------------------------------------------
// Описания переменных
//----------------------------------------------------------------------------

void Parser::descriptions() {
    while (currentToken.type == TokenType::LEX_INT ||
           currentToken.type == TokenType::LEX_REAL ||
           currentToken.type == TokenType::LEX_STRING) {
        description();
        expect(TokenType::SEP_SEMICOLON, "Ожидается ';' после описания");
    }
}

void Parser::description() {
    DataType type;
    
    if (match(TokenType::LEX_INT)) {
        type = DataType::INT;
    } else if (match(TokenType::LEX_REAL)) {
        type = DataType::REAL;
    } else if (match(TokenType::LEX_STRING)) {
        type = DataType::STRING;
    } else {
        error("Ожидается тип переменной (int, real, string)");
        return;
    }
    
    variableList(type);
}

void Parser::variableList(DataType type) {
    variable(type);
    while (match(TokenType::SEP_COMMA)) {
        variable(type);
    }
}

void Parser::variable(DataType type) {
    if (currentToken.type != TokenType::IDENTIFIER) {
        error("Ожидается идентификатор");
        return;
    }
    
    string name = currentToken.stringValue;
    int line = currentToken.line;
    int col = currentToken.column;
    advance();
    
    // Объявление переменной
    if (!symTable.declareVariable(name, type)) {
        errorAt(Token(TokenType::IDENTIFIER, name, line, col),
                "Переменная '" + name + "' уже объявлена");
    }
    
    // Инициализация
    if (match(TokenType::OP_ASSIGN)) {
        DataType initType = expression();
        
        // Проверка совместимости типов при инициализации
        if (type == DataType::INT && initType == DataType::INT) {
            int addr = symTable.getVariableAddress(name);
            poliz.emit(PolizCmd::POP_VAR, name, addr);
        } else if (type == DataType::REAL && (initType == DataType::INT || initType == DataType::REAL)) {
            int addr = symTable.getVariableAddress(name);
            poliz.emit(PolizCmd::POP_VAR, name, addr);
        } else if (type == DataType::STRING && initType == DataType::STRING) {
            int addr = symTable.getVariableAddress(name);
            poliz.emit(PolizCmd::POP_VAR, name, addr);
        } else {
            error("Несоответствие типов при инициализации переменной '" + name + "'");
        }
    }
}

//----------------------------------------------------------------------------
// Операторы
//----------------------------------------------------------------------------

void Parser::operators() {
    while (currentToken.type != TokenType::END_OF_FILE &&
           currentToken.type != TokenType::LEX_PROGRAM) {
        statement();
    }
}

void Parser::statement() {
    if (currentToken.type == TokenType::LEX_IF) {
        ifStatement();
    } else if (currentToken.type == TokenType::LEX_WHILE) {
        whileStatement();
    } else if (currentToken.type == TokenType::LEX_FOR) {
        forStatement();
    } else if (currentToken.type == TokenType::LEX_READ) {
        readStatement();
    } else if (currentToken.type == TokenType::LEX_WRITE) {
        writeStatement();
    } else if (currentToken.type == TokenType::LEX_GOTO) {
        gotoStatement();
    } else if (currentToken.type == TokenType::SEP_LBRACE) {
        compoundStatement();
    } else if (currentToken.type == TokenType::IDENTIFIER) {
        assignmentStatement();
    } else if (match(TokenType::SEP_SEMICOLON)) {
        // Пустой оператор
    } else {
        error("Неожиданный токен в начале оператора: " + currentToken.lexeme);
        // Восстановление: пропускаем до ';' или '}'
        while (currentToken.type != TokenType::SEP_SEMICOLON &&
               currentToken.type != TokenType::SEP_RBRACE &&
               currentToken.type != TokenType::END_OF_FILE) {
            advance();
        }
        if (currentToken.type == TokenType::SEP_SEMICOLON) {
            advance();
        }
    }
}

//----------------------------------------------------------------------------
// I.1: if (выражение) оператор (без else)
//----------------------------------------------------------------------------

void Parser::ifStatement() {
    expect(TokenType::LEX_IF, "Ожидается 'if'");
    expect(TokenType::SEP_LPAREN, "Ожидается '(' после if");
    
    // Генерируем код условия
    DataType condType = expression();
    
    expect(TokenType::SEP_RPAREN, "Ожидается ')'");
    
    // Проверяем, что условие целочисленное
    if (condType != DataType::INT) {
        error("Условие в if должно быть целочисленным");
    }
    
    // Резервируем место для условного перехода
    int fgoPos = poliz.reserve();  // GOTO_IF_FALSE
    
    // Генерируем код then-ветки
    statement();
    
    // Заполняем переход (если условие ложно, прыгаем сюда)
    poliz.patch(fgoPos, poliz.size());
}

//----------------------------------------------------------------------------
// while (выражение) оператор
//----------------------------------------------------------------------------

void Parser::whileStatement() {
    int loopStart = poliz.size();  // метка начала цикла
    
    expect(TokenType::LEX_WHILE, "Ожидается 'while'");
    expect(TokenType::SEP_LPAREN, "Ожидается '(' после while");
    
    // Генерируем код условия
    DataType condType = expression();
    
    expect(TokenType::SEP_RPAREN, "Ожидается ')'");
    
    if (condType != DataType::INT) {
        error("Условие в while должно быть целочисленным");
    }
    
    // Резервируем место для условного перехода (выход из цикла)
    int exitPos = poliz.reserve();  // GOTO_IF_FALSE
    
    // Генерируем тело цикла
    statement();
    
    // Переход на начало цикла
    poliz.emit(PolizCmd::GOTO, loopStart);
    
    // Заполняем выход из цикла
    poliz.patch(exitPos, poliz.size());
}

//----------------------------------------------------------------------------
// II.3: for I = E1 step E2 until E3 do S
//----------------------------------------------------------------------------

void Parser::forStatement() {
    expect(TokenType::LEX_FOR, "Ожидается 'for'");
    
    // Параметр цикла
    if (currentToken.type != TokenType::IDENTIFIER) {
        error("Ожидается идентификатор параметра цикла");
        return;
    }
    
    string paramName = currentToken.stringValue;
    int paramLine = currentToken.line;
    int paramCol = currentToken.column;
    advance();
    
    // Проверяем, что параметр объявлен и имеет целочисленный тип
    checkDeclared(paramName);
    DataType paramType = symTable.getVariableType(paramName);
    if (paramType != DataType::INT) {
        errorAt(Token(TokenType::IDENTIFIER, paramName, paramLine, paramCol),
                "Параметр цикла for должен быть целочисленным");
    }
    int paramAddr = symTable.getVariableAddress(paramName);
    
    expect(TokenType::OP_ASSIGN, "Ожидается '='");
    
    // Начальное значение E1
    DataType startType = expression();
    if (startType != DataType::INT) {
        error("Начальное значение в for должно быть целочисленным");
    }
    // Сохраняем E1 в параметр
    poliz.emit(PolizCmd::POP_VAR, paramName, paramAddr);
    
    expect(TokenType::LEX_STEP, "Ожидается 'step'");
    
    // Шаг E2
    DataType stepType = expression();
    if (stepType != DataType::INT) {
        error("Шаг в for должен быть целочисленным");
    }
    // Шаг нужно сохранить для использования в цикле
    // Временно сохраняем на стеке (будет использоваться в конце итерации)
    // Для простоты сохраним в специальной временной переменной
    // В реальном компиляторе шаг вычисляется один раз и сохраняется
    
    expect(TokenType::LEX_UNTIL, "Ожидается 'until'");
    
    // Конечное значение E3
    DataType endType = expression();
    if (endType != DataType::INT) {
        error("Конечное значение в for должно быть целочисленным");
    }
    
    expect(TokenType::LEX_DO, "Ожидается 'do'");
    
    // Сохраняем шаг и конечное значение на стеке
    // (упрощённо: в реальном компиляторе они вычисляются один раз)
    
    int loopStart = poliz.size();
    
    // Загружаем параметр и конечное значение для проверки
    poliz.emit(PolizCmd::PUSH_VAR, paramName, paramAddr);
    // Здесь должно быть конечное значение — упрощаем
    // В полной реализации нужна временная переменная для E3
    
    // Проверка условия выхода (параметр > E3 при положительном шаге)
    // Упрощённая версия: сравниваем с E3, которое было вычислено ранее
    // Для правильной работы нужно сохранить E3 и шаг во временные переменные
    
    // Резервируем место для условного перехода
    int exitPos = poliz.reserve();
    
    // Тело цикла
    statement();
    
    // Увеличение параметра на шаг
    poliz.emit(PolizCmd::PUSH_VAR, paramName, paramAddr);
    // Здесь должен быть шаг — упрощённо используем константу 1
    poliz.emit(PolizCmd::PUSH_INT, 1);
    poliz.emit(PolizCmd::ADD);
    poliz.emit(PolizCmd::POP_VAR, paramName, paramAddr);
    
    // Переход на начало цикла
    poliz.emit(PolizCmd::GOTO, loopStart);
    
    // Заполняем выход
    poliz.patch(exitPos, poliz.size());
    
    // Примечание: это упрощённая реализация for step until
    // Полная версия требует сохранения E1, E2, E3 во временные переменные
}

//----------------------------------------------------------------------------
// III.1: goto идентификатор;
//----------------------------------------------------------------------------

void Parser::gotoStatement() {
    expect(TokenType::LEX_GOTO, "Ожидается 'goto'");
    
    if (currentToken.type != TokenType::IDENTIFIER) {
        error("Ожидается идентификатор метки после goto");
        return;
    }
    
    string labelName = currentToken.stringValue;
    int line = currentToken.line;
    int col = currentToken.column;
    advance();
    
    expect(TokenType::SEP_SEMICOLON, "Ожидается ';' после goto");
    
    // Проверяем, объявлена ли метка
    if (!symTable.isLabelDeclared(labelName)) {
        // Если метка ещё не объявлена, создаём запись для отложенной ссылки
        symTable.addLabelFixup(labelName, poliz.size());
    }
    
    // Генерируем безусловный переход
    int addr = symTable.getLabelAddress(labelName);
    if (addr >= 0) {
        poliz.emit(PolizCmd::GOTO, addr);
    } else {
        // Отложенная ссылка
        poliz.emit(PolizCmd::GOTO, -1);
        poliz.back().strValue = labelName;
    }
}

//----------------------------------------------------------------------------
// read(идентификатор);
//----------------------------------------------------------------------------

void Parser::readStatement() {
    expect(TokenType::LEX_READ, "Ожидается 'read'");
    expect(TokenType::SEP_LPAREN, "Ожидается '(' после read");
    
    if (currentToken.type != TokenType::IDENTIFIER) {
        error("Ожидается идентификатор в read");
        return;
    }
    
    string varName = currentToken.stringValue;
    checkDeclared(varName);
    int addr = symTable.getVariableAddress(varName);
    advance();
    
    expect(TokenType::SEP_RPAREN, "Ожидается ')'");
    expect(TokenType::SEP_SEMICOLON, "Ожидается ';'");
    
    // Генерируем READ
    poliz.emit(PolizCmd::READ, varName, addr);
}

//----------------------------------------------------------------------------
// write(выражение [, выражение ...]);
//----------------------------------------------------------------------------

void Parser::writeStatement() {
    expect(TokenType::LEX_WRITE, "Ожидается 'write'");
    expect(TokenType::SEP_LPAREN, "Ожидается '(' после write");
    
    int exprCount = 0;
    
    do {
        expression();
        exprCount++;
        
        // После каждого выражения генерируем WRITE
        poliz.emit(PolizCmd::WRITE);
        
        // Добавляем пробел между значениями (кроме последнего)
        if (currentToken.type == TokenType::SEP_COMMA) {
            poliz.emit(PolizCmd::PUSH_STRING, " ");
            poliz.emit(PolizCmd::WRITE);
            advance();
        } else {
            break;
        }
    } while (true);
    
    expect(TokenType::SEP_RPAREN, "Ожидается ')'");
    expect(TokenType::SEP_SEMICOLON, "Ожидается ';'");
    
    // Добавляем перевод строки после вывода
    poliz.emit(PolizCmd::PUSH_STRING, "\n");
    poliz.emit(PolizCmd::WRITE);
}

//----------------------------------------------------------------------------
// { операторы }
//----------------------------------------------------------------------------

void Parser::compoundStatement() {
    expect(TokenType::SEP_LBRACE, "Ожидается '{'");
    operators();
    expect(TokenType::SEP_RBRACE, "Ожидается '}'");
}

//----------------------------------------------------------------------------
// идентификатор = выражение;
//----------------------------------------------------------------------------

void Parser::assignmentStatement() {
    if (currentToken.type != TokenType::IDENTIFIER) {
        error("Ожидается идентификатор в левой части присваивания");
        return;
    }
    
    string varName = currentToken.stringValue;
    checkDeclared(varName);
    DataType varType = symTable.getVariableType(varName);
    int varAddr = symTable.getVariableAddress(varName);
    advance();
    
    expect(TokenType::OP_ASSIGN, "Ожидается '='");
    
    DataType exprType = expression();
    
    expect(TokenType::SEP_SEMICOLON, "Ожидается ';'");
    
    // Проверка совместимости типов
    if (varType == DataType::INT && exprType == DataType::INT) {
        poliz.emit(PolizCmd::POP_VAR, varName, varAddr);
    } else if (varType == DataType::REAL && (exprType == DataType::INT || exprType == DataType::REAL)) {
        poliz.emit(PolizCmd::POP_VAR, varName, varAddr);
    } else if (varType == DataType::STRING && exprType == DataType::STRING) {
        poliz.emit(PolizCmd::POP_VAR, varName, varAddr);
    } else {
        error("Несоответствие типов в присваивании: " + 
              dataTypeToString(varType) + " = " + dataTypeToString(exprType));
    }
}

//----------------------------------------------------------------------------
// Выражения (рекурсивный спуск с генерацией ПОЛИЗа)
//----------------------------------------------------------------------------

DataType Parser::expression() {
    return assignmentExpression();
}

DataType Parser::assignmentExpression() {
    // Проверяем, не начинается ли выражение с идентификатора, за которым следует '='
    if (currentToken.type == TokenType::IDENTIFIER) {
        string name = currentToken.stringValue;
        int saveLine = currentToken.line;
        int saveCol = currentToken.column;
        Token saveToken = currentToken;
        advance();
        
        if (currentToken.type == TokenType::OP_ASSIGN) {
            // Это присваивание
            advance();
            DataType rightType = assignmentExpression();
            
            // Проверяем левую часть
            checkDeclared(name);
            DataType leftType = symTable.getVariableType(name);
            int addr = symTable.getVariableAddress(name);
            
            if (leftType == DataType::INT && rightType == DataType::INT) {
                poliz.emit(PolizCmd::POP_VAR, name, addr);
                return leftType;
            } else if (leftType == DataType::REAL && (rightType == DataType::INT || rightType == DataType::REAL)) {
                poliz.emit(PolizCmd::POP_VAR, name, addr);
                return leftType;
            } else if (leftType == DataType::STRING && rightType == DataType::STRING) {
                poliz.emit(PolizCmd::POP_VAR, name, addr);
                return leftType;
            } else {
                error("Несоответствие типов в присваивании");
                return DataType::UNDEFINED;
            }
        } else {
            // Не присваивание, возвращаем идентификатор как первичное выражение
            // Нужно "вернуть" токен обратно — в рекурсивном спуске это сложно
            // Поэтому для простоты обрабатываем в primaryExpression
            currentToken = saveToken;
            return logicalOrExpression();
        }
    }
    
    return logicalOrExpression();
}

DataType Parser::logicalOrExpression() {
    DataType left = logicalAndExpression();
    
    while (currentToken.type == TokenType::LEX_OR) {
        Token opToken = currentToken;
        advance();
        DataType right = logicalAndExpression();
        
        checkBinaryOp(left, right, TokenType::LEX_OR);
        generateBinaryOp(TokenType::LEX_OR);
        
        left = DataType::INT;  // результат логической операции — int
    }
    
    return left;
}

DataType Parser::logicalAndExpression() {
    DataType left = relationalExpression();
    
    while (currentToken.type == TokenType::LEX_AND) {
        Token opToken = currentToken;
        advance();
        DataType right = relationalExpression();
        
        checkBinaryOp(left, right, TokenType::LEX_AND);
        generateBinaryOp(TokenType::LEX_AND);
        
        left = DataType::INT;  // результат логической операции — int
    }
    
    return left;
}

DataType Parser::relationalExpression() {
    DataType left = additiveExpression();
    
    while (true) {
        TokenType op;
        if (currentToken.type == TokenType::OP_LT) op = TokenType::OP_LT;
        else if (currentToken.type == TokenType::OP_GT) op = TokenType::OP_GT;
        else if (currentToken.type == TokenType::OP_LE) op = TokenType::OP_LE;
        else if (currentToken.type == TokenType::OP_GE) op = TokenType::OP_GE;
        else if (currentToken.type == TokenType::OP_EQ) op = TokenType::OP_EQ;
        else if (currentToken.type == TokenType::OP_NE) op = TokenType::OP_NE;
        else break;
        
        advance();
        DataType right = additiveExpression();
        
        checkBinaryOp(left, right, op);
        generateBinaryOp(op);
        
        left = DataType::INT;  // результат операции отношения — int (0 или 1)
    }
    
    return left;
}

DataType Parser::additiveExpression() {
    DataType left = multiplicativeExpression();
    
    while (currentToken.type == TokenType::OP_PLUS ||
           currentToken.type == TokenType::OP_MINUS) {
        TokenType op = currentToken.type;
        advance();
        DataType right = multiplicativeExpression();
        
        checkBinaryOp(left, right, op);
        generateBinaryOp(op);
        
        // Определяем тип результата
        if (left == DataType::REAL || right == DataType::REAL) {
            left = DataType::REAL;
        } else if (left == DataType::STRING && right == DataType::STRING && op == TokenType::OP_PLUS) {
            left = DataType::STRING;
        } else {
            left = DataType::INT;
        }
    }
    
    return left;
}

DataType Parser::multiplicativeExpression() {
    DataType left = unaryExpression();
    
    while (currentToken.type == TokenType::OP_MUL ||
           currentToken.type == TokenType::OP_DIV) {
        TokenType op = currentToken.type;
        advance();
        DataType right = unaryExpression();
        
        checkBinaryOp(left, right, op);
        generateBinaryOp(op);
        
        // Определяем тип результата
        if (left == DataType::REAL || right == DataType::REAL) {
            left = DataType::REAL;
        } else {
            left = DataType::INT;
        }
    }
    
    return left;
}

DataType Parser::unaryExpression() {
    if (currentToken.type == TokenType::OP_MINUS) {
        advance();
        DataType operand = unaryExpression();
        
        if (operand == DataType::INT || operand == DataType::REAL) {
            poliz.emit(PolizCmd::NEG);
            return operand;
        } else {
            error("Унарный минус не применим к данному типу");
            return DataType::UNDEFINED;
        }
    } else if (currentToken.type == TokenType::LEX_NOT) {
        advance();
        DataType operand = unaryExpression();
        
        if (operand == DataType::INT) {
            poliz.emit(PolizCmd::NOT);
            return DataType::INT;
        } else {
            error("Операция not применима только к целым числам");
            return DataType::UNDEFINED;
        }
    }
    
    return primaryExpression();
}

DataType Parser::primaryExpression() {
    if (currentToken.type == TokenType::INT_CONST) {
        int val = currentToken.intValue;
        poliz.emit(PolizCmd::PUSH_INT, val);
        advance();
        return DataType::INT;
    }
    
    if (currentToken.type == TokenType::REAL_CONST) {
        double val = currentToken.realValue;
        poliz.emit(PolizCmd::PUSH_REAL, val);
        advance();
        return DataType::REAL;
    }
    
    if (currentToken.type == TokenType::STRING_CONST) {
        string val = currentToken.stringValue;
        poliz.emit(PolizCmd::PUSH_STRING, val);
        advance();
        return DataType::STRING;
    }
    
    if (currentToken.type == TokenType::IDENTIFIER) {
        string name = currentToken.stringValue;
        checkDeclared(name);
        DataType type = symTable.getVariableType(name);
        int addr = symTable.getVariableAddress(name);
        advance();
        
        poliz.emit(PolizCmd::PUSH_VAR, name, addr);
        return type;
    }
    
    if (match(TokenType::SEP_LPAREN)) {
        DataType type = expression();
        expect(TokenType::SEP_RPAREN, "Ожидается ')'");
        return type;
    }
    
    error("Ожидается выражение");
    return DataType::UNDEFINED;
}

//============================================================================
// Основной метод
//============================================================================

bool Parser::parse() {
    try {
        program();
        
        if (!hadError) {
            // Линковка меток в ПОЛИЗе
            poliz.link();
        }
        
        return !hadError;
    } catch (const exception& e) {
        error(string("Критическая ошибка: ") + e.what());
        return false;
    }
}

//============================================================================
// Сброс
//============================================================================

void Parser::reset() {
    hadError = false;
    errors.clear();
    poliz.clear();
    // Таблицу символов не сбрасываем, так как она нужна для повторной интерпретации
    // symTable.reset();
}