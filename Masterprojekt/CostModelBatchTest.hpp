#pragma once

#include <algorithm>
#include <cmath>
#include <cctype>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "model/Parameter.h"
#include "cost/CostModel.h"
#include "cost/CostMatrix.h"
#include "model/Ontology.h"
#include "parametrization/GlobalImplicit.h"

namespace CostModelBatchTest
{
    using nlohmann::json;

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

    inline Parameter readParameter(const json& item)
    {
        Parameter p{};

        p.id = item.value("id", 0);
        p.type = item.value("type", std::string{});
        p.color = item.value("color", std::string{});
        p.descriptor = item.value("descriptor", std::string{});

        if (item.contains("number") && !item["number"].is_null())
        {
            p.number = item["number"].get<int>();
        }

        return p;
    }

    inline void run(const std::string& projectRoot = ".")
    {
        const json programsJson = readJson(
            projectRoot + "/programs/plausibility_programs.json");

        const json stimuliJson = readJson(
            projectRoot + "/programs/plausibility_stimuli.json");

        std::map<int, json> programs;

        for (const auto& program : programsJson.at("programs"))
        {
            programs[program.at("id").get<int>()] = program;
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

        std::size_t stimuliProcessed = 0;
        std::size_t comparisons = 0;
        std::size_t undefinedCosts = 0;
        std::size_t missingTypes = 0;
        std::size_t tiedMatches = 0;
        std::size_t assignmentFailures = 0;


        for (const auto& stimulus : stimuliJson.at("stimuli"))
        {
            const int programId =
                stimulus.at("programId").get<int>();

            const auto programIt = programs.find(programId);

            if (programIt == programs.end())
            {
                std::cout
                    << "\nERROR: No program found for programId "
                    << programId << '\n';
                continue;
            }

            const json& program = programIt->second;

            const std::string programName =
                program.value("name", std::string{ "Unnamed program" });

            const std::string domain =
                program.value("domain", std::string{});

            const std::string variant =
                stimulus.value("variant", std::string{});

            //delete if all variants wanted
            if (toLower(variant) != "c")
            {
                continue;
            }

            if (domain.empty())
            {
                std::cout
                    << "\nERROR: Program " << programId
                    << " has no domain\n";
                continue;
            }

            CostModel& costModel = getCostModel(domain);
            Ontology& ontology = *ontologies.at(toLower(domain));

            std::vector<Parameter> slots_;

            for (const auto& slotJson : program.at("parameters"))
            {
                slots_.push_back(readParameter(slotJson));
            }

            if (!stimulus.contains("objects") ||
                !stimulus.at("objects").is_array())
            {
                std::cout
                    << "\nERROR: Stimulus for program "
                    << programId
                    << "Variant " << variant
                    << "as no valid objects array\n";
                continue;
            }

            std::vector<Parameter> inputs_;

            for (const auto& inputJson : stimulus.at("objects"))
            {
                inputs_.push_back(readParameter(inputJson));
            }

            ++stimuliProcessed;

            

       
            for (const auto& input : inputs_)
            {
                

                const bool inputTypeExists =
                    ontology.containsType(input.type);

                if (!inputTypeExists)
                {
                    ++missingTypes;

                    std::cout
                        << "Input type "
                        << input.type
                        << "is not in the ontology\n";
                }

                std::vector<std::pair<int, double>> validCosts;

                
                for (const auto& slot : slots_)
                {
                    ++comparisons;

                    if (!ontology.containsType(slot.type))
                    {
                        ++missingTypes;

                        std::cout
                            << "  " << std::setw(7) << slot.id
                            << " | " << std::setw(19) << slot.type
                            << " | Invalid slot type\n";

                        continue;
                    }

                    const std::optional<double> cost =
                        costModel.calculateCost(slot, input);

                    if (!cost.has_value())
                    {
                        ++undefinedCosts;

                        std::cout
                            << "  " << std::setw(7) << slot.id
                            << " | " << std::setw(19) << slot.type
                            << " | Undefined\n";

                        continue;
                    }

                    validCosts.emplace_back(slot.id, *cost);

                    
                }

                if (validCosts.empty())
                {
                    std::cout
                        << "No valid slot costs for this input\n";
                    continue;
                }

                const double bestCost = std::min_element(
                    validCosts.begin(),
                    validCosts.end(),
                    [](const auto& a, const auto& b)
                    {
                        return a.second < b.second;
                    })->second;

                std::vector<int> bestSlots;

                for (const auto& [slotId, cost] : validCosts)
                {
                    if (std::abs(cost - bestCost) < 1e-9)
                    {
                        bestSlots.push_back(slotId);
                    }
                }

                
            
            try
            {
                CostMatrix costMatrix(slots_, inputs_, costModel);

                GlobalImplicit parametrizer;

                const auto result = parametrizer.parametrize(
                    costMatrix,
                    slots_,
                    inputs_);


                if (result.assignments.empty())
                {
                    std::cout
                        << "No assignments returned\n";
                }
              
            }
            catch (const std::exception& e)
            {
                ++assignmentFailures;

                std::cout
                    << "NM Implicit failed: "
                    << e.what() << '\n';
            }
        }

    }
}
