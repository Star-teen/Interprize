#ifndef LEXER_H
#define LEXER_H
// ============================================================
// lexer.h — лексический анализатор (Лексер)
//
// Лексер читает исходный текст программы символ за символом
// и выдаёт вектор токенов (vector<Token>).
//
// Реализован как конечный автомат (КА):
//   смотрим на текущий символ → решаем в каком "состоянии" →
//   читаем до конца лексемы → возвращаем Token.
//
// Конечный автомат (состояния):
//
//   НАЧАЛО
//     ├── пробел/\n/\t  → пропустить, остаться в НАЧАЛО
//     ├── '/' + '*'     → КОММЕНТАРИЙ (пропускаем до */)
//     ├── '"'           → СТРОКА (читаем до закрывающей ")
//     ├── цифра [0-9]   → ЦЕЛОЕ (накапливаем цифры)
//     │     └── '.' + цифра → ВЕЩЕСТВЕННОЕ
//     ├── буква / '_'   → ИДЕНТИФИКАТОР (накапливаем буквы/цифры/_)
//     │     └── сравниваем с таблицей ключевых слов
//     ├── '<' + '='     → токен LE
//     ├── '>' + '='     → токен GE
//     ├── '=' + '='     → токен EQ
//     ├── '!' + '='     → токен NEQ
//     └── одиночный символ → соответствующий токен (+, -, :, ; и т.д.)
// ============================================================
#include "token.h"
#include <vector>
#include <string>
#include <stdexcept>
#include <cctype>

class Lexer {
    std::string src;   // весь исходный текст программы
    size_t      pos;   // текущая позиция чтения
    int         ln;    // текущий номер строки (для сообщений об ошибках)

    // Текущий символ (или '\0' если конец файла)
    char cur() {
        return pos < src.size() ? src[pos] : '\0';
    }

    // Следующий символ — смотрим вперёд без сдвига (look-ahead)
    char peekNext() {
        return (pos + 1) < src.size() ? src[pos + 1] : '\0';
    }

    // Сдвинуть позицию вперёд и вернуть прочитанный символ
    char advance() {
        char c = src[pos++];
        if (c == '\n') ln++;   // считаем строки для сообщений об ошибках
        return c;
    }

    // Пропустить все пробельные символы (пробел, \n, \t, \r)
    void skipWhitespace() {
        while (pos < src.size() && std::isspace((unsigned char)cur()))
            advance();
    }

    // Пропустить комментарий /* ... */
    void skipComment() {
        advance(); advance();   // съедаем '/' и '*'
        while (pos + 1 < src.size()) {
            if (cur() == '*' && peekNext() == '/') {
                advance(); advance();   // съедаем '*' и '/'
                return;
            }
            advance();
        }
        throw std::runtime_error("Незакрытый комментарий /* без */");
    }

    // Прочитать число: целое или вещественное
    Token readNumber(int L) {
        std::string s;
        while (pos < src.size() && std::isdigit((unsigned char)cur()))
            s += advance();

        // Если после цифр стоит точка и ещё цифра → это REAL
        if (cur() == '.' && std::isdigit((unsigned char)peekNext())) {
            s += advance();   // добавляем '.'
            while (pos < src.size() && std::isdigit((unsigned char)cur()))
                s += advance();
            return Token(TT::REAL_L, s, L);
        }

        return Token(TT::INT_L, s, L);
    }

    // Прочитать строку в кавычках "..."
    Token readString(int L) {
        advance();   // пропускаем открывающую "
        std::string s;
        while (pos < src.size() && cur() != '"') {
            if (cur() == '\n')
                throw std::runtime_error(
                    "Строка не закрыта (строка " + std::to_string(ln) + ")");
            s += advance();
        }
        if (cur() != '"')
            throw std::runtime_error("Строка не закрыта (достигнут конец файла)");
        advance();   // пропускаем закрывающую "
        return Token(TT::STR_L, s, L);
    }

    // Прочитать идентификатор или ключевое слово
    Token readIdentOrKeyword(int L) {
        std::string s;
        while (pos < src.size() &&
               (std::isalnum((unsigned char)cur()) || cur() == '_'))
            s += advance();

        // Сравниваем с таблицей ключевых слов
        if (s == "program") return Token(TT::LEX_PROGRAM, s, L);
        if (s == "int")     return Token(TT::LEX_INT,     s, L);
        if (s == "string")  return Token(TT::LEX_STRING,  s, L);
        if (s == "real")    return Token(TT::LEX_REAL,     s, L);
        if (s == "if")      return Token(TT::LEX_IF,       s, L);
        if (s == "else")    return Token(TT::LEX_ELSE,     s, L);
        if (s == "while")   return Token(TT::LEX_WHILE,    s, L);
        if (s == "for")     return Token(TT::LEX_FOR,      s, L);
        if (s == "step")    return Token(TT::LEX_STEP,     s, L);
        if (s == "until")   return Token(TT::LEX_UNTIL,    s, L);
        if (s == "do")      return Token(TT::LEX_DO,       s, L);
        if (s == "goto")    return Token(TT::LEX_GOTO,     s, L);
        if (s == "read")    return Token(TT::LEX_READ,     s, L);
        if (s == "write")   return Token(TT::LEX_WRITE,    s, L);
        if (s == "and")     return Token(TT::LEX_AND,      s, L);
        if (s == "or")      return Token(TT::LEX_OR,       s, L);
        if (s == "not")     return Token(TT::LEX_NOT,      s, L);

        // Не ключевое слово → имя переменной или метки
        return Token(TT::ID, s, L);
    }

public:
    explicit Lexer(std::string text) : src(std::move(text)), pos(0), ln(1) {}

    // Главный метод: разбить весь текст на поток токенов
    std::vector<Token> tokenize() {
        std::vector<Token> result;

        while (true) {
            // Пропускаем пробелы и комментарии (их может быть несколько подряд)
            while (true) {
                skipWhitespace();
                if (cur() == '/' && peekNext() == '*') {
                    skipComment();
                    continue;
                }
                break;
            }

            // Конец файла
            if (cur() == '\0') {
                result.push_back(Token(TT::EOF_TOK, "", ln));
                break;
            }

            int L = ln;    // запоминаем строку для текущего токена
            char c = cur();

            // Число
            if (std::isdigit((unsigned char)c)) {
                result.push_back(readNumber(L));
                continue;
            }
            // Идентификатор / ключевое слово
            if (std::isalpha((unsigned char)c) || c == '_') {
                result.push_back(readIdentOrKeyword(L));
                continue;
            }
            // Строка
            if (c == '"') {
                result.push_back(readString(L));
                continue;
            }

            // Двухсимвольные операторы (проверяем пару символов)
            if (c=='<' && peekNext()=='=') { advance(); advance(); result.push_back(Token(TT::LE,  "<=", L)); continue; }
            if (c=='>' && peekNext()=='=') { advance(); advance(); result.push_back(Token(TT::GE,  ">=", L)); continue; }
            if (c=='=' && peekNext()=='=') { advance(); advance(); result.push_back(Token(TT::EQ,  "==", L)); continue; }
            if (c=='!' && peekNext()=='=') { advance(); advance(); result.push_back(Token(TT::NEQ, "!=", L)); continue; }

            // Односимвольные операторы
            advance();
            switch (c) {
                case '+': result.push_back(Token(TT::PLUS,      "+", L)); break;
                case '-': result.push_back(Token(TT::MINUS,     "-", L)); break;
                case '*': result.push_back(Token(TT::STAR,      "*", L)); break;
                case '/': result.push_back(Token(TT::SLASH,     "/", L)); break;
                case '<': result.push_back(Token(TT::LT,        "<", L)); break;
                case '>': result.push_back(Token(TT::GT,        ">", L)); break;
                case '=': result.push_back(Token(TT::ASSIGN,    "=", L)); break;
                case '(': result.push_back(Token(TT::LPAREN,    "(", L)); break;
                case ')': result.push_back(Token(TT::RPAREN,    ")", L)); break;
                case '{': result.push_back(Token(TT::LBRACE,    "{", L)); break;
                case '}': result.push_back(Token(TT::RBRACE,    "}", L)); break;
                case ';': result.push_back(Token(TT::SEMICOLON, ";", L)); break;
                case ',': result.push_back(Token(TT::COMMA,     ",", L)); break;
                case ':': result.push_back(Token(TT::COLON,     ":", L)); break;
                default:
                    throw std::runtime_error(
                        "Строка " + std::to_string(L) +
                        ": неизвестный символ '" + c + "'");
            }
        }

        return result;
    }
};
#endif // LEXER_H
