
#include <iostream>
#include <vector>

#include "cost/CostModel.h"
#include "cost/CostMatrix.h"
#include "parametrization/LocalParametrization.h"
#include "parametrization/ImplicitExhaustive.h"

int main()
{
    CostModel costModel;

    std::vector<Parameter> programParameters =
    {
        {1, "container", "red"},
        {2, "mug", "blue"},
        {3, "stirrer", "yellow"}
    };

    std::vector<Parameter> inputParameters =
    {
        {1, "tallMug", "green"},
        {2, "mug", "blue"},
        {3, "stirrer", "yellow"}
    };

    CostMatrix matrix(
        programParameters,
        inputParameters,
        costModel
    );

    LocalParametrization local;
    ImplicitExhaustive exhaustive;

    AssignmentResult localResult = local.parametrize(
        matrix, programParameters, inputParameters
    );

    AssignmentResult exhaustiveResult = exhaustive.parametrize(
        matrix, programParameters, inputParameters
    );

    std::cout << "Local Parametrization:\n";

    for (const Assignment& assignment : localResult.assignments)
    {
        std::cout << "Input " << assignment.inputParameterId
            << " -> Program " << assignment.programParameterId
            << ", Cost: " << assignment.cost << '\n';
    }

    std::cout << "Total cost: "
        << localResult.totalCost << "\n\n";

    std::cout << "Global Implicit Exhaustive:\n";

    for (const Assignment& assignment : exhaustiveResult.assignments)
    {
        std::cout << "Input " << assignment.inputParameterId
            << " -> Program " << assignment.programParameterId
            << ", Cost: " << assignment.cost << '\n';
    }

    std::cout << "Total cost: "
        << exhaustiveResult.totalCost << '\n';

    return 0;
}