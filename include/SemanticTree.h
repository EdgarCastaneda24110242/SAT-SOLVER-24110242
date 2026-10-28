#ifndef SAT_SEMANTIC_TREE_H
#define SAT_SEMANTIC_TREE_H

#include "Formula.h"
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

class SemanticTree {
public:
    using Assignment = std::map<std::string, bool>;
    using Evaluation = std::pair<Assignment, bool>;
    struct Node {
        std::string atom;
        bool branchValue = false;
        bool isLeaf = false;
        bool open = false;
        Assignment assignment;
        std::shared_ptr<Node> falseBranch;
        std::shared_ptr<Node> trueBranch;
    };

    using NodePtr = std::shared_ptr<Node>;

    std::vector<Evaluation> build(const Formula& formula, const std::vector<std::string>& atoms) const;
    NodePtr buildTree(const Formula& formula, const std::vector<std::string>& atoms) const;
    std::size_t openLeaves(const Formula& formula, const std::vector<std::string>& atoms) const;
    std::size_t closedLeaves(const Formula& formula, const std::vector<std::string>& atoms) const;

private:
    NodePtr buildBranch(const Formula& formula, const std::vector<std::string>& atoms,
                        std::size_t depth, Assignment& assignment, bool branchValue) const;
};

#endif
