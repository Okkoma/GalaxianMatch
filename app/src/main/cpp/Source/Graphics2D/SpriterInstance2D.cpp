#include <Urho3D/Urho3D.h>

#include <Urho3D/IO/Log.h>

#include <Urho3D/Graphics/DrawableEvents.h>

#include <Urho3D/Scene/Component.h>
#include <Urho3D/Scene/Node.h>

#include <cmath>

#include "Sprite2D.h"

#include "SpriterInstance2D.h"

extern const char* loopModeNames[];


namespace Spriter
{

SpriterInstance::SpriterInstance(Component* owner, SpriterData* spriteData) :
    owner_(owner),
    spriterData_(spriteData),
    entity_(nullptr),
    animation_(nullptr),
    looping_(false)
{
}

SpriterInstance::~SpriterInstance()
{
    Dispose();

    OnSetAnimation(nullptr);
    OnSetEntity(nullptr);
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
    if (animation)
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
    if (!animation_)
        return true;

    return looping_ ? false : currentTime_ == animation_->length_;
}

bool SpriterInstance::GetLooping() const
{ 
    return looping_;
}

void SpriterInstance::OnSetEntity(Entity* entity)
{
    if (entity == this->entity_)
        return;

    OnSetAnimation(nullptr);

    this->entity_ = entity;
}

void SpriterInstance::OnSetAnimation(Animation* animation, LoopMode loopMode)
{
    if (loopMode == Default)
        looping_ = animation ? animation->looping_ : false;
    else if (loopMode == ForceLooped)
        looping_ = true;
    else
        looping_ = false;

    if (animation != this->animation_)
    {
        animation_ = animation;
        currentTime_ = 0.0f;
        mainlineKey_ = nullptr;

        RestoreKeys();
    }
}

bool SpriterInstance::Update(float deltaTime)
{
    if (!animation_)
        return false;

    if (!looping_ && HasFinishedAnimation())
        return false;

    if (!UpdateMainlineKeys(deltaTime))
        return false;

    RestoreKeys();

    UpdateTimelineKeys();

    return true;
}

void SpriterInstance::SetCurrentTime(float time)
{
    currentTime_ = time;
    Update(0.f);
}

void SpriterInstance::ResetCurrentTime()
{
    if (!animation_)
        return;

    currentTime_ = 0.f;
    mainlineKey_ = nullptr;

    RestoreKeys();
}

bool SpriterInstance::UpdateMainlineKeys(float deltaTime)
{
    currentTime_ += deltaTime;

    if (currentTime_ > animation_->length_)
        currentTime_ = looping_ ? fmod(currentTime_, animation_->length_) : animation_->length_;

    const PODVector<MainlineKey*>& mainlineKeys = animation_->mainlineKeys_;

    prevmainlineKey_ = mainlineKey_;

    mainlineKey_ = nullptr;
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
    numBoneKeys_ = mainlineKey_->boneRefs_.Size();
    for (unsigned i = 0; i < mainlineKey_->boneRefs_.Size(); ++i)
    {
        Ref* ref = mainlineKey_->boneRefs_[i];
        Timeline* timeline = animation_->timelines_[ref->timeline_];

        if (i < boneKeys_.Size())
        {
            // reuse key
            BoneTimelineKey*& tKey = boneKeys_[i];
            tKey = (BoneTimelineKey*) GetTimelineKey(timeline, ref, adjustedTime_, tKey);
            tKey->info_.UnmapFromParent(ref->parent_ >= 0 ? boneKeys_[ref->parent_]->info_ : spatialInfo_);
        }
        else
        {
            BoneTimelineKey* tKey = (BoneTimelineKey*) GetTimelineKey(timeline, ref, adjustedTime_, 0);
            tKey->info_.UnmapFromParent(ref->parent_ >= 0 ? boneKeys_[ref->parent_]->info_ : spatialInfo_);
            boneKeys_.Push(tKey);
        }
    }

    numSpriteKeys_ = 0;
    for (unsigned i = 0; i < mainlineKey_->objectRefs_.Size(); ++i)
    {
        Ref* ref = mainlineKey_->objectRefs_[i];
        Timeline* timeline = animation_->timelines_[ref->timeline_];

        if (timeline->objectType_ == Spriter::BOX)
        {
            BoxTimelineKey*& tKey = physicTriggers_[timeline];
            tKey = (BoxTimelineKey*) GetTimelineKey(timeline, ref, adjustedTime_, tKey);
            tKey->info_.UnmapFromParent(ref->parent_ >= 0 ? boneKeys_[ref->parent_]->info_ : spatialInfo_);
        }
        else if (timeline->objectType_ == Spriter::SPRITE)
        {
            if (numSpriteKeys_ < spriteKeys_.Size())
            {
                // reuse key
                SpriteTimelineKey*& tKey = spriteKeys_[numSpriteKeys_];
                tKey = (SpriteTimelineKey*) GetTimelineKey(timeline, ref, adjustedTime_, tKey);
                tKey->info_.UnmapFromParent(ref->parent_ >= 0 ? boneKeys_[ref->parent_]->info_ : spatialInfo_);
                tKey->zIndex_ = ref->zIndex_;
                tKey->color_ = ref->color_;
            }
            else
            {
                SpriteTimelineKey* tKey = (SpriteTimelineKey*) GetTimelineKey(timeline, ref, adjustedTime_, 0);
                tKey->info_.UnmapFromParent(ref->parent_ >= 0 ? boneKeys_[ref->parent_]->info_ : spatialInfo_);
                tKey->zIndex_ = ref->zIndex_;
                tKey->color_ = ref->color_;
                spriteKeys_.Push(tKey);
            }
            numSpriteKeys_++;
        }
        else if (timeline->objectType_ == Spriter::POINT)
        {
            if (timeline->name_.StartsWith("IN"))
            {
                String name = timeline->name_.Substring(3);
                bool resetcomponent = !nodeUpdaters_.Contains(name);
                NodeUpdater& updater = nodeUpdaters_[name];

                PointTimelineKey*& tKey = updater.timekey_;
                tKey = (PointTimelineKey*) GetTimelineKey(timeline, ref, adjustedTime_, tKey);
                tKey->info_.UnmapFromParent(ref->parent_ >= 0 ? boneKeys_[ref->parent_]->info_ : spatialInfo_);
                tKey->zIndex_ = ref->zIndex_;

                if (resetcomponent)
                    updater.ucomponent_ = nullptr;
            }
            else
            {
                PointTimelineKey*& tKey = eventTriggers_[timeline];
                tKey = (PointTimelineKey*) GetTimelineKey(timeline, ref, adjustedTime_, tKey);
                tKey->info_.UnmapFromParent(ref->parent_ >= 0 ? boneKeys_[ref->parent_]->info_ : spatialInfo_);
                tKey->zIndex_ = ref->zIndex_;
            }
        }
    }
}

TimelineKey* SpriterInstance::GetTimelineKey(Timeline* timeline, Ref* ref, float targetTime, TimelineKey* entry) const
{
    TimelineKey* timelineKey;
    if (entry) // reuse key
    {     
        timeline->keys_[ref->key_]->Copy(entry);
        timelineKey = entry;
    }
    else // clone key
        timelineKey = timeline->keys_[ref->key_]->Clone();

    // keep ref to timeline
    timelineKey->timeline_ = timeline;

    if (mainlineKey_->curveType_ == INSTANT)
        return timelineKey;

    unsigned nextTimelineKeyIndex = ref->key_ + 1;

    if (nextTimelineKeyIndex >= timeline->keys_.Size())
    {
        if (looping_)
            nextTimelineKeyIndex = 0;
        else
            return timelineKey;
    }

    const TimelineKey& nextTimelineKey = *timeline->keys_[nextTimelineKeyIndex];
    timelineKey->Interpolate(nextTimelineKey, timelineKey->GetFactor(timelineKey->time_, nextTimelineKey.time_, animation_->length_, targetTime));

    return timelineKey;
}

void SpriterInstance::RestoreKeys()
{
    if (!eventTriggers_.Empty())
    {
        for (HashMap<Timeline*, PointTimelineKey* >::ConstIterator it=eventTriggers_.Begin();it!=eventTriggers_.End(); ++it)
            delete it->second_;
        eventTriggers_.Clear();
    }
    if (!physicTriggers_.Empty())
    {
        for (HashMap<Timeline*, BoxTimelineKey* >::ConstIterator it=physicTriggers_.Begin();it!=physicTriggers_.End(); ++it)
            delete it->second_;
        physicTriggers_.Clear();
    }
}

void SpriterInstance::Dispose()
{
    if (!boneKeys_.Empty())
    {
        for (unsigned i = 0; i < boneKeys_.Size(); ++i)
            delete boneKeys_[i];
        boneKeys_.Clear();
    }
    if (!spriteKeys_.Empty())
    {
        for (unsigned i = 0; i < spriteKeys_.Size(); ++i)
            delete spriteKeys_[i];
        spriteKeys_.Clear();
    }
    if (!eventTriggers_.Empty())
    {
        for (HashMap<Timeline*, PointTimelineKey* >::ConstIterator it=eventTriggers_.Begin();it!=eventTriggers_.End(); ++it)
            delete it->second_;
        eventTriggers_.Clear();
    }
    if (!physicTriggers_.Empty())
    {
        for (HashMap<Timeline*, BoxTimelineKey* >::ConstIterator it=physicTriggers_.Begin();it!=physicTriggers_.End(); ++it)
            delete it->second_;
        physicTriggers_.Clear();
    }
    if (!nodeUpdaters_.Empty())
    {
        for (HashMap<String, NodeUpdater >::ConstIterator it=nodeUpdaters_.Begin();it!=nodeUpdaters_.End(); ++it)
            delete it->second_.timekey_;
        nodeUpdaters_.Clear();
    }
}

}
