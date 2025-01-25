#include <cstdio>

#include <Urho3D/Urho3D.h>

#include <Urho3D/Core/Context.h>

#include <Urho3D/IO/Serializer.h>
#include <Urho3D/IO/Deserializer.h>
#include <Urho3D/IO/FileSystem.h>
#include <Urho3D/IO/Log.h>

#include <Urho3D/Urho2D/Drawable2D.h>

#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Resource/XMLFile.h>


#include "LevelGraph.h"

const String filter_("M");


LevelGraph::LevelGraph(Context* context) :
    Resource(context)
{ }

LevelGraph::~LevelGraph()
{ }

void LevelGraph::RegisterObject(Context* context)
{
    context->RegisterFactory<LevelGraph>();
}

bool LevelGraph::BeginLoad(Deserializer& source)
{
    points_.Clear();
    orderedpoints_.Clear();

    SetName(source.GetName());

    URHO3D_LOGINFOF("LevelGraph() - BeginLoad : file %s ...", GetName().CString());

    String extension = GetExtension(GetName());

    if (extension == ".svg")
        return BeginLoadFromXMLFile(source);

    URHO3D_LOGERRORF("LevelGraph() - BeginLoad : Unsupported file type %s !", extension.CString());

    return false;
}

bool LevelGraph::EndLoad()
{
    URHO3D_LOGINFOF("LevelGraph() - EndLoad : file %s ...", GetName().CString());

    if (loadXMLFile_)
        return EndLoadFromXMLFile();

    return false;
}

bool LevelGraph::BeginLoadFromXMLFile(Deserializer& source)
{
    loadXMLFile_ = new XMLFile(context_);
    if (!loadXMLFile_->Load(source))
    {
        URHO3D_LOGERROR("LevelGraph() - BeginLoadFromXMLFile : could not load file !");
        loadXMLFile_.Reset();
        return false;
    }

    SetMemoryUse(source.GetSize());

    if (!loadXMLFile_->GetRoot("svg"))
    {
        points_.Clear();
        orderedpoints_.Clear();

        URHO3D_LOGERROR("LevelGraph() - BeginLoadFromXMLFile : Invalid LevelGraph File !");
        loadXMLFile_.Reset();
        return false;
    }

    return true;
}

inline bool CompareLevelGraphPoints(LevelGraphPoint* lhs, LevelGraphPoint* rhs)
{
    return lhs->id_ < rhs->id_;
}

bool LevelGraph::EndLoadFromXMLFile()
{
    bool state = false;

    XMLElement rootElem = loadXMLFile_->GetRoot("svg");
    XMLElement layerElem = rootElem.GetChild("g");

    if (layerElem.GetAttribute("id") == "LevelGraph")
    {
        // Get reference frame
        XMLElement rect = layerElem.GetChild("rect");
        while (rect)
        {
            String id = rect.GetAttribute("id");
            if (id == "Frame")
            {
                framesize_.x_ = rect.GetFloat("width");
                framesize_.y_ = rect.GetFloat("height");
                break;
            }
            rect = layerElem.GetNext("rect");
        }

        // Get ellipse elements
        XMLElement ellipse = layerElem.GetChild("ellipse");
        while (ellipse)
        {
            String name = ellipse.GetAttribute("id");

            if (name.StartsWith(filter_))
            {
                LevelGraphPoint& point = points_[StringHash(name)];
                point.id_ = ToUInt(name.Substring(filter_.Length()));
                point.name_ = name;
                point.position_ = Vector2(ellipse.GetFloat("cx") - framesize_.x_ * 0.5f, framesize_.y_ * 0.5f - ellipse.GetFloat("cy")) * PIXEL_SIZE;
                 // Approximate Circle Radius
                point.radius_ = (ellipse.GetFloat("rx") + ellipse.GetFloat("ry")) * 0.5f * PIXEL_SIZE;

                // Title used for Gains
                String titlevalue = ellipse.GetChild("title").GetValue();
                if (!titlevalue.Empty())
                    point.gains_= titlevalue;

                point.levelData_ = 0;

                orderedpoints_.Push(&point);
            }

            ellipse = ellipse.GetNext("ellipse");
        }

        // Get circle elements
        XMLElement circle = layerElem.GetChild("circle");
        while (circle)
        {
            String name = circle.GetAttribute("id");

            if (name.StartsWith(filter_))
            {
                LevelGraphPoint& point = points_[StringHash(name)];
                point.id_ = ToUInt(name.Substring(filter_.Length()));
                point.name_ = name;
                point.position_ = Vector2(circle.GetFloat("cx") - framesize_.x_ * 0.5f, framesize_.y_ * 0.5f - circle.GetFloat("cy")) * PIXEL_SIZE;
                point.radius_ = circle.GetFloat("r") * PIXEL_SIZE;

                // Title used for Gains
                String titlevalue = circle.GetChild("title").GetValue();
                if (!titlevalue.Empty())
                    point.gains_= titlevalue;

                point.levelData_ = 0;

                orderedpoints_.Push(&point);
            }

            circle = circle.GetNext("circle");
        }

        // Get Links between mission
        XMLElement path = layerElem.GetChild("path");
        while (path)
        {
            Vector<String> names = path.GetAttribute("id").Split('_');
            if (names[0] != names[1])
            {
                LevelGraphPoint& point1 = points_[StringHash(names[0])];
                LevelGraphPoint& point2 = points_[StringHash(names[1])];
                point1.linkedpoints_.Push(&point2);
                point2.linkedpoints_.Push(&point1);
            }
            path = path.GetNext("path");
        }

        state = true;
    }

    // Order Points
    Urho3D::Sort(orderedpoints_.Begin(), orderedpoints_.End(), CompareLevelGraphPoints);

    loadXMLFile_.Reset();

    return state;
}


void LevelGraph::Dump() const
{
    URHO3D_LOGINFO("LevelGraph() - Dump() ...");

    URHO3D_LOGINFO("LevelGraph() - Dump() ... OK !");
}
