#include "lexer.h"
#include "number.h"

#include <iostream>

int main(void) {

    Number n1(12, 6);
    Number n2((i64)5);
    Number n3(9.81);

    std::cout << "N1: " << n1.toString() << "\nN2: " << n2.toString() << "\nN3: " << n3.toString() << "\n\n";
    std::cout << "N1 + N2 = " << Number(n1 + n2).toString() << "\n";
    std::cout << "N3 - N1 = " << Number(n3 - n1).toString() << "\n";

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