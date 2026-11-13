#include "ui/ProgramLoader.h"

#include <nlohmann/json.hpp>
#include <fstream>
#include <stdexcept>
#include <utility>

using json = nlohmann::json;

std::vector<Program> loadPrograms(const std::string& filePath)
{
    std::ifstream file(filePath);

    if (!file.is_open())
    {
        throw std::runtime_error(
            "Could not open programs file: " + filePath);
    }

    json root;
    file >> root;

    std::vector<Program> programs;

    for (const auto& item : root.at("programs"))
    {
            Program program{
         item.at("id").get<int>(),
         item.at("name").get<std::string>(),
         {},  //parameters
         {},  //commands
         item.at("domain").get<std::string>(),
         {}   //instructions
            };

        for (const auto& parameterJson : item.at("parameters"))
        {
            Parameter parameter{
                parameterJson.at("id").get<int>(),
                parameterJson.at("type").get<std::string>(),
                parameterJson.at("color").get<std::string>(),
                std::nullopt
            };

            if (parameterJson.contains("number")
                && !parameterJson.at("number").is_null())
            {
                parameter.number =
                    parameterJson.at("number").get<int>();
            }

            program.parameters.push_back({
                parameter.id,
                {parameter}
                });
        }

        for (const auto& instructionJson : item.at("instructions"))
        {
            std::vector<InstructionSegment> instruction;

            for (const auto& segmentJson : instructionJson)
            {
                if (segmentJson.contains("slot"))
                {
                    instruction.push_back({
                        true,
                        "",
                        segmentJson.at("slot").get<int>()
                        });
                }
                else
                {
                    instruction.push_back({
                        false,
                        segmentJson.at("text").get<std::string>(),
                        -1
                        });
                }
            }

            program.instructions.push_back(std::move(instruction));
        }

        if (item.contains("commands"))
        {
            for (const auto& commandJson : item.at("commands"))
            {
                const std::string type =
                    commandJson.at("type").get<std::string>();

                if (type == "PickUp")
                {
                    program.commands.push_back(
                        PickUpCommand{
                            commandJson.at("objectSlotId").get<int>()
                        }
                    );
                }
                else if (type == "Place")
                {
                    const std::string relation =
                        commandJson.at("relation").get<std::string>();

                    PlacementRelation placementRelation;

                    if (relation == "On")
                        placementRelation = PlacementRelation::On;
                    else if (relation == "In")
                        placementRelation = PlacementRelation::In;
                    else if (relation == "NextTo")
                        placementRelation = PlacementRelation::NextTo;
                    else if (relation == "InFrontOf")
                        placementRelation = PlacementRelation::InFrontOf;
                    else if (relation == "InMiddleOf")
                        placementRelation = PlacementRelation::InMiddleOf;
                    else if (relation == "Above")
                        placementRelation = PlacementRelation::Above;
                    else if (relation == "High")
                        placementRelation = PlacementRelation::High;
                    else if (relation == "Behind")
                        placementRelation = PlacementRelation::Behind;
                    else if (relation == "Beside")
                        placementRelation = PlacementRelation::Beside;
                    else if (relation == "NextTo2")
                        placementRelation = PlacementRelation::NextTo2;
                    else
                        throw std::runtime_error(
                            "Unknown placement relation: " + relation);

                    program.commands.push_back(
                        PlaceCommand{
                            commandJson.at("objectSlotId").get<int>(),
                            commandJson.at("targetSlotId").get<int>(),
                            placementRelation
                        }
                    );
                }
                else if (type == "Press")
                {
                    program.commands.push_back(
                        PressCommand{
                            commandJson.at("objectSlotId").get<int>()
                        }
                    );
                }
                else if (type == "Dip")
                {
                    const int objectSlotId = commandJson.at("objectSlotId").get<int>();
                    const int targetSlotId = commandJson.at("targetSlotId").get<int>();

                    const std::string relation =
                        commandJson.at("relation").get<std::string>();

                    if (relation != "In")
                        throw std::runtime_error("Dip only supports the In relation.");

                    program.commands.push_back(
                        DipCommand{
                            objectSlotId,
                            targetSlotId,
                            PlacementRelation::In
                        }
                    );
                }
                else
                {
                    throw std::runtime_error(
                        "Unknown command type: " + type);
                }
            }
        }

        programs.push_back(std::move(program));
    }

    return programs;
}