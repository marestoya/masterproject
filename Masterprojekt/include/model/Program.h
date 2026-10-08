#pragma once

#include <string>
#include <vector>

#include "Parameter.h"

struct Program
{
    int id;
    std::string name;

    std::vector<Parameter> parameters;
};