#pragma once

#include <Eigen/Geometry>

#include "model/Program.h"
#include "unity_simulation.hpp"

class RobotController
{
public:
    explicit RobotController(ursim::unity_simulation& simulation);

    bool initializeScene(Program& program);

    void pickUp(const Parameter& object);
    void place(const Eigen::Affine3f& world_T_tcp);
    void press();
    void dip(
        const Eigen::Affine3f& world_T_tcp,
        float depth);
private:
    ursim::unity_simulation& simulation_;
    Eigen::Affine3f lastTcpPose_ = Eigen::Affine3f::Identity();
    bool hasLastTcpPose_ = false;

    static constexpr const char* ROBOT_ID = "franka_1";

    Eigen::Matrix<double, 7, 1> getCurrentJointConfig() const;

    void moveLinear(
        const Eigen::Affine3f& start,
        const Eigen::Affine3f& end,
        uint64_t steps);
    void moveToTcpPose(const Eigen::Affine3f& world_T_tcp);
    void openGripper();
    void closeGripper();
    void waitUntilReached(
        const Eigen::Matrix<double, 7, 1>& target,
        double tolerance = 0.02,
        int timeoutMs = 10000);
};