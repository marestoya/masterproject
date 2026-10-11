
#include <cmath>
#include <exception>
#include <iostream>
#include <string>
#include <vector>

#include "model/Parameter.h"
#include "model/Program.h"
#include "model/AssignmentResult.h"

#include "cost/CostModel.h"
#include "cost/CostMatrix.h"

#include "parametrization/LocalParametrization.h"
#include "model/ProgramInstantiator.h"

void printProgram(const Program& program)
{
    std::cout << "\nProgram: " << program.name
        << " (ID: " << program.id << ")\n";

    for (const Parameter& parameter : program.parameters)
    {
        std::cout << "  Parameter ID: " << parameter.id
            << ", Type: " << parameter.type
            << ", Color: " << parameter.color
            << "\n";
    }
}

void printAssignments(const AssignmentResult& result)
{
    std::cout << "\n ASSIGNMENTS \n";

    for (const Assignment& assignment : result.assignments)
    {
        std::cout << "Input " << assignment.inputParameterId
            << " -> Program " << assignment.programParameterId
            << ", Cost: " << assignment.cost
            << "\n";
    }

    std::cout << "Total cost: " << result.totalCost << "\n";
}


int main()
{
    try
    {
        Program program;

        program.id = 1;
        program.name = "Laboratory Program";

        program.parameters =
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

        std::cout << "ORIGINAL PROGRAM\n";
        printProgram(program);

        CostModel costModel;
        CostMatrix costMatrix(
            program.parameters,
            inputParameters,
            costModel
        );


        LocalParametrization local;
        AssignmentResult assignmentResult =
            local.parametrize(
                costMatrix,
                program.parameters,
                inputParameters
            );

        printAssignments(assignmentResult);


        ProgramInstantiator instantiator;

        Program updatedProgram = instantiator.instantiate(
            program,
            inputParameters,
            assignmentResult
        );


        std::cout << "\nUPDATED PROGRAM\n";
        printProgram(updatedProgram);
        
        
    }
    catch (const std::exception& e)
    {
        std::cerr << "\nERROR: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
