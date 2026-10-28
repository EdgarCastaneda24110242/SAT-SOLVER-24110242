#ifndef SAT_KNOWLEDGE_BASE_H
#define SAT_KNOWLEDGE_BASE_H

#include "Formula.h"
#include <string>
#include <vector>

class KnowledgeBase {
public:
    void addSentence(const std::string& sentence, const FormulaPtr& formula);
    const std::vector<std::string>& sentences() const;
    FormulaPtr formula() const;
private:
    std::vector<std::string> sentences_;
    FormulaPtr formula_;
};

#endif
