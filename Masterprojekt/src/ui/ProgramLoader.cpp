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
            {},
            item.at("domain").get<std::string>(),
            {}
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

        programs.push_back(std::move(program));
    }

    return programs;
}