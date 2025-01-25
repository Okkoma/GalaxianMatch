#include <Urho3D/Urho3D.h>

#include <Urho3D/IO/Log.h>

#include <ctime>

#include "GameRand.h"

#ifndef ACTIVE_STDRANDOMIZER
//#if __cplusplus >= 201103L
    #include <chrono>
#endif // __cplusplus

Vector<GameRand> GameRand::registeredRandomizer_;


void GameRand::InitTable()
{
    URHO3D_LOGINFOF("GameRand() - InitTable : %u Randomizers", NUM_GAMERANDRNG);

    for (unsigned i=0; i < NUM_GAMERANDRNG; ++i)
    {
        registeredRandomizer_.Push(GameRand());
        registeredRandomizer_.Back().id_ = i;
    }
}

unsigned GameRand::GetTimeSeed()
{
    unsigned seed;

#ifdef ACTIVE_STDRANDOMIZER
//#if __cplusplus < 201103L
    seed = (unsigned)time(NULL);
    URHO3D_LOGINFOF("GameRand() - SetSeed : ... set time random seed = %u", seed);
#else
    seed = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    URHO3D_LOGINFOF("GameRand() - SetSeed : ... set time random seed (C++11) = %u", seed);
#endif // __cplusplus

    return seed;
}

void GameRand::Dump10Value()
{
    unsigned seed = GetSeedRand(MAPRAND);
    String numbers;
    for (unsigned i=0; i < 10; ++i)
        numbers += String(GetRand(MAPRAND, 100)) + " ";

    URHO3D_LOGINFOF("random(seed=%u) => (%s)", seed, numbers.CString());
}

void GameRand::SetSeed(unsigned seed)
{
    seed_ = seed;
    rng_ = RngT(seed);

    URHO3D_LOGINFOF("GameRand() - SetSeed : r=%d seed=%u", id_, seed);
}

void GameRand::SetSeedRand(GameRandRng r, unsigned seed)
{
    if (r == ALLRAND)
    {
    #ifdef ACTIVE_STDRANDOMIZER
//    #if __cplusplus < 201103L
        srand(seed);
    #endif
        for (unsigned i=0; i < NUM_GAMERANDRNG; ++i)
            registeredRandomizer_[i].SetSeed(seed);
    }
    else
        registeredRandomizer_[r].SetSeed(seed);

    URHO3D_LOGINFOF("GameRand() - SetSeedRand : r=%d seed=%u", r, seed);
}

