#ifndef SAT_CNF_CONVERTER_H
#define SAT_CNF_CONVERTER_H

#include "Formula.h"

class CNFConverter {
public:
    FormulaPtr convert(const FormulaPtr& formula) const;
private:
    FormulaPtr eliminateIff(const FormulaPtr& formula) const;
    FormulaPtr eliminateImplies(const FormulaPtr& formula) const;
    FormulaPtr pushNegations(const FormulaPtr& formula, bool negated = false) const;
    FormulaPtr distribute(const FormulaPtr& formula) const;
};

#endif
