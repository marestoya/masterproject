#include "cost/CostModel.h"


#include <algorithm>
#include <cmath>
#include <stdexcept>

CostModel::CostModel(bool useNumber)
    : useNumber(useNumber)
{
 
    //example ontology for the first domain
    //TODO: this should be loaded from a file in the future, its fine for now
    addType("object", "");

    addType("container", "object");
    addType("mug", "container");
    addType("tallMug", "mug");
    addType("beaker", "container");

    addType("tool", "object");
    addType("stirrer", "tool");

    addType("measuringInstrument", "object");
    addType("thermometer", "measuringInstrument");

    addType("sampleTube", "container");
    addType("storageBox", "container");
    addType("powder", "object");
    addType("liquid", "object");
    addType("waterBottle", "container");
}

void CostModel::addType(
    const std::string& type,
    const std::string& parent)
{
    ontology[type] = { parent };
}

std::optional<double> CostModel::calculateCost(
    const Parameter& programParameter,
    const Parameter& inputParameter) const
{
    const auto typeCost = calculateTypeCost(
        programParameter.type,
        inputParameter.type
    );

    if (!typeCost.has_value())
    {
        return std::nullopt;
    }

    const double colorCost = calculateColorCost(
        programParameter.color,
        inputParameter.color
    );

    const double numberCost = useNumber
        ? calculateNumberCost(
            programParameter.number,
            inputParameter.number
        )
		: 0.0;

    return typeWeight * typeCost.value()
        + colorWeight * colorCost + numberCost;
}

std::optional<double> CostModel::calculateTypeCost(
    const std::string& programType,
    const std::string& inputType) const
{
    if (ontology.find(programType) == ontology.end() ||
        ontology.find(inputType) == ontology.end())
    {
        return std::nullopt;
    }

    if (programType == inputType)
    {
        return 0.0;
    }

    std::unordered_map<std::string, int> programAncestors;

    std::string current = programType;
    int programUpSteps = 0;

    while (!current.empty())
    {
        programAncestors[current] = programUpSteps;
        current = ontology.at(current).parent;
        ++programUpSteps;
    }

    current = inputType;
    int inputUpSteps = 0;

    while (!current.empty())
    {
        const auto ancestor = programAncestors.find(current);

        if (ancestor != programAncestors.end())
        {
            const int upwardSteps = ancestor->second;
            const int downwardSteps = inputUpSteps;

            return generalizationWeight * upwardSteps
                + specializationWeight * downwardSteps;
        }

        current = ontology.at(current).parent;
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
    {
        return 0.0;
    }

    return programNumber.value() == inputNumber.value() ? 0.0 : 1.0;
}