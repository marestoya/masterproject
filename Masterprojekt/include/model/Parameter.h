#pragma once

#include <string>
#include <optional>

struct Parameter
{
    int id;
    std::string type;
    std::string color;
	std::optional<int> number = std::nullopt;
};