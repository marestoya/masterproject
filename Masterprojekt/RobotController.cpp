#include "RobotController.h"

#include <algorithm>
#include <chrono>
#include <stdexcept>
#include <thread>
#include <iostream>
#include <algorithm>
#include <cctype>

#include <franka_proxy_share/franka_proxy_util.hpp>

#include "model/Parameter.h"
#include "ObjectDefinitions.h"

bool equalsIgnoreCase(const std::string& a, const std::string& b)
{
    if (a.size() != b.size())
        return false;

    return std::equal(
        a.begin(), a.end(),
        b.begin(),
        [](unsigned char c1, unsigned char c2)
        {
            return std::tolower(c1) == std::tolower(c2);
        }
    );
}

RobotController::RobotController(ursim::unity_simulation& simulation)
    : simulation_(simulation)
{
}

Eigen::Matrix<double, 7, 1>
RobotController::getCurrentJointConfig() const
{
    auto feedback = simulation_.info.recent_feedback();

    if (!feedback)
        throw std::runtime_error("No URSIM feedback available.");

    auto robot = std::find_if(
        feedback->robot_info.begin(),
        feedback->robot_info.end(),
        [](const auto& r)
        {
            return r.robot_id == ROBOT_ID;
        });

    if (robot == feedback->robot_info.end())
        throw std::runtime_error("Franka robot not found in URSIM feedback.");

    if (robot->config.size() != 7)
        throw std::runtime_error("Invalid Franka joint configuration.");
    Eigen::Matrix<double, 7, 1> q_current;

    for (int i = 0; i < 7; ++i)
        q_current[i] = robot->config[i];

    //no clue why this is needed, but ik doesnt work without it
    q_current[0] *= -1.0;
    q_current[2] *= -1.0;
    q_current[3] *= -1.0;
    q_current[4] *= -1.0;
    q_current[5] *= -1.0;

    return q_current;
}

void RobotController::moveLinear(
    const Eigen::Affine3f& start,
    const Eigen::Affine3f& end,
    uint64_t steps)
{
    for (uint64_t i = 1; i <= steps; ++i)
    {
        const float t =
            static_cast<float>(i) / static_cast<float>(steps);

        const Eigen::Vector3f position =
            start.translation() +
            (end.translation() - start.translation()) * t;

        const Eigen::Quaternionf rotation =
            Eigen::Quaternionf(start.rotation()).slerp(
                t,
                Eigen::Quaternionf(end.rotation()));

        Eigen::Affine3f waypoint = Eigen::Affine3f::Identity();
        waypoint.translation() = position;
        waypoint.linear() = rotation.toRotationMatrix();
        moveToTcpPose(waypoint);
    }
}

void RobotController::moveToTcpPose(
    const Eigen::Affine3f& world_T_tcp)
{
    const Eigen::Affine3f TCP_T_NSA(
        Eigen::Translation3f(0.0f, 0.0f, -0.214f));

    const Eigen::Affine3f world_T_nsa =
        world_T_tcp * TCP_T_NSA;

    const Eigen::Matrix<double, 7, 1> q_current =
        getCurrentJointConfig();

    
    const auto goal_config =
        franka_proxy::franka_proxy_util::ik_fast_closest(
            world_T_nsa.cast<double>(),
            q_current);


    std::array<float, 7> goal_joints{};

    for (int i = 0; i < 7; ++i)
        goal_joints[i] = static_cast<float>(goal_config[i]);

    simulation_.control.move_robot_to(
        ursim::unity_joint_config<7>(goal_joints),
        ROBOT_ID);
 

    waitUntilReached(goal_config);
    lastTcpPose_ = world_T_tcp;
    hasLastTcpPose_ = true;
}

void RobotController::openGripper()
{
    simulation_.control.open_gripper();
    std::this_thread::sleep_for(std::chrono::seconds(1));
}

void RobotController::closeGripper()
{
    simulation_.control.close_gripper();
    std::this_thread::sleep_for(std::chrono::seconds(2));
}

void RobotController::pickUp(const Parameter& object)
{
    if (!object.graspable)
        throw std::runtime_error(
            "Cannot pick up object: no graspable properties available");

    const Eigen::Affine3f world_T_tcp =
        object.world_T_object * object.graspable->object_T_grasp;

    constexpr float APPROACH_HEIGHT = 0.15f;
    constexpr uint64_t STEPS = 1;

    Eigen::Affine3f world_T_tcp_approach = world_T_tcp;
    world_T_tcp_approach.translation().z() += APPROACH_HEIGHT;

    openGripper();
    moveToTcpPose(world_T_tcp_approach);
    moveLinear(
        world_T_tcp_approach,
        world_T_tcp,
        STEPS);
    closeGripper();
    moveLinear(
        world_T_tcp,
        world_T_tcp_approach,
        STEPS);
}

void RobotController::place(const Eigen::Affine3f& world_T_tcp)
{
    constexpr float APPROACH_HEIGHT = 0.2f;
    constexpr uint64_t STEPS = 1;

    Eigen::Affine3f world_T_tcp_approach = world_T_tcp;
    world_T_tcp_approach.translation().z() += APPROACH_HEIGHT;

    moveToTcpPose(world_T_tcp_approach);

    moveLinear(
        world_T_tcp_approach,
        world_T_tcp,
        STEPS);

    openGripper();

    moveLinear(
        world_T_tcp,
        world_T_tcp_approach,
        STEPS);
}

void RobotController::press()
{
    //needed, otherwise it moves before pressing
    if (!hasLastTcpPose_)
        throw std::runtime_error("No previous TCP pose available");

    constexpr float PRESS_DEPTH = 0.2f;
    constexpr uint64_t STEPS = 1;

    const Eigen::Affine3f world_T_tcp_start = lastTcpPose_;

    Eigen::Affine3f world_T_tcp_press = world_T_tcp_start;
    world_T_tcp_press.translation().z() -= PRESS_DEPTH;
    closeGripper();
    moveLinear(
        world_T_tcp_start,
        world_T_tcp_press,
        STEPS);
   
    moveLinear(
        world_T_tcp_press,
        world_T_tcp_start,
        STEPS);
    openGripper();

}
bool RobotController::initializeScene(Program& program)
{

    std::optional<ursim::unity_feedback> feedback;

    for (int attempt = 0; attempt < 50; ++attempt)
    {
        feedback = simulation_.info.recent_feedback();

        if (feedback.has_value())
            break;

        std::this_thread::sleep_for(
            std::chrono::milliseconds(100));
    }

    if (!feedback.has_value())
    {
        std::cerr << "Could not retrieve feedback from ursim\n";
        return false;
    }

    std::vector<bool> usedParts(
        feedback->part_info.size(), false);

    for (ParameterSlot& slot : program.parameters)
    {
        for (Parameter& parameter : slot.parameters)
        {
            bool found = false;

            for (std::size_t i = 0;
                i < feedback->part_info.size();
                ++i)
            {
                if (usedParts[i])
                    continue;

                const auto& part = feedback->part_info[i];

                const std::string& actualColor =
                    equalsIgnoreCase(part.name, "testTube")
                    ? part.liquid.color
                    : part.color;

            
                // Match by object type. Only check color if the program specifies one.
                if (equalsIgnoreCase(part.name, parameter.type) &&
                    (parameter.color.empty() ||
                        equalsIgnoreCase(actualColor, parameter.color)))
                {
                    parameter.world_T_object = part.transformation;

                    // Remove Unity's scale from the rotation matrix.
                    // Franka Proxy expects a rigid transformation.
                    for (int i = 0; i < 3; ++i)
                    {
                        parameter.world_T_object.linear().col(i).normalize();
                    }
                    ObjectDefinitions::apply(parameter);

                    usedParts[i] = true;
                    found = true;

                    break;
                }
            }

            if (!found)
            {
                std::cerr
                    << "Required object missing: "
                    << parameter.color << " "
                    << parameter.type
                    << " (slot " << slot.id << ")\n";

                return false;
            }
        }
    }


    return true;
}

void RobotController::waitUntilReached(
    const Eigen::Matrix<double, 7, 1>& target,
    double tolerance,
    int timeoutMs)
{ //needed, otherwise the calculation is faster than the simulation
    const auto start = std::chrono::steady_clock::now();

    while (true)
    {
        const auto current = getCurrentJointConfig();

        const double error = (current - target).norm();

        if (error < tolerance)
            return;

        const auto elapsed =
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - start).count();

        if (elapsed > timeoutMs)
            throw std::runtime_error(
                "Robot did not reach target configuration in time");

        std::this_thread::sleep_for(
            std::chrono::milliseconds(10));
    }
}

void RobotController::dip(
    const Eigen::Affine3f& world_T_tcp,
    float depth)
{
    constexpr float APPROACH_HEIGHT = 0.15f;
    constexpr uint64_t STEPS = 1;

    Eigen::Affine3f world_T_tcp_approach = world_T_tcp;
    world_T_tcp_approach.translation().z() += APPROACH_HEIGHT;

    Eigen::Affine3f world_T_tcp_dip = world_T_tcp;
    world_T_tcp_dip.translation().z() -= depth;

    moveToTcpPose(world_T_tcp_approach);

    moveLinear(
        world_T_tcp_approach,
        world_T_tcp_dip,
        STEPS);

    moveLinear(
        world_T_tcp_dip,
        world_T_tcp_approach,
        STEPS);
}