#pragma once

namespace pugi
{
    class xml_node;
}

using namespace Urho3D;

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

/// Object type.
enum SpriterObjectType
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

    static const char* GetCurveTypeStr(CurveType type);

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

    unsigned id_;
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
    unsigned id_;
    unsigned fx_;
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

    unsigned id_;
    String name_;
    Color color_;

    HashMap<StringHash, ObjInfo > objInfos_;
    PODVector<CharacterMap*> characterMaps_;
    PODVector<Animation*> animations_;
};

/// Object Info.
struct ObjInfo
{
    ObjInfo();
    ~ObjInfo();

    static bool Load(const pugi::xml_node& node, ObjInfo& objinfo);

    String name_;
    SpriterObjectType type_;
    float width_;
    float height_;
    float pivotX_;
    float pivotY_;
};

inline unsigned GetKey(unsigned folderid, unsigned fileid) { return (folderid << 16) + fileid; }
inline void GetFolderFile(unsigned key, unsigned& folderid, unsigned& fileid) { folderid = key >> 16; fileid = key & 0xFFFF; }

/// Character map.
struct CharacterMap
{
    CharacterMap();
    ~CharacterMap();

    void Reset();
    bool Load(const pugi::xml_node& node);

    MapInstruction* GetInstruction(unsigned key, bool add=false);
    MapInstruction* GetInstruction(unsigned folder, unsigned file);
    MapInstruction* RemoveInstruction(unsigned key);

    unsigned id_;
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

    void SetOrigin(unsigned spritekey);
    void SetTarget(unsigned targetkey);
    void RemoveTarget();

    unsigned folder_;
    unsigned file_;
    int targetFolder_;
    int targetFile_;

    float targetdx_;
    float targetdy_;
    float targetdangle_;
    float targetscalex_;
    float targetscaley_;
};

/// Animation.
struct Animation
{
    Animation();
    ~Animation();

    void Reset();
    bool Load(const pugi::xml_node& node);

    void GetBoneRefs(unsigned timeline, PODVector<Ref*>& refs, unsigned startmainkeyid=0);
    void GetObjectRefs(unsigned timeline, PODVector<Ref*>& refs, unsigned startmainkeyid=0);

    MainlineKey* GetMainlineKey(float time) const;
    void UnMapToRoot(SpatialTimelineKey* tkey, float time, bool includeFirstKey, SpatialInfo& info) const;

    unsigned id_;
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

    void Copy(Ref& copy) const;

    unsigned id_;
    int parent_;
    unsigned timeline_;
    unsigned key_;
    int zIndex_;
    Color color_;
};



/// Timeline.
struct Timeline
{
    Timeline();
    ~Timeline();

    void Reset();
    bool Load(const pugi::xml_node& node);

    SpatialTimelineKey* GetTimeKey(float time) const;

    unsigned id_;
    String name_;
    StringHash hashname_;
    SpriterObjectType objectType_;
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

    SpatialInfo(float x = 0.0f, float y = 0.0f, float angle = 0.0f, float scale_x = 1, float scale_y = 1, float a = 1, int sp = 1);
    void UnmapFromParent(const SpatialInfo& parentInfo);
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

    unsigned id_;
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

    Ref* GetRef(unsigned timeline) const;
    Ref* GetBoneRef(unsigned timeline) const;
    Ref* GetObjectRef(unsigned timeline) const;

    PODVector<Ref*> boneRefs_;
    PODVector<Ref*> objectRefs_;
};

/// Timeline key.
struct TimelineKey : public TimeKey
{
    TimelineKey(Timeline* timeline);
    virtual ~TimelineKey();

    SpriterObjectType GetObjectType() const { return timeline_->objectType_; }
    virtual TimelineKey* Clone() const = 0;
    virtual void Copy(TimelineKey* copy) const = 0;

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
    BoneTimelineKey();
    BoneTimelineKey(Timeline* timeline);
    virtual ~BoneTimelineKey();

    virtual TimelineKey* Clone() const;
    virtual void Copy(TimelineKey* copy) const;
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
    unsigned folderId_;
    unsigned fileId_;
    unsigned fx_;

    // Run time data.
    int zIndex_;
    Color color_;

    SpriteTimelineKey();
    SpriteTimelineKey(Timeline* timeline);
    virtual ~SpriteTimelineKey();

    virtual TimelineKey* Clone() const;
    virtual void Copy(TimelineKey* copy) const;
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
    BoxTimelineKey();
    BoxTimelineKey(Timeline* timeline);
    virtual ~BoxTimelineKey();

    virtual TimelineKey* Clone() const;
    virtual void Copy(TimelineKey* copy) const;
    virtual bool Load(const pugi::xml_node& node);
    virtual void Interpolate(const TimelineKey& other, float t);
    BoxTimelineKey& operator=(const BoxTimelineKey& rhs);
};

/// Point timeline key.
struct PointTimelineKey : SpatialTimelineKey
{
    // Run time data.
    int zIndex_;

    PointTimelineKey();
    PointTimelineKey(Timeline* timeline);
    virtual ~PointTimelineKey();

    virtual TimelineKey* Clone() const;
    virtual void Copy(TimelineKey* copy) const;
    virtual bool Load(const pugi::xml_node& node);
    virtual void Interpolate(const TimelineKey& other, float t);
    PointTimelineKey& operator=(const PointTimelineKey& rhs);
};

}

