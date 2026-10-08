#include "programs/LaboratoryPrograms.h"

#include <iostream>


Program getSetUpExperiment()
{
    return {
        1,
        "Set Up Experiment",
        {
            {1, "beaker", "red"},
            {2, "testTube", "blue"},
            {3, "thermometer", "silver"},
            {4, "stirrer", "yellow"}
        }
    };
}


Program getStoreSamples()
{
    return {
        2,
        "Store Samples",
        {
            {1, "sampleTube", "red"},
            {2, "sampleTube", "blue"},
            {3, "storageBox", "white"}
        }
    };
}


Program getSynthesize()
{
    return {
        3,
        "Synthesize",
        {
            {1, "powder", "red"},
            {2, "liquid", "blue"},
            {3, "container", "glass"},
            {4, "stirrer", "silver"}
        }
    };
}


Program getPrepareSolution()
{
    return {
        4,
        "Prepare Solution",
        {
            {1, "powder", "green"},
            {2, "waterBottle", "blue"},
            {3, "beaker", "glass"}
        }
    };
}


// --------------------------------------------------
// Execution functions
// --------------------------------------------------

void executeSetUpExperiment(const std::vector<Parameter>& parameters)
{
    // 1. Place the beaker on the left tray.
    std::cout << "Place the " << parameters[0].color
        << " " << parameters[0].type
        << " on the left tray.\n";

    // 2. Place the test tube in the test tube rack.
    std::cout << "Place the " << parameters[1].color
        << " " << parameters[1].type
        << " in the test tube rack.\n";

    // 3. Pick up the thermometer.
    std::cout << "Pick up the "
        << parameters[2].type << ".\n";

    // 4. Move the thermometer to the beaker.
    std::cout << "Move the "
        << parameters[2].type
        << " to the "
        << parameters[0].type << ".\n";

    // 5. Place the stirrer next to the beaker.
    std::cout << "Place the " << parameters[3].color
        << " " << parameters[3].type
        << " next to the "
        << parameters[0].type << ".\n";
}


void executeStoreSamples(const std::vector<Parameter>& parameters)
{
    // 1. Pick up the red sample tube.
    std::cout << "Pick up the "
        << parameters[0].color << " "
        << parameters[0].type << ".\n";

    // 2. Move the sample tube to test tube rack.
    std::cout << "Move the "
        << parameters[0].type
        << " to the "
        << parameters[2].type << ".\n";

    // 3. Place the sample tube in the test tube rack.
    std::cout << "Place the "
        << parameters[0].type
        << " in the "
        << parameters[2].type << ".\n";

    // 4. Pick up the blue sample tube.
    std::cout << "Pick up the "
        << parameters[1].color << " "
        << parameters[1].type << ".\n";

    // 5. Place the second sample tube in the test tube rack.
    std::cout << "Place the "
        << parameters[1].type
        << " in the "
        << parameters[2].type << ".\n";
}


void executeSynthesize(const std::vector<Parameter>& parameters)
{
    // 1. Place the powder in the container.
    std::cout << "Place the " << parameters[0].color
        << " " << parameters[0].type
        << " in the "
        << parameters[2].type << ".\n";

    // 2. Place the liquid in the container.
    std::cout << "Place the " << parameters[1].color
        << " " << parameters[1].type
        << " in the "
        << parameters[2].type << ".\n";

    // 3. Pick up the stirrer.
    std::cout << "Pick up the "
        << parameters[3].type << ".\n";

    // 4. Move the stirrer into the container.
    std::cout << "Move the "
        << parameters[3].type
        << " into the "
        << parameters[2].type << ".\n";

    // 5. Shake the container.
    std::cout << "Shake the "
        << parameters[2].type << ".\n";
}


void executePrepareSolution(const std::vector<Parameter>& parameters)
{
    // 1. Place the powder in the beaker.
    std::cout << "Place the " << parameters[0].color
        << " " << parameters[0].type
        << " in the "
        << parameters[2].type << ".\n";

    // 2. Pick up the water bottle.
    std::cout << "Pick up the "
        << parameters[1].type << ".\n";

    // 3. Move the water bottle above the beaker.
    std::cout << "Move the "
        << parameters[1].type
        << " above the "
        << parameters[2].type << ".\n";

    // 4. Pour the water into the beaker.
    std::cout << "Pour the water into the "
        << parameters[2].type << ".\n";

    // 5. Shake the beaker.
    std::cout << "Shake the "
        << parameters[2].type << ".\n";
}