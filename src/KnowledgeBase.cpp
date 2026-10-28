#include "KnowledgeBase.h"
#include <utility>

void KnowledgeBase::addSentence(const std::string& sentence, const FormulaPtr& formula) {
    sentences_.push_back(sentence);
    formula_ = formula_ ? std::make_shared<BinaryFormula>(FormulaTokenType::And, formula_, formula) : formula;
}
const std::vector<std::string>& KnowledgeBase::sentences() const { return sentences_; }
FormulaPtr KnowledgeBase::formula() const { return formula_; }
