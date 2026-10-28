#ifndef SAT_ATOM_TABLE_H
#define SAT_ATOM_TABLE_H

#include <cstddef>
#include <map>
#include <string>
#include <vector>

class AtomTable {
public:
    std::size_t registerAtom(const std::string& name);
    std::size_t registerAtom(const std::string& name, const std::string& naturalText);
    bool contains(const std::string& name) const;
    std::size_t size() const;
    const std::string& name(std::size_t id) const;
    std::size_t id(const std::string& name) const;
    const std::string& text(const std::string& name) const;
    std::string identifier(std::size_t id) const;
    std::string identifierForText(const std::string& naturalText) const;
    void clear();
    bool save(const std::string& path) const;
    bool load(const std::string& path);
private:
    std::map<std::string, std::size_t> ids_;
    std::vector<std::string> names_;
};

#endif
