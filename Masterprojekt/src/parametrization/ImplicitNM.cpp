#include "parametrization/ImplicitNM.h"

#include <dlib/matrix.h>
#include <dlib/optimization/max_cost_assignment.h>

#include <cmath>
#include <limits>
#include <stdexcept>
#include <vector>

AssignmentResult NMImplicit::parametrize(
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

    AssignmentResult result{};

    if (inputCount == 0)
        return result;

    if (slotCount == 0)
    {
        throw std::runtime_error(
            "N:M parametrization failed: there are no program slots");
    }

    if (slotCount >
        static_cast<std::size_t>(
            std::numeric_limits<long>::max()) / inputCount)
    {
        throw std::overflow_error(
            "N:M assignment matrix is too large");
    }

    const std::size_t matrixSize = slotCount * inputCount;

    if (matrixSize >
        static_cast<std::size_t>(
            std::numeric_limits<long>::max()))
    {
        throw std::overflow_error(
            "N:M assignment matrix is too large");
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
                    "Cost matrix contains a non-finite cost");
            }

            if (!foundCompatiblePair || cost.value() > maxCost)
                maxCost = cost.value();

            foundCompatiblePair = true;
        }
    }

    if (!foundCompatiblePair)
    {
        throw std::runtime_error(
            "N:M parametrization failed: no compatible pairs exist");
    }
    constexpr double scale = 1'000'000.0;
    constexpr double slotReusePenalty = 0.1;

    dlib::matrix<dlib::int64> gainMatrix(
        static_cast<long>(matrixSize),
        static_cast<long>(matrixSize));

    dlib::int64 maxGain = 0;

    for (std::size_t input = 0; input < inputCount; ++input)
    {
        for (std::size_t slot = 0; slot < slotCount; ++slot)
        {
            const auto& cost = costMatrix.getCost(slot, input);

            for (std::size_t copy = 0; copy < inputCount; ++copy)
            {
                const std::size_t column =
                    slot * inputCount + copy;

                dlib::int64 gain = 0;

                if (cost.has_value())
                {
                    const long double baseGain =
                        (static_cast<long double>(maxCost)
                            - static_cast<long double>(cost.value()))
                        * scale;

                    const long double penalty =
                        static_cast<long double>(slotReusePenalty)
                        * static_cast<long double>(copy)
                        * scale;

                    gain = static_cast<dlib::int64>(
                        std::llround(baseGain - penalty));

                    if (gain > maxGain)
                        maxGain = gain;
                }

                gainMatrix(
                    static_cast<long>(input),
                    static_cast<long>(column)) = gain;
            }
        }
    }

    const dlib::int64 maxInt =
        std::numeric_limits<dlib::int64>::max();

    const dlib::int64 factor =
        static_cast<dlib::int64>(inputCount) + 1;

    if (maxGain >= maxInt / factor - 1)
    {
        throw std::overflow_error(
            "Gain range is too large");
    }

    const dlib::int64 incompatibleGain =
        -(maxGain + 1) * factor;

    for (std::size_t input = 0; input < inputCount; ++input)
    {
        for (std::size_t slot = 0; slot < slotCount; ++slot)
        {
            if (costMatrix.getCost(slot, input).has_value())
                continue;

            for (std::size_t copy = 0; copy < inputCount; ++copy)
            {
                const std::size_t column =
                    slot * inputCount + copy;

                gainMatrix(
                    static_cast<long>(input),
                    static_cast<long>(column)) = incompatibleGain;
            }
        }
    }

    for (std::size_t row = inputCount; row < matrixSize; ++row)
    {
        for (std::size_t column = 0; column < matrixSize; ++column)
            gainMatrix(
                static_cast<long>(row),
                static_cast<long>(column)) = 0;
    }

    const std::vector<long> assignment =
        dlib::max_cost_assignment(gainMatrix);

    std::vector<bool> inputAssigned(inputCount, false);

    for (std::size_t input = 0; input < inputCount; ++input)
    {
        const long column = assignment[input];

        if (column < 0 ||
            static_cast<std::size_t>(column) >= matrixSize)
        {
            throw std::runtime_error(
                "N:M parametrization returned an invalid assignment");
        }

        const std::size_t slot =
            static_cast<std::size_t>(column) / inputCount;

        const auto& cost = costMatrix.getCost(slot, input);

        if (!cost.has_value())
        {
            throw std::runtime_error(
                "N:M parametrization failed: no complete compatible assignment exists");
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
                "N:M parametrization failed: not every input parameter was assigned");
        }
    }

    return result;
}
