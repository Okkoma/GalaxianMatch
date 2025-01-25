#pragma once

#include "GameOptions.h"

#ifndef ACTIVE_STDRANDOMIZER
//#if __cplusplus >= 201103L
#include <random>
#endif // __cplusplus

using namespace Urho3D;

const int NUM_GAMERANDRNG = 4;
enum GameRandRng
{
    ALLRAND = 0,
    MAPRAND,
    TILRAND,
    OBJRAND,
};

class GameRand
{
public:
//#if __cplusplus < 201103L
#ifdef ACTIVE_STDRANDOMIZER
	typedef unsigned RngT;
#else
    typedef std::mt19937 RngT;
#endif // __cplusplus

    static void InitTable();
    static GameRand& GetRandomizer(GameRandRng r) { return registeredRandomizer_[r]; }
    static int GetRand(GameRandRng r) { return registeredRandomizer_[r].Get(); }
    static int GetRand(GameRandRng r, int range) { return registeredRandomizer_[r].Get(range); }
    static int GetRand(GameRandRng r, int min, int max) { return registeredRandomizer_[r].Get(min, max); }
    static void SetSeedRand(GameRandRng r, unsigned seed);
    static unsigned GetSeedRand(GameRandRng r) { return registeredRandomizer_[r].GetSeed(); }
    static unsigned GetTimeSeed();
    static void Dump10Value();

    GameRand() : id_(0), seed_(1) { }

    void SetSeed(unsigned seed);
    unsigned GetSeed() const { return seed_ ; }

//#if __cplusplus < 201103L
#ifdef ACTIVE_STDRANDOMIZER
    inline int Get() { seed_ = seed_ * 214013 + 2531011; return (seed_ >> 16) & 32767; }
    // from http://www.open-std.org/jtc1/sc22/wg14/www/docs/n1256.pdf §7.20.2.2 (page 312)
//    inline int Get() { seed_ = seed_ * 1103515245 + 12345; return (seed_/65536) % 32768; }
    inline int Get(int range) { return range > 1 ? Get() % range : 0; }
    inline int Get(int min, int max) { return Get(max-min+1) + min; }
    inline int GetDir() { return Get(4); }
    inline int GetAllDir() { return Get(7); }
#else
    inline int Get() { return std::uniform_int_distribution<int>()(rng_); }
    inline int Get(int range) { return std::uniform_int_distribution<int>(0, range-1)(rng_); }
    inline int Get(int min, int max) { return std::uniform_int_distribution<int>(min, max)(rng_); }
    inline int GetDir() { return std::uniform_int_distribution<int>(0, 3)(rng_); }
    inline int GetAllDir() { return std::uniform_int_distribution<int>(0, 6)(rng_); }
#endif

private:
    int id_;
    unsigned seed_;
    RngT rng_;

    static Vector<GameRand> registeredRandomizer_;
};
