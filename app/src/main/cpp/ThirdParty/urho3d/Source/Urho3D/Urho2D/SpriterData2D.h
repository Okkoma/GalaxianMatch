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

#pragma once

namespace pugi
{
class xml_node;
}

//#define USE_KEYPOOLS

namespace Urho3D
{

namespace Spriter
{

struct SpriterData;

struct File;
struct Folder;

struct Entity;
struct ObjInfo;
struct CharacterMap;
struct MapInstruction;
struct Animation;

struct Ref;
struct Timeline;
struct SpatialInfo;

struct TimeKey;
struct MainlineKey;
struct TimelineKey;

struct SpatialTimelineKey;
struct SpriteTimelineKey;
struct BoneTimelineKey;
struct BoxTimelineKey;

#define DEFAULT_KEYPOOLSIZE 1000

/// Object type.
enum ObjectType
{
    BONE = 0,
    SPRITE,
    POINT,
    BOX
};

/// Curve type.
enum CurveType
{
    INSTANT = 0,
    LINEAR,
    QUADRATIC,
    CUBIC,
    QUARTIC,
    QUINTIC,
    BEZIER
};

/// Spriter data.
struct SpriterData
{
    SpriterData();
    ~SpriterData();

    void Reset();
    bool Load(const pugi::xml_node& node);
    bool Load(const void* data, size_t size);
    void UpdateKeyInfos();

#ifdef USE_KEYPOOLS
    static void InitKeyPools(unsigned poolSize = DEFAULT_KEYPOOLSIZE);
#endif
    static float GetFactor(TimeKey* keyA, TimeKey* keyB, float length, float targetTime);
    static float AdjustTime(TimeKey* keyA, TimeKey* keyB, float length, float targetTime);

    int scmlVersion_;
    String generator_;
    String generatorVersion_;
    PODVector<Folder*> folders_;
    PODVector<Entity*> entities_;
};

/// Folder.
struct Folder
{
    Folder();
    ~Folder();

    void Reset();
    bool Load(const pugi::xml_node& node);

    int id_;
    String name_;
    PODVector<File*> files_;
};

/// File.
struct File
{
    File(Folder* folder);
    ~File();

    bool Load(const pugi::xml_node& node);

    Folder* folder_;
    int id_;
    String name_;
    float width_;
    float height_;
    float pivotX_;
    float pivotY_;
};

/// Entity.
struct Entity
{
    Entity();
    ~Entity();

    void Reset();
    bool Load(const pugi::xml_node& node);

    int id_;
    String name_;

    HashMap<String, ObjInfo > objInfos_;
    PODVector<CharacterMap*> characterMaps_;
    PODVector<Animation*> animations_;
};

/// Object Info.
struct ObjInfo
{
    ObjInfo();
    ~ObjInfo();

    static bool Load(const pugi::xml_node& node, ObjInfo& objinfo);

    ObjectType type_;
    float width_;
    float height_;
    float pivotX_;
    float pivotY_;
};

/// Character map.
struct CharacterMap
{
    CharacterMap();
    ~CharacterMap();

    void Reset();
    bool Load(const pugi::xml_node& node);

    int id_;
    String name_;
    StringHash hashname_;
    PODVector<MapInstruction*> maps_;
};

/// Map instruction.
struct MapInstruction
{
    MapInstruction();
    ~MapInstruction();

    bool Load(const pugi::xml_node& node);

    int folder_;
    int file_;
    int targetFolder_;
    int targetFile_;
};

/// Animation.
struct Animation
{
    Animation();
    ~Animation();

    void Reset();
    bool Load(const pugi::xml_node& node);

    int id_;
    String name_;
    float length_;
    bool looping_;
    PODVector<MainlineKey*> mainlineKeys_;
    PODVector<Timeline*> timelines_;
};

/// Ref.
struct Ref
{
    Ref();
    ~Ref();

    bool Load(const pugi::xml_node& node);

    int id_;
    int parent_;
    int timeline_;
    int key_;
    int zIndex_;
};



/// Timeline.
struct Timeline
{
    Timeline();
    ~Timeline();

    void Reset();
    bool Load(const pugi::xml_node& node);

    int id_;
    String name_;
    ObjectType objectType_;
    PODVector<SpatialTimelineKey*> keys_;
};


/// Spatial info.
struct SpatialInfo
{
    float x_;
    float y_;
    float angle_;
    float scaleX_;
    float scaleY_;
    float alpha_;
    int spin;

    SpatialInfo(float x = 0.0f, float y = 0.0f, float angle = 0.0f, float scale_x = 1, float scale_y = 1, float a = 1, int spin = 1);
    SpatialInfo UnmapFromParent(const SpatialInfo& parentInfo) const;
    void Interpolate(const SpatialInfo& other, float t);
};


struct TimeKey
{
    TimeKey();
    virtual ~TimeKey();

    virtual bool Load(const pugi::xml_node& node);

    float ApplyCurveType(float factor);
    float AdjustTime(float timeA, float timeB, float length, float targetTime);
    float GetFactor(float timeA, float timeB, float length, float targetTime);

    int id_;
    float time_;
    CurveType curveType_;
    float c1_;
    float c2_;
    float c3_;
    float c4_;
};

/// Mainline key.
struct MainlineKey : public TimeKey
{
    MainlineKey();
    virtual ~MainlineKey();

    virtual bool Load(const pugi::xml_node& node);

    void Reset();

    PODVector<Ref*> boneRefs_;
    PODVector<Ref*> objectRefs_;
};

/// Timeline key.
struct TimelineKey : public TimeKey
{
    TimelineKey(Timeline* timeline);
    virtual ~TimelineKey();

    ObjectType GetObjectType() const { return timeline_->objectType_; }
    virtual TimelineKey* Clone() const = 0;

    virtual void Interpolate(const TimelineKey& other, float t) = 0;
    TimelineKey& operator=(const TimelineKey& rhs);

    Timeline* timeline_;
};

/// Spatial timeline key.
struct SpatialTimelineKey : TimelineKey
{
    SpatialInfo info_;

    SpatialTimelineKey(Timeline* timeline);
    virtual ~SpatialTimelineKey();
    virtual bool Load(const pugi::xml_node& node);
    virtual void Interpolate(const TimelineKey& other, float t);
    SpatialTimelineKey& operator=(const SpatialTimelineKey& rhs);
};

/// Bone timeline key.
struct BoneTimelineKey : SpatialTimelineKey
{
//    float length_;
//    float width_;
#ifdef USE_KEYPOOLS
    static BoneTimelineKey* Get();
    static void Free(BoneTimelineKey* elt);
    static void FreeAlls();
    static PODVector<BoneTimelineKey*> freeindexes_;
    static Vector<BoneTimelineKey> pool_;
#endif
    BoneTimelineKey();
    BoneTimelineKey(Timeline* timeline);
    virtual ~BoneTimelineKey();

    virtual TimelineKey* Clone() const;
    virtual bool Load(const pugi::xml_node& node);
    virtual void Interpolate(const TimelineKey& other, float t);
    BoneTimelineKey& operator=(const BoneTimelineKey& rhs);
};

/// Sprite timeline key.
struct SpriteTimelineKey : SpatialTimelineKey
{
    bool useDefaultPivot_;
    float pivotX_;
    float pivotY_;
    int folderId_;
    int fileId_;

    // Run time data.
    int zIndex_;
#ifdef USE_KEYPOOLS
    static SpriteTimelineKey* Get();
    static void Free(SpriteTimelineKey* elt);
    static void FreeAlls();
    static PODVector<SpriteTimelineKey*> freeindexes_;
    static Vector<SpriteTimelineKey> pool_;
#endif
    SpriteTimelineKey();
    SpriteTimelineKey(Timeline* timeline);
    virtual ~SpriteTimelineKey();

    virtual TimelineKey* Clone() const;
    virtual bool Load(const pugi::xml_node& node);
    virtual void Interpolate(const TimelineKey& other, float t);
    SpriteTimelineKey& operator=(const SpriteTimelineKey& rhs);
};

/// Box timeline key.
struct BoxTimelineKey : SpatialTimelineKey
{
    bool useDefaultPivot_;
    float pivotX_;
    float pivotY_;
    float width_;
    float height_;
#ifdef USE_KEYPOOLS
    static BoxTimelineKey* Get();
    static void Free(BoxTimelineKey* elt);
    static void FreeAlls();
    static PODVector<BoxTimelineKey*> freeindexes_;
    static Vector<BoxTimelineKey> pool_;
#endif
    BoxTimelineKey();
    BoxTimelineKey(Timeline* timeline);
    virtual ~BoxTimelineKey();

    virtual TimelineKey* Clone() const;
    virtual bool Load(const pugi::xml_node& node);
    virtual void Interpolate(const TimelineKey& other, float t);
    BoxTimelineKey& operator=(const BoxTimelineKey& rhs);
};

}

}
