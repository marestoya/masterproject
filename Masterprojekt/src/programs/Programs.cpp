#pragma once

#include "model/Program.h"
#include "model/Command.h"

#include <string>
//TODO: DELEEEEEEEEEEEETE
namespace
{
    inline ParameterSlot makeSlot(
        int slotId,
        int parameterId,
        const std::string& type,
        const std::string& color)
    {
        return {
            slotId,
            {{parameterId, type, color}}
        };
    }
}

// 1. Set up experiment
inline Program getSetUpExperiment()
{
    Program program;
    program.id = 1;
    program.name = "Set up experiment";
    program.domain = "Laboratory";
    program.parameters = {
        makeSlot(1, 1, "beaker", "red"),
        makeSlot(2, 2, "tray", "grey"),
        makeSlot(3, 3, "testTube", "blue"),
        makeSlot(4, 4, "rack", ""),
        makeSlot(5, 5, "thermometer", ""),
        makeSlot(6, 6, "longSpoon", "yellow")
    };
    program.commands = {
        PlaceCommand{1, 2, PlacementRelation::On},
        PlaceCommand{3, 4, PlacementRelation::In},
        PickUpCommand{5},
        PlaceCommand{5, 1, PlacementRelation::NextTo},
        PlaceCommand{6, 1, PlacementRelation::NextTo}
    };
    return program;
}

// 2. Store samples
inline Program getStoreSamples()
{
    Program program;
    program.id = 2;
    program.name = "Store samples";
    program.domain = "Laboratory";
    program.parameters = {
        makeSlot(1, 1, "testTube", "red"),
        makeSlot(2, 2, "testTubeRack", ""),
        makeSlot(3, 3, "sampleTube", "blue"),
        makeSlot(4, 4, "testTube", "white")
    };
    program.commands = {
        PickUpCommand{1},
        PlaceCommand{1, 2, PlacementRelation::In},
        PickUpCommand{3},
        PlaceCommand{3, 2, PlacementRelation::In},
        PickUpCommand{4},
        PlaceCommand{4, 2, PlacementRelation::InMiddleOf}
    };
    return program;
}

// 3. Clear table
inline Program getClearTable()
{
    Program program;
    program.id = 3;
    program.name = "Clear table";
    program.domain = "Catering";
    program.parameters = {
        makeSlot(1, 1, "Plate", "Red"),
        makeSlot(2, 2, "Tray", "Brown"),
        makeSlot(3, 3, "Cup", "Blue"),
        makeSlot(4, 4, "Spoon", "Grey")
    };
    program.commands = {
        PickUpCommand{1},
        PlaceCommand{1, 2, PlacementRelation::On},
        PickUpCommand{3},
        PlaceCommand{3, 2, PlacementRelation::On},
        PickUpCommand{4},
        PlaceCommand{4, 1, PlacementRelation::On}
    };
    return program;
}

// 4. Set up table
inline Program getSetUpTable()
{
    Program program;
    program.id = 4;
    program.name = "Set up table";
    program.domain = "Catering";
    program.parameters = {
        makeSlot(1, 1, "plate", "white"),
        makeSlot(2, 2, "tray", ""),
        makeSlot(3, 3, "glass", ""),
        makeSlot(4, 4, "teaBag", ""),
        makeSlot(5, 5, "spoon", "")
    };
    program.commands = {
        PlaceCommand{1, 2, PlacementRelation::On},
        PlaceCommand{3, 1, PlacementRelation::NextTo},
        PlaceCommand{4, 3, PlacementRelation::In},
        PlaceCommand{5, 3, PlacementRelation::In}
    };
    return program;
}

// 5. Return tools
inline Program getReturnTools()
{
    Program program;
    program.id = 5;
    program.name = "Return tools";
    program.domain = "Workshop";
    program.parameters = {
        makeSlot(1, 1, "hammer", "red"),
        makeSlot(2, 2, "container", "red"),
        makeSlot(3, 3, "screwdriver", "blue"),
        makeSlot(4, 4, "tray", "brown")
    };
    program.commands = {
        PickUpCommand{1},
        PlaceCommand{1, 2, PlacementRelation::In},
        PickUpCommand{3},
        PlaceCommand{3, 4, PlacementRelation::On}
    };
    return program;
}

// 6. Galvanize a gear
inline Program getGalvanizeGear()
{
    Program program;
    program.id = 6;
    program.name = "Galvanize a gear";
    program.domain = "Workshop";
    program.parameters = {
        makeSlot(1, 1, "gear", "white"),
        makeSlot(2, 2, "vault", "grey"),
        makeSlot(3, 3, "vault", "green"),
        makeSlot(4, 4, "tray", "")
    };
    program.commands = {
        PickUpCommand{1},
        PlaceCommand{1, 2, PlacementRelation::In},
        PickUpCommand{1},
        PlaceCommand{1, 3, PlacementRelation::In},
        PickUpCommand{1},
        PlaceCommand{1, 4, PlacementRelation::On}
    };
    return program;
}

// 7. Prepare sensor assembly
inline Program getPrepareSensorAssembly()
{
    Program program;
    program.id = 7;
    program.name = "Prepare sensor assembly";
    program.domain = "Electronics";
    program.parameters = {
        makeSlot(1, 1, "wire", "red"),
        makeSlot(2, 2, "battery", ""),
        makeSlot(3, 3, "wire", "blue"),
        makeSlot(4, 4, "button", "")
    };
    program.commands = {
        PlaceCommand{1, 2, PlacementRelation::NextTo},
        PlaceCommand{3, 1, PlacementRelation::NextTo},
        PlaceCommand{4, 2, PlacementRelation::InFrontOf},
        PressCommand{4}
    };
    return program;
}

// 8. Sort components
inline Program getSortComponents()
{
    Program program;
    program.id = 8;
    program.name = "Sort components";
    program.domain = "Electronics";
    program.parameters = {
        makeSlot(1, 1, "button", "black"),
        makeSlot(2, 2, "container", "red"),
        makeSlot(3, 3, "battery", ""),
        makeSlot(4, 4, "wire", "black"),
        makeSlot(5, 5, "container", "blue"),
        makeSlot(6, 6, "wire", "yellow")
    };
    program.commands = {
        PlaceCommand{1, 2, PlacementRelation::In},
        PlaceCommand{3, 2, PlacementRelation::In},
        PlaceCommand{4, 5, PlacementRelation::In},
        PlaceCommand{6, 5, PlacementRelation::In}
    };
    return program;
}
