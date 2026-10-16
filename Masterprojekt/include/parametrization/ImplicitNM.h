#pragma once

#include "parametrization/ParametrizationMethod.h"

class NMImplicit : public ParametrizationMethod
{
public:
    AssignmentResult parametrize(
        const CostMatrix& costMatrix,
        const std::vector<Parameter>& programParameters,
        const std::vector<Parameter>& inputParameters
    ) const override;
};
