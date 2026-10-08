#include <iostream>

#include "programs/LaboratoryPrograms.h"
#include "input/InputParameters.h"

int main()
{
    Program program = getSetUpExperiment();

    std::vector<Parameter> inputParameters = getInputParameters();

    std::cout << "Program: " << program.name << "\n\n";

    std::cout << "Program parameters:\n";

    for (const Parameter& parameter : program.parameters)
    {
        std::cout << "  ID: " << parameter.id
            << ", Type: " << parameter.type
            << ", Color: " << parameter.color
            << "\n";
    }

    std::cout << "\nInput parameters:\n";

    for (const Parameter& parameter : inputParameters)
    {
        std::cout << "  ID: " << parameter.id
            << ", Type: " << parameter.type
            << ", Color: " << parameter.color
            << "\n";
    }

    return 0;
}