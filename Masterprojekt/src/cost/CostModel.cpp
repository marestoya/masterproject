#include "cost/CostModel.h"

#include <unordered_map>

CostModel::CostModel(const Ontology& ontology, bool useNumber)
    : ontology(ontology), useNumber(useNumber)
{
}

std::optional<double> CostModel::calculateCost(
    const Parameter& programParameter,
    const Parameter& inputParameter) const
{
    const auto typeCost = calculateTypeCost(programParameter.type, inputParameter.type);
    if (!typeCost.has_value())
        return std::nullopt;

    const double colorCost = calculateColorCost(programParameter.color, inputParameter.color);
    const double numberCost = useNumber
        ? calculateNumberCost(programParameter.number, inputParameter.number)
        : 0.0;

    return typeWeight * typeCost.value() + colorWeight * colorCost + numberCost;
}

std::optional<double> CostModel::calculateTypeCost(
    const std::string& programType,
    const std::string& inputType) const
{
    const std::string pType = Ontology::normalizeTypeName(programType);
    const std::string iType = Ontology::normalizeTypeName(inputType);

    if (!ontology.containsType(pType) || !ontology.containsType(iType))
        return std::nullopt;
    if (pType == iType)
        return 0.0;

    std::unordered_map<std::string, int> programAncestors;
    std::string current = pType;
    int programUpSteps = 0;
    while (!current.empty())
    {
        programAncestors[current] = programUpSteps++;
        const auto parent = ontology.parentOf(current);
        if (!parent.has_value())
            return std::nullopt;
        current = parent.value();
    }

    current = iType;
    int inputUpSteps = 0;
    while (!current.empty())
    {
        const auto ancestor = programAncestors.find(current);
        if (ancestor != programAncestors.end())
        {
            return generalizationWeight * ancestor->second
                + specializationWeight * inputUpSteps;
        }

        const auto parent = ontology.parentOf(current);
        if (!parent.has_value())
            return std::nullopt;
        current = parent.value();
        ++inputUpSteps;
    }

    return std::nullopt; 
}

double CostModel::calculateColorCost(
    const std::string& programColor,
    const std::string& inputColor) const
{
    return programColor == inputColor ? 0.0 : 1.0;
}

double CostModel::calculateNumberCost(
    const std::optional<int>& programNumber,
    const std::optional<int>& inputNumber) const
{
    if (!programNumber.has_value() || !inputNumber.has_value())
        return 0.0;
    return programNumber.value() == inputNumber.value() ? 0.0 : 1.0;
}
