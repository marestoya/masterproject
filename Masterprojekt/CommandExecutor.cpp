#include "CommandExecutor.h"

#include <stdexcept>
#include <type_traits>
#include <variant>
#include <iostream>

CommandExecutor::CommandExecutor(RobotController& robotController)
    : robotController_(robotController)
{
}

Parameter& CommandExecutor::getParameterForSlot(
    Program& program,
    int slotId) const
{
    auto slot = std::find_if(
        program.parameters.begin(),
        program.parameters.end(),
        [slotId](const ParameterSlot& s)
        {
            return s.id == slotId;
        });
   
    if (slot == program.parameters.end())
        throw std::runtime_error("Command references an unknown parameter slot.");

    if (slot->parameters.size() != 1)
        throw std::runtime_error(
            "Pickup currently expects exactly one assigned object per slot.");

    return slot->parameters.front();
}

void CommandExecutor::executeCommand(
    Program& program,
    const Command& command)
{
    std::visit(
        [&](const auto& cmd)
        {
            using T = std::decay_t<decltype(cmd)>;

            if constexpr (std::is_same_v<T, PickUpCommand>)
            {
                Parameter& object =
                    getParameterForSlot(program, cmd.objectSlotId);

                robotController_.pickUp(object);
            }

            else if constexpr (std::is_same_v<T, DipCommand>)
            {
                Parameter& object =
                    getParameterForSlot(program, cmd.objectSlotId);

                Parameter& target =
                    getParameterForSlot(program, cmd.targetSlotId);

                if (!object.graspable)
                {
                    throw std::runtime_error(
                        "Cannot dip object: no graspable properties available.");
                }

                if (cmd.relation != PlacementRelation::In)
                {
                    throw std::runtime_error(
                        "Dip command currently only supports the In relation.");
                }

                constexpr float IN_HEIGHT = 0.03f;
                constexpr float DIP_DEPTH = 0.01f;

                Eigen::Affine3f world_T_object_dip = object.world_T_object;

                world_T_object_dip.translation() =
                    target.world_T_object.translation();

                world_T_object_dip.translation().z() += IN_HEIGHT;

                const Eigen::Affine3f world_T_tcp_dip =
                    world_T_object_dip * object.graspable->object_T_grasp;

                robotController_.dip(world_T_tcp_dip, DIP_DEPTH);
            }
            else if constexpr (std::is_same_v<T, PlaceCommand>)
            {
                //TODO: make this better, move the placement logic to containers
                Parameter& object =
                    getParameterForSlot(program, cmd.objectSlotId);

                Parameter& target =
                    getParameterForSlot(program, cmd.targetSlotId);

                if (!object.graspable)
                {
                    throw std::runtime_error(
                        "Cannot place object: no graspable properties available");
                }

                constexpr float DROP_HEIGHT = 0.02f;
                constexpr float ON_HEIGHT = 0.06f;
                constexpr float ABOVE_HEIGHT = 0.08f;
                constexpr float HIGH_HEIGHT = 0.2f;
                constexpr float IN_HEIGHT = 0.03f;
                constexpr float NEXT_TO_OFFSET = 0.20f;
                const float NEXT_TO2_OFFSET =
                    object.type == "chamomileTea" ? 0.0f : 0.3f;
                constexpr float BEHIND_OFFSET = 0.07f;
                constexpr float BESIDE_OFFSET = 0.05f;
                constexpr float IN_FRONT_OFFSET = 0.15f;

                Eigen::Vector3f placementPosition;

                
             
                
                if (object.type == "switch")
                {
                    placementPosition =
                        target.world_T_object.translation();

                    placementPosition.y()-= 0.2f;
                }
                else if (target.type == "testTubeHolderFoam")
                {
                    placementPosition =
                        target.world_T_object.translation();

                    placementPosition.z() += 0.1f;
                    placementPosition.x() += 0.08f;
                }
                else if (target.type == "testTubeHolderFoam" && program.name=="Set up experiment")
                {
                    placementPosition =
                        target.world_T_object.translation();

                    placementPosition.z() += 0.1f;
                }

                
                else if (object.type=="hammer" &&target.type == "box")
                {
                    placementPosition =
                        target.world_T_object.translation();

                    const Eigen::Vector3f targetY =
                        target.world_T_object.linear().col(0).normalized();

                    placementPosition -= targetY * 0.09f;
                    placementPosition.z() += 0.12f;
                }
              else if (object.type == "beaker" && target.type == "tray")
                {
                    placementPosition =
                        target.world_T_object.translation();

                    placementPosition.z() += 0.2f;
                    placementPosition.x() += 0.05f;
                }
              else if (object.type == "drill")
                {
                    placementPosition =
                        target.world_T_object.translation();

                    placementPosition.z() += 0.2f;
               
                }
              else if (object.type == "bowl")
                {
                    placementPosition =
                        target.world_T_object.translation();

                    placementPosition.z() += 0.1f;
                }
              else if (target.type == "bowl")
                {
                    placementPosition =
                        target.world_T_object.translation();

                    placementPosition.z() -= 0.05f;
                }
                else if (object.type == "testTube" && target.type == "beaker")
                {
                    placementPosition =
                        target.world_T_object.translation();
                    placementPosition.y() += 0.05f;
                }
                else if (object.type == "spoon" && target.type == "beaker")
                {
                    placementPosition =
                        target.world_T_object.translation();
                    placementPosition.y() += 0.05f;
                }
                else if (object.type == "spoon" && target.type == "cup")
                {
                    placementPosition =
                        target.world_T_object.translation();
                    placementPosition.z() -= 0.05f;
                }
                else if (object.type == "fork" && target.type == "cup")
                {
                    placementPosition =
                        target.world_T_object.translation();
                    placementPosition.z() += 0.09f;
                }
                else if (object.type == "chamomileTea" )
                {
                    placementPosition =
                        target.world_T_object.translation();
                    placementPosition.y() += 0.09f;
                    placementPosition.x() += 0.02f;
                    placementPosition.z() += 0.1f;
                }
                else if (object.type == "glass")
                {
                    placementPosition =
                        target.world_T_object.translation();
                    placementPosition.x() -= 0.03f;
                }
                else
                {
                    placementPosition =
                        target.world_T_object.translation();

                    const Eigen::Vector3f targetX =
                        target.world_T_object.linear().col(0).normalized();

                    const Eigen::Vector3f targetY =
                        target.world_T_object.linear().col(1).normalized();

                    switch (cmd.relation)
                    {
                    case PlacementRelation::On:
                        placementPosition.z() += ON_HEIGHT;
                        break;

                    case PlacementRelation::In:
                    case PlacementRelation::InMiddleOf:
                        placementPosition.z() += IN_HEIGHT;
                        break;

                    case PlacementRelation::NextTo:
                        placementPosition -= targetX * NEXT_TO_OFFSET;
                        placementPosition.z() += DROP_HEIGHT;
                        break;

                    case PlacementRelation::InFrontOf:
                        placementPosition += targetY * IN_FRONT_OFFSET;
                        placementPosition.z() += DROP_HEIGHT;
                        break;

                    case PlacementRelation::Above:
                        placementPosition.z() += ABOVE_HEIGHT;
                        break;

                    case PlacementRelation::High:
                        placementPosition.z() += HIGH_HEIGHT;
                        break;
                    case PlacementRelation::Behind:
                        placementPosition -= targetX * BEHIND_OFFSET;
                        placementPosition.z() += DROP_HEIGHT-0.01f;
                        break;
                    case PlacementRelation::Beside:
                        placementPosition -= targetX * BESIDE_OFFSET;
                        placementPosition.z() += DROP_HEIGHT - 0.02f;
                        break;
                    case PlacementRelation::NextTo2:
                        placementPosition -= targetX * NEXT_TO2_OFFSET;
                        placementPosition.z() += DROP_HEIGHT;
                        break;

                    default:
                        throw std::runtime_error(
                            "Unknown placement relation");
                    }
                }

                Eigen::Affine3f world_T_object_place =
                    object.world_T_object;

                world_T_object_place.translation() =
                    placementPosition;

                const Eigen::Affine3f world_T_tcp_place =
                    world_T_object_place *
                    object.graspable->object_T_grasp;

                robotController_.place(world_T_tcp_place);

                object.world_T_object = world_T_object_place;
            }

            else if constexpr (std::is_same_v<T, PressCommand>)
            {
               

                robotController_.press();
}
        },
        command);
}
void CommandExecutor::execute(Program& program)
{
    for (const Command& command : program.commands) {
        executeCommand(program, command);
    }

}