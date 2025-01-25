#pragma once

#include <Urho3D/Resource/Resource.h>

#include "GameStatics.h"
#include "GameOptions.h"

namespace Urho3D
{
    class XMLFile;
}

using namespace Urho3D;

struct LevelGraphPoint
{
    unsigned id_;
    String name_;
    Vector2 position_;
    float radius_;
    Vector<LevelGraphPoint* > linkedpoints_;
    String gains_;

    LevelData* levelData_;
};

class LevelGraph : public Resource
{
    URHO3D_OBJECT(LevelGraph, Resource);

public:
    LevelGraph(Context* context);
    virtual ~LevelGraph();

    /// Register object factory.
    static void RegisterObject(Context* context);

    /// Load resource from stream. May be called from a worker thread. Return true if successful.
    virtual bool BeginLoad(Deserializer& source);
    /// Finish resource loading. Always called from the main thread. Return true if successful.
    virtual bool EndLoad();

    const Vector2& GetFrameSize() const { return framesize_; }

    const Vector<LevelGraphPoint* >& GetOrderedPoints() const { return orderedpoints_; }
    LevelGraphPoint* GetPoint(unsigned index) const { return orderedpoints_[index]; }

    void Dump() const;

private :
    /// Begin load from XML file.
    bool BeginLoadFromXMLFile(Deserializer& source);
    /// End load from XML file.
    bool EndLoadFromXMLFile();

    Vector2 framesize_;

    HashMap<StringHash, LevelGraphPoint > points_;
    Vector<LevelGraphPoint* > orderedpoints_;

    SharedPtr<XMLFile> loadXMLFile_;
};
