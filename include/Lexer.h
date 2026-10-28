#ifndef SAT_LEXER_H
#define SAT_LEXER_H

#include "Formula.h"
#include <optional>
#include <string>
#include <vector>

struct Token {
    FormulaTokenType type;
    std::string text;
};

class Lexer {
public:
    std::vector<Token> tokenize(const std::string& input) const;
private:
    static std::optional<FormulaTokenType> symbolType(char character);
};

#endif
