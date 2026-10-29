
#pragma once

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "model/Parameter.h"
#include "model/Ontology.h"
#include "cost/CostModel.h"
#include "cost/CostMatrix.h"
#include "parametrization/GlobalImplicit.h"

namespace GlobalImplicitAssignmentTest
{
    using nlohmann::json;

    struct TestCase
    {
        std::string programName;
        std::string domain;
        std::string variant;
        std::vector<Parameter> inputs;
    };

    inline std::string toLower(std::string value)
    {
        std::transform(
            value.begin(),
            value.end(),
            value.begin(),
            [](unsigned char c)
            {
                return static_cast<char>(std::tolower(c));
            });

        return value;
    }

    inline json readJson(const std::string& path)
    {
        std::ifstream file(path);

        if (!file.is_open())
        {
            throw std::runtime_error(
                "Could not open JSON file: " + path);
        }

        json data;
        file >> data;
        return data;
    }

    inline Parameter makeParameter(
        int id,
        const std::string& type,
        const std::string& color = "",
        const std::string& descriptor = "")
    {
        Parameter p{};
        p.id = id;
        p.type = type;
        p.color = color;
        p.descriptor = descriptor;
        return p;
    }

    inline void run(const std::string& projectRoot = ".")
    {
        const json programsJson = readJson(
            projectRoot + "/programs/parametrization_programs.json");

        std::map<std::string, json> programs;

        for (const auto& program : programsJson.at("programs"))
        {
            programs[toLower(
                program.at("name").get<std::string>())] = program;
        }

        std::map<std::string, std::unique_ptr<Ontology>> ontologies;
        std::map<std::string, std::unique_ptr<CostModel>> costModels;

        auto getCostModel = [&](const std::string& domain) -> CostModel&
            {
                const std::string key = toLower(domain);

                if (costModels.find(key) == costModels.end())
                {
                    ontologies[key] = std::make_unique<Ontology>(
                        projectRoot + "/ontologies/" + key + ".json");

                    costModels[key] = std::make_unique<CostModel>(
                        *ontologies[key]);
                }

                return *costModels[key];
            };

 

        const std::vector<TestCase> testCases =
        {
            // LABORATORY

          
            {
                "Set Up Experiment",
                "laboratory",
                "4 inputs",
                {
                    makeParameter(101, "beaker", "green"),
                    makeParameter(102, "tray", "brown"),
                    makeParameter(103, "spoon", "yellow"),
                    makeParameter(104, "box")
                }
            },
           

            // CATERING

            {
                "Clear Table",
                "catering",
                "2 inputs",
                {
                    makeParameter(101, "fork"),
                    makeParameter(102, "cup", "white")
                }
            },
            {
                "Clear Table",
                "catering",
                "4 inputs",
                {
                    makeParameter(101, "fork"),
                    makeParameter(102, "cup", "white"),
                    makeParameter(103, "plate", "", "small"),
                    makeParameter(104, "board")
                }
            },
            {
                "Set Up Table",
                "catering",
                "2 inputs",
                {
                    makeParameter(101, "coffee"),
                    makeParameter(102, "glass", "", "tall")
                }
            },
            {
                "Set Up Table",
                "catering",
                "4 inputs",
                {
                    makeParameter(101, "coffee"),
                    makeParameter(102, "glass", "", "tall"),
                    makeParameter(103, "spoon", "", "long"),
                    makeParameter(104, "bowl")
                }
            },

         

            // ELECTRONICS

            {
                "Prepare Sensor Assembly",
                "electronics",
                "2 inputs",
                {
                    makeParameter(101, "switch"),
                    makeParameter(102, "cable", "blue")
                }
            },
            {
                "Prepare Sensor Assembly",
                "electronics",
                "4 inputs",
                {
                    makeParameter(101, "switch"),
                    makeParameter(102, "cable", "blue"),
                    makeParameter(103, "cable", "red"),
                    makeParameter(104, "powerUnit")
                }
            },
            {
                "Sort Components",
                "electronics",
                "2 inputs",
                {
                    makeParameter(101, "switch", "green"),
                    makeParameter(102, "box", "blue")
                }
            },
            {
                "Sort Components",
                "electronics",
                "4 inputs",
                {
                    makeParameter(101, "switch", "green"),
                    makeParameter(102, "box", "blue"),
                    makeParameter(103, "cable", "black"),
                    makeParameter(104, "powerUnit")
                }
            }
        };

        GlobalImplicit parametrizer;

        std::size_t testsPassed = 0;
        std::size_t testsFailed = 0;


        for (std::size_t testIndex = 0;
            testIndex < testCases.size();
            ++testIndex)
        {
            const TestCase& test = testCases[testIndex];

            std::cout
                << "Test " << testIndex + 1 << ": "
                << test.programName << '\n'
                << "Domain: " << test.domain << '\n'
                << "Variant: " << test.variant << '\n';

            try
            {
                const auto programIt =
                    programs.find(toLower(test.programName));

                if (programIt == programs.end())
                {
                    throw std::runtime_error(
                        "Program not found in plausibility_programs.json: "
                        + test.programName);
                }

                const json& program = programIt->second;

                std::vector<Parameter> slots_;

                for (const auto& slotJson : program.at("parameters"))
                {
                    Parameter slot{};

                    slot.id = slotJson.value("id", 0);
                    slot.type = slotJson.value(
                        "type", std::string{});
                    slot.color = slotJson.value(
                        "color", std::string{});
                    slot.descriptor = slotJson.value(
                        "descriptor", std::string{});

                    if (slotJson.contains("number") &&
                        !slotJson["number"].is_null())
                    {
                        slot.number =
                            slotJson["number"].get<int>();
                    }

                    slots_.push_back(slot);
                }

                CostModel& costModel = getCostModel(test.domain);

                CostMatrix costMatrix(
                    slots_,
                    test.inputs,
                    costModel);

                const AssignmentResult result =
                    parametrizer.parametrize(
                        costMatrix,
                        slots_,
                        test.inputs);

                if (result.assignments.empty())
                {
                    std::cout << "No assignments!\n";
                }
                else
                {
                    for (const auto& assignment : result.assignments)
                    {
                        std::cout
                            << "Input ID "
                            << assignment.inputParameterId
                            << " -> Slot ID "
                            << assignment.programParameterId
                            << '\n';
                    }
                }

                ++testsPassed;
            }
            catch (const std::exception& e)
            {
                ++testsFailed;

                std::cout
                    << "TEST FAILED: "
                    << e.what() << '\n';
            }
        }

   
    }
}
