#include <Urho3D/Urho3D.h>

#include <Urho3D/IO/Log.h>

#include <Urho3D/Graphics/Camera.h>
#include <Urho3D/Urho2D/AnimatedSprite2D.h>

#include <Urho3D/UI/UI.h>
#include <Urho3D/UI/UIEvents.h>

#include "GameEvents.h"
#include "GameHelpers.h"
#include "GameStatics.h"
#include "AnimatedSprite.h"
#include "DelayInformer.h"

#include "DelayAction.h"


Pool<DelayAction> DelayAction::pool_;

/// Delayed Action Class

void DelayAction::Reset(int size)
{
    pool_.Resize(GameStatics::context_, size);
}

DelayAction* DelayAction::Get()
{
    DelayAction* action = pool_.Get();
    action->inpool_ = false;
    return action;
}

DelayAction* DelayAction::Get(const String& soundname, float delay)
{
    return Get()->Set(soundname, delay);
}

DelayAction* DelayAction::Get(StringHash eventType, StringHash eventTypeStop, float delay, const String& animationfilename, const String& entityname,
                              Animatable* parent, const Vector2& position, float angle)
{
    return Get()->Set(eventType, eventTypeStop, delay, animationfilename, entityname, parent, position, angle);
}

DelayAction* DelayAction::Get(StringHash eventType, StringHash eventTypeStop, float delay, int actionType, const String& animationfilename, const String& entityname,
                              const String& parentname, const Vector2& position, float angle)
{
    return Get()->Set(eventType, eventTypeStop, delay, actionType, animationfilename, entityname, parentname, position, angle);
}

DelayAction::DelayAction(Context* context) : Object(context), inpool_(true), running_(false)
{ }

DelayAction::~DelayAction()
{ }

DelayAction* DelayAction::Set(const String& soundname, float delay)
{
    actionType_ = PLAY_SOUND;
    eventType_ = eventTypeStop_ = StringHash::ZERO;
    delay_ = delay;
    name_ = soundname;

    Start();

    return this;
}

DelayAction* DelayAction::Set(StringHash eventType, StringHash eventTypeStop, float delay, const String& animationfilename, const String& entityname,
                              Animatable* parent, const Vector2& position, float angle)
{
    actionType_ = dynamic_cast<Node*>(parent) ? PLAY_ANIMATION : PLAY_UI;
    eventType_ = eventType;
    eventTypeStop_ = eventTypeStop;
    delay_ = delay;
    name_ = entityname;
    parent_ = parent;
    running_ = false;

    if (actionType_ == PLAY_ANIMATION)
    {
        AnimatedSprite2D* animatedSprite = GameHelpers::AddAnimatedSprite2D(static_cast<Node*>(parent_.Get()), name_, animationfilename, name_, position, angle);
        animatedSprite->GetNode()->SetEnabled(false);
        object_ = SharedPtr<Animatable>(animatedSprite);
    }
    else
    {
        AnimatedSprite* animatedSprite = GameHelpers::AddAnimatedSpriteUI(static_cast<UIElement*>(parent_.Get()), name_, animationfilename, name_, position, angle);
        animatedSprite->SetVisible(false);
        object_ = SharedPtr<Animatable>(animatedSprite);
    }

    return this;
}

DelayAction* DelayAction::Set(StringHash eventType, StringHash eventTypeStop, float delay, int actionType, const String& animationfilename, const String& entityname,
                              const String& parentname, const Vector2& position, float angle)
{
    actionType_ = actionType;
    eventType_ = eventType;
    eventTypeStop_ = eventTypeStop;
    delay_ = delay;

    name_ = entityname;
    params_.animationfilename_ = animationfilename;
    params_.parentname_ = parentname;
    params_.position_ = position;
    params_.angle_ = angle;

    running_ = false;

    return this;

}

bool DelayAction::IsRunning() const
{
    return running_ && eventTypeStop_ && eventTypeStop_ != MESSAGE_STOP;
}

void DelayAction::Free()
{
    if (running_)
        Stop();

    if (!inpool_)
    {
        URHO3D_LOGINFOF("DelayAction() - Free : name=%s ...", name_.CString());
        inpool_ = true;
        pool_.Free(this);
    }
}

void DelayAction::Start()
{
    URHO3D_LOGINFOF("DelayAction() - Start : name=%s eventType=%u eventTypeStop=%u...", name_.CString(), eventType_.Value(), eventTypeStop_.Value());

    running_ = true;

    if (!parent_ && !params_.parentname_.Empty())
        parent_ = GameStatics::rootScene_->GetChild(params_.parentname_, true);

    if (eventType_)
        SubscribeToEvent(eventType_, URHO3D_HANDLER(DelayAction, HandleEvent));
    else
        HandleEvent(StringHash::ZERO, context_->GetEventDataMap());

    if (eventTypeStop_)
    {
        if (parent_)
            SubscribeToEvent(parent_, eventTypeStop_, URHO3D_HANDLER(DelayAction, HandleStop));
        else
            SubscribeToEvent(eventTypeStop_, URHO3D_HANDLER(DelayAction, HandleStop));
    }
}

void DelayAction::Stop()
{
    URHO3D_LOGINFOF("DelayAction() - Stop : name=%s eventType=%u eventTypeStop=%u !", name_.CString(), eventType_.Value(), eventTypeStop_.Value());

    running_ = false;

    UnsubscribeFromAllEvents();

    if (object_)
    {
        if (actionType_ == PLAY_ANIMATION)
        {
            AnimatedSprite2D* animatedsprite = static_cast<AnimatedSprite2D*>(object_.Get());
            if (animatedsprite && animatedsprite->GetNode())
                animatedsprite->GetNode()->Remove();
        }
        else
            static_cast<UIElement*>(object_.Get())->Remove();
    }

    object_.Reset();
}

void DelayAction::ExecuteAction()
{
    switch (actionType_)
    {
    case PLAY_SOUND:
        GameHelpers::SpawnSound(GameStatics::rootScene_, name_.CString());
        break;

    case PLAY_ANIMATION:
    {
        if (!object_)
        {
            if (!parent_ && !params_.parentname_.Empty())
                parent_ = GameStatics::rootScene_->GetChild(params_.parentname_, true);
            if (parent_)
            {
                AnimatedSprite2D* animatedSprite = GameHelpers::AddAnimatedSprite2D(static_cast<Node*>(parent_.Get()), name_, params_.animationfilename_, name_, params_.position_, params_.angle_);
                object_ = SharedPtr<Animatable>(animatedSprite);
            }
        }
        if (object_)
        {
            AnimatedSprite2D* animatedSprite = static_cast<AnimatedSprite2D*>(object_.Get());
            if (animatedSprite && animatedSprite->GetNode())
            {
                animatedSprite->SetLayer(100000);
                animatedSprite->GetNode()->SetEnabled(true);
                animatedSprite->GetNode()->SetWorldScale2D(Vector2(0.5f, 0.5f) * GameStatics::uiScale_ / GameStatics::camera_->GetZoom());
            }
        }
    }
        break;

    case PLAY_UI:
    {
        if (!object_)
        {
            if (!parent_ && !params_.parentname_.Empty())
                parent_ = GameStatics::ui_->GetRoot()->GetChild(params_.parentname_, true);
            if (parent_)
            {
                AnimatedSprite* animatedSprite = GameHelpers::AddAnimatedSpriteUI(static_cast<UIElement*>(parent_.Get()), name_, params_.animationfilename_, name_, params_.position_, params_.angle_);
                object_ = SharedPtr<Animatable>(animatedSprite);
            }
        }
        if (object_)
        {
            AnimatedSprite* animatedSprite = static_cast<AnimatedSprite*>(object_.Get());
            if (animatedSprite)
            {
                animatedSprite->SetVisible(true);
                animatedSprite->SetAlignment(HA_LEFT, VA_TOP);
                animatedSprite->SetScale2D(0.5f);

                if (!animatedSprite->GetParent())
                    animatedSprite->SwitchIndependentUpdate();

//                animatedSprite->SetFollowedElement(GameStatics::cursor_);

                URHO3D_LOGINFOF("DelayAction() - ExecuteAction : PLAY_UI animatedSprite=%s pos=%s visible=%s", animatedSprite->GetName().CString(), animatedSprite->GetPosition().ToString().CString(),
                                animatedSprite->IsVisibleEffective() ? "true":"false");
            }
        }
    }
        break;

    case CLICK_UI:
        break;
    }
}

void DelayAction::HandleEvent(StringHash eventType, VariantMap& eventData)
{
    UnsubscribeFromEvent(eventType_);

    if (delay_)
    {
        DelayInformer::Get(this, delay_, DELAYEDACTION_EXECUTE);
        SubscribeToEvent(this, DELAYEDACTION_EXECUTE, URHO3D_HANDLER(DelayAction, HandleDelayedAction));
    }
    else
        HandleDelayedAction(StringHash::ZERO, context_->GetEventDataMap());
}

void DelayAction::HandleDelayedAction(StringHash eventType, VariantMap& eventData)
{
    UnsubscribeFromEvent(this, DELAYEDACTION_EXECUTE);

    ExecuteAction();
}

void DelayAction::HandleStop(StringHash eventType, VariantMap& eventData)
{
    URHO3D_LOGINFOF("DelayAction() - HandleStop : name=%s ...", name_.CString());
    Stop();
}
