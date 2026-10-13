#pragma once

#include <string>
#include <vector>
#include <stdexcept>

#include "Parameter.h"

struct ParameterSlot
{
    int id;
    std::vector<Parameter> parameters;
};


struct Program
{
    int id;
    std::string name;

    std::vector<ParameterSlot> parameters;


};

inline std::vector<Parameter> getFirstParameters(
    const Program& program)
{
    std::vector<Parameter> result;
    result.reserve(program.parameters.size());

    for (const ParameterSlot& slot : program.parameters)
    {
        if (slot.parameters.empty())
        {
            throw std::runtime_error(
                "A program slot has no parameters."
            );
        }

        result.push_back(slot.parameters.front());
    }

    return result;
}