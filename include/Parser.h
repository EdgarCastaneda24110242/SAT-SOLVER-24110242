#ifndef SAT_PARSER_H
#define SAT_PARSER_H

#include "AtomTable.h"
#include "Lexer.h"
#include <map>
#include <optional>
#include <string>
#include <utility>
#include <vector>

class Parser {
public:
    explicit Parser(AtomTable& atomTable);
    FormulaPtr parseSymbolic(const std::string& input) const;
    FormulaPtr parseNaturalLanguage(const std::string& sentence);
    const std::map<std::string, std::string>& naturalLanguageAtoms() const;
private:
    FormulaPtr parseTokens(const std::vector<Token>& tokens) const;
    static std::string clean(std::string text);
    static std::string trim(const std::string& text);
    FormulaPtr parseExpression(const std::string& text);
    std::optional<std::pair<std::size_t, std::string>> findConnector(
        const std::string& text, const std::vector<std::string>& connectors) const;
    FormulaPtr binary(FormulaTokenType type, const std::string& left, const std::string& right);
    static FormulaPtr binary(FormulaTokenType type, FormulaPtr left, FormulaPtr right);
    FormulaPtr atom(const std::string& phrase);
    AtomTable& atomTable_;
    mutable std::vector<Token> tokens_;
    mutable std::size_t position_ = 0;
    std::map<std::string, std::string> atoms_;
    FormulaPtr parseIff() const;
    FormulaPtr parseImplies() const;
    FormulaPtr parseOr() const;
    FormulaPtr parseAnd() const;
    FormulaPtr parseNot() const;
};

#endif
