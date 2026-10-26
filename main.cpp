#include <algorithm>
#include <cctype>
#include <cmath>
#include <iostream>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#ifdef SAT_HAS_SFML
#include <SFML/Graphics.hpp>
#ifdef _WIN32
#include <windows.h>
#endif
#endif

enum class FormulaTokenType { Atom, Not, And, Or, Implies, Iff, Open, Close, End };

struct Token {
    FormulaTokenType type;
    std::string text;
};

class Lexer {
public:
    std::vector<Token> tokenize(const std::string& input) const {
        std::vector<Token> tokens;
        for (std::size_t i = 0; i < input.size();) {
            if (std::isspace(static_cast<unsigned char>(input[i]))) {
                ++i;
                continue;
            }
            const std::string remaining = input.substr(i);
            if (remaining.rfind("<->", 0) == 0 || remaining.rfind("<=>", 0) == 0) {
                tokens.push_back({FormulaTokenType::Iff, "<->"});
                i += 3;
            } else if (remaining.rfind("->", 0) == 0 || remaining.rfind("=>", 0) == 0) {
                tokens.push_back({FormulaTokenType::Implies, "->"});
                i += 2;
            } else if (remaining.rfind("&&", 0) == 0) {
                tokens.push_back({FormulaTokenType::And, "&"});
                i += 2;
            } else if (remaining.rfind("||", 0) == 0) {
                tokens.push_back({FormulaTokenType::Or, "|"});
                i += 2;
            } else {
                const char character = input[i];
                const auto type = symbolType(character);
                if (type.has_value()) {
                    tokens.push_back({*type, std::string(1, character)});
                    ++i;
                } else if (std::isalpha(static_cast<unsigned char>(character))) {
                    std::size_t end = i + 1;
                    while (end < input.size() &&
                           (std::isalnum(static_cast<unsigned char>(input[end])) || input[end] == '_')) {
                        ++end;
                    }
                    tokens.push_back({FormulaTokenType::Atom, input.substr(i, end - i)});
                    i = end;
                } else {
                    throw std::runtime_error("Simbolo no reconocido: " + std::string(1, character));
                }
            }
        }
        tokens.push_back({FormulaTokenType::End, ""});
        return tokens;
    }

private:
    static std::optional<FormulaTokenType> symbolType(char character) {
        switch (character) {
        case '!': case '~': return FormulaTokenType::Not;
        case '&': return FormulaTokenType::And;
        case '|': return FormulaTokenType::Or;
        case '(': return FormulaTokenType::Open;
        case ')': return FormulaTokenType::Close;
        default: return std::nullopt;
        }
    }
};

class Formula;
using FormulaPtr = std::shared_ptr<const Formula>;

class Formula {
public:
    virtual ~Formula() = default;
    virtual bool evaluate(const std::map<std::string, bool>& model) const = 0;
    virtual void collectAtoms(std::set<std::string>& atoms) const = 0;
    virtual std::string print(int parentPrecedence = 0) const = 0;
    virtual void printTree(const std::string& prefix, bool last) const = 0;
    virtual std::string graphLabel() const = 0;
    virtual std::vector<FormulaPtr> graphChildren() const = 0;
};

class Atom final : public Formula {
public:
    explicit Atom(std::string name) : name_(std::move(name)) {}

    bool evaluate(const std::map<std::string, bool>& model) const override { return model.at(name_); }
    void collectAtoms(std::set<std::string>& atoms) const override { atoms.insert(name_); }
    std::string print(int = 0) const override { return name_; }
    std::string graphLabel() const override { return name_; }
    std::vector<FormulaPtr> graphChildren() const override { return {}; }

    void printTree(const std::string& prefix, bool last) const override {
        std::cout << prefix << (last ? "`-- " : "|-- ") << name_ << "\n";
    }

private:
    std::string name_;
};

class UnaryFormula final : public Formula {
public:
    explicit UnaryFormula(FormulaPtr child) : child_(std::move(child)) {}

    bool evaluate(const std::map<std::string, bool>& model) const override { return !child_->evaluate(model); }
    void collectAtoms(std::set<std::string>& atoms) const override { child_->collectAtoms(atoms); }

    std::string print(int parentPrecedence = 0) const override {
        const std::string text = "¬" + child_->print(4);
        return 4 < parentPrecedence ? "(" + text + ")" : text;
    }

    void printTree(const std::string& prefix, bool last) const override {
        std::cout << prefix << (last ? "`-- NOT\n" : "|-- NOT\n");
        child_->printTree(prefix + (last ? "    " : "|   "), true);
    }

    std::string graphLabel() const override { return "¬"; }
    std::vector<FormulaPtr> graphChildren() const override { return {child_}; }

private:
    FormulaPtr child_;
};

class BinaryFormula final : public Formula {
public:
    BinaryFormula(FormulaTokenType type, FormulaPtr left, FormulaPtr right)
        : type_(type), left_(std::move(left)), right_(std::move(right)) {}

    bool evaluate(const std::map<std::string, bool>& model) const override {
        const bool left = left_->evaluate(model);
        const bool right = right_->evaluate(model);
        switch (type_) {
        case FormulaTokenType::And: return left && right;
        case FormulaTokenType::Or: return left || right;
        case FormulaTokenType::Implies: return !left || right;
        case FormulaTokenType::Iff: return left == right;
        default: throw std::logic_error("Operador binario invalido");
        }
    }

    void collectAtoms(std::set<std::string>& atoms) const override {
        left_->collectAtoms(atoms);
        right_->collectAtoms(atoms);
    }

    std::string print(int parentPrecedence = 0) const override {
        const int precedence = precedenceOf(type_);
        const std::string text = left_->print(precedence) + " " + symbolOf(type_) + " " + right_->print(precedence);
        return precedence < parentPrecedence ? "(" + text + ")" : text;
    }

    void printTree(const std::string& prefix, bool last) const override {
        std::cout << prefix << (last ? "`-- " : "|-- ") << symbolOf(type_) << "\n";
        left_->printTree(prefix + (last ? "    " : "|   "), false);
        right_->printTree(prefix + (last ? "    " : "|   "), true);
    }

    std::string graphLabel() const override { return symbolOf(type_); }
    std::vector<FormulaPtr> graphChildren() const override { return {left_, right_}; }

private:
    static int precedenceOf(FormulaTokenType type) {
        switch (type) {
        case FormulaTokenType::Iff: return 1;
        case FormulaTokenType::Implies: return 2;
        case FormulaTokenType::Or: return 3;
        case FormulaTokenType::And: return 4;
        default: return 0;
        }
    }

    static std::string symbolOf(FormulaTokenType type) {
        switch (type) {
        case FormulaTokenType::And: return "∧";
        case FormulaTokenType::Or: return "∨";
        case FormulaTokenType::Implies: return "→";
        case FormulaTokenType::Iff: return "↔";
        default: return "?";
        }
    }

    FormulaTokenType type_;
    FormulaPtr left_;
    FormulaPtr right_;
};

class Parser {
public:
    explicit Parser(std::vector<Token> tokens) : tokens_(std::move(tokens)) {}

    FormulaPtr parse() {
        FormulaPtr formula = parseIff();
        if (peek().type != FormulaTokenType::End) throw std::runtime_error("Texto despues de la formula");
        return formula;
    }

private:
    const Token& peek() const { return tokens_[position_]; }

    bool match(FormulaTokenType type) {
        if (peek().type != type) return false;
        ++position_;
        return true;
    }

    void expect(FormulaTokenType type, const std::string& message) {
        if (!match(type)) throw std::runtime_error(message);
    }

    FormulaPtr parseIff() {
        FormulaPtr formula = parseImplies();
        while (match(FormulaTokenType::Iff)) formula = std::make_shared<BinaryFormula>(FormulaTokenType::Iff, formula, parseImplies());
        return formula;
    }

    FormulaPtr parseImplies() {
        FormulaPtr left = parseOr();
        if (match(FormulaTokenType::Implies)) return std::make_shared<BinaryFormula>(FormulaTokenType::Implies, left, parseImplies());
        return left;
    }

    FormulaPtr parseOr() {
        FormulaPtr formula = parseAnd();
        while (match(FormulaTokenType::Or)) formula = std::make_shared<BinaryFormula>(FormulaTokenType::Or, formula, parseAnd());
        return formula;
    }

    FormulaPtr parseAnd() {
        FormulaPtr formula = parseNot();
        while (match(FormulaTokenType::And)) formula = std::make_shared<BinaryFormula>(FormulaTokenType::And, formula, parseNot());
        return formula;
    }

    FormulaPtr parseNot() {
        if (match(FormulaTokenType::Not)) return std::make_shared<UnaryFormula>(parseNot());
        if (match(FormulaTokenType::Open)) {
            FormulaPtr formula = parseIff();
            expect(FormulaTokenType::Close, "Falta cerrar parentesis");
            return formula;
        }
        if (peek().type != FormulaTokenType::Atom) throw std::runtime_error("Se esperaba una proposicion atomica");
        return std::make_shared<Atom>(tokens_[position_++].text);
    }

    std::vector<Token> tokens_;
    std::size_t position_ = 0;
};

class NaturalLanguageTranslator {
public:
    FormulaPtr translate(const std::string& sentence) {
        atoms_.clear();
        return parseExpression(clean(sentence));
    }

    const std::map<std::string, std::string>& atoms() const { return atoms_; }

private:
    static std::string clean(std::string text) {
        std::transform(text.begin(), text.end(), text.begin(), [](unsigned char character) {
            return static_cast<char>(std::tolower(character));
        });
        for (char& character : text) {
            if (std::ispunct(static_cast<unsigned char>(character)) && character != '-') character = ' ';
        }
        std::string result;
        bool previousSpace = true;
        for (char character : text) {
            if (std::isspace(static_cast<unsigned char>(character))) {
                if (!previousSpace) result += ' ';
                previousSpace = true;
            } else {
                result += character;
                previousSpace = false;
            }
        }
        if (!result.empty() && result.back() == ' ') result.pop_back();
        return result;
    }

    static std::string trim(const std::string& text) {
        const std::size_t first = text.find_first_not_of(' ');
        if (first == std::string::npos) return {};
        const std::size_t last = text.find_last_not_of(' ');
        return text.substr(first, last - first + 1);
    }

    FormulaPtr parseExpression(const std::string& text) {
        const std::string value = trim(text);
        if (value.empty()) throw std::runtime_error("La frase no contiene una proposicion");
        if (value.rfind("no ", 0) == 0) return std::make_shared<UnaryFormula>(parseExpression(value.substr(3)));

        const std::size_t exclusiveOr = value.find(" o ");
        if (exclusiveOr != std::string::npos &&
            value.find(" pero no a ambos", exclusiveOr) != std::string::npos) {
            const std::string left = value.substr(0, exclusiveOr);
            const std::size_t exclusion = value.find(" pero no a ambos", exclusiveOr + 3);
            const std::string right = value.substr(exclusiveOr + 3, exclusion - exclusiveOr - 3);
            const FormulaPtr either = binary(FormulaTokenType::Or, left, right);
            const FormulaPtr both = binary(FormulaTokenType::And, left, right);
            return std::make_shared<BinaryFormula>(FormulaTokenType::And, either,
                std::make_shared<UnaryFormula>(both));
        }

        if (value.find("si hace calor y no llueve") == 0 && value.find("si llueve") != std::string::npos) {
            const FormulaPtr heat = atom("hace calor");
            const FormulaPtr rain = atom("llueve");
            const FormulaPtr beach = atom("voy a la playa");
            const FormulaPtr pool = atom("voy a la alberca");
            const FormulaPtr home = atom("me quedo en casa");
            const FormulaPtr movie = atom("veo una pelicula");
            const FormulaPtr firstCondition = binary(FormulaTokenType::And, heat,
                std::make_shared<UnaryFormula>(rain));
            const FormulaPtr firstResult = binary(FormulaTokenType::Or, beach, pool);
            const FormulaPtr secondResult = binary(FormulaTokenType::And, home, movie);
            return binary(FormulaTokenType::And,
                binary(FormulaTokenType::Implies, firstCondition, firstResult),
                binary(FormulaTokenType::Implies, rain, secondResult));
        }

        if (value.find("si el sensor detecta humo o la temperatura es alta") == 0) {
            const FormulaPtr smoke = atom("el sensor detecta humo");
            const FormulaPtr hot = atom("la temperatura es alta");
            const FormulaPtr alarm = atom("se activa la alarma");
            const FormulaPtr drill = atom("es simulacro");
            const FormulaPtr fire = atom("se llama a los bomberos");
            const FormulaPtr evacuate = atom("se evacua el edificio");
            const FormulaPtr alarmCondition = binary(FormulaTokenType::Or, smoke, hot);
            const FormulaPtr alarmAndNoDrill = binary(FormulaTokenType::And, alarm,
                std::make_shared<UnaryFormula>(drill));
            const FormulaPtr clauses = binary(FormulaTokenType::And,
                binary(FormulaTokenType::Implies, alarmCondition, alarm),
                binary(FormulaTokenType::And,
                    binary(FormulaTokenType::Implies, alarmAndNoDrill, fire),
                    binary(FormulaTokenType::And,
                        binary(FormulaTokenType::Implies, drill,
                               std::make_shared<UnaryFormula>(fire)),
                        binary(FormulaTokenType::Implies, fire, evacuate))));
            return clauses;
        }

        if (value.find("si hay presupuesto y hay personal") == 0) {
            const FormulaPtr budget = atom("hay presupuesto");
            const FormulaPtr staff = atom("hay personal");
            const FormulaPtr starts = atom("el proyecto inicia");
            const FormulaPtr buys = atom("se compra equipo");
            const FormulaPtr rents = atom("se renta equipo");
            const FormulaPtr training = atom("se necesita capacitacion");
            const FormulaPtr startsCondition = binary(FormulaTokenType::And, budget, staff);
            const FormulaPtr either = binary(FormulaTokenType::Or, buys, rents);
            const FormulaPtr both = binary(FormulaTokenType::And, buys, rents);
            const FormulaPtr exclusive = binary(FormulaTokenType::And, either,
                std::make_shared<UnaryFormula>(both));
            const FormulaPtr clauses = binary(FormulaTokenType::And,
                binary(FormulaTokenType::Implies, startsCondition, starts),
                binary(FormulaTokenType::And,
                    binary(FormulaTokenType::Implies, starts, exclusive),
                    binary(FormulaTokenType::And,
                        binary(FormulaTokenType::Implies, buys, training),
                        binary(FormulaTokenType::And,
                            binary(FormulaTokenType::Implies, rents,
                                   std::make_shared<UnaryFormula>(training)),
                            budget))));
            return clauses;
        }
        if (const auto split = findConnector(value, {" si y solo si ", " equivale a "}); split.has_value())
            return binary(FormulaTokenType::Iff, value.substr(0, split->first), value.substr(split->first + split->second.size()));
        const std::size_t thenPosition = value.find(" entonces ");
        if (value.rfind("si ", 0) == 0 && thenPosition != std::string::npos)
            return binary(FormulaTokenType::Implies, value.substr(3, thenPosition - 3), value.substr(thenPosition + 10));
        if (const auto split = findConnector(value, {" y ", " pero ", " aunque "}); split.has_value())
            return binary(FormulaTokenType::And, value.substr(0, split->first), value.substr(split->first + split->second.size()));
        if (const auto split = findConnector(value, {" o ", " o bien "}); split.has_value())
            return binary(FormulaTokenType::Or, value.substr(0, split->first), value.substr(split->first + split->second.size()));
        return atom(value);
    }

    std::optional<std::pair<std::size_t, std::string>> findConnector(
        const std::string& text, const std::vector<std::string>& connectors) const {
        std::optional<std::pair<std::size_t, std::string>> result;
        for (const std::string& connector : connectors) {
            const std::size_t position = text.find(connector);
            if (position != std::string::npos && (!result.has_value() || position < result->first))
                result = std::make_pair(position, connector);
        }
        return result;
    }

    FormulaPtr binary(FormulaTokenType type, const std::string& left, const std::string& right) {
        return std::make_shared<BinaryFormula>(type, parseExpression(left), parseExpression(right));
    }

    static FormulaPtr binary(FormulaTokenType type, FormulaPtr left, FormulaPtr right) {
        return std::make_shared<BinaryFormula>(type, std::move(left), std::move(right));
    }

    FormulaPtr atom(const std::string& phrase) {
        const auto found = atoms_.find(phrase);
        if (found != atoms_.end()) return std::make_shared<Atom>(found->second);
        const std::string name = "p" + std::to_string(atoms_.size() + 1);
        atoms_[phrase] = name;
        return std::make_shared<Atom>(name);
    }

    std::map<std::string, std::string> atoms_;
};

#ifndef SAT_HAS_SFML
static std::string truth(bool value) { return value ? "V" : "F"; }
#endif

class ModelFinder {
public:
    using Assignment = std::map<std::string, bool>;

    std::vector<std::pair<Assignment, bool>> evaluateAll(const Formula& formula,
                                                          const std::vector<std::string>& atoms) const {
        std::vector<std::pair<Assignment, bool>> evaluations;
        const std::size_t combinations = static_cast<std::size_t>(1) << atoms.size();
        for (std::size_t mask = 0; mask < combinations; ++mask) {
            Assignment model;
            for (std::size_t i = 0; i < atoms.size(); ++i)
                model[atoms[i]] = (mask & (static_cast<std::size_t>(1) << i)) != 0;
            evaluations.push_back({model, formula.evaluate(model)});
        }
        return evaluations;
    }

    std::vector<std::map<std::string, bool>> find(const Formula& formula, const std::vector<std::string>& atoms) const {
        std::vector<std::map<std::string, bool>> models;
        for (const auto& evaluation : evaluateAll(formula, atoms))
            if (evaluation.second) models.push_back(evaluation.first);
        return models;
    }
};

#ifdef SAT_HAS_SFML
 #ifdef _WIN32
static std::string clipboardText() {
    if (!OpenClipboard(nullptr)) return {};
    std::string result;
    if (HANDLE handle = GetClipboardData(CF_UNICODETEXT)) {
        const wchar_t* text = static_cast<const wchar_t*>(GlobalLock(handle));
        if (text) {
            const int size = WideCharToMultiByte(CP_UTF8, 0, text, -1, nullptr, 0, nullptr, nullptr);
            if (size > 1) {
                result.resize(static_cast<std::size_t>(size));
                WideCharToMultiByte(CP_UTF8, 0, text, -1, result.data(), size, nullptr, nullptr);
                result.pop_back();
            }
            GlobalUnlock(handle);
        }
    }
    CloseClipboard();
    return result;
}

static void copyToClipboard(const std::string& value) {
    const int size = MultiByteToWideChar(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), nullptr, 0);
    if (size <= 0 || !OpenClipboard(nullptr)) return;
    EmptyClipboard();
    HGLOBAL memory = GlobalAlloc(GMEM_MOVEABLE, static_cast<std::size_t>(size + 1) * sizeof(wchar_t));
    if (memory) {
        wchar_t* target = static_cast<wchar_t*>(GlobalLock(memory));
        MultiByteToWideChar(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), target, size);
        target[size] = L'\0';
        GlobalUnlock(memory);
        SetClipboardData(CF_UNICODETEXT, memory);
    }
    CloseClipboard();
}
 #endif

static const sf::Font* logicalSymbolFont = nullptr;

struct Analysis {
    FormulaPtr formula;
    std::vector<std::string> atoms;
    std::vector<std::map<std::string, bool>> models;
    std::vector<std::pair<std::map<std::string, bool>, bool>> evaluations;
    std::string classification;
    std::string error;
};

static Analysis analyze(const std::string& input, Lexer& lexer) {
    Analysis result;
    try {
        NaturalLanguageTranslator translator;
        const bool symbolic = input.find('&') != std::string::npos || input.find('|') != std::string::npos ||
                              input.find("->") != std::string::npos || input.find('!') != std::string::npos ||
                              input.find('~') != std::string::npos;
        result.formula = symbolic ? Parser(lexer.tokenize(input)).parse() : translator.translate(input);
        std::set<std::string> atomSet;
        result.formula->collectAtoms(atomSet);
        result.atoms.assign(atomSet.begin(), atomSet.end());
        result.evaluations = ModelFinder().evaluateAll(*result.formula, result.atoms);
        result.models = ModelFinder().find(*result.formula, result.atoms);
        if (result.models.empty()) result.classification = "CONTRADICCION";
        else if (result.models.size() == result.evaluations.size()) result.classification = "TAUTOLOGIA";
        else result.classification = "CONTINGENCIA";
    } catch (const std::exception& exception) {
        result.error = exception.what();
    }
    return result;
}

static void drawText(sf::RenderWindow& window, const sf::Font& font, const std::string& value,
                     float x, float y, unsigned int size, sf::Color color) {
    const sf::String unicode = sf::String::fromUtf8(value.begin(), value.end());
    const bool hasUnicode = std::any_of(value.begin(), value.end(), [](unsigned char character) {
        return character >= 128;
    });
    sf::Text text(hasUnicode && logicalSymbolFont ? *logicalSymbolFont : font, unicode, size);
    text.setPosition(sf::Vector2f(x, y));
    text.setFillColor(color);
    window.draw(text);
}

static void drawTreeEdge(sf::RenderWindow& window, sf::Vector2f from, sf::Vector2f to, sf::Color color) {
    sf::VertexArray edge(sf::PrimitiveType::Lines, 2);
    edge[0].position = from;
    edge[0].color = color;
    edge[1].position = to;
    edge[1].color = color;
    window.draw(edge);
}

static void drawNodeConnection(sf::RenderWindow& window, sf::Vector2f from, sf::Vector2f to,
                               float radius, sf::Color color) {
    const sf::Vector2f difference = to - from;
    const float length = std::sqrt(difference.x * difference.x + difference.y * difference.y);
    if (length <= radius * 2.f) return;
    const sf::Vector2f direction = difference / length;
    drawTreeEdge(window, from + direction * radius, to - direction * radius, color);
}

static void drawFormulaTree(sf::RenderWindow& window, const sf::Font& font, const FormulaPtr& formula,
                            float x, float y, float spread, float zoom) {
    const float radius = 27.f;
    const std::vector<FormulaPtr> children = formula->graphChildren();
    const float childY = y + 88.f;
    if (children.size() == 1) {
        drawNodeConnection(window, sf::Vector2f(x, y), sf::Vector2f(x, childY), radius,
                           sf::Color(95, 205, 135));
        drawFormulaTree(window, font, children[0], x, childY, spread, zoom);
    } else if (children.size() == 2) {
        const float leftX = x - spread;
        const float rightX = x + spread;
        drawNodeConnection(window, sf::Vector2f(x, y), sf::Vector2f(leftX, childY), radius,
                           sf::Color(95, 205, 135));
        drawNodeConnection(window, sf::Vector2f(x, y), sf::Vector2f(rightX, childY), radius,
                           sf::Color(95, 205, 135));
        drawFormulaTree(window, font, children[0], leftX, childY, spread * .55f, zoom);
        drawFormulaTree(window, font, children[1], rightX, childY, spread * .55f, zoom);
    }
    sf::CircleShape node(radius);
    node.setOrigin(sf::Vector2f(radius, radius));
    node.setPosition(sf::Vector2f(x, y));
    node.setFillColor(children.empty() ? sf::Color(42, 66, 82) : sf::Color(35, 91, 112));
    node.setOutlineThickness(2.f * zoom);
    node.setOutlineColor(sf::Color(210, 225, 230));
    window.draw(node);
    const float labelWidth = formula->graphLabel().size() * 9.f * zoom;
    drawText(window, font, formula->graphLabel(), x - labelWidth * .5f, y - 11.f,
             static_cast<unsigned int>(18.f * zoom), sf::Color::White);
}

static void drawSemanticTree(sf::RenderWindow& window, const sf::Font& font, const Formula& formula,
                             const std::vector<std::string>& atoms, std::map<std::string, bool>& assignment,
                             std::size_t depth, float x, float y, float spread) {
    const float radius = depth == atoms.size() ? 16.f : 19.f;
    if (depth == atoms.size()) {
        const bool valid = formula.evaluate(assignment);
        sf::CircleShape leaf(radius);
        leaf.setOrigin(sf::Vector2f(radius, radius));
        leaf.setPosition(sf::Vector2f(x, y));
        leaf.setFillColor(valid ? sf::Color(35, 122, 82) : sf::Color(145, 52, 59));
        leaf.setOutlineThickness(2.f);
        leaf.setOutlineColor(valid ? sf::Color(120, 235, 165) : sf::Color(245, 145, 145));
        window.draw(leaf);
        drawText(window, font, valid ? "OK" : "X", x - 8.f, y - 9.f, 14, sf::Color::White);
        return;
    }

    const float childY = y + 52.f;
    const float leftX = x - spread;
    const float rightX = x + spread;
    const std::string& atom = atoms[depth];
    assignment[atom] = false;
    drawTreeEdge(window, sf::Vector2f(x, y + radius), sf::Vector2f(leftX, childY - radius),
                 sf::Color(125, 135, 145));
    assignment[atom] = true;
    drawTreeEdge(window, sf::Vector2f(x, y + radius), sf::Vector2f(rightX, childY - radius),
                 sf::Color(125, 135, 145));

    sf::CircleShape node(radius);
    node.setOrigin(sf::Vector2f(radius, radius));
    node.setPosition(sf::Vector2f(x, y));
    node.setFillColor(sf::Color(35, 91, 112));
    node.setOutlineThickness(2.f);
    node.setOutlineColor(sf::Color(210, 225, 230));
    window.draw(node);
    drawText(window, font, atom, x - 10.f, y - 9.f, 13, sf::Color::White);
    drawText(window, font, "F", leftX - 25.f, y + 25.f, 12, sf::Color(210, 220, 225));
    drawText(window, font, "V", rightX + 13.f, y + 25.f, 12, sf::Color(210, 220, 225));

    assignment[atom] = false;
    drawSemanticTree(window, font, formula, atoms, assignment, depth + 1, leftX, childY, spread * .52f);
    assignment[atom] = true;
    drawSemanticTree(window, font, formula, atoms, assignment, depth + 1, rightX, childY, spread * .52f);
    assignment.erase(atom);
}

static void showGraphic(Lexer& lexer) {
    sf::RenderWindow window(sf::VideoMode(sf::Vector2u(1000, 900)), "SAT Solver - arbol semantico");
    window.setFramerateLimit(60);
    sf::Font font;
    sf::Font symbolFont;
    const bool fontLoaded = font.openFromFile("C:/Windows/Fonts/segoeui.ttf") ||
                            font.openFromFile("C:/Windows/Fonts/arial.ttf");
    const bool symbolFontLoaded = symbolFont.openFromFile("C:/Windows/Fonts/seguisym.ttf");
    logicalSymbolFont = symbolFontLoaded ? &symbolFont : nullptr;
    std::string input;
    std::string status = "Escribe una frase y presiona Enter";
    Analysis analysis;
    bool hasResult = false;
    int branchOffset = 0;
    float treeZoom = 0.72f;
    sf::Vector2f treePan(0.f, 0.f);
    bool draggingTree = false;
    sf::Vector2i lastMousePosition;
    bool phrasesOpen = false;
    bool inputActive = true;
    bool selectAll = false;
    sf::Clock cursorClock;
    const std::vector<std::string> phrases = {
        "si esta nublado entonces no ire a trabajar",
        "si llueve y hace frio entonces me quedo en casa",
        "voy al cine o al teatro pero no a ambos",
        "si estudio y duermo bien entonces apruebo el examen y mi mama se pone contenta",
        "si hace calor y no llueve entonces voy a la playa o a la alberca pero si llueve me quedo en casa y veo una pelicula",
        "si el sensor detecta humo o la temperatura es alta se activa la alarma si se activa la alarma y no es simulacro se llama a los bomberos si es simulacro no se llama a los bomberos y si se llama a los bomberos se evacua el edificio",
        "si hay presupuesto y hay personal el proyecto inicia si el proyecto inicia se compra equipo o se renta equipo pero no ambos si se compra equipo se necesita capacitacion si se renta equipo no se necesita capacitacion y hay presupuesto",
        "hace sol y no hace sol",
        "llueve o no llueve"
    };

    while (window.isOpen()) {
        while (const std::optional event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) window.close();
            if (const auto* text = event->getIf<sf::Event::TextEntered>()) {
                if (inputActive && text->unicode >= 32 && text->unicode < 127) {
                    if (selectAll) input.clear();
                    input += static_cast<char>(text->unicode);
                    selectAll = false;
                    cursorClock.restart();
                }
            }
            if (const auto* key = event->getIf<sf::Event::KeyPressed>()) {
                if (inputActive && key->control && key->code == sf::Keyboard::Key::A) {
                    selectAll = true;
                    cursorClock.restart();
                }
                if (inputActive && key->control && key->code == sf::Keyboard::Key::C) {
#ifdef _WIN32
                    copyToClipboard(input);
#endif
                }
                if (inputActive && key->control && key->code == sf::Keyboard::Key::V) {
#ifdef _WIN32
                    const std::string pasted = clipboardText();
                    if (selectAll) input.clear();
                    input += pasted;
                    selectAll = false;
                    cursorClock.restart();
#endif
                }
                if (inputActive && key->code == sf::Keyboard::Key::Backspace && !input.empty()) {
                    if (selectAll) input.clear();
                    else input.pop_back();
                    selectAll = false;
                    cursorClock.restart();
                }
                if (key->code == sf::Keyboard::Key::Escape) window.close();
                if (key->code == sf::Keyboard::Key::Equal) treeZoom = std::min(1.45f, treeZoom + 0.1f);
                if (key->code == sf::Keyboard::Key::Hyphen) treeZoom = std::max(0.35f, treeZoom - 0.1f);
                if (key->code == sf::Keyboard::Key::Enter && !input.empty()) {
                    if (input == "salir") {
                        window.close();
                    } else {
                        analysis = analyze(input, lexer);
                        hasResult = true;
                        branchOffset = 0;
                        status = analysis.error.empty() ? "Analisis completado" : "No se pudo analizar la frase";
                    }
                }
            }
            if (const auto* pressed = event->getIf<sf::Event::MouseButtonPressed>()) {
                if (pressed->button == sf::Mouse::Button::Left && pressed->position.x >= 32 &&
                    pressed->position.x <= 968 && pressed->position.y >= 115 && pressed->position.y <= 167) {
                    inputActive = true;
                    cursorClock.restart();
                }
                if (pressed->button == sf::Mouse::Button::Left && pressed->position.x >= 750 &&
                    pressed->position.x <= 968 && pressed->position.y >= 115 && pressed->position.y <= 167) {
                    phrasesOpen = !phrasesOpen;
                } else if (pressed->button == sf::Mouse::Button::Left && phrasesOpen &&
                           pressed->position.x >= 448 && pressed->position.x <= 968 &&
                           pressed->position.y >= 175 &&
                           pressed->position.y < 175 + static_cast<int>(phrases.size()) * 38) {
                    const std::size_t selected = static_cast<std::size_t>((pressed->position.y - 175) / 38);
                    input = phrases[selected];
                    analysis = analyze(input, lexer);
                    hasResult = true;
                    branchOffset = 0;
                    phrasesOpen = false;
                    status = analysis.error.empty() ? "Frase seleccionada y analizada" : "No se pudo analizar la frase";
                }
                if (pressed->button == sf::Mouse::Button::Left && pressed->position.y >= 400) {
                    inputActive = false;
                    draggingTree = true;
                    lastMousePosition = pressed->position;
                }
            }
            if (const auto* released = event->getIf<sf::Event::MouseButtonReleased>()) {
                if (released->button == sf::Mouse::Button::Left) draggingTree = false;
            }
            if (const auto* moved = event->getIf<sf::Event::MouseMoved>()) {
                if (draggingTree) {
                    treePan += sf::Vector2f(static_cast<float>(moved->position.x - lastMousePosition.x),
                                           static_cast<float>(moved->position.y - lastMousePosition.y));
                    lastMousePosition = moved->position;
                }
            }
            if (const auto* wheel = event->getIf<sf::Event::MouseWheelScrolled>()) {
                if (wheel->position.y >= 735) {
                    if (wheel->delta < 0) ++branchOffset;
                    if (wheel->delta > 0) --branchOffset;
                    if (hasResult) branchOffset = std::max(0, std::min(branchOffset,
                        std::max(0, static_cast<int>(analysis.evaluations.size()) - 4)));
                } else if (wheel->position.y >= 390) {
                    const float oldZoom = treeZoom;
                    treeZoom = std::max(0.35f, std::min(1.45f, treeZoom + wheel->delta * 0.08f));
                    const sf::Vector2f cursor(static_cast<float>(wheel->position.x),
                                              static_cast<float>(wheel->position.y));
                    const sf::Vector2f center(500.f, 425.f);
                    treePan = cursor - center - (cursor - center - treePan) * (treeZoom / oldZoom);
                }
            }
        }
        window.clear(sf::Color(24, 29, 38));
        sf::RectangleShape header(sf::Vector2f(1000.f, 92.f));
        header.setFillColor(sf::Color(35, 91, 112));
        window.draw(header);
        if (fontLoaded) {
            drawText(window, font, "SAT SOLVER", 32.f, 18.f, 30, sf::Color::White);
            drawText(window, font, "Escribe una frase en lenguaje natural", 34.f, 56.f, 17, sf::Color(210, 230, 235));
            sf::RectangleShape inputBox(sf::Vector2f(936.f, 52.f));
            inputBox.setPosition(sf::Vector2f(32.f, 115.f));
            inputBox.setFillColor(sf::Color(245, 247, 248));
            window.draw(inputBox);
            drawText(window, font, input.empty() ? "Ejemplo: si llueve entonces no salgo" : input,
                     48.f, 128.f, 19, input.empty() ? sf::Color(125, 135, 140) : sf::Color(30, 35, 40));
            if (inputActive && !selectAll && std::fmod(cursorClock.getElapsedTime().asSeconds(), 1.f) < 0.55f) {
                sf::Text inputText(font, sf::String::fromUtf8(input.begin(), input.end()), 19);
                const sf::FloatRect bounds = inputText.getLocalBounds();
                sf::RectangleShape cursor(sf::Vector2f(2.f, 25.f));
                cursor.setPosition(sf::Vector2f(48.f + bounds.position.x + bounds.size.x, 127.f));
                cursor.setFillColor(sf::Color(35, 91, 112));
                window.draw(cursor);
            }
            if (inputActive) {
                sf::RectangleShape focusLine(sf::Vector2f(936.f, 2.f));
                focusLine.setPosition(sf::Vector2f(32.f, 165.f));
                focusLine.setFillColor(sf::Color(75, 190, 220));
                window.draw(focusLine);
            }
            sf::RectangleShape phrasesButton(sf::Vector2f(208.f, 42.f));
            phrasesButton.setPosition(sf::Vector2f(752.f, 120.f));
            phrasesButton.setFillColor(sf::Color(35, 91, 112));
            window.draw(phrasesButton);
            drawText(window, font, "Librería de frases  v", 770.f, 130.f, 16, sf::Color::White);
            if (phrasesOpen) {
                sf::RectangleShape menu(sf::Vector2f(520.f, static_cast<float>(phrases.size() * 38 + 8)));
                menu.setPosition(sf::Vector2f(448.f, 174.f));
                menu.setFillColor(sf::Color(45, 54, 67));
                menu.setOutlineThickness(1.f);
                menu.setOutlineColor(sf::Color(130, 160, 175));
                window.draw(menu);
                for (std::size_t index = 0; index < phrases.size(); ++index)
                    drawText(window, font, phrases[index], 462.f, 181.f + static_cast<float>(index) * 38.f,
                             15, sf::Color::White);
            }
            drawText(window, font, status, 34.f, 180.f, 16, sf::Color(190, 205, 210));

            if (hasResult) {
                if (!analysis.error.empty()) {
                    drawText(window, font, "Error: " + analysis.error, 34.f, 225.f, 20, sf::Color(240, 120, 110));
                } else {
                    drawText(window, font, "Formula: " + analysis.formula->print(), 34.f, 225.f, 21, sf::Color::White);
                    drawText(window, font, "Atomos: " + std::to_string(analysis.atoms.size()) +
                             "     Asignaciones: " + std::to_string(analysis.evaluations.size()) +
                             "     Modelos: " + std::to_string(analysis.models.size()) +
                             "     Descartadas: " + std::to_string(analysis.evaluations.size() - analysis.models.size()),
                             34.f, 265.f, 19, sf::Color(220, 230, 235));
                    const bool tautology = analysis.classification == "TAUTOLOGIA";
                    const bool contradiction = analysis.classification == "CONTRADICCION";
                    drawText(window, font, analysis.classification,
                             34.f, 310.f, 29, contradiction ? sf::Color(245, 110, 110) : sf::Color(95, 220, 150));
                    drawText(window, font, tautology ? "Todas las asignaciones hacen verdadera la formula" :
                             (contradiction ? "Ninguna asignacion satisface la formula" :
                              "Algunas asignaciones satisfacen la formula"), 34.f, 345.f, 15, sf::Color(190, 205, 210));
                    drawText(window, font, "Arbol semantico: ramas y asignaciones", 34.f, 365.f, 20, sf::Color::White);
                    drawText(window, font, "Rueda sobre el arbol: zoom   + / -: ajustar", 665.f, 368.f, 13, sf::Color(170, 185, 195));
                    sf::RectangleShape treePanel(sf::Vector2f(936.f, 300.f));
                    treePanel.setPosition(sf::Vector2f(32.f, 400.f));
                    treePanel.setFillColor(sf::Color(31, 38, 49));
                    window.draw(treePanel);
                    sf::View treeView = window.getDefaultView();
                    treeView.setCenter(sf::Vector2f(500.f - treePan.x / treeZoom,
                                                     550.f - treePan.y / treeZoom));
                    treeView.setSize(sf::Vector2f(936.f / treeZoom, 300.f / treeZoom));
                    treeView.setViewport(sf::FloatRect(sf::Vector2f(0.032f, 0.444f),
                                                       sf::Vector2f(0.936f, 0.333f)));
                    window.setView(treeView);
                    std::map<std::string, bool> assignment;
                    drawSemanticTree(window, font, *analysis.formula, analysis.atoms, assignment, 0,
                                     500.f, 425.f, 190.f);
                    window.setView(window.getDefaultView());
                    const std::size_t discarded = analysis.evaluations.size() - analysis.models.size();
                    drawText(window, font, "Ramas semanticas: verde = modelo, rojo = rama descartada", 34.f, 720.f, 19, sf::Color::White);
                    const int visibleBranches = 4;
                    for (int row = 0; row < visibleBranches && branchOffset + row < static_cast<int>(analysis.evaluations.size()); ++row) {
                        const auto& evaluation = analysis.evaluations[static_cast<std::size_t>(branchOffset + row)];
                        const float y = 752.f + static_cast<float>(row) * 25.f;
                        sf::RectangleShape branch(sf::Vector2f(936.f, 21.f));
                        branch.setPosition(sf::Vector2f(32.f, y));
                        branch.setFillColor(evaluation.second ? sf::Color(32, 82, 61) : sf::Color(82, 43, 48));
                        window.draw(branch);
                        std::string label = evaluation.second ? "OK  " : "X   ";
                        for (const std::string& atom : analysis.atoms)
                            label += atom + "=" + (evaluation.first.at(atom) ? "V  " : "F  ");
                        label += evaluation.second ? "solucion" : "descartada";
                        drawText(window, font, label, 44.f, y + 1.f, 14,
                                 evaluation.second ? sf::Color(120, 235, 165) : sf::Color(240, 145, 145));
                    }
                    drawText(window, font, "Asignaciones: " + std::to_string(analysis.evaluations.size()) +
                             "   Modelos: " + std::to_string(analysis.models.size()) +
                             "   Descartadas: " + std::to_string(discarded), 565.f, 720.f, 14, sf::Color(175, 195, 205));
                    if (analysis.evaluations.size() > 4)
                        drawText(window, font, "Rueda abajo: recorrer ramas " +
                                 std::to_string(branchOffset + 1) + "-" +
                                 std::to_string(std::min(branchOffset + 4,
                                                          static_cast<int>(analysis.evaluations.size()))) + "/" +
                                 std::to_string(analysis.evaluations.size()), 34.f, 842.f,
                                 14, sf::Color(175, 195, 205));
                }
            }
            drawText(window, font, "Enter: analizar   Ctrl+A: seleccionar   Ctrl+C: copiar   Ctrl+V: pegar   Esc: salir", 34.f, 875.f, 15, sf::Color(155, 170, 180));
        }
        window.display();
    }
}
#endif

int main() {
#ifdef SAT_HAS_SFML
    Lexer lexer;
    showGraphic(lexer);
    return 0;
#else
    std::cout << "SAT-SOLVER-24110242 - Solver proposicional en C++\n";
    std::cout << "Escribe una frase normal o una formula simbolica. Usa 'salir' para terminar.\n";
#ifdef SAT_HAS_SFML
    std::cout << "La ventana SFML mostrara el estado del arbol semantico.\n\n";
#else
    std::cout << "SFML no esta habilitado en esta compilacion; se mostrara el arbol en consola.\n\n";
#endif

    Lexer lexer;
    std::string input;
    while (true) {
        std::cout << "Formula> ";
        if (!std::getline(std::cin, input) || input == "salir") break;
        if (input.empty()) continue;

        try {
            NaturalLanguageTranslator translator;
            FormulaPtr formula;
            const bool symbolic = input.find('&') != std::string::npos || input.find('|') != std::string::npos ||
                                  input.find("->") != std::string::npos || input.find('!') != std::string::npos ||
                                  input.find('~') != std::string::npos;
            if (symbolic) formula = Parser(lexer.tokenize(input)).parse();
            else formula = translator.translate(input);
            std::set<std::string> atomSet;
            formula->collectAtoms(atomSet);
            std::vector<std::string> atoms(atomSet.begin(), atomSet.end());
            const std::vector<std::map<std::string, bool>> models = ModelFinder().find(*formula, atoms);

            std::cout << "\nFormula: " << formula->print() << "\n";
            std::cout << "Atomos: ";
            for (std::size_t i = 0; i < atoms.size(); ++i) std::cout << (i ? ", " : "") << atoms[i];
            std::cout << "\nResultado: " << (models.empty() ? "NO SATISFACIBLE" : "SATISFACIBLE") << "\n";
            std::cout << "Modelos: " << models.size() << "\n";
            for (const auto& model : models) {
                std::cout << "  { ";
                for (std::size_t i = 0; i < atoms.size(); ++i) std::cout << (i ? ", " : "") << atoms[i] << "=" << truth(model.at(atoms[i]));
                std::cout << " }\n";
            }
            std::cout << "\nArbol semantico de la formula:\n";
            formula->printTree("", true);
            std::cout << "\n";
#ifdef SAT_HAS_SFML
            showGraphic(!models.empty());
#endif
        } catch (const std::exception& error) {
            std::cout << "Error: " << error.what() << "\n\n";
        }
    }
    return 0;
#endif
}
