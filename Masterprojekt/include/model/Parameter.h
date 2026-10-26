#pragma once

#include <string>
#include <optional>

struct Parameter
{
    int id = 0;
    std::string type;
    std::string color;
    std::optional<int> number;
    std::string descriptor;
};