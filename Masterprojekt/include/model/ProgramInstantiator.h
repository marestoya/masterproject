
#pragma once

#include <vector>

#include "model/Program.h"
#include "model/Parameter.h"
#include "model/AssignmentResult.h"

class ProgramInstantiator
{
public:
    Program instantiate(
        const Program& program,
        const std::vector<Parameter>& inputParameters,
        const AssignmentResult& assignmentResult
    ) const;
};
