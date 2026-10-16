
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
#include "parametrization/GlobalImplicit.h"
#include "parametrization/ImplicitExhaustive.h"
#include "parametrization/GlobalExplicit.h"
#include "model/ProgramInstantiator.h"

void printProgram(const Program& program)
{
    std::cout << "\nProgram: " << program.name
        << " (ID: " << program.id << ")\n";

    for (const ParameterSlot& slot : program.parameters)
    {
        std::cout << "  Slot ID: " << slot.id << '\n';

        for (const Parameter& parameter : slot.parameters)
        {
            std::cout << "    Parameter ID: " << parameter.id
                << ", Type: " << parameter.type
                << ", Color: " << parameter.color
                << '\n';
        }
    }
}

void printAssignments(const AssignmentResult& result)
{
    std::cout << "\nASSIGNMENTS\n";

    for (const Assignment& assignment : result.assignments)
    {
        std::cout << "Input " << assignment.inputParameterId
            << " -> Program slot "
            << assignment.programParameterId
            << ", Cost: " << assignment.cost
            << '\n';
    }

    std::cout << "Total cost: " << result.totalCost << '\n';
}

void runParametrization(
    const ParametrizationMethod& method,
    const Program& program,
    const std::vector<Parameter>& inputParameters)
{
    CostModel costModel(false);
    const std::vector<Parameter> programParameters =
        getFirstParameters(program);
    CostMatrix costMatrix(
        programParameters,
        inputParameters,
        costModel
    );

    AssignmentResult assignmentResult = method.parametrize(
        costMatrix,
        programParameters,
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


int main()
{
    try
    {
        Program program;

        program.id = 1;
        program.name = "Laboratory Program";

        program.parameters = {
            {1, {{1, "container", "red"}}},
            {2, {{2, "mug", "blue"}}},
            {3, {{3, "stirrer", "yellow"}}}
        };

        std::vector<Parameter> inputParameters =
        {
            {101, "tallMug", "green"},
            {102, "mug", "blue"},
            {103, "stirrer", "yellow"}

        };

        std::cout << "ORIGINAL PROGRAM\n";
        printProgram(program);

        LocalParametrization local;
        runParametrization(local, program, inputParameters);

        GlobalImplicit implicit;
        runParametrization(implicit, program, inputParameters);

		ImplicitExhaustive exhaustive;
		runParametrization(exhaustive, program, inputParameters);

        GlobalExplicit explicitMethod;

        std::vector<Assignment> userAssignments =
        {
            {101, 1, 2.0},
            {102, 2, 0.0},
            {103, 3, 0.0} 
        };

        std::cout << "\nGLOBAL EXPLICIT \n";

        AssignmentResult explicitResult =
            explicitMethod.applyUserAssignments(
                program,
                inputParameters,
                userAssignments
            );

        printAssignments(explicitResult);
        
    }
    catch (const std::exception& e)
    {
        std::cerr << "\nERROR: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
