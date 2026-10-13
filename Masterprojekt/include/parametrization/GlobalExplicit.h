#pragma once

#include "model/Program.h"
#include "model/Assignment.h"
#include "model/AssignmentResult.h"
#include <vector>

class GlobalExplicit
{
public:
    AssignmentResult applyUserAssignments(
        const Program& program,
        const std::vector<Parameter>& inputParameters,
        const std::vector<Assignment>& userAssignments
    ) const;
};
