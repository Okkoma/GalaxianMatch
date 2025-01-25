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
#include "../Graphics/DrawableEvents.h"
#include "../Scene/Component.h"
#include "../Scene/Node.h"
#include "../Urho2D/SpriterInstance2D.h"

#include <cmath>

namespace Urho3D
{

namespace Spriter
{

SpriterInstance::SpriterInstance(Component* owner, SpriterData* spriteData) :
    owner_(owner),
    spriterData_(spriteData),
    entity_(0),
    animation_(0),
    loopfinished_(false)
{
}

SpriterInstance::~SpriterInstance()
{
    Clear();

    OnSetAnimation(0);
    OnSetEntity(0);
}

bool SpriterInstance::SetEntity(int index)
{
    if (!spriterData_)
        return false;

    if (index < (int)spriterData_->entities_.Size())
    {
        OnSetEntity(spriterData_->entities_[index]);
        return true;
    }

    return false;
}

bool SpriterInstance::SetEntity(const String& entityName)
{
    if (!spriterData_)
        return false;

    for (unsigned i = 0; i < spriterData_->entities_.Size(); ++i)
    {
        if (spriterData_->entities_[i]->name_ == entityName)
        {
//            URHO3D_LOGINFOF("SpriterInstance() - SetEntity : entity = %s", entityName.CString());

            OnSetEntity(spriterData_->entities_[i]);
            return true;
        }
    }

    return false;
}

bool SpriterInstance::SetAnimation(int index, LoopMode loopMode)
{
    if (!entity_)
        return false;

    // if animation is already set skip
    if (animation_ && index == animation_->id_)
        return false;

    if (index < (int)entity_->animations_.Size())
    {
        OnSetAnimation(entity_->animations_[index], loopMode);
        return true;
    }

    return false;
}

bool SpriterInstance::SetAnimation(const String& animationName, LoopMode loopMode)
{
    if (!entity_)
        return false;

    Animation* animation = GetAnimation(animationName);

    if (animation && animation != animation_)
    {
        OnSetAnimation(animation, loopMode);
        return true;
    }

    return false;
}

Animation* SpriterInstance::GetAnimation(const String& name) const
{
    if (!entity_)
        return 0;

    for (unsigned i = 0; i < entity_->animations_.Size(); ++i)
    {
        if (entity_->animations_[i]->name_ == name)
            return entity_->animations_[i];
    }

    return 0;
}

void SpriterInstance::SetSpatialInfo(const SpatialInfo& spatialInfo)
{
    this->spatialInfo_ = spatialInfo;
}

void SpriterInstance::SetSpatialInfo(float x, float y, float angle, float scaleX, float scaleY)
{
    spatialInfo_ = SpatialInfo(x, y, angle, scaleX, scaleY);
}

bool SpriterInstance::HasFinishedAnimation() const
{
//    return ((currentTime_ == animation_->length_) && !looping_);
    return loopfinished_;
}

void SpriterInstance::SetTime(float time)
{
    currentTime_ = time;

    Update(0.f);
}

void SpriterInstance::OnSetEntity(Entity* entity)
{
    if (entity == this->entity_)
        return;

    OnSetAnimation(0);

    this->entity_ = entity;
}

void SpriterInstance::OnSetAnimation(Animation* animation, LoopMode loopMode)
{
    if (animation == this->animation_)
        return;

    animation_ = animation;
    if (animation_)
    {
        if (loopMode == Default)
            looping_ = animation_->looping_;
        else if (loopMode == ForceLooped)
            looping_ = true;
        else
            looping_ = false;
    }

    currentTime_ = 0.f;
    mainlineKey_ = 0;
    loopfinished_ = false;

    Clear();
}

bool SpriterInstance::Update(float deltaTime)
{
    if (!animation_)
        return false;

    if (!looping_ && HasFinishedAnimation())
        return false;

    if (!deltaTime)
        currentTime_ = 0.f;

    if (!UpdateMainlineKeys(deltaTime))
        return false;

    Clear();

    UpdateTimelineKeys();

    return true;
}

void SpriterInstance::ResetCurrentTime()
{
    if (!animation_)
        return;

    currentTime_ = 0.f;
    mainlineKey_ = 0;
    loopfinished_ = false;

    Clear();
}

bool SpriterInstance::UpdateMainlineKeys(float deltaTime)
{
    currentTime_ += deltaTime;

    if (currentTime_ > animation_->length_)
    {
        if (looping_)
            currentTime_ = fmod(currentTime_, animation_->length_);
        else
            currentTime_ = animation_->length_;

        loopfinished_ = true;
    }
    else if (looping_ && loopfinished_)
        loopfinished_= false;

    const PODVector<MainlineKey*>& mainlineKeys = animation_->mainlineKeys_;

    prevmainlineKey_ = mainlineKey_;

    mainlineKey_ = 0;
    for (unsigned i = 0; i < mainlineKeys.Size(); ++i)
    {
        if (mainlineKeys[i]->time_ > currentTime_)
            break;

        mainlineKey_ = mainlineKeys[i];
    }

    if (!mainlineKey_)
        mainlineKey_ = mainlineKeys.Back();

    if (mainlineKeys.Size() > 1)
    {
        if (mainlineKey_->curveType_ == INSTANT)
        {
            if (mainlineKey_ == prevmainlineKey_)
                return false;
        }

        if (mainlineKey_ == mainlineKeys.Back())
            adjustedTime_ = mainlineKey_->AdjustTime(mainlineKey_->time_, mainlineKeys[0]->time_, animation_->length_, currentTime_);
        else
            adjustedTime_ = mainlineKey_->AdjustTime(mainlineKey_->time_, mainlineKeys[mainlineKey_->id_ + 1]->time_, animation_->length_, currentTime_);
    }
    else
        adjustedTime_ = 0.f;

    return true;
}

void SpriterInstance::UpdateTimelineKeys()
{
    for (unsigned i = 0; i < mainlineKey_->boneRefs_.Size(); ++i)
    {
        Ref* ref = mainlineKey_->boneRefs_[i];
        Timeline* timeline = animation_->timelines_[ref->timeline_];

        BoneTimelineKey* tKey = (BoneTimelineKey*) GetTimelineKey(ref, adjustedTime_);

        tKey->info_ = ref->parent_ >= 0 ? tKey->info_.UnmapFromParent(boneKeys_[ref->parent_]->info_) : tKey->info_.UnmapFromParent(spatialInfo_);

        boneKeys_.Push(tKey);

        if (timeline->name_.StartsWith("TrigNode"))
            nodeTriggers_[timeline->name_.Substring(9)] = tKey;
    }

    for (unsigned i = 0; i < mainlineKey_->objectRefs_.Size(); ++i)
    {
        Ref* ref = mainlineKey_->objectRefs_[i];
        Timeline* timeline = animation_->timelines_[ref->timeline_];

        if (timeline->objectType_ == Spriter::BOX)
        {
            BoxTimelineKey* tKey = (BoxTimelineKey*) GetTimelineKey(ref, adjustedTime_);
            tKey->info_ = ref->parent_ >= 0 ? tKey->info_.UnmapFromParent(boneKeys_[ref->parent_]->info_) : tKey->info_.UnmapFromParent(spatialInfo_);

            physicTriggers_[timeline] = tKey;
            continue;
        }

        SpriteTimelineKey* tKey = (SpriteTimelineKey*) GetTimelineKey(ref, adjustedTime_);
        tKey->info_ = ref->parent_ >= 0 ? tKey->info_.UnmapFromParent(boneKeys_[ref->parent_]->info_) : tKey->info_.UnmapFromParent(spatialInfo_);
        tKey->zIndex_ = ref->zIndex_;

        if (timeline->objectType_ == Spriter::SPRITE)
            spriteKeys_.Push(tKey);

        else if (timeline->objectType_ == Spriter::POINT)
            eventTriggers_[timeline] = tKey;
    }
}

TimelineKey* SpriterInstance::GetTimelineKey(Ref* ref, float targetTime) const
{
    Timeline* timeline = animation_->timelines_[ref->timeline_];
    TimelineKey* timelineKey = timeline->keys_[ref->key_]->Clone();

    if (mainlineKey_->curveType_ == INSTANT)
        return timelineKey;

    unsigned nextTimelineKeyIndex = ref->key_ + 1;

    if (nextTimelineKeyIndex >= timeline->keys_.Size())
    {
        if (animation_->looping_)
            nextTimelineKeyIndex = 0;
        else
            return timelineKey;
    }

    TimelineKey* nextTimelineKey = timeline->keys_[nextTimelineKeyIndex];
//    float time = lastMainInstantTime_ > targetTime ? lastMainInstantTime_ : targetTime;
    timelineKey->Interpolate(*nextTimelineKey, timelineKey->GetFactor(timelineKey->time_, nextTimelineKey->time_, animation_->length_, targetTime));

    return timelineKey;
}

void SpriterInstance::Clear()
{
//    URHO3D_LOGINFOF("SpriterInstance() - Clear : ...");

    if (!boneKeys_.Empty())
    {
        for (unsigned i = 0; i < boneKeys_.Size(); ++i)
        {
        #ifdef USE_KEYPOOLS
            BoneTimelineKey::Free(boneKeys_[i]);
        #else
            delete boneKeys_[i];
        #endif
        }
        boneKeys_.Clear();
    }
    if (!spriteKeys_.Empty())
    {
        for (unsigned i = 0; i < spriteKeys_.Size(); ++i)
        {
        #ifdef USE_KEYPOOLS
            SpriteTimelineKey::Free(spriteKeys_[i]);
        #else
            delete spriteKeys_[i];
        #endif
        }
        spriteKeys_.Clear();
    }

    nodeTriggers_.Clear();

    if (!eventTriggers_.Empty())
    {
        for (HashMap<Timeline*, SpriteTimelineKey* >::ConstIterator it=eventTriggers_.Begin();it!=eventTriggers_.End(); ++it)
        {
        #ifdef USE_KEYPOOLS
            SpriteTimelineKey::Free(it->second_);
        #else
            delete it->second_;
        #endif
        }
        eventTriggers_.Clear();
    }
    if (!physicTriggers_.Empty())
    {
        for (HashMap<Timeline*, BoxTimelineKey* >::ConstIterator it=physicTriggers_.Begin();it!=physicTriggers_.End(); ++it)
        {
        #ifdef USE_KEYPOOLS
            BoxTimelineKey::Free(it->second_);
        #else
            delete it->second_;
        #endif
        }
        physicTriggers_.Clear();
    }

//    URHO3D_LOGINFOF("SpriterInstance() - Clear : ... OK !");

}

}

}
