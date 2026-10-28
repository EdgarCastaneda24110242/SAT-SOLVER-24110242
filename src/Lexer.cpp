#include "Lexer.h"
#include <cctype>
#include <optional>
#include <stdexcept>

std::vector<Token> Lexer::tokenize(const std::string& input) const {
    std::vector<Token> tokens;
    for (std::size_t i = 0; i < input.size();) {
        if (std::isspace(static_cast<unsigned char>(input[i]))) { ++i; continue; }
        const std::string remaining = input.substr(i);
        if (remaining.rfind("<->", 0) == 0 || remaining.rfind("<=>", 0) == 0) {
            tokens.push_back({FormulaTokenType::Iff, "<->"}); i += 3;
        } else if (remaining.rfind("->", 0) == 0 || remaining.rfind("=>", 0) == 0) {
            tokens.push_back({FormulaTokenType::Implies, "->"}); i += 2;
        } else if (remaining.rfind("&&", 0) == 0) {
            tokens.push_back({FormulaTokenType::And, "&"}); i += 2;
        } else if (remaining.rfind("||", 0) == 0) {
            tokens.push_back({FormulaTokenType::Or, "|"}); i += 2;
        } else {
            const char character = input[i];
            const auto type = symbolType(character);
            if (type.has_value()) { tokens.push_back({*type, std::string(1, character)}); ++i; }
            else if (std::isalpha(static_cast<unsigned char>(character))) {
                std::size_t end = i + 1;
                while (end < input.size() && (std::isalnum(static_cast<unsigned char>(input[end])) || input[end] == '_')) ++end;
                tokens.push_back({FormulaTokenType::Atom, input.substr(i, end - i)}); i = end;
            } else throw std::runtime_error("Simbolo no reconocido: " + std::string(1, character));
        }
    }
    tokens.push_back({FormulaTokenType::End, ""});
    return tokens;
}
std::optional<FormulaTokenType> Lexer::symbolType(char character) {
    switch (character) {
    case '!': case '~': return FormulaTokenType::Not;
    case '&': return FormulaTokenType::And;
    case '|': return FormulaTokenType::Or;
    case '(': return FormulaTokenType::Open;
    case ')': return FormulaTokenType::Close;
    default: return std::nullopt;
    }
}
