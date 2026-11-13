#pragma once

#include <string>
#include <optional>
#include <vector>
#include <Eigen/Geometry>

struct GraspableProperties
{
    Eigen::Affine3f object_T_grasp =
        Eigen::Affine3f::Identity();
};

struct ContainerSlot
{
    std::string id;

    Eigen::Affine3f container_T_slot =
        Eigen::Affine3f::Identity();

    Eigen::Affine3f container_T_release_point =
        Eigen::Affine3f::Identity();

    Eigen::Vector3f extents =
        Eigen::Vector3f::Zero();

    bool isDropOff = false;
    bool isOccupied = false;
    float maxVolume = 0.0f;
};

struct ContainerProperties
{
    std::vector<ContainerSlot> containerSlots;
};

struct Parameter
{
    int id = 0;
    std::string type;
    std::string color;
    std::optional<int> number;
    std::string descriptor;

    Eigen::Affine3f world_T_object =
        Eigen::Affine3f::Identity();

    std::optional<GraspableProperties> graspable;
    std::optional<ContainerProperties> container;
};