#ifndef ERROR_H
#define ERROR_H

#include <stdexcept>
#include <string>

struct LexerError : public std::runtime_error {
    size_t pos;
    LexerError(size_t pos, const std::string& message)
        : std::runtime_error(message), pos(pos) {}
};

struct ParserError : public std::runtime_error {
    size_t pos;
    ParserError(size_t pos, const std::string& message)
        : std::runtime_error(message), pos(pos) {}
};

struct TransformerError : public std::runtime_error {
    size_t pos;
    TransformerError(size_t pos, const std::string& message)
        : std::runtime_error(message), pos(pos) {}
};

#endif