#include "SATSolver.h"
#include <set>
#include <utility>

SATSolver::SATSolver(SemanticTree semanticTree) : semanticTree_(std::move(semanticTree)) {}
SATResult SATSolver::solve(const Formula& formula) const {
    SATResult result;
    std::set<std::string> atomSet;
    formula.collectAtoms(atomSet);
    result.atoms.assign(atomSet.begin(), atomSet.end());
    result.tree = semanticTree_.buildTree(formula, result.atoms);
    result.evaluations = semanticTree_.build(formula, result.atoms);
    for (const auto& evaluation : result.evaluations) {
        if (evaluation.second) result.models.push_back(evaluation.first);
    }
    result.modelCount = result.models.size();
    result.discardedAssignments = result.evaluations.size() - result.modelCount;
    result.satisfiable = !result.models.empty();
    return result;
}
