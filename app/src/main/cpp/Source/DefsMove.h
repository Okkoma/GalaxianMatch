#pragma once

using namespace Urho3D;

enum MovingBits
{
    MV_NONE = 0,

    // MoveType Flags
    MV_WALK = 1 << 0,
    MV_FLY = 1 << 1,
    MV_SWIM = 1 << 2,
    MV_CLIMB = 1 << 3,
    MV_CLIMBFIRSTBOX = 1 << 4,
    MV_MOUNT = 1 << 5,
    // Motion States
    MV_INMOVE = 1 << 6,
    MV_INJUMP = 1 << 7,
    MV_INFALL = 1 << 8,
    // Area States
    MV_INAIR = 1 << 9,
    MV_INLIQUID = 1 << 10,
    // Collision States
    MV_TOUCHGROUND = 1 << 11,
    MV_TOUCHWALL = 1 << 12,
    MV_TOUCHROOF = 1 << 13,
    // Direction States
    MV_DIRECTION = 1 << 14,
    MV_KEEPDIRECTION = 1 << 15,
};

enum MoveTypeMode
{
    MOVE2D_WALK = 0,
    MOVE2D_WALKCLIMB = 1,
    MOVE2D_WALKANDJUMPFIRSTBOX = 2,
    MOVE2D_WALKANDCLIMBANDSWIM = 3,
    MOVE2D_WALKANDJUMPFIRSTBOXANDSWIM = 4,
    MOVE2D_WALKANDFLY = 5,
    MOVE2D_WALKANDFLYANDSWIM = 6,
    MOVE2D_FLY = 7,
    MOVE2D_FLYCLIMB = 8,
    MOVE2D_SWIM = 9,
    MOVE2D_SWIMCLIMB = 10,
    MOVE2D_MOUNT = 11,
};

enum MoveMask
{
    MSK_MV_WALKFLYAIR      = MV_WALK | MV_FLY | MV_SWIM,
    MSK_MV_FLYAIR          = MV_FLY | MV_INAIR,
    MSK_MV_SWIMLIQUID      = MV_SWIM | MV_INLIQUID,
    MSK_MV_CLIMBWALL       = MV_CLIMB | MV_TOUCHWALL,
    MSK_MV_CLIMBROOF       = MV_CLIMB | MV_TOUCHROOF,
    MSK_MV_TOUCHWALLORROOF = MV_TOUCHWALL | MV_TOUCHROOF,
    MSK_MV_TOUCHWALLS      = MV_TOUCHGROUND | MV_TOUCHWALL | MV_TOUCHROOF,
    MSK_MV_CANWALKONGROUND = MV_WALK | MV_TOUCHGROUND,
};

// AI Move
const Vector2 defaultMinWlkRangeTarget = Vector2(1.5f, 2.5f);
const Vector2 defaultMaxWlkRangeTarget = Vector2(5.f, 2.5f);
const Vector2 defaultMinAttRangeTarget = Vector2(1.f, 0.5f);
const Vector2 defaultMaxAttRangeTarget = Vector2(1.5f, 2.5f);
