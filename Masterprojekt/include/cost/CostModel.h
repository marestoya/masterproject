
#pragma once

#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "model/Parameter.h"

class CostModel
{
public:
    explicit CostModel(bool useNumber = true);

    std::optional<double> calculateCost(
        const Parameter& programParameter,
        const Parameter& inputParameter
    ) const;

private:
    struct OntologyNode
    {
        std::string parent;
    };

    std::unordered_map<std::string, OntologyNode> ontology;

    double typeWeight = 1.0;
    double colorWeight = 1.0;

    double generalizationWeight = 2.0;
    double specializationWeight = 1.0;

    bool useNumber;

    void addType(
        const std::string& type,
        const std::string& parent
    );

    std::optional<double> calculateTypeCost(
        const std::string& programType,
        const std::string& inputType
    ) const;

    double calculateColorCost(
        const std::string& programColor,
        const std::string& inputColor
    ) const;

    double calculateNumberCost(
        const std::optional<int>& programNumber,
        const std::optional<int>& inputNumber
    ) const;
};