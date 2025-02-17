//
// Copyright (c) 2008-2016 the Urho3D project.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#include "../Precompiled.h"

#include "../IO/Log.h"

#include "../Math/MathDefs.h"
#include "../Urho2D/SpriterData2D.h"

#include <PugiXml/pugixml.hpp>

#include <cstring>

using namespace pugi;

namespace Urho3D
{

namespace Spriter
{

const int FloatPrecision = 6;
const char* ScmlVersion = "1.0";
const char* ScmlGeneratorStr = "Urho3DSCML";
const char* ScmlGeneratorVersionStr = "r1";

const char* ObjectTypeStr[] =
{
    "bone",
    "sprite",
    "point",
    "box"
};

const char* CurveTypeStr[] =
{
    "instant",
    "linear",
    "quadratic",
    "cubic",
    "quartic",
    "quintic",
    "bezier"
};

String GetFloatStr(float number, int precision)
{
    int numDigitBeforeDecimal = String(static_cast<int>(number)).Length();
    char resultat[20];
    sprintf(resultat, "%.*g", precision + numDigitBeforeDecimal, number);
    return String(resultat);
}


SpriterData::SpriterData()
{
}

SpriterData::~SpriterData()
{
    Reset();
}

void SpriterData::Reset()
{
    if (!folders_.Empty())
    {
        for (size_t i = 0; i < folders_.Size(); ++i)
            delete folders_[i];
        folders_.Clear();
    }

    if (!entities_.Empty())
    {
        for (size_t i = 0; i < entities_.Size(); ++i)
            delete entities_[i];
        entities_.Clear();
    }
}

const char* SpriterData::GetCurveTypeStr(CurveType type)
{
    return CurveTypeStr[type];
}

bool SpriterData::Load(const pugi::xml_node& node)
{
    Reset();

    if (strcmp(node.name(), "spriter_data"))
        return false;

    scmlVersion_ = node.attribute("scml_version").as_int();
    generator_ = node.attribute("generator").as_string();
    generatorVersion_ = node.attribute("generator_version").as_string();

    for (xml_node folderNode = node.child("folder"); !folderNode.empty(); folderNode = folderNode.next_sibling("folder"))
    {
        folders_.Push(new Folder());
        if (!folders_.Back()->Load(folderNode))
        {
            URHO3D_LOGERRORF("SpriterData : Error In Folders !");
            return false;
        }
    }

    for (xml_node entityNode = node.child("entity"); !entityNode.empty(); entityNode = entityNode.next_sibling("entity"))
    {
        entities_.Push(new Entity());
        if (!entities_.Back()->Load(entityNode))
        {
            URHO3D_LOGERRORF("SpriterData : Error In Entities !");
            return false;
        }
    }

    UpdateKeyInfos();

    return true;
}

bool SpriterData::Load(const void* data, size_t size)
{
    xml_document document;
    if (!document.load_buffer(data, size))
        return false;

    bool loaded = Load(document.child("spriter_data"));
    return loaded;
}

void SpriterData::UpdateKeyInfos()
{
    for (PODVector<Entity*>::ConstIterator entity = entities_.Begin(); entity != entities_.End(); ++entity)
    {
        const PODVector<Animation*>& animations = (*entity)->animations_;
        for (PODVector<Animation*>::ConstIterator animation = animations.Begin(); animation != animations.End(); ++animation)
        {
            const PODVector<Timeline*>& timelines = (*animation)->timelines_;
            for (PODVector<Timeline*>::ConstIterator timelinet = timelines.Begin(); timelinet != timelines.End(); ++timelinet)
            {
                Timeline* timeline = *timelinet;
                if (timeline->objectType_ != SPRITE && timeline->objectType_ != BOX)
                    continue;

                const PODVector<SpatialTimelineKey*>& keys = timeline->keys_;

                for (PODVector<SpatialTimelineKey*>::ConstIterator key = keys.Begin(); key != keys.End(); ++key)
                {
                    if ((*key)->GetObjectType() == SPRITE)
                    {
                        SpriteTimelineKey* spriteKey = (SpriteTimelineKey*) (*key);
                        File* file = folders_[spriteKey->folderId_]->files_[spriteKey->fileId_];
                        spriteKey->fx_ = file->fx_;
                        if (spriteKey->useDefaultPivot_)
                        {
                            spriteKey->pivotX_ = file->pivotX_;
                            spriteKey->pivotY_ = file->pivotY_;
                        }
                    }
                    else if ((*key)->GetObjectType() == BOX)
                    {
                        HashMap<StringHash, ObjInfo >::Iterator objit = (*entity)->objInfos_.Find(timeline->hashname_);
                        if (objit != (*entity)->objInfos_.End())
                        {
                            BoxTimelineKey* boxKey = (BoxTimelineKey*) (*key);
                            boxKey->width_ = objit->second_.width_;
                            boxKey->height_ = objit->second_.height_;
                            if (boxKey->useDefaultPivot_)
                            {
                                boxKey->pivotX_ = objit->second_.pivotX_;
                                boxKey->pivotY_ = objit->second_.pivotY_;
                            }
                        }
                    }
                }
            }
        }
    }
}

Folder::Folder()
{

}

Folder::~Folder()
{
    Reset();
}

void Folder::Reset()
{
    for (size_t i = 0; i < files_.Size(); ++i)
        delete files_[i];
    files_.Clear();
}

bool Folder::Load(const pugi::xml_node& node)
{
    Reset();

    if (strcmp(node.name(), "folder"))
        return false;

    id_ = node.attribute("id").as_uint();
    name_ = node.attribute("name").as_string();

    for (xml_node fileNode = node.child("file"); !fileNode.empty(); fileNode = fileNode.next_sibling("file"))
    {
        files_.Push(new  File(this));
        if (!files_.Back()->Load(fileNode))
            return false;
    }

    return true;
}

File::File(Folder* folder) :
    folder_(folder)
{
}

File::~File()
{
}

bool File::Load(const pugi::xml_node& node)
{
    if (strcmp(node.name(), "file"))
        return false;

    id_ = node.attribute("id").as_uint(0);
    fx_ = node.attribute("fx").as_uint(0);
    name_ = node.attribute("name").as_string();
    width_ = node.attribute("width").as_float();
    height_ = node.attribute("height").as_float();
    pivotX_ = node.attribute("pivot_x").as_float(0.0f);
    pivotY_ = node.attribute("pivot_y").as_float(1.0f);

    return true;
}

Entity::Entity()
{

}

Entity::~Entity()
{
    Reset();
}

void Entity::Reset()
{
    for (size_t i = 0; i < characterMaps_.Size(); ++i)
        delete characterMaps_[i];
    characterMaps_.Clear();

    for (size_t i = 0; i < animations_.Size(); ++i)
        delete animations_[i];
    animations_.Clear();
}

bool Entity::Load(const pugi::xml_node& node)
{
    Reset();

    if (strcmp(node.name(), "entity"))
        return false;

    id_ = node.attribute("id").as_uint();
    name_ = String(node.attribute("name").as_string());

    xml_attribute colorAttr = node.attribute("color");
    if (!colorAttr.empty())
        color_ = ToColor(String(node.attribute("color").as_string()));
    
    for (xml_node objInfoNode = node.child("obj_info"); !objInfoNode.empty(); objInfoNode = objInfoNode.next_sibling("obj_info"))
    {
        String name(objInfoNode.attribute("name").as_string());
        if (!name.Empty())
        {
            StringHash hashname(name);
            if (!ObjInfo::Load(objInfoNode, objInfos_[hashname]))
            {
                URHO3D_LOGERRORF("SpriterFXData : Error In Entities:ObjInfo !");
                return false;
            }
        }
    }

    for (xml_node characterMapNode = node.child("character_map"); !characterMapNode.empty(); characterMapNode = characterMapNode.next_sibling("character_map"))
    {
        characterMaps_.Push(new CharacterMap());
        if (!characterMaps_.Back()->Load(characterMapNode))
        {
            URHO3D_LOGERRORF("SpriterData : Error In Entities:CharacterMap !");
            return false;
        }
    }

    for (xml_node animationNode = node.child("animation"); !animationNode.empty(); animationNode = animationNode.next_sibling("animation"))
    {
        animations_.Push(new  Animation());
        if (!animations_.Back()->Load(animationNode))
        {
            URHO3D_LOGERRORF("SpriterData : Error In Entities:Animation !");
            return false;
        }
    }

    return true;
}

ObjInfo::ObjInfo()
{

}

ObjInfo::~ObjInfo()
{

}

bool ObjInfo::Load(const pugi::xml_node& node, ObjInfo& objinfo)
{
    if (strcmp(node.name(), "obj_info"))
        return false;

    String type(node.attribute("type").as_string("bone"));

    if (type == "bone")
        objinfo.type_ = BONE;
    else if (type == "point")
        objinfo.type_ = POINT;
    else if (type == "box")
        objinfo.type_ = BOX;
    else
        return false;

    objinfo.name_   = node.attribute("name").as_string();
    objinfo.width_ = node.attribute("w").as_float(10.f);
    objinfo.height_ = node.attribute("h").as_float(10.f);
    objinfo.pivotX_ = node.attribute("pivot_x").as_float(0.f);
    objinfo.pivotY_ = node.attribute("pivot_y").as_float(1.f);

    return true;
}


CharacterMap::CharacterMap()
{

}

CharacterMap::~CharacterMap()
{

    Reset();
}

void CharacterMap::Reset()
{
    for (size_t i = 0; i < maps_.Size(); ++i)
        delete maps_[i];
    maps_.Clear();
}

bool CharacterMap::Load(const pugi::xml_node& node)
{
    Reset();

    if (strcmp(node.name(), "character_map"))
        return false;

    id_ = node.attribute("id").as_uint();
    name_ = String(node.attribute("name").as_string());
    hashname_ = StringHash(name_);

    for (xml_node mapNode = node.child("map"); !mapNode.empty(); mapNode = mapNode.next_sibling("map"))
    {
        maps_.Push(new MapInstruction());
        if (!maps_.Back()->Load(mapNode))
        {
            URHO3D_LOGERRORF("SpriterData : Error In Entities:CharacterMap:MapInstruction !");
            return false;
        }
    }

    return true;
}

MapInstruction* CharacterMap::GetInstruction(unsigned key, bool add)
{
    unsigned folder;
    unsigned file;
    GetFolderFile(key, folder, file);

    MapInstruction* instruction = GetInstruction(folder, file);

    for (PODVector<MapInstruction*>::ConstIterator it = maps_.Begin(); it != maps_.End(); ++it)
    {
        MapInstruction* mapinstruct = *it;
        if (mapinstruct->folder_ == folder && mapinstruct->file_ == file)
        {
            return mapinstruct;
        }
    }

    if (!instruction && add)
    {
        instruction = new MapInstruction();
        instruction->SetOrigin(key);
        maps_.Push(instruction);
    }

    return instruction;
}

MapInstruction* CharacterMap::GetInstruction(unsigned folder, unsigned file)
{
    for (PODVector<MapInstruction*>::ConstIterator it = maps_.Begin(); it != maps_.End(); ++it)
    {
        MapInstruction* mapinstruct = *it;
        if (mapinstruct->folder_ == folder && mapinstruct->file_ == file)
            return mapinstruct;
    }
    return 0;
}

MapInstruction* CharacterMap::RemoveInstruction(unsigned key)
{
    unsigned folder;
    unsigned file;
    GetFolderFile(key, folder, file);

    for (PODVector<MapInstruction*>::Iterator it = maps_.Begin(); it != maps_.End(); ++it)
    {
        MapInstruction* mapinstruct = *it;
        if (mapinstruct->folder_ == folder && mapinstruct->file_ == file)
        {
            maps_.Erase(it);
            delete mapinstruct;
            return 0;
        }
    }

    return 0;
}


MapInstruction::MapInstruction() :
    targetdx_(0.f),
    targetdy_(0.f),
    targetdangle_(0.f),
    targetscalex_(1.f),
    targetscaley_(1.f)
{ }

MapInstruction::~MapInstruction()
{ }

bool MapInstruction::Load(const pugi::xml_node& node)
{
    if (strcmp(node.name(), "map"))
        return false;

    folder_ = node.attribute("folder").as_uint();
    file_ = node.attribute("file").as_uint();
    targetFolder_ = node.attribute("target_folder").as_int(-1);
    targetFile_ = node.attribute("target_file").as_int(-1);

    targetdx_ = node.attribute("target_dx").as_float(0.f);
    targetdy_ = node.attribute("target_dy").as_float(0.f);
    targetdangle_ = node.attribute("target_dangle").as_float(0.f);
    targetscalex_ = node.attribute("target_scalex").as_float(1.f);
    targetscaley_ = node.attribute("target_scaley").as_float(1.f);

    return true;
}

void MapInstruction::SetOrigin(unsigned spritekey)
{
    GetFolderFile(spritekey, folder_, file_);
}

void MapInstruction::SetTarget(unsigned targetkey)
{
    unsigned targetFolder, targetFile;
    GetFolderFile(targetkey, targetFolder, targetFile);
    targetFolder_ = targetFolder;
    targetFile_ = targetFile;
}

void MapInstruction::RemoveTarget()
{
    targetFolder_ = -1;
    targetFile_   = -1;
}

Animation::Animation()
{

}

Animation::~Animation()
{
    Reset();
}

void Animation::Reset()
{
    if (!mainlineKeys_.Empty())
    {
        for (size_t i = 0; i < mainlineKeys_.Size(); ++i)
            delete mainlineKeys_[i];
        mainlineKeys_.Clear();
    }

    for (size_t i = 0; i < timelines_.Size(); ++i)
        delete timelines_[i];
    timelines_.Clear();
}

bool Animation::Load(const pugi::xml_node& node)
{
    Reset();

    if (strcmp(node.name(), "animation"))
        return false;

    id_ = node.attribute("id").as_uint();
    name_ = String(node.attribute("name").as_string());
    length_ = node.attribute("length").as_float() * 0.001f;
    looping_ = node.attribute("looping").as_bool(true);

    xml_node mainlineNode = node.child("mainline");
    for (xml_node keyNode = mainlineNode.child("key"); !keyNode.empty(); keyNode = keyNode.next_sibling("key"))
    {
        mainlineKeys_.Push(new MainlineKey());
        if (!mainlineKeys_.Back()->Load(keyNode))
            return false;
    }

    unsigned id = 0;
    for (xml_node timelineNode = node.child("timeline"); !timelineNode.empty(); timelineNode = timelineNode.next_sibling("timeline"))
    {
        timelines_.Push(new Timeline());
        if (!timelines_.Back()->Load(timelineNode))
            return false;
        timelines_.Back()->id_ = id++;
    }

    return true;
}

void Animation::GetBoneRefs(unsigned timeline, PODVector<Ref*>& refs, unsigned startmainkeyid)
{
    if (!mainlineKeys_.Size())
        return;

    for (unsigned i = startmainkeyid; i < mainlineKeys_.Size(); i++)
    {
        Ref* ref = mainlineKeys_[i]->GetBoneRef(timeline);
        if (ref)
            refs.Push(ref);
    }
}

void Animation::GetObjectRefs(unsigned timeline, PODVector<Ref*>& refs, unsigned startmainkeyid)
{
    if (!mainlineKeys_.Size())
        return;

    for (unsigned i = startmainkeyid; i < mainlineKeys_.Size(); i++)
    {
        Ref* ref = mainlineKeys_[i]->GetObjectRef(timeline);
        if (ref)
            refs.Push(ref);
    }
}

MainlineKey* Animation::GetMainlineKey(float time) const
{
    MainlineKey* mainkey = mainlineKeys_.Front();
    for (PODVector<MainlineKey*>::ConstIterator it = mainlineKeys_.End() - 1; it != mainlineKeys_.Begin(); --it)
    {
        if (time >= (*it)->time_)
        {
            mainkey = *it;
            break;
        }
    }
    return mainkey;
}

void Animation::UnMapToRoot(SpatialTimelineKey* tkey, float time, bool includeFirstKey, SpatialInfo& info) const
{
    const unsigned timeline = tkey->timeline_->id_;

    if (includeFirstKey)
        info = tkey->info_;

    // Get the Main Key at time
    MainlineKey* mainkey = GetMainlineKey(time);

    // Get the Ref of the timeline in the boneRefs
    Ref* ref = mainkey->GetBoneRef(timeline);
    // Get the Ref of the timeline in the objectRefs if has no ref in the bone.
    if (!ref)
        ref = mainkey->GetObjectRef(timeline);

    // UnMap the spatialinfo until reaching the root
    while (ref && ref->parent_ != -1)
    {
        ref = mainkey->boneRefs_[ref->parent_];
        info.UnmapFromParent(timelines_[ref->timeline_]->GetTimeKey(time)->info_);
    }
}

// From http://www.brashmonkey.com/ScmlDocs/ScmlReference.html

inline float Linear(float a, float b, float t)
{
    return a + (b - a) * t;
}

inline float ReverseLinear(float a, float b, float t)
{
    return b != a ? (t - a) / (b - a) : a;
}

inline float AngleLinear(float a, float b, int spin, float t)
{
    if (spin == 0) return a;
    if (spin > 0 && (b - a) < 0) b += 360.0f;
    if (spin < 0 && (b - a) > 0) b -= 360.0f;
    return Linear(a, b, t);
}

inline float Quadratic(float a, float b, float c, float t)
{
    return Linear(Linear(a, b, t), Linear(b, c, t), t);
}

inline float Cubic(float a, float b, float c, float d, float t)
{
    return Linear(Quadratic(a, b, c, t), Quadratic(b, c, d, t), t);
}



TimeKey::TimeKey()
{

}

TimeKey::~TimeKey()
{

}

bool TimeKey::Load(const pugi::xml_node& node)
{
    if (strcmp(node.name(), "key"))
        return false;

    id_ = node.attribute("id").as_uint();

    time_ = node.attribute("time").as_float(0.f) * 0.001f;

    String curveType = node.attribute("curve_type").as_string("linear");

    if (curveType == "linear")
        curveType_ = LINEAR;
    else if (curveType == "instant")
        curveType_ = INSTANT;
    else if (curveType == "quadratic")
        curveType_ = QUADRATIC;
    else if (curveType == "cubic")
        curveType_ = CUBIC;
    else if (curveType == "quartic")
        curveType_ = QUARTIC;
    else if (curveType == "quintic")
        curveType_ = QUINTIC;
    else if (curveType == "bezier")
        curveType_ = BEZIER;
    else
        curveType_ = LINEAR;

    c1_ = node.attribute("c1").as_float();
    c2_ = node.attribute("c2").as_float();
    c3_ = node.attribute("c3").as_float();
    c4_ = node.attribute("c4").as_float();

    return true;
}

float TimeKey::ApplyCurveType(float factor)
{
    switch (curveType_)
    {
        case INSTANT :
            factor = 0.0f;
            break;
        case LINEAR :
            break;
        case QUADRATIC :
            factor = Quadratic(0.0f, c1_, 1.0f, factor);
            break;
        case CUBIC :
            factor = Cubic(0.0f, c1_, c2_, 1.0f, factor);
            break;
        case QUARTIC :
//            factor = Quartic(0.0f, c1_, c2_, c3_, 1.0f, factor);
            break;
        case QUINTIC :
//            factor = Quintic(0.0f, c1_,  c2_, c3_, c4_, 1.0f, factor);
            break;
        case BEZIER :
//            factor = Bezier(c1_, c2_, c3_, c4_, factor);
            break;
    }

    return factor;
}

float TimeKey::GetFactor(float timeA, float timeB, float length, float targetTime)
{
    if (timeA > timeB)
    {
        timeB += length;
        if (targetTime < timeA) targetTime += length;
    }

    float time = ReverseLinear(timeA, timeB, targetTime);

    time = ApplyCurveType(time);

    return time;
}

float TimeKey::AdjustTime(float timeA, float timeB, float length, float targetTime)
{
    float nextTime = timeB > timeA ? timeB : length;

    return Linear(timeA, nextTime, GetFactor(timeA, timeB, length, targetTime));
}



MainlineKey::MainlineKey()
{

}

MainlineKey::~MainlineKey()
{
    Reset();
}

void MainlineKey::Reset()
{
    for (size_t i = 0; i < boneRefs_.Size(); ++i)
        delete boneRefs_[i];
    boneRefs_.Clear();

    for (size_t i = 0; i < objectRefs_.Size(); ++i)
        delete objectRefs_[i];
    objectRefs_.Clear();
}

bool MainlineKey::Load(const pugi::xml_node& node)
{
    if (!TimeKey::Load(node))
        return false;

    for (xml_node boneRefNode = node.child("bone_ref"); !boneRefNode.empty(); boneRefNode = boneRefNode.next_sibling("bone_ref"))
    {
        boneRefs_.Push(new Ref());
        if (!boneRefs_.Back()->Load(boneRefNode))
            return false;
    }

    for (xml_node objectRefNode = node.child("object_ref"); !objectRefNode.empty(); objectRefNode = objectRefNode.next_sibling("object_ref"))
    {
        objectRefs_.Push(new Ref());
        if (!objectRefs_.Back()->Load(objectRefNode))
            return false;
    }

    return true;
}

Ref* MainlineKey::GetRef(unsigned timeline) const
{
    for (PODVector<Ref*>::ConstIterator it = boneRefs_.Begin(); it != boneRefs_.End(); ++it)
    {
        if ((*it)->timeline_ == timeline)
            return *it;
    }
    for (PODVector<Ref*>::ConstIterator it = objectRefs_.Begin(); it != objectRefs_.End(); ++it)
    {
        if ((*it)->timeline_ == timeline)
            return *it;
    }
    return 0;
}

Ref* MainlineKey::GetBoneRef(unsigned timeline) const
{
    for (PODVector<Ref*>::ConstIterator it = boneRefs_.Begin(); it != boneRefs_.End(); ++it)
    {
        if ((*it)->timeline_ == timeline)
            return *it;
    }
    return 0;
}

Ref* MainlineKey::GetObjectRef(unsigned timeline) const
{
    for (PODVector<Ref*>::ConstIterator it = objectRefs_.Begin(); it != objectRefs_.End(); ++it)
    {
        if ((*it)->timeline_ == timeline)
            return *it;
    }
    return 0;
}


Ref::Ref() { }

Ref::~Ref() { }

bool Ref::Load(const pugi::xml_node& node)
{
    if (strcmp(node.name(), "bone_ref") && strcmp(node.name(), "object_ref"))
        return false;

    id_ = node.attribute("id").as_uint();
    parent_ = node.attribute("parent").as_int(-1);
    timeline_ = node.attribute("timeline").as_uint();
    key_ = node.attribute("key").as_uint();
    zIndex_ = node.attribute("z_index").as_int(-1);
    
    xml_attribute colorAttr = node.attribute("color");
    color_ = colorAttr.empty() ? Color::WHITE : ToColor(colorAttr.as_string());

    return true;
}

void Ref::Copy(Ref& copy) const
{
    copy = *this;
}

Timeline::Timeline()
{

}

Timeline::~Timeline()
{
    Reset();
}

void Timeline::Reset()
{
    for (size_t i = 0; i < keys_.Size(); ++i)
        delete keys_[i];
    keys_.Clear();
}

bool Timeline::Load(const pugi::xml_node& node)
{
    Reset();

    if (strcmp(node.name(), "timeline"))
        return false;

    name_ = String(node.attribute("name").as_string());
    hashname_ = StringHash(name_);

    String typeString;
    xml_attribute typeAttr = node.attribute("type");
    if (typeAttr.empty())
        typeString = node.attribute("object_type").as_string("sprite");
    else
        typeString = typeAttr.as_string("sprite");

    if (typeString == "bone")
    {
        objectType_ = BONE;
        for (xml_node keyNode = node.child("key"); !keyNode.empty(); keyNode = keyNode.next_sibling("key"))
        {
            keys_.Push(new BoneTimelineKey(this));
            if (!keys_.Back()->Load(keyNode))
                return false;
        }
    }
    else if (typeString == "sprite")
    {
        objectType_ = SPRITE;
        for (xml_node keyNode = node.child("key"); !keyNode.empty(); keyNode = keyNode.next_sibling("key"))
        {
            keys_.Push(new SpriteTimelineKey(this));
            if (!keys_.Back()->Load(keyNode))
                return false;
        }
    }
    else if (typeString == "point")
    {
        objectType_ = POINT;
        for (xml_node keyNode = node.child("key"); !keyNode.empty(); keyNode = keyNode.next_sibling("key"))
        {
            keys_.Push(new PointTimelineKey(this));
            if (!keys_.Back()->Load(keyNode))
                return false;
        }
    }
    else if (typeString == "box")
    {
        objectType_ = BOX;
        for (xml_node keyNode = node.child("key"); !keyNode.empty(); keyNode = keyNode.next_sibling("key"))
        {
            keys_.Push(new BoxTimelineKey(this));
            if (!keys_.Back()->Load(keyNode))
                return false;
        }
    }

    return true;
}

SpatialTimelineKey* Timeline::GetTimeKey(float time) const
{
    if (time == 0.f)
        return keys_.Front();

    for (PODVector<SpatialTimelineKey*>::ConstIterator it = keys_.End()-1; it != keys_.Begin(); --it)
    {
        if (time >= (*it)->time_)
            return *it;
    }

    return keys_.Front();
}


TimelineKey::TimelineKey(Timeline* timeline) :
    timeline_(timeline)
{ }

TimelineKey::~TimelineKey()
{
}

TimelineKey& TimelineKey::operator=(const TimelineKey& rhs)
{
    id_ = rhs.id_;
    time_ = rhs.time_;
    curveType_ = rhs.curveType_;
    c1_ = rhs.c1_;
    c2_ = rhs.c2_;
    c3_ = rhs.c3_;
    c4_ = rhs.c4_;
    return *this;
}

SpatialInfo::SpatialInfo(float x, float y, float angle, float scale_x, float scale_y, float a, int spin)
{
    this->x_ = x;
    this->y_ = y;
    this->angle_ = angle;
    this->scaleX_ = scale_x;
    this->scaleY_ = scale_y;
    this->alpha_ = a;
    this->spin = spin;
}

void SpatialInfo::UnmapFromParent(const SpatialInfo& parentInfo)
{
    angle_ = parentInfo.angle_ + Sign(parentInfo.scaleX_*parentInfo.scaleY_) * angle_;
    if (angle_ >= 360.f)
        angle_ -= 360.f;

    scaleX_ = scaleX_ * parentInfo.scaleX_;
    scaleY_ = scaleY_ * parentInfo.scaleY_;
    alpha_ = alpha_ * parentInfo.alpha_;

    if (x_ != 0.0f || y_ != 0.0f)
    {
        float preMultX = x_ * parentInfo.scaleX_;
        float preMultY = y_ * parentInfo.scaleY_;

        float s = Sin(parentInfo.angle_);
        float c = Cos(parentInfo.angle_);

        x_ = (preMultX * c) - (preMultY * s) + parentInfo.x_;
        y_ = (preMultX * s) + (preMultY * c) + parentInfo.y_;
    }
    else
    {
        x_ = parentInfo.x_;
        y_ = parentInfo.y_;
    }
}


void SpatialInfo::Interpolate(const SpatialInfo& other, float t)
{
    x_ = Linear(x_, other.x_, t);
    y_ = Linear(y_, other.y_, t);
    scaleX_ = Linear(scaleX_, other.scaleX_, t);
    scaleY_ = Linear(scaleY_, other.scaleY_, t);
    alpha_ = Linear(alpha_, other.alpha_, t);
    angle_ = AngleLinear(angle_, other.angle_, spin, t);
}


SpatialTimelineKey::SpatialTimelineKey(Timeline* timeline) :
    TimelineKey(timeline) { }

SpatialTimelineKey::~SpatialTimelineKey() { }

bool SpatialTimelineKey::Load(const xml_node& node)
{
    if (!TimelineKey::Load(node))
        return false;

    xml_node childNode = node.child("bone");
    if (childNode.empty())
        childNode = node.child("object");

    info_.x_ = childNode.attribute("x").as_float();
    info_.y_ = childNode.attribute("y").as_float();
    info_.angle_ = childNode.attribute("angle").as_float();
    info_.scaleX_ = childNode.attribute("scale_x").as_float(1.0f);
    info_.scaleY_ = childNode.attribute("scale_y").as_float(1.0f);
    info_.alpha_ = childNode.attribute("a").as_float(1.0f);

    info_.spin = node.attribute("spin").as_int(1);

    return true;
}

SpatialTimelineKey& SpatialTimelineKey::operator=(const SpatialTimelineKey& rhs)
{
    TimelineKey::operator=(rhs);
    info_ = rhs.info_;
    return *this;
}

void SpatialTimelineKey::Interpolate(const TimelineKey& other, float t)
{
    const SpatialTimelineKey& o = (const SpatialTimelineKey&)other;
    info_.Interpolate(o.info_, t);
}


BoneTimelineKey::BoneTimelineKey() :
    SpatialTimelineKey(0)
{

}

BoneTimelineKey::BoneTimelineKey(Timeline* timeline) :
    SpatialTimelineKey(timeline)
{

}

BoneTimelineKey::~BoneTimelineKey()
{

}

TimelineKey* BoneTimelineKey::Clone() const
{
    BoneTimelineKey* result = new BoneTimelineKey(timeline_);
    *result = *this;
    return result;
}

void BoneTimelineKey::Copy(TimelineKey* copy) const
{
    BoneTimelineKey* c = static_cast<BoneTimelineKey*>(copy);
    if (!c)
        return;
    *c = *this;
}

bool BoneTimelineKey::Load(const xml_node& node)
{
    if (!SpatialTimelineKey::Load(node))
        return false;

    xml_node boneNode = node.child("bone");
    return true;
}

BoneTimelineKey& BoneTimelineKey::operator=(const BoneTimelineKey& rhs)
{
    SpatialTimelineKey::operator=(rhs);

    return *this;
}

void BoneTimelineKey::Interpolate(const TimelineKey& other, float t)
{
    SpatialTimelineKey::Interpolate(other, t);

    const BoneTimelineKey& o = (const BoneTimelineKey&)other;
}


SpriteTimelineKey::SpriteTimelineKey() :
    SpatialTimelineKey(0)
{

}

SpriteTimelineKey::SpriteTimelineKey(Timeline* timeline) :
    SpatialTimelineKey(timeline)
{

}

SpriteTimelineKey::~SpriteTimelineKey()
{

}

TimelineKey* SpriteTimelineKey::Clone() const
{
    SpriteTimelineKey* result = new SpriteTimelineKey(timeline_);
    *result = *this;
    return result;
}

void SpriteTimelineKey::Copy(TimelineKey* copy) const
{
    SpriteTimelineKey* c = static_cast<SpriteTimelineKey*>(copy);
    if (!c)
        return;
    *c = *this;
}

bool SpriteTimelineKey::Load(const pugi::xml_node& node)
{
    if (!SpatialTimelineKey::Load(node))
        return false;

    xml_node objectNode = node.child("object");
    folderId_ = objectNode.attribute("folder").as_uint(0);
    fileId_ = objectNode.attribute("file").as_uint(0);
    fx_ = objectNode.attribute("fx").as_uint(0);

    xml_attribute pivotXAttr = objectNode.attribute("pivot_x");
    xml_attribute pivotYAttr = objectNode.attribute("pivot_y");
    if (pivotXAttr.empty() && pivotYAttr.empty())
        useDefaultPivot_ = true;
    else
    {
        useDefaultPivot_ = false;
        pivotX_ = pivotXAttr.as_float(0.0f);
        pivotY_ = pivotYAttr.as_float(1.0f);
    }

    return true;
}

void SpriteTimelineKey::Interpolate(const TimelineKey& other, float t)
{
    SpatialTimelineKey::Interpolate(other, t);

    const SpriteTimelineKey& o = (const SpriteTimelineKey&)other;
    pivotX_ = Linear(pivotX_, o.pivotX_, t);
    pivotY_ = Linear(pivotY_, o.pivotY_, t);
}

SpriteTimelineKey& SpriteTimelineKey::operator=(const SpriteTimelineKey& rhs)
{
    SpatialTimelineKey::operator=(rhs);

    folderId_ = rhs.folderId_;
    fileId_ = rhs.fileId_;
    fx_ = rhs.fx_;
    useDefaultPivot_ = rhs.useDefaultPivot_;
    pivotX_ = rhs.pivotX_;
    pivotY_ = rhs.pivotY_;

    return *this;
}


BoxTimelineKey::BoxTimelineKey() :
    SpatialTimelineKey(0)
{

}

BoxTimelineKey::BoxTimelineKey(Timeline* timeline) :
    SpatialTimelineKey(timeline)
{
}

BoxTimelineKey::~BoxTimelineKey()
{

}

TimelineKey* BoxTimelineKey::Clone() const
{
    BoxTimelineKey* result = new BoxTimelineKey(timeline_);
    *result = *this;
    return result;
}

void BoxTimelineKey::Copy(TimelineKey* copy) const
{
    BoxTimelineKey* c = static_cast<BoxTimelineKey*>(copy);
    if (!c)
        return;
    *c = *this;
}

bool BoxTimelineKey::Load(const pugi::xml_node& node)
{
    if (!SpatialTimelineKey::Load(node))
        return false;

    xml_node objectNode = node.child("object");

    xml_attribute pivotXAttr = objectNode.attribute("pivot_x");
    xml_attribute pivotYAttr = objectNode.attribute("pivot_y");

    if (pivotXAttr.empty() && pivotYAttr.empty())
        useDefaultPivot_ = true;
    else
    {
        useDefaultPivot_ = false;
        pivotX_ = pivotXAttr.as_float(0.0f);
        pivotY_ = pivotYAttr.as_float(1.0f);
    }

    return true;
}

void BoxTimelineKey::Interpolate(const TimelineKey& other, float t)
{
    SpatialTimelineKey::Interpolate(other, t);

    const BoxTimelineKey& o = (const BoxTimelineKey&)other;
    pivotX_ = Linear(pivotX_, o.pivotX_, t);
    pivotY_ = Linear(pivotY_, o.pivotY_, t);
    width_ = Linear(width_, o.width_, t);
    height_ = Linear(height_, o.height_, t);
}

BoxTimelineKey& BoxTimelineKey::operator=(const BoxTimelineKey& rhs)
{
    SpatialTimelineKey::operator=(rhs);

    width_ = rhs.width_;
    height_ = rhs.height_;
    useDefaultPivot_ = rhs.useDefaultPivot_;
    pivotX_ = rhs.pivotX_;
    pivotY_ = rhs.pivotY_;

    return *this;
}

PointTimelineKey::PointTimelineKey() :
    SpatialTimelineKey(0)
{

}

PointTimelineKey::PointTimelineKey(Timeline* timeline) :
    SpatialTimelineKey(timeline)
{

}

PointTimelineKey::~PointTimelineKey()
{

}

TimelineKey* PointTimelineKey::Clone() const
{
    PointTimelineKey* result = new PointTimelineKey(timeline_);
    *result = *this;
    return result;
}

void PointTimelineKey::Copy(TimelineKey* copy) const
{
    PointTimelineKey* c = static_cast<PointTimelineKey*>(copy);
    if (!c)
        return;
    *c = *this;
}

bool PointTimelineKey::Load(const xml_node& node)
{
    if (!SpatialTimelineKey::Load(node))
        return false;

    xml_node boneNode = node.child("bone");

    return true;
}

PointTimelineKey& PointTimelineKey::operator=(const PointTimelineKey& rhs)
{
    SpatialTimelineKey::operator=(rhs);

    return *this;
}

void PointTimelineKey::Interpolate(const TimelineKey& other, float t)
{
    SpatialTimelineKey::Interpolate(other, t);
}

}

}
