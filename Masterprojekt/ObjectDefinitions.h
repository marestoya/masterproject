#pragma once

#include <string>
#include <unordered_map>
#include <optional>

#include "model/Parameter.h"

namespace ObjectDefinitions
{
    inline Eigen::Affine3f makeTransform(
        const Eigen::Vector3f& position,
        const Eigen::Vector3f& axis,
        float angle)
    {
        Eigen::Affine3f transform = Eigen::Affine3f::Identity();

        transform.translation() = position;
        transform.linear() =
            Eigen::AngleAxisf(angle, axis.normalized()).toRotationMatrix();

        return transform;
    }

    inline Eigen::Affine3f makePosition(float x, float y, float z)
    {
        Eigen::Affine3f transform = Eigen::Affine3f::Identity();
        transform.translation() = Eigen::Vector3f(x, y, z);
        return transform;
    }

    inline void apply(Parameter& parameter)
    {
        parameter.graspable.reset();
        parameter.container.reset();

        const float pi = 3.14159265359f;
        const Eigen::Vector3f xAxis = Eigen::Vector3f::UnitX();
        const Eigen::Vector3f yAxis = Eigen::Vector3f::UnitY();

        //GRASPABLE
        //TODO: merge into one function
        if (parameter.type == "bolt")
        {
            parameter.graspable = GraspableProperties{
                makeTransform(Eigen::Vector3f(0, 0, 0), xAxis, pi)
            };
        }

        else if (parameter.type == "gear")
        {
            parameter.graspable = GraspableProperties{
                makeTransform(
                    Eigen::Vector3f(0, 0, 0),
                    xAxis,
                    pi
                )
            };
        }
        else if (parameter.type == "battery")
        {
            parameter.graspable = GraspableProperties{
                makeTransform(
                    Eigen::Vector3f(0, 0, 0),
                    xAxis,
                    pi
                )
            };
        }
        else if (parameter.type == "powerUnit")
        {
            parameter.graspable = GraspableProperties{
                makeTransform(
                    Eigen::Vector3f(0, 0, 0),
                    xAxis,
                    pi
                )
            };
        }
        else if (parameter.type == "beaker")
        {
            parameter.graspable = GraspableProperties{
                makeTransform(
                    Eigen::Vector3f(-0.02f, 0, 0.03f),
                    xAxis,
                    pi
                )
            };
        }
        else if (parameter.type == "testTube")
        {
            parameter.graspable = GraspableProperties{
                makeTransform(
                    Eigen::Vector3f(0, 0, 0.09f),
                    xAxis,
                    pi
                )
            };
        }
    
        else if (parameter.type == "thermometer")
        {
            parameter.graspable = GraspableProperties{
                makeTransform(
                    Eigen::Vector3f(0, 0, 0),
                    xAxis,
                    pi
                )
            };
        }
        else if (parameter.type == "wire")
        {
            parameter.graspable = GraspableProperties{
                makeTransform(
                    Eigen::Vector3f(0, 0, 0.02f),
                    xAxis,
                    pi
                )
            };
        }
        else if (parameter.type == "switch")
        {
            parameter.graspable = GraspableProperties{
                makeTransform(
                    Eigen::Vector3f(0, 0, 0),
                    xAxis,
                    pi
                )
            };
        }
        else if (parameter.type == "button")
        {
            parameter.graspable = GraspableProperties{
                makeTransform(
                    Eigen::Vector3f(0, 0, 0),
                    xAxis,
                    pi
                )
            };
        }
        else if (parameter.type == "hammer")
        {
            parameter.graspable = GraspableProperties{
                makeTransform(
                    Eigen::Vector3f(0.09f, -0.03f, 0.05f),
                    xAxis,
                    pi
                )
            };
        }
        else if (parameter.type == "drill")
        {
            parameter.graspable = GraspableProperties{
                makeTransform(
                    Eigen::Vector3f(0.0f, 0, 0.08f),
                    xAxis,
                    pi
                )
            };
        }
        
        else if (parameter.type == "matchaTeaBag")
        {
            parameter.graspable = GraspableProperties{
                makeTransform(Eigen::Vector3f(0.05f, 0.2f, 0), xAxis, pi)
            };
        }
        else if (parameter.type == "chamomileTea" )
            {
                parameter.graspable = GraspableProperties{
                    makeTransform(Eigen::Vector3f(0, -0.16f, 0), xAxis, pi)
                };
                }
        else if (parameter.type == "spoon")
        {
            parameter.graspable = GraspableProperties{
                makeTransform(Eigen::Vector3f(0, 0, 0.09f), xAxis, pi)
            };
        }
        else if (parameter.type == "fork")
        {
         
            parameter.graspable = GraspableProperties{
                makeTransform(
                    Eigen::Vector3f(0.05f, 0, -0.07f),
                    xAxis,
                    pi
                )
            };
        }
        else if (parameter.type == "plate" ||
            parameter.type == "smallPlate")
        {
            const float graspRadius =
                parameter.type == "plate" ? 0.12f : 0.06f;

            const float graspHeight =
                parameter.type == "plate" ? 0.045f : 0.06f;

            parameter.graspable = GraspableProperties{
                makeTransform(
                    Eigen::Vector3f(graspRadius, 0.0f, graspHeight),
                    xAxis,
                    pi
                )
            };
        }
        else if (parameter.type == "bowl")
        {
            parameter.graspable = GraspableProperties{
                makeTransform(
                    Eigen::Vector3f(0.09f, 0, 0.03f),
                    xAxis,
                    pi
                )
            };
        }
        else if (parameter.type == "conductor")
        {
            parameter.graspable = GraspableProperties{
                makeTransform(
                    Eigen::Vector3f(0, 0, 0.02f),
                    xAxis,
                    pi
                )
            };
            }
        else if (parameter.type == "longSpoon")
        {
            parameter.graspable = GraspableProperties{
                makeTransform(
                    Eigen::Vector3f(0, -0.05f, 0.02f),
                    xAxis,
                    pi
                )
            };
        }

        else if (parameter.type == "cup" ||
            parameter.type == "largeCup")
        {
            GraspableProperties grasp;

            if (parameter.type == "cup")
                grasp.object_T_grasp = makeTransform(
                    Eigen::Vector3f(0, 0, 0.035f), xAxis, pi);
        
            else
                grasp.object_T_grasp = makeTransform(
                    Eigen::Vector3f(0, 0, 0.04f), xAxis, pi);

            parameter.graspable = grasp;
        }
        else if (parameter.type == "glass")
        {
            parameter.graspable = GraspableProperties{
                makeTransform(
                    Eigen::Vector3f(0.01f, 0, 0.035f),
                    xAxis,
                    pi
                )
            };
        }

       //TODO: THIS ISNT USED SO FAR, merge this logic with the hard coded placement logic

        if (parameter.type == "Box")
        {
            ContainerProperties container;
            ContainerSlot slot;

            slot.id = "0";
            slot.container_T_slot = makePosition(0.01f, 0, -0.01f);
            slot.container_T_release_point = makePosition(0.01f, 0, 0.09f);
            slot.extents = Eigen::Vector3f(0.255f, 0.17f, 0.10f);
            slot.isDropOff = true;
            slot.maxVolume = 0.004335f;

            container.containerSlots.push_back(slot);
            parameter.container = container;
        }
        else if (parameter.type == "tray" ||
            parameter.type == "Vat")
        {
            ContainerProperties container;

            ContainerSlot slot;
            slot.id = "0";
            slot.container_T_slot = makePosition(0, 0, -0.001f);
            slot.container_T_release_point = makePosition(0, 0, 0.03f);
            slot.extents = Eigen::Vector3f(0.255f, 0.17f, 0.10f);
            slot.isDropOff = true;
            slot.maxVolume = 0.004335f;

            container.containerSlots.push_back(slot);
            parameter.container = container;
        }
        else if (parameter.type == "bowl")
        {
            ContainerProperties container;
            ContainerSlot slot;

            slot.id = "0";
            slot.container_T_slot = makePosition(0, 0, 0.01f);
            slot.container_T_release_point = makePosition(0, 0, 0.09f);
            slot.extents = Eigen::Vector3f(0.18f, 0.18f, 0.08f);
            slot.isDropOff = true;
            slot.maxVolume = 0.002592f;

            container.containerSlots.push_back(slot);
            parameter.container = container;
        }
        else if (parameter.type == "cup" ||
            parameter.type == "glass" ||
            parameter.type == "largeCup")
        {
            ContainerProperties container;
            ContainerSlot slot;

            slot.id = "0";
            slot.isDropOff = true;

            if (parameter.type == "cup")
            {
                slot.container_T_slot = makePosition(0, 0, 0.005f);
                slot.container_T_release_point = makePosition(0, 0, 0.04f);
                slot.extents = Eigen::Vector3f(0.06f, 0.06f, 0.08f);
                slot.maxVolume = 0.000288f;
            }
            else if (parameter.type == "glass")
            {
                slot.container_T_slot = makePosition(0, 0, 0.005f);
                slot.container_T_release_point = makePosition(0, 0, 0.06f);
                slot.extents = Eigen::Vector3f(0.07f, 0.07f, 0.12f);
                slot.maxVolume = 0.000588f;
            }
            else
            {
                slot.container_T_slot = makePosition(0, 0, 0.005f);
                slot.container_T_release_point = makePosition(0, 0, 0.06f);
                slot.extents = Eigen::Vector3f(0.08f, 0.08f, 0.12f);
                slot.maxVolume = 0.000768f;
            }

            container.containerSlots.push_back(slot);
            parameter.container = container;
        }
        else if (parameter.type == "cuttingBoard")
        {
    
            ContainerProperties container;

            const float xs[] = { 0.1225f, 0.1225f, -0.1225f, -0.1225f };
            const float ys[] = { 0.06f, -0.06f, 0.06f, -0.06f };

            for (int i = 0; i < 4; ++i)
            {
                ContainerSlot slot;

                slot.id = std::to_string(i);
                slot.container_T_slot =
                    makePosition(xs[i], ys[i], 0.029f);
                slot.container_T_release_point =
                    makePosition(xs[i], ys[i], 0.0065f);

                slot.extents = Eigen::Vector3f(0.14f, 0.085f, 0.05f);
                slot.isDropOff = false;
                slot.maxVolume = 0.000595f;

                container.containerSlots.push_back(slot);
            }

            parameter.container = container;
        }
        else if (parameter.type == "Plate")
        {
            ContainerProperties container;
            ContainerSlot slot;

            slot.id = "0";

            slot.container_T_slot =
                makePosition(0, 0, 0.015f);

            slot.container_T_release_point =
                makePosition(0, 0, 0.045f);

            slot.extents = Eigen::Vector3f(0.14f, 0.14f, 0.025f);
            slot.isDropOff = true;
            slot.maxVolume = 0.0005f;

            container.containerSlots.push_back(slot);
            parameter.container = container;
            }
        else if (parameter.type == "smallPlate")
        {
            ContainerProperties container;
            ContainerSlot slot;

            slot.id = "0";

            slot.container_T_slot =
                makePosition(0, 0, 0.01f);

            slot.container_T_release_point =
                makePosition(0, 0, 0.035f);

            slot.extents = Eigen::Vector3f(0.09f, 0.09f, 0.02f);
            slot.isDropOff = true;
            slot.maxVolume = 0.0002f;

            container.containerSlots.push_back(slot);
            parameter.container = container;
            }
     
        else if (parameter.type == "longSpoon")
        {
            ContainerProperties container;
            ContainerSlot slot;

            slot.id = "0";

            slot.container_T_slot =
                makePosition(0.12f, 0, 0.01f);

            slot.container_T_release_point =
                makePosition(0.12f, 0, 0.04f);

            slot.extents = Eigen::Vector3f(0.045f, 0.025f, 0.02f);
            slot.isDropOff = true;
            slot.maxVolume = 0.00002f;

            container.containerSlots.push_back(slot);
            parameter.container = container;
            }
    }
}