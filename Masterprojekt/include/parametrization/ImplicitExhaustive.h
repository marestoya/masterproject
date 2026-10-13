
//implemented so far as a DFS with backtracking, skipping only on incompatible parameter pairs. Guarantees optimum but probably not the best algo
//TODO: think about a better algo, branch and bound??? do both? 
#pragma once

#include "parametrization/ParametrizationMethod.h"

class ImplicitExhaustive : public ParametrizationMethod
{
public:
    AssignmentResult parametrize(
        const CostMatrix& costMatrix,
        const std::vector<Parameter>& programParameters,
        const std::vector<Parameter>& inputParameters
    ) const override;
};
