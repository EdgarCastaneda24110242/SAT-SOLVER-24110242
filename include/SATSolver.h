#ifndef SAT_SAT_SOLVER_H
#define SAT_SAT_SOLVER_H

#include "SemanticTree.h"

struct SATResult {
    bool satisfiable = false;
    std::size_t modelCount = 0;
    std::vector<SemanticTree::Assignment> models;
    std::size_t discardedAssignments = 0;
    std::vector<SemanticTree::Evaluation> evaluations;
    std::vector<std::string> atoms;
    SemanticTree::NodePtr tree;
};

class SATSolver {
public:
    explicit SATSolver(SemanticTree semanticTree = SemanticTree{});
    SATResult solve(const Formula& formula) const;
private:
    SemanticTree semanticTree_;
};

#endif
