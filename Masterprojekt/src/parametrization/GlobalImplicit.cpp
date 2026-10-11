
#include "parametrization/GlobalImplicit.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <vector>

AssignmentResult GlobalImplicit::parametrize(
    const CostMatrix& costMatrix,
    const std::vector<Parameter>& programParameters,
    const std::vector<Parameter>& inputParameters) const
{
    if (costMatrix.getRowCount() != programParameters.size() ||
        costMatrix.getColumnCount() != inputParameters.size())
    {
        throw std::invalid_argument(
            "Cost matrix dimensions do not match the parameter lists!!"
        );
    }

    const std::size_t n = inputParameters.size();
    const std::size_t m = programParameters.size();

    if (n > m)
    {
        throw std::runtime_error(
            "Global implicit parametrization failed: not enough program parameters for a one-to-one assignment."
        );
    }

    AssignmentResult result{};
    result.totalCost = 0.0;

    if (n == 0)
    {
        return result;
    }

    double maxAbsCost = 0.0;

    for (std::size_t i = 0; i < m; ++i)
    {
        for (std::size_t j = 0; j < n; ++j)
        {
            const auto& cost = costMatrix.getCost(i, j);

            if (!cost.has_value())
            {
                continue;
            }

            if (!std::isfinite(cost.value()))
            {
                throw std::invalid_argument(
                    "Cost matrix contains a non-finite cost."
                );
            }

            maxAbsCost = std::max(
                maxAbsCost, std::abs(cost.value())
            );
        }
    }

   

    const double forbiddenCost =
        (2.0 * static_cast<double>(n) + 1.0)
        * (maxAbsCost + 1.0);

    if (!std::isfinite(forbiddenCost))
    {
        throw std::overflow_error("Could not construct a finite penalty for incompatible pairs");
    }

    std::vector<std::vector<double>> costs(
        n, std::vector<double>(m, forbiddenCost)
    );

    for (std::size_t inputIndex = 0;
        inputIndex < n;
        ++inputIndex)
    {
        for (std::size_t programIndex = 0;
            programIndex < m;
            ++programIndex)
        {
            const auto& cost = costMatrix.getCost(
                programIndex, inputIndex
            );

            if (cost.has_value())
            {
                costs[inputIndex][programIndex] = cost.value();
            }
        }
    }

    const double INF = std::numeric_limits<double>::infinity();

    std::vector<double> u(n + 1, 0.0);
    std::vector<double> v(m + 1, 0.0);
    std::vector<std::size_t> p(m + 1, 0);
    std::vector<std::size_t> way(m + 1, 0);

    for (std::size_t i = 1; i <= n; ++i)
    {
        p[0] = i;

        std::size_t j0 = 0;

        std::vector<double> minv(m + 1, INF);
        std::vector<bool> used(m + 1, false);

        do
        {
            used[j0] = true;
            const std::size_t i0 = p[j0];
            double delta = INF;
            std::size_t j1 = 0;

            for (std::size_t j = 1; j <= m; ++j)
            {
                if (used[j])
                {
                    continue;
                }

                const double current =
                    costs[i0 - 1][j - 1] - u[i0] - v[j];

                if (current < minv[j])
                {
                    minv[j] = current;
                    way[j] = j0;
                }

                if (minv[j] < delta)
                {
                    delta = minv[j];
                    j1 = j;
                }
            }

            if (!std::isfinite(delta))
            {
                throw std::runtime_error(
                    "Hungarian algorithm could not find an assignment"
                );
            }

            for (std::size_t j = 0; j <= m; ++j)
            {
                if (used[j])
                {
                    u[p[j]] += delta;
                    v[j] -= delta;
                }
                else if (j != 0)
                {
                    minv[j] -= delta;
                }
            }

            j0 = j1;

        } while (p[j0] != 0);

        do
        {
            const std::size_t j1 = way[j0];

            p[j0] = p[j1];
            j0 = j1;

        } while (j0 != 0);
    }

    std::vector<std::size_t> assignedProgramForInput(n, m);

    for (std::size_t j = 1; j <= m; ++j)
    {
        if (p[j] != 0)
        {
            assignedProgramForInput[p[j] - 1] = j - 1;
        }
    }

    for (std::size_t inputIndex = 0;
        inputIndex < n;
        ++inputIndex)
    {
        const std::size_t programIndex =
            assignedProgramForInput[inputIndex];

        if (programIndex >= m)
        {
            throw std::runtime_error(
                "Global implicit parametrization failed, no complete assignment exists."
            );
        }

        const auto& originalCost = costMatrix.getCost(
            programIndex, inputIndex
        );

        if (!originalCost.has_value())
        {
            throw std::runtime_error(
                "Global implicit parametrization failed, no complete compatible assignment exists."
            );
        }

        result.assignments.push_back({
            inputParameters[inputIndex].id,
            programParameters[programIndex].id,
            originalCost.value()
            });

        result.totalCost += originalCost.value();
    }

    return result;
}
