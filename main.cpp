#include <algorithm>
#include <cctype>
#include <iostream>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

enum class TokenType { Atom, Not, And, Or, Implies, Iff, Open, Close, End };

struct Token {
    TokenType type;
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
                tokens.push_back({TokenType::Iff, "<->"});
                i += 3;
            } else if (remaining.rfind("->", 0) == 0 || remaining.rfind("=>", 0) == 0) {
                tokens.push_back({TokenType::Implies, "->"});
                i += 2;
            } else if (remaining.rfind("&&", 0) == 0) {
                tokens.push_back({TokenType::And, "&"});
                i += 2;
            } else if (remaining.rfind("||", 0) == 0) {
                tokens.push_back({TokenType::Or, "|"});
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
                    tokens.push_back({TokenType::Atom, input.substr(i, end - i)});
                    i = end;
                } else {
                    throw std::runtime_error("Simbolo no reconocido: " + std::string(1, character));
                }
            }
        }
        tokens.push_back({TokenType::End, ""});
        return tokens;
    }

private:
    static std::optional<TokenType> symbolType(char character) {
        switch (character) {
        case '!': case '~': return TokenType::Not;
        case '&': return TokenType::And;
        case '|': return TokenType::Or;
        case '(': return TokenType::Open;
        case ')': return TokenType::Close;
        default: return std::nullopt;
        }
    }
};

class Formula {
public:
    virtual ~Formula() = default;
    virtual bool evaluate(const std::map<std::string, bool>& model) const = 0;
    virtual void collectAtoms(std::set<std::string>& atoms) const = 0;
    virtual std::string print(int parentPrecedence = 0) const = 0;
    virtual void printTree(const std::string& prefix, bool last) const = 0;
};

using FormulaPtr = std::shared_ptr<const Formula>;

class Atom final : public Formula {
public:
    explicit Atom(std::string name) : name_(std::move(name)) {}

    bool evaluate(const std::map<std::string, bool>& model) const override { return model.at(name_); }
    void collectAtoms(std::set<std::string>& atoms) const override { atoms.insert(name_); }
    std::string print(int = 0) const override { return name_; }

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
        const std::string text = "!" + child_->print(4);
        return 4 < parentPrecedence ? "(" + text + ")" : text;
    }

    void printTree(const std::string& prefix, bool last) const override {
        std::cout << prefix << (last ? "`-- NOT\n" : "|-- NOT\n");
        child_->printTree(prefix + (last ? "    " : "|   "), true);
    }

private:
    FormulaPtr child_;
};

class BinaryFormula final : public Formula {
public:
    BinaryFormula(TokenType type, FormulaPtr left, FormulaPtr right)
        : type_(type), left_(std::move(left)), right_(std::move(right)) {}

    bool evaluate(const std::map<std::string, bool>& model) const override {
        const bool left = left_->evaluate(model);
        const bool right = right_->evaluate(model);
        switch (type_) {
        case TokenType::And: return left && right;
        case TokenType::Or: return left || right;
        case TokenType::Implies: return !left || right;
        case TokenType::Iff: return left == right;
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

private:
    static int precedenceOf(TokenType type) {
        switch (type) {
        case TokenType::Iff: return 1;
        case TokenType::Implies: return 2;
        case TokenType::Or: return 3;
        case TokenType::And: return 4;
        default: return 0;
        }
    }

    static std::string symbolOf(TokenType type) {
        switch (type) {
        case TokenType::And: return "&";
        case TokenType::Or: return "|";
        case TokenType::Implies: return "->";
        case TokenType::Iff: return "<->";
        default: return "?";
        }
    }

    TokenType type_;
    FormulaPtr left_;
    FormulaPtr right_;
};

class Parser {
public:
    explicit Parser(std::vector<Token> tokens) : tokens_(std::move(tokens)) {}

    FormulaPtr parse() {
        FormulaPtr formula = parseIff();
        if (peek().type != TokenType::End) throw std::runtime_error("Texto despues de la formula");
        return formula;
    }

private:
    const Token& peek() const { return tokens_[position_]; }

    bool match(TokenType type) {
        if (peek().type != type) return false;
        ++position_;
        return true;
    }

    void expect(TokenType type, const std::string& message) {
        if (!match(type)) throw std::runtime_error(message);
    }

    FormulaPtr parseIff() {
        FormulaPtr formula = parseImplies();
        while (match(TokenType::Iff)) formula = std::make_shared<BinaryFormula>(TokenType::Iff, formula, parseImplies());
        return formula;
    }

    FormulaPtr parseImplies() {
        FormulaPtr left = parseOr();
        if (match(TokenType::Implies)) return std::make_shared<BinaryFormula>(TokenType::Implies, left, parseImplies());
        return left;
    }

    FormulaPtr parseOr() {
        FormulaPtr formula = parseAnd();
        while (match(TokenType::Or)) formula = std::make_shared<BinaryFormula>(TokenType::Or, formula, parseAnd());
        return formula;
    }

    FormulaPtr parseAnd() {
        FormulaPtr formula = parseNot();
        while (match(TokenType::And)) formula = std::make_shared<BinaryFormula>(TokenType::And, formula, parseNot());
        return formula;
    }

    FormulaPtr parseNot() {
        if (match(TokenType::Not)) return std::make_shared<UnaryFormula>(parseNot());
        if (match(TokenType::Open)) {
            FormulaPtr formula = parseIff();
            expect(TokenType::Close, "Falta cerrar parentesis");
            return formula;
        }
        if (peek().type != TokenType::Atom) throw std::runtime_error("Se esperaba una proposicion atomica");
        return std::make_shared<Atom>(tokens_[position_++].text);
    }

    std::vector<Token> tokens_;
    std::size_t position_ = 0;
};

class ModelFinder {
public:
    std::vector<std::map<std::string, bool>> find(const Formula& formula, const std::vector<std::string>& atoms) const {
        std::vector<std::map<std::string, bool>> models;
        const std::size_t combinations = static_cast<std::size_t>(1) << atoms.size();
        for (std::size_t mask = 0; mask < combinations; ++mask) {
            std::map<std::string, bool> model;
            for (std::size_t i = 0; i < atoms.size(); ++i) model[atoms[i]] = (mask & (static_cast<std::size_t>(1) << i)) != 0;
            if (formula.evaluate(model)) models.push_back(model);
        }
        return models;
    }
};

static std::string truth(bool value) { return value ? "V" : "F"; }

int main() {
    std::cout << "SAT-SOLVER-24110242 - Solver proposicional en C++\n";
    std::cout << "Conectores: ! o ~ (no), & (y), | (o), -> (implica), <-> (equivalencia)\n";
    std::cout << "Escribe una formula o 'salir'.\n\n";

    Lexer lexer;
    std::string input;
    while (true) {
        std::cout << "Formula> ";
        if (!std::getline(std::cin, input) || input == "salir") break;
        if (input.empty()) continue;

        try {
            FormulaPtr formula = Parser(lexer.tokenize(input)).parse();
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
        } catch (const std::exception& error) {
            std::cout << "Error: " << error.what() << "\n\n";
        }
    }
    return 0;
}