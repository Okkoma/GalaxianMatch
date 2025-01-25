#pragma once

#include <Urho3D/Core/Timer.h>
#include <Urho3D/Scene/LogicComponent.h>
#include <Urho3D/Scene/Serializable.h>
#include <Urho3D/Urho2D/StaticSprite2D.h>
#include <Urho3D/Urho2D/AnimatedSprite2D.h>

using namespace Urho3D;

class SplinePath2D;
class SceneAction2D;
class SceneTimeline2D;
class SceneAnimation2D;


URHO3D_EVENT(E_SCENEANIMATIONTIMECHANGED, SceneAnimationTimeChanged)
{
    URHO3D_PARAM(P_ELEMENT, Element);  // Ptr to SceneAnimation Component
    URHO3D_PARAM(P_TIME, Time);        // float
}

URHO3D_EVENT(E_SCENEANIMATIONFINISHED, SceneAnimationFinished)
{
    URHO3D_PARAM(P_ELEMENT, Element);  // Ptr to SceneAnimation Component
    URHO3D_PARAM(P_TIME, Time);        // float
}


const float MAXDURATION = 1000.f;

struct ObjectActionState2D
{
    ObjectActionState2D() : alpha_(1.f), animationid_(-1), traveled_(0.f), duration_(MAXDURATION) { }

	// action start position datas
    Vector3 translation_;
    Vector3 scale_;
    Quaternion rotation_;
    float alpha_;
    int animationid_;

    // motion datas
    Matrix3x4 transform_;
    float traveled_;
    float duration_;
};

class SceneObject2D : public Serializable
{
    URHO3D_OBJECT(SceneObject2D, Serializable);

public:
    SceneObject2D(Context* context);
    static void RegisterObject(Context* context);

    void SetNodeRef(unsigned ref);
    unsigned GetNodeRef() const { return noderef_; }

    void SetSpeedFactor(float speedfactor);
    float GetSpeedFactor() const { return speedfactor_; }

    void ApplyAttributes();

    bool UpdateRefs(Scene* scene);

    void GetCurrentStates(ObjectActionState2D& state);
    void CopyState(const ObjectActionState2D& src, ObjectActionState2D& dst);
    void UpdateActionStates(int key, const Vector<SharedPtr<SceneAction2D> >& actions, float maxduration, bool setPosition);
    void UpdateActionStates(const Vector<SharedPtr<SceneAction2D> >& actions, bool setPosition);

    void Reset();

    // Node Ref
    unsigned noderef_;
    WeakPtr<Node > node_;

    // Sprite Ref
    WeakPtr<StaticSprite2D > sprite_;
    WeakPtr<AnimatedSprite2D > animatedsprite_;

    // fixed motion datas
    float speedfactor_;
    Vector3 absoluteoffset_;

    // states : initial + action states => action states begin at index 1
    Vector<ObjectActionState2D > states_;

    bool dirty_;
};

class SceneAction2D : public Serializable
{
    URHO3D_OBJECT(SceneAction2D, Serializable);

public:
    SceneAction2D(Context* context);
    static void RegisterObject(Context* context);

    void Init(SceneTimeline2D* timeline, float time);

    void SetStartPosition(const Vector3& position);
    const Vector3& GetStartPosition() const { return position_; }
    void SetPathRef(unsigned ref);
    unsigned GetPathRef() const { return pathref_; }
    void SetSpeedRef(unsigned ref);
    unsigned GetSpeedRef() const { return speedref_; }
    void SetAnimatedObjects(const String& value);
    String GetAnimatedObjects() const;

    void ApplyAttributes();

    static float GetDuration(unsigned key, SceneAction2D* action, SceneObject2D* object, float maxduration=MAXDURATION);
    static Vector3 GetPosition(unsigned key, SceneAction2D* action, SceneObject2D* object, float time, bool travelupdate=true);
    static Vector3 GetTravel(unsigned key, SceneAction2D* action, SceneObject2D* object, float time, bool travelupdate=true);
    static float GetTangentAngleXY(unsigned key, SceneAction2D* action, SceneObject2D* object, float time);
    static float GetAlpha(int key, const Vector<SharedPtr<SceneAction2D> >& actions, SceneObject2D* object, float time);

    void Execute(unsigned key, const Vector<SharedPtr<SceneAction2D> >& actions, float time);
    bool UpdateRefs(Scene* scene);
    void Reset(unsigned id);

    // Generic data
    SceneTimeline2D* timeline_;
    float starttime_, nextkeytime_;

    // Node Position Modifier
    bool motionok_;
    bool rotate_;
    bool absolute_;
    bool positionset_;
    Vector3 position_;
    unsigned pathref_;
    WeakPtr<SplinePath2D > path_;
    unsigned speedref_;
    WeakPtr<SplinePath2D > speedcurve_;

    // Node Enable Modifier
    bool enableok_;
    bool enabled_;

    // Sprite Animation Modifier
    bool animationok_;
    PODVector<unsigned> animatedobjects_;
    String animation_;

    // Sprite Alpha Modifier
    bool alphaok_;
    float alphaspeed_;
    float alphagoal_;

    // Event
    bool eventok_;
    String eventname_;
    StringHash event_;

    // action state
    float updatedtime_;
    bool started_, finished_;
    bool dirty_;
};

/// Timeline 2D.
class SceneTimeline2D : public Serializable
{
    URHO3D_OBJECT(SceneTimeline2D, Serializable);

public:
    explicit SceneTimeline2D(Context* context);
    virtual ~SceneTimeline2D();

    static void RegisterObject(Context* context);

    /// Load from XML data. Return true if successful.
    bool LoadXML(const XMLElement& source);
    /// Save as XML data. Return true if successful.
    bool SaveXML(XMLElement& dest) const;

    virtual void ApplyAttributes();

    void Update(float time);
    bool UpdateRefs(bool setPosition=false);
    void UpdateActionStates(unsigned keyindex);
    void UpdateActionStatesNear(int index);
    void UpdateObjectPosition(Node* node, float time);
    void Reset(float time);

    bool IsFinished() const { return finished_; }
    int GetActionIndex(SceneAction2D* action) const;
    int GetActionIndex(float time) const;

    SceneAnimation2D* animation_;

    float updatedtime_;
    bool finished_;
    String name_;

    Vector<SharedPtr<SceneObject2D> > objects_;
    Vector<SharedPtr<SceneAction2D> > actions_;
};

///
class SceneAnimation2D : public LogicComponent
{
    URHO3D_OBJECT(SceneAnimation2D, LogicComponent);

public:
    /// Construct.
    explicit SceneAnimation2D(Context* context);
    /// Destruct.
    virtual ~SceneAnimation2D();
    /// Register object factory.
    static void RegisterObject(Context* context);

    virtual bool LoadXML(const XMLElement& source, bool setInstanceDefault = false, bool applyAttr = true);
    /// Save as XML data. Return true if successful.
    virtual bool SaveXML(XMLElement& dest) const;

	virtual void OnSetEnabled();

	void SetRunning(bool state);

    /// Called before the first update. At this point all other components of the node should exist. Will also be called if update events are not wanted; in that case the event is immediately unsubscribed afterward.
    virtual void DelayedStart();
    /// Called when the component is detached from a scene node, usually on destruction. Note that you will no longer have access to the node and scene at that point.
    virtual void Stop();
    /// Called on scene update, variable timestep.
    virtual void Update(float timeStep);

    void SetTime(float time);
    float GetTime() const { return elapsedTime_; }
    bool IsFinished() const { return finished_; }
    bool IsEmpty() const { return timelines_.Size() == 0; }

    /// Editor Setters & Getters
    void AddTimeLine(unsigned index, const String& name);
    void RemoveTimeLine(const String& name);
    void AddNodeObject(Node* node, unsigned line, unsigned index);
    void RemoveNodeObject(unsigned line, unsigned index);
    void AddAction(unsigned line, float time);
    void RemoveAction(unsigned line, unsigned index);
    int SetTimeKey(unsigned timeline, unsigned key, float time);
    void PermuteTimeKey(unsigned line, unsigned key1, unsigned key2);

    int GetTimeLineIndex(const String& name) const;
    const Vector<SharedPtr<SceneTimeline2D> >& GetTimeLines() const { return timelines_; }
    Vector<String> GetTimeLineNames() const;
    Vector<String> GetTimeLineObjectNames(unsigned line) const;
    const Vector<SharedPtr<SceneObject2D> >& GetTimeLineObjects(unsigned line) const { return timelines_[line]->objects_; }
    Serializable* GetTimeLine(unsigned line) const;
    Serializable* GetTimeLineObject(unsigned line, unsigned index) const;
    unsigned GetTimeLineObjectNodeID(unsigned line, unsigned index) const;
    Serializable* GetTimeLineActionSpeedCurve(unsigned line, unsigned index) const;
    Serializable* GetTimeLineAction(unsigned line, unsigned index) const;
    Serializable* GetTimeLineActionPath(unsigned line, unsigned index) const;

protected:
   virtual void OnMarkedDirty(Node* node);

private:
    Timer timer_;
    float startTime_, elapsedTime_;
    bool refsUpdated_, started_, finished_, updateRunning_, timeSetting_;
    Vector<SharedPtr<SceneTimeline2D> > timelines_;
};

