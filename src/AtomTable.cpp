#include "AtomTable.h"
#include <fstream>
#include <stdexcept>

std::size_t AtomTable::registerAtom(const std::string& name) {
    return registerAtom(name, name);
}
std::size_t AtomTable::registerAtom(const std::string& name, const std::string& naturalText) {
    const auto found = ids_.find(name);
    if (found != ids_.end()) {
        names_[found->second] = naturalText;
        return found->second;
    }
    const std::size_t newId = names_.size();
    ids_[name] = newId;
    names_.push_back(naturalText);
    return newId;
}
const std::string& AtomTable::text(const std::string& atomName) const {
    return name(id(atomName));
}
std::string AtomTable::identifier(std::size_t atomId) const {
    if (atomId >= names_.size()) throw std::out_of_range("Id de atomo invalido");
    return "p" + std::to_string(atomId + 1);
}
std::string AtomTable::identifierForText(const std::string& naturalText) const {
    for (std::size_t i = 0; i < names_.size(); ++i)
        if (names_[i] == naturalText) return identifier(i);
    return {};
}
bool AtomTable::contains(const std::string& name) const { return ids_.find(name) != ids_.end(); }
std::size_t AtomTable::size() const { return names_.size(); }
const std::string& AtomTable::name(std::size_t atomId) const {
    if (atomId >= names_.size()) throw std::out_of_range("Id de atomo invalido");
    return names_[atomId];
}
std::size_t AtomTable::id(const std::string& atomName) const {
    const auto found = ids_.find(atomName);
    if (found == ids_.end()) throw std::out_of_range("Atomo no registrado: " + atomName);
    return found->second;
}
void AtomTable::clear() { ids_.clear(); names_.clear(); }
bool AtomTable::save(const std::string& path) const {
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    if (!output) return false;
    for (std::size_t i = 0; i < names_.size(); ++i)
        output << "p" << i + 1 << '\t' << names_[i] << '\n';
    return static_cast<bool>(output);
}
bool AtomTable::load(const std::string& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) return false;
    clear();
    std::string idName;
    std::string naturalText;
    while (std::getline(input, idName, '\t') && std::getline(input, naturalText)) {
        if (idName.empty()) continue;
        ids_[idName] = names_.size();
        names_.push_back(naturalText);
    }
    return static_cast<bool>(input) || input.eof();
}
