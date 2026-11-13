#pragma once

#include "model/Program.h"
#include "RobotController.h"

class CommandExecutor
{
public:
    explicit CommandExecutor(RobotController& robotController);

    void execute(Program& program);

private:
    RobotController& robotController_;

   Parameter& getParameterForSlot(
        Program& program,
        int slotId) const;

    void executeCommand(
        Program& program,
        const Command& command);
};