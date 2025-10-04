#pragma once

#include <Urho3D/Math/StringHash.h>

struct MatchObjective
{
    MatchObjective() = default;
    MatchObjective(const MatchObjective& mobj) = default;
    MatchObjective& operator = (const MatchObjective& mobj) = default;
    
    void Reset() { type_ = Urho3D::StringHash::ZERO; target_ = 0; count_ = 0; ctype_ = 0; }
    
    Urho3D::StringHash type_;
    int target_;
    int count_;
    unsigned char ctype_;
};