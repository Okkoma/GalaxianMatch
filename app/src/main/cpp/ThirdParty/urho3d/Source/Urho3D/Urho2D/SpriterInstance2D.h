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

#include "../Urho2D/SpriterData2D.h"

namespace Urho3D
{

class Component;

/// Points Events
URHO3D_EVENT(SPRITER_, SPRITER_Event)
{
    URHO3D_PARAM(TYPE, type);
    URHO3D_PARAM(DATAS, datas);
    URHO3D_PARAM(ZINDEX, zindex);
    URHO3D_PARAM(XPOSITION, xposition);
    URHO3D_PARAM(YPOSITION, yposition);
}

namespace Spriter
{

/// Loop Mode.
enum LoopMode
{
    Default = 0,
    ForceLooped,
    ForceClamped,
};

struct NodeUpdater
{
    NodeUpdater() : timekey_(0), ucomponent_(0) { }
    PointTimelineKey* timekey_;
    void* ucomponent_;
};

/// Spriter instance.
class SpriterInstance
{
public:
    /// Constructor with spriter data.
    SpriterInstance(Component* owner, SpriterData* spriteData);
    /// Destructor.
    ~SpriterInstance();

    /// Set current entity.
    bool SetEntity(int index);
    /// Set current entity.
    bool SetEntity(const String& entityName);
    /// Set current animation.
    bool SetAnimation(int index, LoopMode loopMode = Default);
    /// Set current animation.
    bool SetAnimation(const String& animationName, LoopMode loopMode = Default);
    /// Set root spatial info.
    void SetSpatialInfo(const SpatialInfo& spatialInfo);
    /// Set root spatial info.
    void SetSpatialInfo(float x, float y, float angle, float scaleX, float scaleY);

    void SetCurrentTime(float time);
    void ResetCurrentTime();

    /// Return current entity.
    Entity* GetEntity() const { return entity_; }
    /// Return the entity at index.
    Entity* GetEntity(int index) const { return spriterData_->entities_[index]; }
    unsigned GetNumEntities() const { return spriterData_->entities_.Size(); }

    /// Return current animation.
    Animation* GetAnimation() const { return animation_; }
    /// Return animation by index.
    Animation* GetAnimation(int index) const { return index < entity_->animations_.Size() ? entity_->animations_[index] : 0; }
    /// Return animation by name.
    Animation* GetAnimation(const String& name) const;
    /// Return root spatial info.
    const SpatialInfo& GetSpatialInfo() const { return spatialInfo_; }
    /// Return animation result timeline keys.
    unsigned GetNumBoneKeys() const { return numBoneKeys_; }
    const PODVector<BoneTimelineKey* >& GetBoneKeys() const { return boneKeys_; }
    unsigned GetNumSpriteKeys() const { return numSpriteKeys_; }
    const PODVector<SpriteTimelineKey* >& GetSpriteKeys() const { return spriteKeys_; }
    /// Return animation triggers.
    HashMap<String, NodeUpdater >& GetNodeUpdaters() { return nodeUpdaters_; }
    const HashMap<String, NodeUpdater >& GetNodeUpdaters() const { return nodeUpdaters_; }
    const HashMap<Timeline*, PointTimelineKey* >& GetEventTriggers() const { return eventTriggers_; }
    const HashMap<Timeline*, BoxTimelineKey* >& GetPhysicTriggers() const { return physicTriggers_; }
    
/// Return time passed on the current animation.
    float GetCurrentTime() const { return currentTime_; }

    Spriter::MainlineKey* GetCurrentMainKey() const { return mainlineKey_; }

    bool HasFinishedAnimation() const;
    bool GetLooping() const;

    /// Update animation.
    bool Update(float deltaTime);
    /// Update timeline keys.
    void UpdateTimelineKeys();

private:
    /// Clear mainline key and timeline keys.
    void RestoreKeys();
    void Dispose();

    /// Handle set entity.
    void OnSetEntity(Entity* entity);
    /// Handle set animation.
    void OnSetAnimation(Animation* animation, LoopMode loopMode = Default);

    /// Update mainline keys.
    bool UpdateMainlineKeys(float deltaTime);

    /// Get timeline key by ref.
    TimelineKey* GetTimelineKey(Timeline* timeline, Ref* ref, float targetTime, TimelineKey* reuse) const;

    /// Parent component.
    Component* owner_;
    /// Spriter data.
    SpriterData* spriterData_;
    /// Current entity.
    Entity* entity_;
    /// Current animation.
    Animation* animation_;
    /// Next animation.
    Animation* nextanimation_;
    /// Looping.
    bool looping_;
    /// Root spatial info.
    SpatialInfo spatialInfo_;
    /// Current time.
    float currentTime_, adjustedTime_;
//    float lastMainInstantTime_;

    /// Current mainline key.
    MainlineKey* prevmainlineKey_;
    MainlineKey* mainlineKey_;

    /// Current timeline keys.
    unsigned numBoneKeys_, numSpriteKeys_;
    PODVector<BoneTimelineKey* > boneKeys_;
    PODVector<SpriteTimelineKey* > spriteKeys_;

    /// Current event keys.
    HashMap<String, NodeUpdater > nodeUpdaters_;
    HashMap<Timeline*, PointTimelineKey* > eventTriggers_;
    HashMap<Timeline*, BoxTimelineKey* > physicTriggers_;
};

}

}
