#include <cmath>
#include <exception>
#include <iostream>
#include <string>
#include <vector>

#include "model/Parameter.h"
#include "model/AssignmentResult.h"

#include "cost/CostModel.h"
#include "cost/CostMatrix.h"

#include "parametrization/LocalParametrization.h"
#include "parametrization/ImplicitExhaustive.h"
#include "parametrization/GlobalImplicit.h"


void printResult(
    const std::string& methodName,
    const AssignmentResult& result)
{
    std::cout << "\n========== " << methodName
        << " ==========\n";

    for (const Assignment& assignment : result.assignments)
    {
        std::cout
            << "Input parameter "
            << assignment.inputParameterId
            << " -> Program parameter "
            << assignment.programParameterId
            << ", Cost: "
            << assignment.cost
            << "\n";
    }

    std::cout << "Total cost: "
        << result.totalCost
        << "\n";
}

void printCostMatrix(const CostMatrix& costMatrix)
{
    std::cout << "\n COST MATRIX \n";

    std::cout << "Rows = program parameters\n";
    std::cout << "Columns = input parameters\n\n";

    for (std::size_t i = 0;
        i < costMatrix.getRowCount();
        ++i)
    {
        for (std::size_t j = 0;
            j < costMatrix.getColumnCount();
            ++j)
        {
            const auto& cost = costMatrix.getCost(i, j);

            if (cost.has_value())
            {
                std::cout << cost.value() << "\t";
            }
            else
            {
                std::cout << "X\t";
            }
        }

        std::cout << "\n";
    }
}


int main()
{
    try
    {

        std::vector<Parameter> programParameters =
        {
            {1, "container", "red"},
            {2, "mug", "blue"},
            {3, "stirrer", "yellow"}
        };

        std::vector<Parameter> inputParameters =
        {
            {101, "tallMug", "green"},
            {102, "mug", "blue"},
            {103, "stirrer", "yellow"}
        };

        CostModel costModel;
        CostMatrix costMatrix(
            programParameters,
            inputParameters,
            costModel
        );

        printCostMatrix(costMatrix);

        LocalParametrization local;
        ImplicitExhaustive exhaustive;
        GlobalImplicit hungarian;

        AssignmentResult localResult =
            local.parametrize(
                costMatrix,
                programParameters,
                inputParameters
            );

        AssignmentResult exhaustiveResult =
            exhaustive.parametrize(
                costMatrix,
                programParameters,
                inputParameters
            );

        AssignmentResult hungarianResult =
            hungarian.parametrize(
                costMatrix,
                programParameters,
                inputParameters
            );

        printResult("LOCAL PARAMETRIZATION", localResult);

        printResult(
            "EXHAUSTIVE PARAMETRIZATION",
            exhaustiveResult
        );

        printResult(
            "HUNGARIAN ALGORITHM",
            hungarianResult
        );
      
    }
    catch (const std::exception& e)
    {
        std::cerr << "\nERROR: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
