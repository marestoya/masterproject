#pragma once

#include <optional>
#include <string>
#include "model/Parameter.h"
#include "model/Ontology.h"

class CostModel
{
public:
    explicit CostModel(const Ontology& ontology, bool useNumber = false);

    std::optional<double> calculateCost(
        const Parameter& programParameter,
        const Parameter& inputParameter) const;

    double typeWeight = 1.0;
    double colorWeight = 1.0;
    double generalizationWeight = 1.0;
    double specializationWeight = 1.0;

private:
    const Ontology& ontology;
    bool useNumber;

    std::optional<double> calculateTypeCost(
        const std::string& programType,
        const std::string& inputType) const;
    double calculateColorCost(
        const std::string& programColor,
        const std::string& inputColor) const;
    double calculateNumberCost(
        const std::optional<int>& programNumber,
        const std::optional<int>& inputNumber) const;
};
