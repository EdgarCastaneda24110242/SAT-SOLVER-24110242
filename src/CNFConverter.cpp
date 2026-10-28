#include "CNFConverter.h"
#include <stdexcept>
#include <utility>

namespace {
FormulaPtr binary(FormulaTokenType type, FormulaPtr left, FormulaPtr right) {
    return std::make_shared<BinaryFormula>(type, std::move(left), std::move(right));
}
}

FormulaPtr CNFConverter::convert(const FormulaPtr& formula) const {
    return distribute(pushNegations(eliminateImplies(eliminateIff(formula))));
}
FormulaPtr CNFConverter::eliminateIff(const FormulaPtr& formula) const {
    if (formula->graphChildren().empty()) return formula;
    const auto children = formula->graphChildren();
    if (children.size() == 1) return std::make_shared<UnaryFormula>(eliminateIff(children[0]));
    const auto left = eliminateIff(children[0]), right = eliminateIff(children[1]);
    if (formula->graphLabel() == "\xE2\x86\x94")
        return binary(FormulaTokenType::And, binary(FormulaTokenType::Implies, left, right), binary(FormulaTokenType::Implies, right, left));
    FormulaTokenType type = formula->graphLabel() == "\xE2\x88\xA7" ? FormulaTokenType::And :
                            formula->graphLabel() == "\xE2\x88\xA8" ? FormulaTokenType::Or : FormulaTokenType::Implies;
    return binary(type, left, right);
}
FormulaPtr CNFConverter::eliminateImplies(const FormulaPtr& formula) const {
    const auto children = formula->graphChildren();
    if (children.empty()) return formula;
    if (children.size() == 1) return std::make_shared<UnaryFormula>(eliminateImplies(children[0]));
    const auto left = eliminateImplies(children[0]), right = eliminateImplies(children[1]);
    if (formula->graphLabel() == "\xE2\x86\x92")
        return binary(FormulaTokenType::Or, std::make_shared<UnaryFormula>(left), right);
    FormulaTokenType type = formula->graphLabel() == "\xE2\x88\xA7" ? FormulaTokenType::And : FormulaTokenType::Or;
    return binary(type, left, right);
}
FormulaPtr CNFConverter::pushNegations(const FormulaPtr& formula, bool negated) const {
    const auto children = formula->graphChildren();
    if (children.empty()) return negated ? std::make_shared<UnaryFormula>(formula) : formula;
    if (children.size() == 1) {
        const bool childNegated = formula->graphLabel() == "\xC2\xAC" ? !negated : negated;
        return pushNegations(children[0], childNegated);
    }
    const bool isAnd = formula->graphLabel() == "\xE2\x88\xA7";
    const FormulaTokenType type = (negated ? !isAnd : isAnd) ? FormulaTokenType::And : FormulaTokenType::Or;
    return binary(type, pushNegations(children[0], negated), pushNegations(children[1], negated));
}
FormulaPtr CNFConverter::distribute(const FormulaPtr& formula) const {
    const auto children = formula->graphChildren();
    if (children.size() != 2) return formula;
    const auto left = distribute(children[0]), right = distribute(children[1]);
    if (formula->graphLabel() != "\xE2\x88\xA8") return binary(FormulaTokenType::And, left, right);
    const auto leftChildren = left->graphChildren(), rightChildren = right->graphChildren();
    if (left->graphLabel() == "\xE2\x88\xA7")
        return binary(FormulaTokenType::And, distribute(binary(FormulaTokenType::Or, leftChildren[0], right)),
                      distribute(binary(FormulaTokenType::Or, leftChildren[1], right)));
    if (right->graphLabel() == "\xE2\x88\xA7")
        return binary(FormulaTokenType::And, distribute(binary(FormulaTokenType::Or, left, rightChildren[0])),
                      distribute(binary(FormulaTokenType::Or, left, rightChildren[1])));
    return binary(FormulaTokenType::Or, left, right);
}
