#include "parametrization/GlobalExplicit.h"
#include "model/ProgramInstantiator.h"

#include <stdexcept>
#include <unordered_set>
#include <cmath>

AssignmentResult GlobalExplicit::applyUserAssignments(
    const Program& program,
    const std::vector<Parameter>& inputParameters,
    const std::vector<Assignment>& userAssignments
) const
{
    std::unordered_set<int> inputIds;
    for (const Parameter& input : inputParameters)
    {
        if (!inputIds.insert(input.id).second)
            throw std::runtime_error(
                "Duplicate parameter IDs detected!");
    }

    std::unordered_set<int> slotIds;
    for (const ParameterSlot& slot : program.parameters)
    {
        if (!slotIds.insert(slot.id).second)
            throw std::runtime_error(
                "Duplicate slot IDs detected!");
    }

    AssignmentResult result;
    std::unordered_set<int> assignedInputIds;
    std::unordered_set<int> filledSlotIds;

    for (const Assignment& assignment : userAssignments)
    {
        if (!inputIds.count(assignment.inputParameterId))
            throw std::runtime_error(
                "Unknown input parameter");

        if (!slotIds.count(assignment.programParameterId))
            throw std::runtime_error(
                "Unknown program slot");
        if (!assignedInputIds.insert(
            assignment.inputParameterId).second)
            throw std::runtime_error(
                "An input parameter was assigned more than once!");

        if (!std::isfinite(assignment.cost) || assignment.cost < 0.0)
            throw std::runtime_error(
                "Assignment cost must be finite and non-negative!");

        result.assignments.push_back(assignment);
        result.totalCost += assignment.cost;
        filledSlotIds.insert(assignment.programParameterId);
    }

    if (assignedInputIds.size() != inputParameters.size())
        throw std::runtime_error(
            "Not every input parameter has been assigned!");

    if (filledSlotIds.size() != program.parameters.size())
        throw std::runtime_error(
            "Every program slot must receive at least one input!");

    return result;
}
