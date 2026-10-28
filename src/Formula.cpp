#include "Formula.h"

#include <iostream>
#include <stdexcept>
#include <utility>

Atom::Atom(std::string name) : name_(std::move(name)) {}
bool Atom::evaluate(const std::map<std::string, bool>& model) const { return model.at(name_); }
void Atom::collectAtoms(std::set<std::string>& atoms) const { atoms.insert(name_); }
std::string Atom::print(int) const { return name_; }
void Atom::printTree(const std::string& prefix, bool last) const {
    std::cout << prefix << (last ? "`-- " : "|-- ") << name_ << "\n";
}
std::string Atom::graphLabel() const { return name_; }
std::vector<FormulaPtr> Atom::graphChildren() const { return {}; }

UnaryFormula::UnaryFormula(FormulaPtr child) : child_(std::move(child)) {}
bool UnaryFormula::evaluate(const std::map<std::string, bool>& model) const { return !child_->evaluate(model); }
void UnaryFormula::collectAtoms(std::set<std::string>& atoms) const { child_->collectAtoms(atoms); }
std::string UnaryFormula::print(int parentPrecedence) const {
    const std::string text = "\xC2\xAC" + child_->print(4);
    return 4 < parentPrecedence ? "(" + text + ")" : text;
}
void UnaryFormula::printTree(const std::string& prefix, bool last) const {
    std::cout << prefix << (last ? "`-- NOT\n" : "|-- NOT\n");
    child_->printTree(prefix + (last ? "    " : "|   "), true);
}
std::string UnaryFormula::graphLabel() const { return "\xC2\xAC"; }
std::vector<FormulaPtr> UnaryFormula::graphChildren() const { return {child_}; }

BinaryFormula::BinaryFormula(FormulaTokenType type, FormulaPtr left, FormulaPtr right)
    : type_(type), left_(std::move(left)), right_(std::move(right)) {}
bool BinaryFormula::evaluate(const std::map<std::string, bool>& model) const {
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
void BinaryFormula::collectAtoms(std::set<std::string>& atoms) const {
    left_->collectAtoms(atoms);
    right_->collectAtoms(atoms);
}
std::string BinaryFormula::print(int parentPrecedence) const {
    const int precedence = precedenceOf(type_);
    const std::string text = left_->print(precedence) + " " + symbolOf(type_) + " " + right_->print(precedence);
    return precedence < parentPrecedence ? "(" + text + ")" : text;
}
void BinaryFormula::printTree(const std::string& prefix, bool last) const {
    std::cout << prefix << (last ? "`-- " : "|-- ") << symbolOf(type_) << "\n";
    left_->printTree(prefix + (last ? "    " : "|   "), false);
    right_->printTree(prefix + (last ? "    " : "|   "), true);
}
std::string BinaryFormula::graphLabel() const { return symbolOf(type_); }
std::vector<FormulaPtr> BinaryFormula::graphChildren() const { return {left_, right_}; }
int BinaryFormula::precedenceOf(FormulaTokenType type) {
    switch (type) {
    case FormulaTokenType::Iff: return 1;
    case FormulaTokenType::Implies: return 2;
    case FormulaTokenType::Or: return 3;
    case FormulaTokenType::And: return 4;
    default: return 0;
    }
}
std::string BinaryFormula::symbolOf(FormulaTokenType type) {
    switch (type) {
    case FormulaTokenType::And: return "\xE2\x88\xA7";
    case FormulaTokenType::Or: return "\xE2\x88\xA8";
    case FormulaTokenType::Implies: return "\xE2\x86\x92";
    case FormulaTokenType::Iff: return "\xE2\x86\x94";
    default: return "?";
    }
}
