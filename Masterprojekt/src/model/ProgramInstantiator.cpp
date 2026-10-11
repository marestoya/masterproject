#include "model/ProgramInstantiator.h"

#include <algorithm>
#include <stdexcept>

Program ProgramInstantiator::instantiate(
    const Program& program,
    const std::vector<Parameter>& inputParameters,
    const AssignmentResult& assignmentResult
) const
{
    Program instantiatedProgram = program;

    for (const Assignment& assignment : assignmentResult.assignments)
    {
        const auto inputIt = std::find_if(
            inputParameters.begin(),
            inputParameters.end(),
            [&assignment](const Parameter& parameter)
            {
                return parameter.id == assignment.inputParameterId;
            }
        );

        if (inputIt == inputParameters.end())
        {
            throw std::runtime_error(
                "Program instantiation failed: input parameter not found"
            );
        }

        const auto programIt = std::find_if(
            instantiatedProgram.parameters.begin(),
            instantiatedProgram.parameters.end(),
            [&assignment](const Parameter& parameter)
            {
                return parameter.id == assignment.programParameterId;
            }
        );

        if (programIt == instantiatedProgram.parameters.end())
        {
            throw std::runtime_error(
                "Program instantiation failed: program parameter not found"
            );
        }

        programIt->type = inputIt->type;
        programIt->color = inputIt->color;
    }

    return instantiatedProgram;
}
