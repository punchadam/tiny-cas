#pragma once

#include <vector>
#include <string>
#include <stdexcept>
#include <stdint.h>

using i64 = int64_t;
using i32 = int32_t;
using i16 = int16_t;
using i8 = int8_t;
using u64 = uint64_t;
using u32 = uint32_t;
using u16 = uint16_t;
using u8 = uint8_t;

bool isNumber(const char c) {
    unsigned char usc = static_cast<unsigned char>(c);
    return (usc >= '0' && usc <= '9');
}
bool isLetter(const char c) {
    unsigned char usc = static_cast<unsigned char>(c);
    return ((usc >= 'A' && usc <= 'Z') || (usc >= 'a' && usc <= 'z'));
}

enum class TokenType {
    IDENTIFIER,
    CONSTANT,
    NUMBER,
    LPAREN,
    RPAREN,
    OPERATOR
};

struct Token {
    TokenType type;
    std::string lexeme;

    Token(TokenType t, std::string l) {
        type = t;
        lexeme = l;
    }
};

enum class LexerStates {
    START,
    READING_NUMBER,
    READING_DECIMAL,
    READING_FRAC,
    READING_TEXT
};

inline void Lex(const std::string& expression, std::vector<Token>& tokens) {
    LexerStates currentState = LexerStates::START;
    std::string lexeme;
    size_t i = 0;
    char c;
    while (i < expression.size()) {
        c = expression[i];
        switch (currentState) {
            case LexerStates::START:
                lexeme.clear();
                if (isNumber(c)) {
                    currentState = LexerStates::READING_NUMBER;
                    break;
                } if (c == '.') {
                    currentState = LexerStates::READING_DECIMAL;
                } if (isLetter(c)) {
                    currentState = LexerStates::READING_TEXT;
                    break;
                } if (c == '+' || c == '-' || c == '*' || c == '/' || c == '^') {
                    Token newToken(TokenType::OPERATOR, std::string(1, c));
                    tokens.push_back(newToken);
                    i++;
                    break;
                } if (c == '(') {
                    Token newToken(TokenType::LPAREN, "(");
                    tokens.push_back(newToken);
                    i++;
                    break;
                }
                if (c == ')') {
                    Token newToken(TokenType::RPAREN, ")");
                    tokens.push_back(newToken);
                    i++;
                    break;
                }
                if (c == ' ') {
                    i++;
                    break;
                } else {
                    std::string msg = "Unknown Character: \'";
                    msg += c;
                    msg += "\' at position: ";
                    msg += std::to_string(i);
                    msg += "\n";
                    throw std::invalid_argument(msg);
                }
                break;
            case LexerStates::READING_NUMBER:
                lexeme.push_back(c);
                i++;
                if (!isNumber(expression[i])) {
                    if (expression[i] == '.') {
                        currentState = LexerStates::READING_DECIMAL;
                        break;
                    }
                    currentState = LexerStates::START;
                    Token newToken(TokenType::NUMBER, lexeme);
                    tokens.push_back(newToken);
                }
                break;
            case LexerStates::READING_DECIMAL:
                lexeme.push_back(c);
                i++;
                if (!isNumber(expression[i])) {
                    std::string msg = "Decimal must be followed by number, instead got \'";
                    msg += c;
                    msg += "\' at position: ";
                    msg += std::to_string(i);
                    msg += "\n";
                    throw std::invalid_argument(msg);
                }
                currentState = LexerStates::READING_FRAC;
                break;
            case LexerStates::READING_FRAC:
                lexeme.push_back(c);
                i++;
                if (expression[i] == '.') {
                    std::string msg = "More than one decimal in one number, at position: ";
                    msg += std::to_string(i);
                    msg += "\n";
                    throw std::invalid_argument(msg);
                }
                if (!isNumber(expression[i])) {
                    currentState = LexerStates::START;
                    Token newToken(TokenType::NUMBER, lexeme);
                    tokens.push_back(newToken);
                }
                break;
            case LexerStates::READING_TEXT:
                lexeme.push_back(c);
                i++;
                if (expression[i] == '(') {
                    currentState = LexerStates::START;
                    Token newText(TokenType::IDENTIFIER, lexeme);
                    tokens.push_back(newText);
                    Token newToken(TokenType::LPAREN, "(");
                    tokens.push_back(newToken);
                    i++;
                    break;
                }
                if (!isLetter(expression[i])) {
                    currentState = LexerStates::START;
                    if (lexeme == "e" || lexeme == "pi") {
                        Token newToken(TokenType::CONSTANT, lexeme);
                        tokens.push_back(newToken);
                        break;
                    } else {
                        Token newToken(TokenType::IDENTIFIER, lexeme);
                        tokens.push_back(newToken);
                        break;
                    }
                }
            // all cases done, repeat switch statement
        }
    }
}