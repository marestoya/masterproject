#pragma once

#include <optional>
#include <string>
#include <unordered_map>

class Ontology
{
public:
       explicit Ontology(const std::string& jsonFilePath);

    void addType(const std::string& type, const std::string& parent = "");
    bool containsType(const std::string& type) const;
    std::optional<std::string> parentOf(const std::string& type) const;
    const std::string& domain() const noexcept { return domainName; }
    static std::string normalizeTypeName(const std::string& type);

private:
    struct TypeInfo
    {
        std::string parent;
    };

    std::string domainName;
    std::unordered_map<std::string, TypeInfo> types;

    void loadFromFile(const std::string& jsonFilePath);
    void validate() const;
};
