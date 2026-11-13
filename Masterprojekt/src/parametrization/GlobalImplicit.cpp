
#include "parametrization/GlobalImplicit.h"

#include <dlib/matrix.h>
#include <dlib/optimization/max_cost_assignment.h>

#include <cmath>
#include <limits>
#include <stdexcept>
#include <vector>

namespace
{
    constexpr long long SCALE = 1'000'000;
    constexpr long INCOMPATIBLE_GAIN = -1'000'000'000L;

    long long maximumAssignmentGain(
        const dlib::matrix<long>& gains,
        const std::vector<std::size_t>& rows,
        const std::vector<std::size_t>& columns)
    {
        if (rows.empty())
            return 0;

        if (rows.size() != columns.size())
        {
            throw std::invalid_argument(
                "Assignment subproblem must have equal row and column counts");
        }

        const long n = static_cast<long>(rows.size());
        dlib::matrix<long> subGains(n, n);

        for (long r = 0; r < n; ++r)
        {
            for (long c = 0; c < n; ++c)
            {
                subGains(r, c) =
                    gains(rows[static_cast<std::size_t>(r)],
                        columns[static_cast<std::size_t>(c)]);
            }
        }

        const std::vector<long> assignment =
            dlib::max_cost_assignment(subGains);

        long long totalGain = 0;

        for (long r = 0; r < n; ++r)
        {
            const long c = assignment[static_cast<std::size_t>(r)];

            if (c < 0 || c >= n)
            {
                throw std::runtime_error(
                    "Invalid assignment returned by Hungarian algorithm");
            }

            totalGain += subGains(r, c);
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

    if (inputCount > slotCount)
    {
        throw std::invalid_argument(
            "Global implicit parametrization failed: more inputs than program slots");
    }

    AssignmentResult result;

    if (inputCount == 0)
        return result;
    double maxCost = 0.0;

    for (std::size_t slot = 0; slot < slotCount; ++slot)
    {
        for (std::size_t input = 0; input < inputCount; ++input)
        {
            const auto cost = costMatrix.getCost(slot, input);

            if (cost.has_value() && cost.value() > maxCost)
                maxCost = cost.value();
        }
    }

    // dummy columns representing unused slots
    dlib::matrix<long> gains(
        static_cast<long>(slotCount),
        static_cast<long>(slotCount));

    for (std::size_t slot = 0; slot < slotCount; ++slot)
    {
        for (std::size_t input = 0; input < inputCount; ++input)
        {
            const auto cost = costMatrix.getCost(slot, input);

            if (cost.has_value())
            {
                const double gain =
                    (maxCost - cost.value()) * static_cast<double>(SCALE);

                gains(static_cast<long>(slot), static_cast<long>(input)) =
                    static_cast<long>(std::llround(gain));
            }
            else
            {
                gains(static_cast<long>(slot), static_cast<long>(input)) =
                    INCOMPATIBLE_GAIN;
            }
        }

        for (std::size_t dummy = inputCount; dummy < slotCount; ++dummy)
        {
            gains(static_cast<long>(slot), static_cast<long>(dummy)) = 0;
        }
    }
    std::vector<std::size_t> allRows(slotCount);
    std::vector<std::size_t> allColumns(slotCount);

    for (std::size_t i = 0; i < slotCount; ++i)
    {
        allRows[i] = i;
        allColumns[i] = i;
    }

    const long long optimalGain =
        maximumAssignmentGain(gains, allRows, allColumns);

    {
        const std::vector<long> assignment =
            dlib::max_cost_assignment(gains);

        std::vector<bool> assignedInputs(inputCount, false);

        for (std::size_t slot = 0; slot < slotCount; ++slot)
        {
            const long column = assignment[slot];

            if (column < 0 ||
                static_cast<std::size_t>(column) >= inputCount)
            {
                continue;
            }

            const std::size_t input = static_cast<std::size_t>(column);

            if (!costMatrix.getCost(slot, input).has_value())
            {
                std::cerr
                    << "Incompatible assignment:"
                    << "\n  Program slot: " << programParameters[slot].type
                    << "\n  Input object: " << inputParameters[input].type
                    << '\n';

                throw std::runtime_error(
                    "Global implicit parametrization failed: "
                    "no complete compatible assignment exists");
            }

            assignedInputs[input] = true;
        }

        for (std::size_t input = 0; input < inputCount; ++input)
        {
            if (!assignedInputs[input])
            {
                throw std::runtime_error(
                    "Global implicit parametrization failed: "
                    "not all inputs could be assigned");
            }
        }
    }

    // Deterministic tie breaking
    std::vector<bool> usedSlots(slotCount, false);
    std::vector<bool> usedInputs(inputCount, false);
    std::vector<std::size_t> selectedSlots(inputCount);

    long long fixedGain = 0;

    for (std::size_t input = 0; input < inputCount; ++input)
    {
        bool found = false;

        for (std::size_t slot = 0; slot < slotCount; ++slot)
        {
            if (usedSlots[slot])
                continue;

            const auto cost = costMatrix.getCost(slot, input);

            if (!cost.has_value())
                continue;

            const long long candidateGain =
                gains(static_cast<long>(slot), static_cast<long>(input));

            std::vector<std::size_t> remainingRows;
            std::vector<std::size_t> remainingColumns;

            for (std::size_t otherSlot = 0;
                otherSlot < slotCount;
                ++otherSlot)
            {
                if (otherSlot != slot && !usedSlots[otherSlot])
                    remainingRows.push_back(otherSlot);
            }

            for (std::size_t otherInput = 0;
                otherInput < slotCount;
                ++otherInput)
            {
                if (otherInput < inputCount)
                {
                    if (!usedInputs[otherInput] && otherInput != input)
                        remainingColumns.push_back(otherInput);
                }
                else
                {
                    remainingColumns.push_back(otherInput);
                }
            }

            if (remainingRows.size() != remainingColumns.size())
                continue;

            const long long remainingGain =
                maximumAssignmentGain(
                    gains, remainingRows, remainingColumns);

            if (fixedGain + candidateGain + remainingGain == optimalGain)
            {
                selectedSlots[input] = slot;
                usedSlots[slot] = true;
                usedInputs[input] = true;
                fixedGain += candidateGain;
                found = true;
                break;
            }
        }

        if (!found)
        {
            throw std::runtime_error(
                "Global implicit parametrization failed: "
                "could not resolve a compatible optimal assignment");
        }
    }

    for (std::size_t input = 0; input < inputCount; ++input)
    {
        const std::size_t slot = selectedSlots[input];
        const auto cost = costMatrix.getCost(slot, input);

        if (!cost.has_value())
        {
            throw std::runtime_error(
                "Global implicit parametrization failed: "
                "selected assignment is incompatible");
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
