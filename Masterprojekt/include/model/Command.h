#pragma once

#include <variant>

//TODO: make this more useful, nextto2 is dumb
enum class PlacementRelation
{
    On,
    In,
    NextTo,
    InFrontOf,
    InMiddleOf,
    Above,
    High,
    Behind,
    Beside,
    NextTo2
};

struct PickUpCommand
{
    int objectSlotId;
};

struct PlaceCommand
{
    int objectSlotId;
    int targetSlotId;
    PlacementRelation relation;
};

struct PourCommand
{
    int vesselSlotId;
    int targetSlotId;
    int contentSlotId = -1;
};

struct PressCommand
{
    int objectSlotId;
};

struct DipCommand
{
    int objectSlotId;
    int targetSlotId;
    PlacementRelation relation;
};

using Command = std::variant<
    PickUpCommand,
    PlaceCommand,
    PourCommand,
    PressCommand,
	DipCommand
>;