
#include "parametrization/ImplicitExhaustive.h"

#include <functional>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

AssignmentResult ImplicitExhaustive::parametrize(
    const CostMatrix& costMatrix,
    const std::vector<Parameter>& programParameters,
    const std::vector<Parameter>& inputParameters) const
{
    if (costMatrix.getRowCount() != programParameters.size() ||
        costMatrix.getColumnCount() != inputParameters.size())
    {
        throw std::invalid_argument(
            "Cost matrix dimensions do not match the parameter list"
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

    std::vector<bool> programParameterUsed(
        programParameters.size(), false
    );
    std::vector<Assignment> currentAssignments;
    AssignmentResult bestResult{};
    bestResult.totalCost = std::numeric_limits<double>::infinity();

    bool foundCompleteAssignment = false;

    std::function<void(std::size_t, double)> search;

    search = [&](std::size_t inputIndex, double currentCost)
        {
            if (inputIndex == inputParameters.size())
            {
                if (!foundCompleteAssignment ||
                    currentCost < bestResult.totalCost)
                {
                    bestResult.assignments = currentAssignments;
                    bestResult.totalCost = currentCost;
                    foundCompleteAssignment = true;
                }

                return;
            }

            for (std::size_t programIndex = 0;
                programIndex < programParameters.size();
                ++programIndex)
            {
                if (programParameterUsed[programIndex])
                {
                    continue;
                }

                const auto& cost = costMatrix.getCost(
                    programIndex, inputIndex
                );

                if (!cost.has_value())
                {
                    continue;
                }

                programParameterUsed[programIndex] = true;
                currentAssignments.push_back({
                    inputParameters[inputIndex].id,
                    programParameters[programIndex].id,
                    cost.value()
                    });

                search(
                    inputIndex + 1,
                    currentCost + cost.value()
                );

                currentAssignments.pop_back();
                programParameterUsed[programIndex] = false;
            }
        };

    search(0, 0.0);

    if (!foundCompleteAssignment)
    {
        throw std::runtime_error(
            "Exhaustive parametrization failed: no complete "
            "compatible assignment exists."
        );
    }

    return bestResult;
}