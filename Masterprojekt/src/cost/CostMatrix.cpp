
#include "cost/CostMatrix.h"

CostMatrix::CostMatrix(
    const std::vector<Parameter>& programParameters,
    const std::vector<Parameter>& inputParameters,
    const CostModel& costModel)
{
    costs.resize(programParameters.size());
    for (std::size_t i = 0; i < programParameters.size(); ++i)
    {
        costs[i].resize(inputParameters.size());
        for (std::size_t j = 0; j < inputParameters.size(); ++j)
        {
            costs[i][j] = costModel.calculateCost(
                programParameters[i],
                inputParameters[j]
            );
        }
    }
}

std::size_t CostMatrix::getRowCount() const
{
    return costs.size();
}

std::size_t CostMatrix::getColumnCount() const
{
    if (costs.empty())
    {
        return 0;
    }

    return costs[0].size();
}

const std::optional<double>& CostMatrix::getCost(
    std::size_t row,
    std::size_t column) const
{
    return costs.at(row).at(column);
}