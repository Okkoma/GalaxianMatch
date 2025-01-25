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
    /// Update animation.
    bool Update(float deltaTime);
    void SetTime(float time);

    /// Return current entity.
    Entity* GetEntity() const { return entity_; }
    /// Return the entity at index.
    Entity* GetEntity(int index) const { return spriterData_->entities_[index]; }

    /// Return current animation.
    Animation* GetAnimation() const { return animation_; }
    /// Return animation by index.
    Animation* GetAnimation(int index) const { return entity_->animations_[index]; }
    /// Return animation by name.
    Animation* GetAnimation(const String& name) const;
    /// Return root spatial info.
    const SpatialInfo& GetSpatialInfo() const { return spatialInfo_; }
    /// Return animation result timeline keys.
    const PODVector<BoneTimelineKey* >& GetBoneKeys() const { return boneKeys_; }
    const PODVector<SpriteTimelineKey* >& GetSpriteKeys() const { return spriteKeys_; }
    /// Return animation triggers.
    const HashMap<String, BoneTimelineKey* >& GetNodeTriggers() const { return nodeTriggers_; }
    const HashMap<Timeline*, SpriteTimelineKey* >& GetEventTriggers() const { return eventTriggers_; }
    const HashMap<Timeline*, BoxTimelineKey* >& GetPhysicTriggers() const { return physicTriggers_; }
    /// Return time passed on the current animation.
    float GetCurrentTime() const { return currentTime_; }

    void ResetCurrentTime();
    bool HasFinishedAnimation() const;
    bool GetLooping() const { return looping_; }

private:
    /// Handle set entity.
    void OnSetEntity(Entity* entity);
    /// Handle set animation.
    void OnSetAnimation(Animation* animation, LoopMode loopMode = Default);

    /// Update mainline keys.
    bool UpdateMainlineKeys(float deltaTime);
    /// Update timeline keys.
    void UpdateTimelineKeys();

    /// Get timeline key by ref.
    TimelineKey* GetTimelineKey(Ref* ref, float targetTime) const;
    /// Clear mainline key and timeline keys.
    void Clear();

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
    bool looping_, loopfinished_;
    /// Root spatial info.
    SpatialInfo spatialInfo_;
    /// Current time.
    float currentTime_, adjustedTime_;
//    float lastMainInstantTime_;

    /// Current mainline key.
    MainlineKey* prevmainlineKey_;
    MainlineKey* mainlineKey_;

    /// Current timeline keys.
    PODVector<BoneTimelineKey* > boneKeys_;
    PODVector<SpriteTimelineKey* > spriteKeys_;

    /// Current event keys.
    HashMap<String, BoneTimelineKey* > nodeTriggers_;
    HashMap<Timeline*, SpriteTimelineKey* > eventTriggers_;
    HashMap<Timeline*, BoxTimelineKey* > physicTriggers_;
};

}

}
