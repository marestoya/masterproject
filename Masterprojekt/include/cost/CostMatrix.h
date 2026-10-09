
#pragma once

#include <cstddef>
#include <optional>
#include <vector>

#include "model/Parameter.h"
#include "cost/CostModel.h"

class CostMatrix
{
public:
    CostMatrix(
        const std::vector<Parameter>& programParameters,
        const std::vector<Parameter>& inputParameters,
        const CostModel& costModel
    );

    std::size_t getRowCount() const;
    std::size_t getColumnCount() const;

    const std::optional<double>& getCost(
        std::size_t row,
        std::size_t column
    ) const;

private:
    std::vector<std::vector<std::optional<double>>> costs;
};