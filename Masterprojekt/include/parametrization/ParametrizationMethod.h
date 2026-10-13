#pragma once

#include "cost/CostMatrix.h"
#include "model/AssignmentResult.h"
#include "model/Parameter.h"

#include <vector>

class ParametrizationMethod
{
public:
    virtual ~ParametrizationMethod() = default;

    virtual AssignmentResult parametrize(
        const CostMatrix& costMatrix,
        const std::vector<Parameter>& programParameters,
        const std::vector<Parameter>& inputParameters
    ) const = 0;
};
