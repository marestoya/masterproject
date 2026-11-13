#include "model/Ontology.h"

#include <cctype>
#include <fstream>
#include <stdexcept>
#include <nlohmann/json.hpp>

using nlohmann::json;

Ontology::Ontology(const std::string& jsonFilePath)
{
    loadFromFile(jsonFilePath);
    validate();
}

void Ontology::loadFromFile(const std::string& jsonFilePath)
{
    std::ifstream input(jsonFilePath);
    if (!input)
        throw std::runtime_error("Could not open ontology file: " + jsonFilePath);

    json root;
    input >> root;

    if (!root.is_object() || !root.contains("domain") || !root["domain"].is_string())
        throw std::runtime_error("Ontology JSON must contain a string field named 'domain'");
    if (!root.contains("types") || !root["types"].is_array())
        throw std::runtime_error("Ontology JSON must contain an array field named 'types'");

    domainName = root["domain"].get<std::string>();

    for (const auto& entry : root["types"])
    {
        if (!entry.is_object() || !entry.contains("name") || !entry["name"].is_string())
            throw std::runtime_error("Each ontology type must have a string 'name'");

        const std::string name = entry["name"].get<std::string>();
        std::string parent;
        if (entry.contains("parent") && !entry["parent"].is_null())
        {
            if (!entry["parent"].is_string())
                throw std::runtime_error("Ontology parent must be a string or null: " + name);
            parent = entry["parent"].get<std::string>();
        }
        addType(name, parent);
    }
}

void Ontology::addType(const std::string& type, const std::string& parent)
{
    const std::string normalizedType = normalizeTypeName(type);
    const std::string normalizedParent = parent.empty() ? std::string{} : normalizeTypeName(parent);

    if (normalizedType.empty())
        throw std::invalid_argument("Ontology type name cannot be empty.");
    if (normalizedType == normalizedParent)
        throw std::invalid_argument("An ontology type cannot be its own parent: " + type);

    const auto inserted = types.emplace(normalizedType, TypeInfo{ normalizedParent });
    if (!inserted.second)
        throw std::invalid_argument("Duplicate ontology type: " + type);
}

bool Ontology::containsType(const std::string& type) const
{
    return types.find(normalizeTypeName(type)) != types.end();
}

std::optional<std::string> Ontology::parentOf(const std::string& type) const
{
    const auto it = types.find(normalizeTypeName(type));
    if (it == types.end())
        return std::nullopt;
    if (it->second.parent.empty())
        return std::string{};
    return it->second.parent;
}

std::string Ontology::normalizeTypeName(const std::string& typeName)
{
    std::string result;
    result.reserve(typeName.size());

    for (std::size_t i = 0; i < typeName.size(); ++i)
    {
        const char c = typeName[i];

        if (c == ' ' || c == '-')
        {
            if (!result.empty() && result.back() != '_')
                result += '_';
        }
        else if (std::isupper(static_cast<unsigned char>(c)))
        {
            if (!result.empty() && result.back() != '_')
                result += '_';

            result += static_cast<char>(
                std::tolower(static_cast<unsigned char>(c)));
        }
        else
        {
            result += c;
        }
    }

    while (!result.empty() && result.back() == '_')
        result.pop_back();

    static const std::unordered_map<std::string, std::string> plurals = {
        {"gloves", "glove"},
        {"cotton_pads", "cotton_pad"},
        {"stickers", "sticker"},
        {"sale_stickers", "sale_sticker"},
        {"cups", "cup"},
        {"price_tags", "price_tag"},
        {"napkins", "napkin"},
        {"muffins", "muffin"},
        {"plates", "plate"},
        {"branches", "branch"},
        {"shelves", "shelf"}
    };

    const auto pluralIt = plurals.find(result);
    if (pluralIt != plurals.end())
        return pluralIt->second;

    return result;
}

void Ontology::validate() const
{
    if (types.empty())
        throw std::runtime_error("Ontology contains no types.");

    for (const auto& item : types)
    {
        const std::string& type = item.first;
        const std::string& parent = item.second.parent;
        if (!parent.empty() && types.find(parent) == types.end())
            throw std::runtime_error("Unknown parent '" + parent + "' for ontology type '" + type + "'.");

        std::unordered_map<std::string, bool> visited;
        std::string current = type;
        while (!current.empty())
        {
            if (visited.find(current) != visited.end())
                throw std::runtime_error("Cycle detected in ontology at type '" + current + "'.");
            visited[current] = true;
            const auto it = types.find(current);
            if (it == types.end())
                break;
            current = it->second.parent;
        }
    }
}
