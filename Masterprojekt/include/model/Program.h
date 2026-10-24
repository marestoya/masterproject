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

struct InstructionSegment
{
    bool isSlot = false;
    std::string text;
    int slotId = -1;
};

struct Program
{
    int id;
    std::string name;
    std::vector<ParameterSlot> parameters;

    // Additional data for the UI
    std::string domain;
    std::vector<std::vector<InstructionSegment>> instructions;
};

inline std::vector<Parameter> getFirstParameters(
    const Program& program)
{
    std::vector<Parameter> result;
    result.reserve(program.parameters.size());

    for (const ParameterSlot& slot : program.parameters)
    {
        if (slot.parameters.empty())
            throw std::runtime_error(
                "A program slot has no parameters.");

        result.push_back(slot.parameters.front());
    }

    return result;
}