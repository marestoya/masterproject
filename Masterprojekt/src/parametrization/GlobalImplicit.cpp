
#include "parametrization/GlobalImplicit.h"

#include <dlib/matrix.h>
#include <dlib/optimization/max_cost_assignment.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <vector>

namespace
{
    using Gain = dlib::int64;

    constexpr double scale = 1'000'000.0;

    Gain maximumAssignmentGain(
        const std::vector<std::vector<Gain>>& gains,
        const std::vector<std::size_t>& rows,
        const std::vector<std::size_t>& columns)
    {
        if (rows.size() != columns.size())
        {
            throw std::invalid_argument(
                "Assignment subproblem must be square");
        }

        const std::size_t n = rows.size();

        if (n == 0)
        {
            return 0;
        }

        dlib::matrix<Gain> matrix(
            static_cast<long>(n),
            static_cast<long>(n));

        for (std::size_t r = 0; r < n; ++r)
        {
            for (std::size_t c = 0; c < n; ++c)
            {
                matrix(
                    static_cast<long>(r),
                    static_cast<long>(c)) =
                    gains[rows[r]][columns[c]];
            }
        }

        const std::vector<long> assignment =
            dlib::max_cost_assignment(matrix);

        Gain totalGain = 0;

        for (std::size_t r = 0; r < n; ++r)
        {
            const long c = assignment[r];

            if (c < 0 || static_cast<std::size_t>(c) >= n)
            {
                throw std::runtime_error(
                    "Could not solve assignment subproblem");
            }

            totalGain += matrix(
                static_cast<long>(r), c);
        }

        return totalGain;
    }
}

AssignmentResult GlobalImplicit::parametrize(
    const CostMatrix& costMatrix,
    const std::vector<Parameter>& programParameters,
    const std::vector<Parameter>& inputParameters) const
{
    const std::size_t slotCount = programParameters.size();
    const std::size_t inputCount = inputParameters.size();

    if (costMatrix.getRowCount() != slotCount ||
        costMatrix.getColumnCount() != inputCount)
    {
        throw std::invalid_argument(
            "Cost matrix dimensions do not match the parameter list!");
    }

    if (inputCount > slotCount)
    {
        throw std::runtime_error(
            "Global implicit parametrization failed: not enough "
            "program parameters for a one-to-one assignment!");
    }

    if (inputCount == 0)
    {
        return AssignmentResult{};
    }

    double maxCost = -std::numeric_limits<double>::infinity();
    bool foundCompatiblePair = false;

    for (std::size_t slot = 0; slot < slotCount; ++slot)
    {
        for (std::size_t input = 0; input < inputCount; ++input)
        {
            const auto& cost = costMatrix.getCost(slot, input);

            if (!cost.has_value())
            {
                continue;
            }

            if (!std::isfinite(cost.value()))
            {
                throw std::runtime_error(
                    "Cost matrix contains a non-finite cost");
            }

            maxCost = foundCompatiblePair
                ? std::max(maxCost, cost.value())
                : cost.value();

            foundCompatiblePair = true;
        }
    }

    if (!foundCompatiblePair)
    {
        throw std::runtime_error(
            "Global implicit parametrization failed: no compatible "
            "input/program parameter pairs exist");
    }

    const std::size_t matrixSize = slotCount;

    const Gain maxInt = std::numeric_limits<Gain>::max();
    const Gain factor = static_cast<Gain>(inputCount) + 1;

    std::vector<std::vector<Gain>> gains(
        matrixSize,
        std::vector<Gain>(matrixSize, 0));

    Gain maxGain = 0;

    for (std::size_t slot = 0; slot < slotCount; ++slot)
    {
        for (std::size_t input = 0; input < inputCount; ++input)
        {
            const auto& cost = costMatrix.getCost(slot, input);

            if (!cost.has_value())
            {
                continue;
            }

            const long double scaledGain =
                (static_cast<long double>(maxCost) -
                    static_cast<long double>(cost.value())) * scale;

            if (!std::isfinite(scaledGain) ||
                scaledGain < 0 ||
                scaledGain >
                static_cast<long double>(maxInt) / 2)
            {
                throw std::overflow_error(
                    "Cost range is too large to construct the gain matrix");
            }

            const Gain gain = static_cast<Gain>(
                std::llround(scaledGain));

            gains[slot][input] = gain;
            maxGain = std::max(maxGain, gain);
        }
    }

    if (maxGain >= maxInt / factor - 1)
    {
        throw std::overflow_error(
            "Gain range is too big");
    }

    const Gain incompatibleGain =
        -(maxGain + 1) * factor;

    for (std::size_t slot = 0; slot < slotCount; ++slot)
    {
        for (std::size_t input = 0; input < inputCount; ++input)
        {
            if (!costMatrix.getCost(slot, input).has_value())
            {
                gains[slot][input] = incompatibleGain;
            }
        }
    }

    std::vector<std::size_t> allRows(matrixSize);
    std::vector<std::size_t> allColumns(matrixSize);

    for (std::size_t i = 0; i < matrixSize; ++i)
    {
        allRows[i] = i;
        allColumns[i] = i;
    }

    const Gain optimalGain = maximumAssignmentGain(
        gains, allRows, allColumns);

    //deterministic tie breaker
    std::vector<std::size_t> inputToSlot(inputCount);
    std::vector<bool> slotUsed(slotCount, false);

    Gain fixedGain = 0;

    for (std::size_t input = 0; input < inputCount; ++input)
    {
        bool foundChoice = false;

        for (std::size_t slot = 0; slot < slotCount; ++slot)
        {
            if (slotUsed[slot])
            {
                continue;
            }
            if (!costMatrix.getCost(slot, input).has_value())
            {
                continue;
            }

            const Gain candidateGain = gains[slot][input];

            std::vector<std::size_t> remainingRows;
            std::vector<std::size_t> remainingColumns;

            for (std::size_t remainingSlot = 0;
                remainingSlot < slotCount;
                ++remainingSlot)
            {
                if (slotUsed[remainingSlot] ||
                    remainingSlot == slot)
                {
                    continue;
                }

                remainingRows.push_back(remainingSlot);
            }

            for (std::size_t remainingInput = input + 1;
                remainingInput < inputCount;
                ++remainingInput)
            {
                remainingColumns.push_back(remainingInput);
            }

            for (std::size_t dummy = inputCount;
                dummy < slotCount;
                ++dummy)
            {
                remainingColumns.push_back(dummy);
            }

            const Gain remainingGain = maximumAssignmentGain(
                gains,
                remainingRows,
                remainingColumns);

            if (fixedGain + candidateGain + remainingGain ==
                optimalGain)
            {
                inputToSlot[input] = slot;
                slotUsed[slot] = true;
                fixedGain += candidateGain;
                foundChoice = true;
                break;
            }
        }

        if (!foundChoice)
        {
            throw std::runtime_error(
                "Global implicit parametrization failed: "
                "could not resolve a compatible optimal assignment");
        }
    }

    AssignmentResult result{};

    for (std::size_t input = 0; input < inputCount; ++input)
    {
        const std::size_t slot = inputToSlot[input];
        const auto& cost = costMatrix.getCost(slot, input);

        if (!cost.has_value())
        {
            throw std::runtime_error(
                "Global implicit parametrization failed: "
                "selected assignment has no valid cost");
        }

        result.assignments.push_back({
            inputParameters[input].id,
            programParameters[slot].id,
            cost.value()
            });

        result.totalCost += cost.value();
    }

    return result;
}
