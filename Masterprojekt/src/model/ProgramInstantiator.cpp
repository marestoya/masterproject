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
        auto input = std::find_if(
            inputParameters.begin(),
            inputParameters.end(),
            [&](const Parameter& parameter)
            {
                return parameter.id == assignment.inputParameterId;
            }
        );

        if (input == inputParameters.end())
        {
            throw std::runtime_error(
                "Assignment references an unknown input parameter!"
            );
        }

        auto slot = std::find_if(
            instantiatedProgram.parameters.begin(),
            instantiatedProgram.parameters.end(),
            [&](const ParameterSlot& candidate)
            {
                return candidate.id == assignment.programParameterId;
            }
        );

        if (slot == instantiatedProgram.parameters.end())
        {
            throw std::runtime_error(
                "Assignment references an unknown program slot!"
            );
        }

        if (slot->parameters.size() == 1 &&
            slot->parameters.front().id == assignment.programParameterId)
        {
            slot->parameters.clear();
        }

        slot->parameters.push_back(*input);
    
    }

    return instantiatedProgram;
}