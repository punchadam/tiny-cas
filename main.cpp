#include "lexer.h"

#include <iostream>

int main(void) {

    std::string expression = "4.5sin(23x + 3) /89";
    std::vector<Token> tokens;

    std::cout << "Original Expression:\n    " << expression << "\n\n";

    try {
        Lex(expression, tokens);

        std::cout << "Tokenized Expression:\n";

        for (Token t : tokens) {
            std::cout << "    [" << t.lexeme << "]\n";
        }
        std::cout << "    [END]";

    } catch (std::exception& e) {
        std::cout << e.what();
    }

    

    return 0;
}