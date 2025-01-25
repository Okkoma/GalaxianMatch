#pragma once

#include <climits>

#include <Urho3D/Core/Context.h>
#include <Urho3D/Scene/Node.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Urho2D/Drawable2D.h>

#include "MemoryObjects.h"

// Remove conflicts with winusr
#ifdef WIN32
#undef MessageBox
#undef GetObject
#endif

#ifdef __ANDROID__
#define GAME_JAVA_INTERFACE(function) Java_ ## com_okkomastudio_galaxianmatch ## _ ## GalaxianMatch ## _ ## function
#endif

using namespace Urho3D;


typedef Vector<VariantMap > NodeAttributes;

struct GOTInfo
{
    GOTInfo();

    String typename_;
    String filename_;
    StringHash category_;
    unsigned properties_;
    // allow replicated node CreateMode or not
    bool replicatedMode_;
    // pool quantity
    unsigned poolqty_;
    int scaleVariation_;
    bool entityVariation_;
    // collectable infos
    int maxdropqty_;
    int defaultvalue_;

    static const GOTInfo EMPTY;
};

const String TRIGATTACK("Trig_Attack");
