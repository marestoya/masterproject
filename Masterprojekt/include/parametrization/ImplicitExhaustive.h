#pragma once

#include "cost/CostMatrix.h"
#include "model/AssignmentResult.h"

#include <vector>


//implemented so far as a DFS with backtracking, skipping only on incompatible parameter pairs. Guarantees optimum but probably not the best algo
//TODO: think about a better algo, branch and bound??? do both? 
class ImplicitExhaustive
{
public:
    AssignmentResult parametrize(
        const CostMatrix& costMatrix,
        const std::vector<Parameter>& programParameters,
        const std::vector<Parameter>& inputParameters
    ) const;
};