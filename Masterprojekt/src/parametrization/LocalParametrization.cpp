
#include "parametrization/LocalParametrization.h"

#include <stdexcept>
#include <vector>

AssignmentResult LocalParametrization::parametrize(
    const CostMatrix& costMatrix,
    const std::vector<Parameter>& programParameters,
    const std::vector<Parameter>& inputParameters) const
{
    if (costMatrix.getRowCount() != programParameters.size() ||
        costMatrix.getColumnCount() != inputParameters.size())
    {
        throw std::invalid_argument(
            "Cost matrix dimensions do not match the parameter lists!!"
        );
    }

    //a check for 1:1 assignment
    if (inputParameters.size() > programParameters.size())
    {
        throw std::runtime_error(
            "Exhaustive parametrization failed: not enough "
            "program parameters for a one-to-one assignment."
        );
    }

    AssignmentResult result{};
    result.totalCost = 0.0;

    std::vector<bool> programParameterUsed(
        programParameters.size(), false
    );

    for (std::size_t j = 0; j < inputParameters.size(); ++j)
    {
        double lowestCost = 0.0;
        std::size_t bestProgramIndex = 0;
        bool foundCandidate = false;

        for (std::size_t i = 0; i < programParameters.size(); ++i)
        {
            if (programParameterUsed[i])
            {
                continue;
            }

            const auto& cost = costMatrix.getCost(i, j);

            if (!cost.has_value())
            {
                continue;
            }

            if (!foundCandidate || cost.value() < lowestCost)
            {
                lowestCost = cost.value();
                bestProgramIndex = i;
                foundCandidate = true;
            }
        }

        if (!foundCandidate)
        {
            throw std::runtime_error(
                "Local parametrization failed: no program parameter remains for input parameter ID "
                + std::to_string(inputParameters[j].id)
            );
        }

        result.assignments.push_back({
            inputParameters[j].id,
            programParameters[bestProgramIndex].id,
            lowestCost
            });

        result.totalCost += lowestCost;
        programParameterUsed[bestProgramIndex] = true;
    }

    return result;
}