#include "SemanticTree.h"

SemanticTree::NodePtr SemanticTree::buildBranch(
    const Formula& formula, const std::vector<std::string>& atoms, std::size_t depth,
    Assignment& assignment, bool branchValue) const {
    auto node = std::make_shared<Node>();
    node->branchValue = branchValue;
    node->assignment = assignment;
    if (depth == atoms.size()) {
        node->isLeaf = true;
        node->open = formula.evaluate(assignment);
        return node;
    }

    node->atom = atoms[depth];
    assignment[node->atom] = false;
    node->falseBranch = buildBranch(formula, atoms, depth + 1, assignment, false);
    assignment[node->atom] = true;
    node->trueBranch = buildBranch(formula, atoms, depth + 1, assignment, true);
    assignment.erase(node->atom);
    return node;
}

SemanticTree::NodePtr SemanticTree::buildTree(
    const Formula& formula, const std::vector<std::string>& atoms) const {
    Assignment assignment;
    if (atoms.empty()) {
        auto root = std::make_shared<Node>();
        root->isLeaf = true;
        root->open = formula.evaluate(assignment);
        return root;
    }
    auto root = std::make_shared<Node>();
    root->atom = atoms.front();
    assignment[root->atom] = false;
    root->falseBranch = buildBranch(formula, atoms, 1, assignment, false);
    assignment[root->atom] = true;
    root->trueBranch = buildBranch(formula, atoms, 1, assignment, true);
    return root;
}

std::vector<SemanticTree::Evaluation> SemanticTree::build(
    const Formula& formula, const std::vector<std::string>& atoms) const {
    std::vector<Evaluation> evaluations;
    const NodePtr root = buildTree(formula, atoms);
    const auto visit = [&evaluations](const auto& self, const NodePtr& node) -> void {
        if (!node) return;
        if (node->isLeaf) {
            evaluations.push_back({node->assignment, node->open});
            return;
        }
        self(self, node->falseBranch);
        self(self, node->trueBranch);
    };
    visit(visit, root);
    return evaluations;
}
std::size_t SemanticTree::openLeaves(const Formula& formula, const std::vector<std::string>& atoms) const {
    std::size_t result = 0;
    for (const auto& evaluation : build(formula, atoms)) if (evaluation.second) ++result;
    return result;
}
std::size_t SemanticTree::closedLeaves(const Formula& formula, const std::vector<std::string>& atoms) const {
    return build(formula, atoms).size() - openLeaves(formula, atoms);
}
