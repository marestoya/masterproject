#pragma once

#include <vector>

#include "Assignment.h"

struct AssignmentResult
{
    std::vector<Assignment> assignments;
    double totalCost = 0.0;
};