#ifndef SAT_FORMULA_H
#define SAT_FORMULA_H

#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

enum class FormulaTokenType { Atom, Not, And, Or, Implies, Iff, Open, Close, End };

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
    explicit Atom(std::string name);
    bool evaluate(const std::map<std::string, bool>& model) const override;
    void collectAtoms(std::set<std::string>& atoms) const override;
    std::string print(int parentPrecedence = 0) const override;
    void printTree(const std::string& prefix, bool last) const override;
    std::string graphLabel() const override;
    std::vector<FormulaPtr> graphChildren() const override;
private:
    std::string name_;
};

class UnaryFormula final : public Formula {
public:
    explicit UnaryFormula(FormulaPtr child);
    bool evaluate(const std::map<std::string, bool>& model) const override;
    void collectAtoms(std::set<std::string>& atoms) const override;
    std::string print(int parentPrecedence = 0) const override;
    void printTree(const std::string& prefix, bool last) const override;
    std::string graphLabel() const override;
    std::vector<FormulaPtr> graphChildren() const override;
private:
    FormulaPtr child_;
};

class BinaryFormula final : public Formula {
public:
    BinaryFormula(FormulaTokenType type, FormulaPtr left, FormulaPtr right);
    bool evaluate(const std::map<std::string, bool>& model) const override;
    void collectAtoms(std::set<std::string>& atoms) const override;
    std::string print(int parentPrecedence = 0) const override;
    void printTree(const std::string& prefix, bool last) const override;
    std::string graphLabel() const override;
    std::vector<FormulaPtr> graphChildren() const override;
private:
    static int precedenceOf(FormulaTokenType type);
    static std::string symbolOf(FormulaTokenType type);
    FormulaTokenType type_;
    FormulaPtr left_;
    FormulaPtr right_;
};

#endif
