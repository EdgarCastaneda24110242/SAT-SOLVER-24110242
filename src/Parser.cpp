#include "Parser.h"
#include <algorithm>
#include <cctype>
#include <set>
#include <stdexcept>
#include <utility>

Parser::Parser(AtomTable& atomTable) : atomTable_(atomTable) {}
FormulaPtr Parser::parseSymbolic(const std::string& input) const {
    tokens_ = Lexer().tokenize(input);
    FormulaPtr formula = parseTokens(tokens_);
    std::set<std::string> atoms;
    formula->collectAtoms(atoms);
    for (const std::string& atomName : atoms) atomTable_.registerAtom(atomName);
    return formula;
}
FormulaPtr Parser::parseTokens(const std::vector<Token>& tokens) const {
    tokens_ = tokens; position_ = 0;
    FormulaPtr result = parseIff();
    if (tokens_[position_].type != FormulaTokenType::End) throw std::runtime_error("Texto despues de la formula");
    return result;
}
const std::map<std::string, std::string>& Parser::naturalLanguageAtoms() const { return atoms_; }
std::string Parser::clean(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    for (char& c : text) {
        if (std::ispunct(static_cast<unsigned char>(c)) && c != '-' && c != ',') c = ' ';
    }
    std::string result; bool previousSpace = true;
    for (char c : text) {
        if (c == ',') {
            if (!result.empty() && result.back() == ' ') result.pop_back();
            result += " , ";
            previousSpace = true;
            continue;
        }
        if (std::isspace(static_cast<unsigned char>(c))) { if (!previousSpace) result += ' '; previousSpace = true; }
        else { result += c; previousSpace = false; }
    }
    if (!result.empty() && result.back() == ' ') result.pop_back();
    return result;
}
std::string Parser::trim(const std::string& text) {
    const std::size_t first = text.find_first_not_of(' ');
    if (first == std::string::npos) return {};
    const std::size_t last = text.find_last_not_of(' ');
    return text.substr(first, last - first + 1);
}
FormulaPtr Parser::parseNaturalLanguage(const std::string& sentence) {
    atoms_.clear();
    return parseExpression(clean(sentence));
}
FormulaPtr Parser::parseExpression(const std::string& text) {
    const std::string value = trim(text);
    if (value.empty()) throw std::runtime_error("La frase no contiene una proposicion");
    if (value.rfind("no ", 0) == 0) return std::make_shared<UnaryFormula>(parseExpression(value.substr(3)));
    if (value.rfind("ni ", 0) == 0) {
        const std::size_t second = value.find(" ni ", 3);
        if (second != std::string::npos) {
            return binary(FormulaTokenType::And,
                std::make_shared<UnaryFormula>(parseExpression(value.substr(3, second - 3))),
                std::make_shared<UnaryFormula>(parseExpression(value.substr(second + 4))));
        }
    }
    if (value.rfind("o ", 0) == 0) {
        const std::size_t second = value.find(" o ", 2);
        if (second != std::string::npos) {
            const std::string left = value.substr(2, second - 2);
            const std::string right = value.substr(second + 3);
            return binary(FormulaTokenType::And, binary(FormulaTokenType::Or, left, right),
                          std::make_shared<UnaryFormula>(binary(FormulaTokenType::And, left, right)));
        }
    }
    const std::size_t exclusiveOr = value.find(" o ");
    if (exclusiveOr != std::string::npos && value.find(" pero no a ambos", exclusiveOr) != std::string::npos) {
        const std::size_t exclusion = value.find(" pero no a ambos", exclusiveOr + 3);
        const FormulaPtr either = binary(FormulaTokenType::Or, value.substr(0, exclusiveOr),
                                         value.substr(exclusiveOr + 3, exclusion - exclusiveOr - 3));
        const FormulaPtr both = binary(FormulaTokenType::And, value.substr(0, exclusiveOr),
                                       value.substr(exclusiveOr + 3, exclusion - exclusiveOr - 3));
        return std::make_shared<BinaryFormula>(FormulaTokenType::And, either, std::make_shared<UnaryFormula>(both));
    }
    if (value.find("si hace calor y no llueve") == 0 && value.find("si llueve") != std::string::npos) {
        const FormulaPtr heat = atom("hace calor"), rain = atom("llueve"), beach = atom("voy a la playa");
        const FormulaPtr pool = atom("voy a la alberca"), home = atom("me quedo en casa"), movie = atom("veo una pelicula");
        return binary(FormulaTokenType::And,
            binary(FormulaTokenType::Implies, binary(FormulaTokenType::And, heat, std::make_shared<UnaryFormula>(rain)),
                   binary(FormulaTokenType::Or, beach, pool)),
            binary(FormulaTokenType::Implies, rain, binary(FormulaTokenType::And, home, movie)));
    }
    if (value.find("si el sensor detecta humo o la temperatura es alta") == 0) {
        const FormulaPtr smoke = atom("el sensor detecta humo"), hot = atom("la temperatura es alta");
        const FormulaPtr alarm = atom("se activa la alarma"), drill = atom("es simulacro");
        const FormulaPtr fire = atom("se llama a los bomberos"), evacuate = atom("se evacua el edificio");
        return binary(FormulaTokenType::And, binary(FormulaTokenType::Implies, binary(FormulaTokenType::Or, smoke, hot), alarm),
            binary(FormulaTokenType::And, binary(FormulaTokenType::Implies, binary(FormulaTokenType::And, alarm, std::make_shared<UnaryFormula>(drill)), fire),
                binary(FormulaTokenType::And, binary(FormulaTokenType::Implies, drill, std::make_shared<UnaryFormula>(fire)),
                       binary(FormulaTokenType::Implies, fire, evacuate))));
    }
    if (value.find("si hay presupuesto y hay personal") == 0) {
        const FormulaPtr budget = atom("hay presupuesto"), staff = atom("hay personal"), starts = atom("el proyecto inicia");
        const FormulaPtr buys = atom("se compra equipo"), rents = atom("se renta equipo"), training = atom("se necesita capacitacion");
        const FormulaPtr exclusive = binary(FormulaTokenType::And, binary(FormulaTokenType::Or, buys, rents),
                                            std::make_shared<UnaryFormula>(binary(FormulaTokenType::And, buys, rents)));
        return binary(FormulaTokenType::And, binary(FormulaTokenType::Implies, binary(FormulaTokenType::And, budget, staff), starts),
            binary(FormulaTokenType::And, binary(FormulaTokenType::Implies, starts, exclusive),
                binary(FormulaTokenType::And, binary(FormulaTokenType::Implies, buys, training),
                    binary(FormulaTokenType::And, binary(FormulaTokenType::Implies, rents, std::make_shared<UnaryFormula>(training)), budget))));
    }
    if (value.rfind("si ", 0) == 0) {
        const std::size_t thenPosition = value.find(" entonces ");
        const std::size_t commaPosition = value.find(" , ");
        if (thenPosition != std::string::npos) {
            return binary(FormulaTokenType::Implies, value.substr(3, thenPosition - 3),
                          value.substr(thenPosition + 10));
        }
        if (commaPosition != std::string::npos) {
            std::size_t consequenceStart = commaPosition + 3;
            if (value.compare(consequenceStart, 10, "entonces ") == 0) consequenceStart += 10;
            return binary(FormulaTokenType::Implies, value.substr(3, commaPosition - 3),
                          value.substr(consequenceStart));
        }
        throw std::runtime_error(
            "No pude separar la condicion de la consecuencia; prueba con una coma o con 'entonces'");
    }
    if (const auto split = findConnector(value, {" si y solo si ", " equivale a "}); split.has_value())
        return binary(FormulaTokenType::Iff, value.substr(0, split->first), value.substr(split->first + split->second.size()));
    if (const std::size_t ifPosition = value.find(" si "); ifPosition != std::string::npos)
        return binary(FormulaTokenType::Implies, value.substr(ifPosition + 4), value.substr(0, ifPosition));
    if (const auto split = findConnector(value, {" y ", " pero ", " aunque "}); split.has_value())
        return binary(FormulaTokenType::And, value.substr(0, split->first), value.substr(split->first + split->second.size()));
    if (const auto split = findConnector(value, {" o ", " o bien "}); split.has_value())
        return binary(FormulaTokenType::Or, value.substr(0, split->first), value.substr(split->first + split->second.size()));
    return atom(value);
}
std::optional<std::pair<std::size_t, std::string>> Parser::findConnector(
    const std::string& text, const std::vector<std::string>& connectors) const {
    std::optional<std::pair<std::size_t, std::string>> result;
    for (const std::string& connector : connectors) {
        const std::size_t position = text.find(connector);
        if (position != std::string::npos && (!result.has_value() || position < result->first)) result = {position, connector};
    }
    return result;
}
FormulaPtr Parser::binary(FormulaTokenType type, const std::string& left, const std::string& right) {
    const FormulaPtr leftFormula = parseExpression(left);
    const FormulaPtr rightFormula = parseExpression(right);
    return std::make_shared<BinaryFormula>(type, leftFormula, rightFormula);
}
FormulaPtr Parser::binary(FormulaTokenType type, FormulaPtr left, FormulaPtr right) {
    return std::make_shared<BinaryFormula>(type, std::move(left), std::move(right));
}
FormulaPtr Parser::atom(const std::string& phrase) {
    const auto found = atoms_.find(phrase);
    if (found != atoms_.end()) return std::make_shared<Atom>(found->second);
    const std::string existing = atomTable_.identifierForText(phrase);
    const std::string name = existing.empty()
        ? "p" + std::to_string(atomTable_.size() + 1)
        : existing;
    atoms_[phrase] = name; atomTable_.registerAtom(name, phrase);
    return std::make_shared<Atom>(name);
}
FormulaPtr Parser::parseIff() const {
    FormulaPtr formula = parseImplies();
    while (tokens_[position_].type == FormulaTokenType::Iff) { ++position_; formula = std::make_shared<BinaryFormula>(FormulaTokenType::Iff, formula, parseImplies()); }
    return formula;
}
FormulaPtr Parser::parseImplies() const {
    FormulaPtr left = parseOr();
    if (tokens_[position_].type == FormulaTokenType::Implies) { ++position_; return std::make_shared<BinaryFormula>(FormulaTokenType::Implies, left, parseImplies()); }
    return left;
}
FormulaPtr Parser::parseOr() const {
    FormulaPtr formula = parseAnd();
    while (tokens_[position_].type == FormulaTokenType::Or) { ++position_; formula = std::make_shared<BinaryFormula>(FormulaTokenType::Or, formula, parseAnd()); }
    return formula;
}
FormulaPtr Parser::parseAnd() const {
    FormulaPtr formula = parseNot();
    while (tokens_[position_].type == FormulaTokenType::And) { ++position_; formula = std::make_shared<BinaryFormula>(FormulaTokenType::And, formula, parseNot()); }
    return formula;
}
FormulaPtr Parser::parseNot() const {
    if (tokens_[position_].type == FormulaTokenType::Not) { ++position_; return std::make_shared<UnaryFormula>(parseNot()); }
    if (tokens_[position_].type == FormulaTokenType::Open) {
        ++position_; FormulaPtr formula = parseIff();
        if (tokens_[position_].type != FormulaTokenType::Close) throw std::runtime_error("Falta cerrar parentesis");
        ++position_; return formula;
    }
    if (tokens_[position_].type != FormulaTokenType::Atom) throw std::runtime_error("Se esperaba una proposicion atomica");
    return std::make_shared<Atom>(tokens_[position_++].text);
}
