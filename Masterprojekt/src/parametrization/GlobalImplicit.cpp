
#include "parametrization/GlobalImplicit.h"

#include <dlib/matrix.h>
#include <dlib/optimization/max_cost_assignment.h>

#include <cmath>
#include <limits>
#include <stdexcept>
#include <vector>

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
            "Global implicit parametrization failed: not enough program parameters for a one-to-one assignment!");
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
                continue;

            if (!std::isfinite(cost.value()))
            {
                throw std::runtime_error(
                    "Cost matrix contains a non-finite cost.");
            }

            maxCost = foundCompatiblePair
                ? (cost.value() > maxCost ? cost.value() : maxCost)
                : cost.value();

            foundCompatiblePair = true;
        }
    }

    if (!foundCompatiblePair)
    {
        throw std::runtime_error(
            "Global implicit parametrization failed: no compatible "
            "input/program parameter pairs exist.");
    }

    const long matrixSize = static_cast<long>(slotCount);

    dlib::matrix<dlib::int64> gainMatrix(matrixSize, matrixSize);

    constexpr double scale = 1'000'000.0;
    dlib::int64 maxGain = 0;

    for (std::size_t slot = 0; slot < slotCount; ++slot)
    {
        for (std::size_t input = 0; input < inputCount; ++input)
        {
            const auto& cost = costMatrix.getCost(slot, input);

            if (!cost.has_value())
            {
                gainMatrix(
                    static_cast<long>(slot),
                    static_cast<long>(input)) = 0;

                continue;
            }

            const long double scaledGain =
                (static_cast<long double>(maxCost) - cost.value())
                * scale;

            if (scaledGain >
                static_cast<long double>(
                    std::numeric_limits<dlib::int64>::max()) / 2)
            {
                throw std::overflow_error(
                    "Value is too high");
            }

            const auto gain = static_cast<dlib::int64>(
                std::llround(scaledGain));

            gainMatrix(
                static_cast<long>(slot),
                static_cast<long>(input)) = gain;

            if (gain > maxGain)
                maxGain = gain;
        }

        //dummies for the square matrix
        for (std::size_t dummy = inputCount;
            dummy < slotCount;
            ++dummy)
        {
            gainMatrix(
                static_cast<long>(slot),
                static_cast<long>(dummy)) = 0;
        }
    }

    const dlib::int64 maxInt =
        std::numeric_limits<dlib::int64>::max();

    const dlib::int64 factor =
        static_cast<dlib::int64>(inputCount) + 1;

    if (maxGain >= maxInt / factor - 1)
    {
        throw std::overflow_error(
            "gain range is too big");
    }

    const dlib::int64 incompatibleGain =
        -(maxGain + 1) * factor;

    for (std::size_t slot = 0; slot < slotCount; ++slot)
    {
        for (std::size_t input = 0; input < inputCount; ++input)
        {
            if (!costMatrix.getCost(slot, input).has_value())
            {
                gainMatrix(
                    static_cast<long>(slot),
                    static_cast<long>(input)) = incompatibleGain;
            }
        }
    }

    const std::vector<long> assignment =
        dlib::max_cost_assignment(gainMatrix);

    AssignmentResult result{};
    std::vector<bool> inputAssigned(inputCount, false);

    for (std::size_t slot = 0; slot < slotCount; ++slot)
    {
        const long column = assignment[slot];

        if (column < 0 ||
            static_cast<std::size_t>(column) >= inputCount)
        {
            continue;
        }

        const std::size_t input = static_cast<std::size_t>(column);
        const auto& cost = costMatrix.getCost(slot, input);

        if (!cost.has_value())
        {
            throw std::runtime_error(
                "Global implicit parametrization failed: no complete compatible assignment exists.");
        }

        inputAssigned[input] = true;

        result.assignments.push_back({
            inputParameters[input].id,
            programParameters[slot].id,
            cost.value()
            });

        result.totalCost += cost.value();
    }

    for (bool assigned : inputAssigned)
    {
        if (!assigned)
        {
            throw std::runtime_error(
                "Global implicit parametrization failed: not every input parameter was assigned");
        }
    }

    return result;
}

