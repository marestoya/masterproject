
#include <cmath>
#include <exception>
#include <iostream>
#include <string>
#include <vector>

#include "model/Parameter.h"
#include "model/Program.h"
#include "model/AssignmentResult.h"
#include "model/ProgramInstantiator.h"

#include "cost/CostModel.h"
#include "cost/CostMatrix.h"

#include "parametrization/LocalParametrization.h"
#include "parametrization/GlobalImplicit.h"
#include "parametrization/ImplicitExhaustive.h"
#include "parametrization/GlobalExplicit.h"
#include "parametrization/ImplicitNM.h"

#include "tests/ParametrizationTests.h"
#include <unity_simulation.hpp>

void runParametrization(
    const ParametrizationMethod& method,
    const Program& program,
    const std::vector<Parameter>& inputParameters,
    bool useNumber)
{
    CostModel costModel(false);
    const std::vector<Parameter> programParameters =
        getFirstParameters(program);
    CostMatrix costMatrix(
        programParameters,
        inputParameters,
        costModel
    );

   /* AssignmentResult assignmentResult = method.parametrize(
        costMatrix,
        programParameters,
        inputParameters
    );

    printAssignments(assignmentResult);*/

    ProgramInstantiator instantiator;
    AssignmentResult testResult;
    testResult.assignments = {
        {101, 1, 0.0},  
        {102, 1, 1.0}  
    };
    Program updatedProgram = instantiator.instantiate(
        program,
        inputParameters,
        testResult
    );

    std::cout << "\n--- Program Instantiation Test ---\n";

    for (const ParameterSlot& slot : updatedProgram.parameters)
    {
        std::cout << "Slot " << slot.id << ":\n";

        for (const Parameter& parameter : slot.parameters)
        {
            std::cout << "  Parameter ID: " << parameter.id
                << ", Type: " << parameter.type
                << ", Color: " << parameter.color
                << '\n';
        }
    }
}


int main()
{
    try
    {
        ursim::unity_simulation unity_sim;

        std::cout << "waiting" << std::endl;

        for (int i = 0; i < 50; ++i)
        {
            auto feedback = unity_sim.info.recent_feedback();

            if (feedback.has_value())
            {
                std::cout << "feedback" << std::endl;
                return 0;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        std::cout << "no feedback after 5 secs" << std::endl;
        return 0;
        
    }
    catch (const std::exception& e)
    {
        std::cerr << "\nERROR: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
