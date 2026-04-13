// lexer.cpp
#include "lexer.h"
#include <cctype>
#include <sstream>
#include <algorithm>

using namespace std;

//============================================================================
// Конструктор
//============================================================================

Lexer::Lexer(const string& sourceCode) 
    : source(sourceCode), pos(0), line(1), column(1), state(State::H) {
    
    initCharSets();
    initOperatorMap();
    
    // Инициализируем currentToken пустым токеном
    currentToken = Token(TokenType::END_OF_FILE, "", line, column);
}

//============================================================================
// Инициализация множеств символов
//============================================================================

void Lexer::initCharSets() {
    // Символы, с которых может начинаться идентификатор
    // Согласно правилам: буквы (латиница) и подчёркивание
    for (char c = 'a'; c <= 'z'; ++c) identifierStartChars.insert(c);
    for (char c = 'A'; c <= 'Z'; ++c) identifierStartChars.insert(c);
    identifierStartChars.insert('_');
    
    // Символы, которые могут быть внутри идентификатора (буквы, цифры, подчёркивание)
    identifierChars = identifierStartChars;
    for (char c = '0'; c <= '9'; ++c) identifierChars.insert(c);
}

//============================================================================
// Инициализация карты операторов
//============================================================================

void Lexer::initOperatorMap() {
    // Односимвольные операторы и разделители
    operatorMap["="] = TokenType::OP_ASSIGN;
    operatorMap["+"] = TokenType::OP_PLUS;
    operatorMap["-"] = TokenType::OP_MINUS;
    operatorMap["*"] = TokenType::OP_MUL;
    operatorMap["/"] = TokenType::OP_DIV;
    operatorMap["%"] = TokenType::OP_MOD;
    operatorMap["<"] = TokenType::OP_LT;
    operatorMap[">"] = TokenType::OP_GT;
    operatorMap[";"] = TokenType::SEP_SEMICOLON;
    operatorMap[","] = TokenType::SEP_COMMA;
    operatorMap["("] = TokenType::SEP_LPAREN;
    operatorMap[")"] = TokenType::SEP_RPAREN;
    operatorMap["{"] = TokenType::SEP_LBRACE;
    operatorMap["}"] = TokenType::SEP_RBRACE;
    operatorMap[":"] = TokenType::SEP_COLON;
    
    // Двухсимвольные операторы (обрабатываются отдельно в автомате)
    // но карта нужна для быстрого поиска после накопления
    operatorMap["<="] = TokenType::OP_LE;
    operatorMap[">="] = TokenType::OP_GE;
    operatorMap["=="] = TokenType::OP_EQ;
    operatorMap["!="] = TokenType::OP_NE;
}

//============================================================================
// Работа с символами
//============================================================================

char Lexer::peek() const {
    if (isEOF()) return '\0';
    return source[pos];
}

char Lexer::peekNext() const {
    if (pos + 1 >= source.length()) return '\0';
    return source[pos + 1];
}

char Lexer::advance() {
    if (isEOF()) return '\0';
    char c = source[pos++];
    if (c == '\n') {
        line++;
        column = 1;
    } else {
        column++;
    }
    return c;
}

//============================================================================
// Пропуск пробелов
//============================================================================

void Lexer::skipWhitespace() {
    while (!isEOF()) {
        char c = peek();
        if (isspace(c)) {
            advance();
        } else {
            break;
        }
    }
}

//============================================================================
// Пропуск комментария /* ... */
//============================================================================

void Lexer::skipComment() {
    // Ожидаем, что текущий символ — начало комментария '/'
    // и следующий символ — '*'
    if (peek() == '/' && peekNext() == '*') {
        advance();  // пропускаем '/'
        advance();  // пропускаем '*'
        
        while (!isEOF()) {
            if (peek() == '*' && peekNext() == '/') {
                advance();  // пропускаем '*'
                advance();  // пропускаем '/'
                return;
            }
            advance();
        }
        // Если дошли до конца файла — комментарий не закрыт
        // Будет обнаружено при попытке прочитать токен
    }
}

//============================================================================
// Вспомогательные методы для буфера
//============================================================================

void Lexer::resetBuffer() {
    buffer.clear();
    tempInt = 0;
    tempReal = 0.0;
    realDivider = 0.1;
}

void Lexer::addToBuffer(char c) {
    buffer.push_back(c);
}

void Lexer::addToBuffer() {
    buffer.push_back(peek());
}

//============================================================================
// Обработка начального состояния H
//============================================================================

Token Lexer::processH() {
    skipWhitespace();
    
    if (isEOF()) {
        state = State::H;
        return Token(TokenType::END_OF_FILE, "", line, column);
    }
    
    char c = peek();
    
    // Начало идентификатора или служебного слова
    if (identifierStartChars.find(c) != identifierStartChars.end()) {
        resetBuffer();
        addToBuffer(c);
        advance();
        state = State::IDENT;
        return Token(TokenType::ERROR, "", line, column); // ещё не готов
    }
    
    // Начало числа
    if (isdigit(c)) {
        resetBuffer();
        tempInt = c - '0';
        addToBuffer(c);
        advance();
        state = State::NUMB;
        return Token(TokenType::ERROR, "", line, column);
    }
    
    // Начало строковой константы
    if (c == '"') {
        resetBuffer();
        advance();  // пропускаем открывающую кавычку
        state = State::STRING;
        return Token(TokenType::ERROR, "", line, column);
    }
    
    // Начало комментария
    if (c == '/' && peekNext() == '*') {
        skipComment();
        // После комментария остаёмся в состоянии H
        state = State::H;
        return processH();  // рекурсивно продолжаем поиск токена
    }
    
    // Начало составного оператора (:, <, >, =)
    if (c == ':' || c == '<' || c == '>' || c == '=') {
        resetBuffer();
        addToBuffer(c);
        advance();
        state = State::ALE;
        return Token(TokenType::ERROR, "", line, column);
    }
    
    // Начало !=
    if (c == '!') {
        resetBuffer();
        addToBuffer(c);
        advance();
        state = State::NEQ;
        return Token(TokenType::ERROR, "", line, column);
    }
    
    // Одиночный разделитель
    resetBuffer();
    addToBuffer(c);
    advance();
    state = State::DELIM;
    return Token(TokenType::ERROR, "", line, column);
}

//============================================================================
// Обработка состояния IDENT (идентификатор)
//============================================================================

Token Lexer::processIDENT() {
    // Накопление букв, цифр и подчёркиваний
    while (!isEOF()) {
        char c = peek();
        if (identifierChars.find(c) != identifierChars.end()) {
            addToBuffer(c);
            advance();
        } else {
            break;
        }
    }
    
    state = State::H;
    
    // Проверяем, не служебное ли это слово
    TokenType kwType = Token::getKeywordType(buffer);
    if (kwType != TokenType::IDENTIFIER) {
        // Служебное слово
        return Token(kwType, buffer, line, column - buffer.length());
    }
    
    // Обычный идентификатор
    return Token::identifier(buffer, line, column - buffer.length());
}

//============================================================================
// Обработка состояния NUMB (целое число)
//============================================================================

Token Lexer::processNUMB() {
    // Накопление цифр (схема Горнера)
    while (!isEOF()) {
        char c = peek();
        if (isdigit(c)) {
            tempInt = tempInt * 10 + (c - '0');
            addToBuffer(c);
            advance();
        } else {
            break;
        }
    }
    
    // Проверяем, не начинается ли вещественное число
    if (peek() == '.') {
        // Переходим в состояние REAL
        addToBuffer('.');
        advance();
        tempReal = static_cast<double>(tempInt);
        realDivider = 0.1;
        state = State::REAL;
        return Token(TokenType::ERROR, "", line, column);
    }
    
    state = State::H;
    return Token(tempInt, line, column - buffer.length());
}

//============================================================================
// Обработка состояния REAL (вещественное число)
//============================================================================

Token Lexer::processREAL() {
    // Накопление дробной части
    while (!isEOF()) {
        char c = peek();
        if (isdigit(c)) {
            tempReal += (c - '0') * realDivider;
            realDivider *= 0.1;
            addToBuffer(c);
            advance();
        } else {
            break;
        }
    }
    
    state = State::H;
    return Token(tempReal, line, column - buffer.length());
}

//============================================================================
// Обработка состояния STRING (строковая константа)
//============================================================================

Token Lexer::processSTRING() {
    string strValue;
    
    while (!isEOF()) {
        char c = peek();
        if (c == '"') {
            advance();  // пропускаем закрывающую кавычку
            state = State::H;
            return Token(strValue, line, column - buffer.length() - 1);
        }
        strValue += c;
        addToBuffer(c);
        advance();
    }
    
    // Если дошли до конца файла — строка не закрыта
    state = State::H;
    return Token(TokenType::ERROR, "Unterminated string", line, column);
}

//============================================================================
// Обработка состояния COM (комментарий)
//============================================================================

Token Lexer::processCOM() {
    // Этот метод фактически не используется, так как комментарии
    // обрабатываются в skipComment() внутри processH()
    // Но оставляем для полноты
    skipComment();
    state = State::H;
    return nextToken();  // рекурсивно ищем следующий токен
}

//============================================================================
// Обработка состояния ALE (составные операторы)
//============================================================================

Token Lexer::processALE() {
    // Проверяем, есть ли второй символ '='
    if (peek() == '=') {
        addToBuffer('=');
        advance();
    }
    
    state = State::H;
    
    auto it = operatorMap.find(buffer);
    if (it != operatorMap.end()) {
        return Token(it->second, buffer, line, column - buffer.length());
    }
    
    return Token(TokenType::ERROR, buffer, line, column - buffer.length());
}

//============================================================================
// Обработка состояния NEQ (!=)
//============================================================================

Token Lexer::processNEQ() {
    // После '!' должен идти '='
    if (peek() == '=') {
        addToBuffer('=');
        advance();
        state = State::H;
        return Token(TokenType::OP_NE, buffer, line, column - buffer.length());
    }
    
    state = State::H;
    return Token(TokenType::ERROR, "Expected '=' after '!'", line, column);
}

//============================================================================
// Обработка состояния DELIM (одиночный разделитель)
//============================================================================

Token Lexer::processDELIM() {
    state = State::H;
    
    auto it = operatorMap.find(buffer);
    if (it != operatorMap.end()) {
        return Token(it->second, buffer, line, column - 1);
    }
    
    return Token(TokenType::ERROR, buffer, line, column - 1);
}

//============================================================================
// Основной метод — получение следующего токена
//============================================================================

Token Lexer::nextToken() {
    while (true) {
        switch (state) {
            case State::H:
                return processH();
                
            case State::IDENT: {
                Token t = processIDENT();
                if (t.type != TokenType::ERROR) return t;
                break;
            }
            
            case State::NUMB: {
                Token t = processNUMB();
                if (t.type != TokenType::ERROR) return t;
                break;
            }
            
            case State::REAL: {
                Token t = processREAL();
                if (t.type != TokenType::ERROR) return t;
                break;
            }
            
            case State::STRING: {
                Token t = processSTRING();
                if (t.type != TokenType::ERROR) return t;
                break;
            }
            
            case State::COM: {
                Token t = processCOM();
                if (t.type != TokenType::ERROR) return t;
                break;
            }
            
            case State::ALE: {
                Token t = processALE();
                if (t.type != TokenType::ERROR) return t;
                break;
            }
            
            case State::NEQ: {
                Token t = processNEQ();
                if (t.type != TokenType::ERROR) return t;
                break;
            }
            
            case State::DELIM: {
                Token t = processDELIM();
                if (t.type != TokenType::ERROR) return t;
                break;
            }
        }
    }
}

//============================================================================
// Методы для удобства парсера
//============================================================================

bool Lexer::isOneOf(const vector<TokenType>& types) const {
    for (TokenType t : types) {
        if (currentToken.type == t) return true;
    }
    return false;
}

void Lexer::expect(TokenType t, const string& errorMsg) {
    if (currentToken.type != t) {
        throw runtime_error(errorMsg + " at " + getPosition() + 
                           ", found '" + currentToken.lexeme + "'");
    }
    advanceToken();
}

string Lexer::getPosition() const {
    return to_string(line) + ":" + to_string(column);
}

string Lexer::getErrorLocation() const {
    return "Line " + to_string(line) + ", column " + to_string(column);
}