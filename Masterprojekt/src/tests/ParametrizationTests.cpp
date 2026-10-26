#include "tests/ParametrizationTests.h"

#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "model/Program.h"
#include "model/ProgramInstantiator.h"
#include "cost/CostModel.h"
#include "cost/CostMatrix.h"

#include "parametrization/ParametrizationMethod.h"
#include "parametrization/LocalParametrization.h"
#include "parametrization/GlobalImplicit.h"
#include "parametrization/ImplicitExhaustive.h"
#include "parametrization/GlobalExplicit.h"
#include "parametrization/ImplicitNM.h"

//namespace
//{
//    void printProgram(const Program& program)
//    {
//        for (const ParameterSlot& slot : program.parameters)
//        {
//            std::cout << "  Slot " << slot.id << ":\n";
//
//            for (const Parameter& parameter : slot.parameters)
//            {
//                std::cout << "    Parameter " << parameter.id
//                    << " | type=" << parameter.type
//                    << " | color=" << parameter.color;
//
//                if (parameter.number.has_value())
//                    std::cout << " | number=" << parameter.number.value();
//
//                std::cout << '\n';
//            }
//        }
//    }
//
//    void printAssignments(const AssignmentResult& result)
//    {
//        std::cout << "Assignments (input -> slot):\n";
//
//        for (const Assignment& assignment : result.assignments)
//        {
//            std::cout << "  " << assignment.inputParameterId
//                << " -> " << assignment.programParameterId
//                << " (cost=" << assignment.cost << ")\n";
//        }
//
//        std::cout << "Total cost: " << result.totalCost << '\n';
//    }
//
//    void assertAllInputsAssigned(
//        const AssignmentResult& result,
//        const std::vector<Parameter>& inputs,
//        const std::string& testName)
//    {
//        if (result.assignments.size() != inputs.size())
//        {
//            throw std::runtime_error(
//                testName + ": expected every input to be assigned exactly once");
//        }
//
//        for (const Parameter& input : inputs)
//        {
//            int occurrences = 0;
//
//            for (const Assignment& assignment : result.assignments)
//            {
//                if (assignment.inputParameterId == input.id)
//                    ++occurrences;
//            }
//
//            if (occurrences != 1)
//            {
//                throw std::runtime_error(
//                    testName + ": input " + std::to_string(input.id) +
//                    " was not assigned exactly once.");
//            }
//        }
//    }
//
//    void runOneToOneCase(
//        const std::string& testName,
//        const ParametrizationMethod& method,
//        const Program& program,
//        const std::vector<Parameter>& inputs)
//    {
//        std::cout << testName << '\n';
//
//        CostModel costModel(true);
//        const std::vector<Parameter> programParameters =
//            getFirstParameters(program);
//
//        CostMatrix costMatrix(programParameters, inputs, costModel);
//
//        const AssignmentResult result =
//            method.parametrize(costMatrix, programParameters, inputs);
//
//        assertAllInputsAssigned(result, inputs, testName);
//        printAssignments(result);
//
//        ProgramInstantiator instantiator;
//        const Program instantiatedProgram =
//            instantiator.instantiate(program, inputs, result);
//
//        std::cout << "Instantiated program:\n";
//        printProgram(instantiatedProgram);
//    }
//
//    void runNtoMCase(
//        const std::string& testName,
//        const Program& program,
//        const std::vector<Parameter>& inputs)
//    {
//        std::cout << testName << '\n';
//
//        CostModel costModel(false);
//        const std::vector<Parameter> programParameters =
//            getFirstParameters(program);
//
//        CostMatrix costMatrix(programParameters, inputs, costModel);
//
//        NMImplicit method;
//        const AssignmentResult result =
//            method.parametrize(costMatrix, programParameters, inputs);
//
//        assertAllInputsAssigned(result, inputs, testName);
//        printAssignments(result);
//
//        ProgramInstantiator instantiator;
//        const Program instantiatedProgram =
//            instantiator.instantiate(program, inputs, result);
//
//        std::cout << "Instantiated program:\n";
//        printProgram(instantiatedProgram);
//    }
//
//    Program makeTestProgram()
//    {
//        Program program{};
//        program.id = 1;
//        program.name = "Laboratory test program";
//
//        program.parameters = {
//            {1, {{1, "container", "red", 1}}},
//            {2, {{2, "mug", "blue", 2}}},
//            {3, {{3, "stirrer", "yellow", 3}}},
//            {4, {{4, "thermometer", "white", 4}}}
//        };
//
//        return program;
//    }
//
//    Program makeGreedyProgram()
//    {
//        Program program{};
//        program.id = 2;
//        program.name = "Greedy";
//
//        program.parameters = {
//            {1, {{1, "mug", "red", 1}}},
//            {2, {{2, "mug", "red", 1}}},
//            {3, {{3, "mug", "red", 2}}}
//        };
//
//        return program;
//    }
//
//    void runExplicitCase(const Program& program)
//    {
//        std::cout << "Explicit 1:1 parametrization\n";
//
//        const std::vector<Parameter> inputs = {
//            {101, "tallMug", "green", 1},
//            {102, "stirrer", "yellow", 3},
//            {103, "thermometer", "white", 4},
//            {104, "liquid", "blue", 2}
//        };
//
//        const std::vector<Assignment> userAssignments = {
//            {101, 2, 0.0},
//            {102, 3, 0.0},
//            {103, 4, 0.0},
//            {104, 1, 0.0}
//        };
//
//        GlobalExplicit method;
//        const AssignmentResult result =
//            method.applyUserAssignments(program, inputs, userAssignments);
//
//        assertAllInputsAssigned(result, inputs, "Explicit 1:1 parametrization");
//        printAssignments(result);
//
//        ProgramInstantiator instantiator;
//        const Program instantiatedProgram =
//            instantiator.instantiate(program, inputs, result);
//
//        std::cout << "Instantiated program:\n";
//        printProgram(instantiatedProgram);
//    }
//}
//void runGreedyVsOptimalCase()
//{
//    std::cout << "\nGreedy vs. optimal 1:1 assignment\n";
//
//    const Program program = makeGreedyProgram();
//
//    const std::vector<Parameter> inputs = {
//        {601, "mug", "red", 1},
//        {602, "mug", "red", 3},
//        {603, "mug", "red", 1}
//    };
//
//    CostModel costModel(true);
//
//    const std::vector<Parameter> programParameters =
//        getFirstParameters(program);
//
//    CostMatrix costMatrix(
//        programParameters, inputs, costModel);
//
//    LocalParametrization local;
//    GlobalImplicit hungarian;
//    ImplicitExhaustive exhaustive;
//
//    const AssignmentResult localResult =
//        local.parametrize(costMatrix, programParameters, inputs);
//
//    const AssignmentResult hungarianResult =
//        hungarian.parametrize(costMatrix, programParameters, inputs);
//
//    const AssignmentResult exhaustiveResult =
//        exhaustive.parametrize(costMatrix, programParameters, inputs);
//
//    assertAllInputsAssigned(
//        localResult, inputs, "Local");
//
//    assertAllInputsAssigned(
//        hungarianResult, inputs, "Hungarian");
//
//    assertAllInputsAssigned(
//        exhaustiveResult, inputs, "Exhaustive");
//
//    std::cout << "\nLocal result:\n";
//    printAssignments(localResult);
//
//    std::cout << "\nHungarian result:\n";
//    printAssignments(hungarianResult);
//
//    std::cout << "\nExhaustive result:\n";
//    printAssignments(exhaustiveResult);
//
//}
//
//void runParametrizationTests()
//{
//    const Program program = makeTestProgram();
//
//    // Case 1: Different object types
//    const std::vector<Parameter> differentObjects = {
//        {101, "tallMug", "green", 1},
//        {102, "stirrer", "yellow", 3},
//        {103, "thermometer", "white", 4},
//        {104, "liquid", "blue", 2}
//    };
//
//    // Case 2: Repeated/related object types
//    const std::vector<Parameter> repeatedObjects = {
//        {201, "mug", "blue", 2},
//        {202, "mug", "red", 1},
//        {203, "tallMug", "green", 3}
//    };
//
//    // Case 3: Fewer inputs than program slots.
//    const std::vector<Parameter> fewerInputs = {
//        {301, "stirrer", "yellow", 3},
//        {302, "tallMug", "green", 1}
//    };
//
//    LocalParametrization local;
//    GlobalImplicit hungarian;
//    ImplicitExhaustive exhaustive;
//
//    runOneToOneCase("Local 1:1 - different objects",
//        local, program, differentObjects);
//    runOneToOneCase("Local 1:1 - repeated objects",
//        local, program, repeatedObjects);
//    runOneToOneCase("Local 1:1 - fewer inputs than slots",
//        local, program, fewerInputs);
//
//    runOneToOneCase("Hungarian 1:1 - different objects",
//        hungarian, program, differentObjects);
//    runOneToOneCase("Hungarian 1:1 - repeated objects",
//        hungarian, program, repeatedObjects);
//    runOneToOneCase("Hungarian 1:1 - fewer inputs than slots",
//        hungarian, program, fewerInputs);
//
//    runOneToOneCase("Exhaustive 1:1 - different objects",
//        exhaustive, program, differentObjects);
//    runOneToOneCase("Exhaustive 1:1 - repeated objects",
//        exhaustive, program, repeatedObjects);
//    runOneToOneCase("Exhaustive 1:1 - fewer inputs than slots",
//        exhaustive, program, fewerInputs);
//
//    runExplicitCase(program);
//
//    // N:M case 1: different objects
//    const std::vector<Parameter> nmDifferentObjects = {
//        {401, "tallMug", "green", 1},
//        {402, "stirrer", "yellow", 3},
//        {403, "thermometer", "white", 4}
//    };
//
//    // N:M case 2: all inputs are exact matches for the stirrer slot
//    const std::vector<Parameter> nmSameSlot = {
//        {501, "stirrer", "yellow", 1},
//        {502, "stirrer", "yellow", 2},
//        {503, "stirrer", "yellow", 3}
//    };
//
//    runNtoMCase("N:M - different objects", program, nmDifferentObjects);
//    runNtoMCase("N:M - multiple inputs assigned to the same slot",
//        program, nmSameSlot);
//
//	runGreedyVsOptimalCase();
//    std::cout << "\n All completed!! \n";
//}
//
//
//
