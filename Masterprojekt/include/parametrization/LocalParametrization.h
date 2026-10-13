#pragma once

#include "parametrization/ParametrizationMethod.h"

class LocalParametrization : public ParametrizationMethod
{
public:
    AssignmentResult parametrize(
        const CostMatrix& costMatrix,
        const std::vector<Parameter>& programParameters,
        const std::vector<Parameter>& inputParameters
    ) const override;
};
