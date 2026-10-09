#pragma once

#include "cost/CostMatrix.h"
#include "model/AssignmentResult.h"

#include <vector>

class GlobalImplicit
{
public:
	AssignmentResult parametrize(
		const CostMatrix& costMatrix,
		const std::vector<Parameter>& programParameters,
		const std::vector<Parameter>& inputParameters
	) const;
};
