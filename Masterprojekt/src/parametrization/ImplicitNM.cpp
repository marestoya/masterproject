
#include "parametrization/ImplicitNM.h"

#include <dlib/matrix.h>
#include <dlib/optimization/max_cost_assignment.h>

#include <cmath>
#include <limits>
#include <stdexcept>
#include <vector>

namespace
{
    using GainMatrix = dlib::matrix<dlib::int64>;

    GainMatrix makeSubmatrix(
        const GainMatrix& matrix,
        const std::vector<std::size_t>& rows,
        const std::vector<std::size_t>& columns)
    {
        if (rows.size() != columns.size())
        {
            throw std::invalid_argument(
                "Assignment submatrix must be square");
        }

        const auto dimension = static_cast<long>(rows.size());
        GainMatrix submatrix(dimension, dimension);

        for (std::size_t row = 0; row < rows.size(); ++row)
        {
            for (std::size_t column = 0;
                column < columns.size();
                ++column)
            {
                submatrix(
                    static_cast<long>(row),
                    static_cast<long>(column)) =
                    matrix(
                        static_cast<long>(rows[row]),
                        static_cast<long>(columns[column]));
            }
        }

        return submatrix;
    }


    dlib::int64 maximumAssignmentGain(
        const GainMatrix& matrix,
        const std::vector<std::size_t>& rows,
        const std::vector<std::size_t>& columns)
    {
        if (rows.empty())
            return 0;

        const GainMatrix submatrix =
            makeSubmatrix(matrix, rows, columns);

        const std::vector<long> assignment =
            dlib::max_cost_assignment(submatrix);

        dlib::int64 totalGain = 0;

        for (std::size_t row = 0; row < rows.size(); ++row)
        {
            totalGain += submatrix(
                static_cast<long>(row),
                assignment[row]);
        }

        return totalGain;
    }
}

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

    GainMatrix gainMatrix(
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

    //dummy slots 
    for (std::size_t row = inputCount; row < matrixSize; ++row)
    {
        for (std::size_t column = 0; column < matrixSize; ++column)
        {
            gainMatrix(
                static_cast<long>(row),
                static_cast<long>(column)) = 0;
        }
    }

    const std::vector<std::size_t> allRows = [&]()
        {
            std::vector<std::size_t> indices(matrixSize);
            for (std::size_t i = 0; i < matrixSize; ++i)
                indices[i] = i;
            return indices;
        }();

    const std::vector<std::size_t> allColumns = allRows;

    const GainMatrix originalMatrix = gainMatrix;

    const std::vector<long> initialAssignment =
        dlib::max_cost_assignment(originalMatrix);

    dlib::int64 optimalGain = 0;

    for (std::size_t row = 0; row < matrixSize; ++row)
    {
        optimalGain += originalMatrix(
            static_cast<long>(row),
            initialAssignment[row]);
    }

    std::vector<std::size_t> remainingRows = allRows;
    std::vector<std::size_t> remainingColumns = allColumns;

    std::vector<long> assignment(matrixSize, -1);

    dlib::int64 fixedGain = 0;

    //deterministic tie breaker (first choice)
    for (std::size_t input = 0; input < inputCount; ++input)
    {
        bool selected = false;

        for (std::size_t slot = 0;
            slot < slotCount && !selected;
            ++slot)
        {
            if (!costMatrix.getCost(slot, input).has_value())
                continue;

            for (std::size_t copy = 0;
                copy < inputCount && !selected;
                ++copy)
            {
                const std::size_t candidateColumn =
                    slot * inputCount + copy;

                bool columnAvailable = false;

                for (const std::size_t column : remainingColumns)
                {
                    if (column == candidateColumn)
                    {
                        columnAvailable = true;
                        break;
                    }
                }

                if (!columnAvailable)
                    continue;

                std::vector<std::size_t> testRows;
                testRows.reserve(remainingRows.size());

                for (const std::size_t row : remainingRows)
                {
                    if (row != input)
                        testRows.push_back(row);
                }

                std::vector<std::size_t> testColumns;
                testColumns.reserve(remainingColumns.size());

                for (const std::size_t column : remainingColumns)
                {
                    if (column != candidateColumn)
                        testColumns.push_back(column);
                }

                const dlib::int64 candidateGain =
                    gainMatrix(
                        static_cast<long>(input),
                        static_cast<long>(candidateColumn));

                const dlib::int64 remainingGain =
                    maximumAssignmentGain(
                        gainMatrix,
                        testRows,
                        testColumns);

                if (fixedGain + candidateGain + remainingGain
                    != optimalGain)
                {
                    continue;
                }

                assignment[input] =
                    static_cast<long>(candidateColumn);

                fixedGain += candidateGain;

                remainingRows = std::move(testRows);
                remainingColumns = std::move(testColumns);

                selected = true;
            }
        }

        if (!selected)
        {
            throw std::runtime_error(
                "N:M parametrization failed: unable to resolve "
                "an optimal assignment deterministically");
        }
    }

    if (!remainingRows.empty())
    {
        const GainMatrix remainingMatrix =
            makeSubmatrix(
                gainMatrix,
                remainingRows,
                remainingColumns);

        const std::vector<long> remainingAssignment =
            dlib::max_cost_assignment(remainingMatrix);

        for (std::size_t row = 0;
            row < remainingRows.size();
            ++row)
        {
            assignment[remainingRows[row]] =
                static_cast<long>(
                    remainingColumns[
                        static_cast<std::size_t>(
                            remainingAssignment[row])]);
        }
    }

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
                "N:M parametrization failed: no complete compatible "
                "assignment exists");
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
