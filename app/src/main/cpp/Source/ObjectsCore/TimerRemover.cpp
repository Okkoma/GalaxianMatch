#include <Urho3D/Urho3D.h>

#include <Urho3D/Core/Context.h>
#include <Urho3D/Core/CoreEvents.h>
#include <Urho3D/Scene/SceneEvents.h>

#include <Urho3D/Scene/Node.h>
#include <Urho3D/Scene/Component.h>
#include <Urho3D/UI/UIElement.h>

#include "GameStatics.h"

#include "ObjectPool.h"

#include "TimerRemover.h"


//StringHash HandleUpdateEvent = E_UPDATE;
StringHash HandleUpdateEvent = E_SCENEUPDATE;

const char* RemoveStateNames[] =
{
    "POOLRESTORE",
    "FASTPOOLRESTORE",
    "FREEMEMORY",
    "DISABLE",
    "ENABLE",
    "NOREMOVESTATE",
    "REMOVEANIMATION",
};



Pool<TimerRemover> TimerRemover::pool_;

void TimerRemover::Reset(int size)
{
    pool_.Resize(GameStatics::context_, size);
}

TimerRemover* TimerRemover::Get()
{
    return pool_.Get();
}


TimerRemover::TimerRemover(Context* context) : Object(context), startok_(0) { }

TimerRemover::TimerRemover() : Object(0) { }

TimerRemover::TimerRemover(const TimerRemover& timer) : Object(timer.GetContext()) { }

TimerRemover::~TimerRemover() { }

void TimerRemover::Start(Node* object, float delay, RemoveState state, float delayBeforeStart, unsigned userdata1, unsigned userdata2)
{
    objectType_ = NODE;
    removeState_ = state;
    object_ = WeakPtr<Animatable>(static_cast<Animatable*>(object));

    userData_[0] = userdata1;
    userData_[1] = userdata2;

    startok_ = (delayBeforeStart + delay == 0.f);

    if (startok_)
    {
        timer_ = 0.1f;
        Stop(context_->GetEventDataMap());
    }
    else
    {
        timer_ = 0.0f;
        startTime_ = delayBeforeStart;
        expirationTime_ = delay + startTime_;
        SubscribeToEvent(HandleUpdateEvent, URHO3D_HANDLER(TimerRemover, HandleUpdate));
    }
}

void TimerRemover::Start(Component* object, float delay, RemoveState state, float delayBeforeStart, unsigned userdata1, unsigned userdata2)
{
    objectType_ = COMPONENT;
    removeState_ = state;
    object_ = WeakPtr<Animatable>(static_cast<Animatable*>(object));

    timer_ = 0.0f;
    startTime_ = delayBeforeStart;
    expirationTime_ = delay + startTime_;
    userData_[0] = userdata1;
    userData_[1] = userdata2;

    startok_ = false;

    SubscribeToEvent(HandleUpdateEvent, URHO3D_HANDLER(TimerRemover, HandleUpdate));
}

void TimerRemover::Start(UIElement* object, float delay, RemoveState state, float delayBeforeStart, unsigned userdata1, unsigned userdata2)
{
    objectType_ = UIELEMENT;
    removeState_ = state;
    object_ = WeakPtr<Animatable>(static_cast<Animatable*>(object));

    timer_ = 0.0f;
    startTime_ = delayBeforeStart;
    expirationTime_ = delay + startTime_;
    userData_[0] = userdata1;
    userData_[1] = userdata2;

    startok_ = false;

    SubscribeToEvent(HandleUpdateEvent, URHO3D_HANDLER(TimerRemover, HandleUpdate));
}

void TimerRemover::SetSendEvents(Object* sender, const StringHash& eventType1, const StringHash& eventType2)
{
    if (sender)
    {
        sender_ = sender;
        eventType_[0] = eventType1;
        eventType_[1] = eventType2;
    }
}

void TimerRemover::HandleUpdate(StringHash eventType, VariantMap& eventData)
{
    timer_ += eventData[Update::P_TIMESTEP].GetFloat();

    if (!startok_)
        if (timer_ > startTime_)
        {
            if (sender_ && eventType_[0])
            {
                eventData[Go_StartTimer::GO_SENDER] = (void*)object_.Get(); // use (void ptr) for variant assignation to skip weakptr creation, we just need a number to keep track
                eventData[Go_StartTimer::GO_DATA1] = userData_[0];
                eventData[Go_StartTimer::GO_DATA2] = userData_[1];
//                URHO3D_LOGINFOF("TimerRemover() - HandleUpdate : this=%u SendEvent1=%u ...", this, eventType_[0].Value());
                sender_->SendEvent(eventType_[0], eventData);
                eventType_[0] = 0;
            }

            // Set Enable
            if (object_)
            {
                if (objectType_ == NODE)
                    static_cast<Node*>(object_.Get())->SetEnabled(true);
                else if (objectType_ == COMPONENT)
                    static_cast<Component*>(object_.Get())->SetEnabled(true);
                else
                    static_cast<UIElement*>(object_.Get())->SetEnabled(true);
            }
            startok_ = true;
        }

    if (timer_ > expirationTime_)
    {
//        URHO3D_LOGINFOF("TimerRemover() - HandleUpdate : Stop this=%u ...", this);
        Stop(eventData);
    }
}

void TimerRemover::Stop(VariantMap& eventData)
{
    UnsubscribeFromEvent(HandleUpdateEvent);

    if (!object_)
    {
        pool_.Free(this);
        return;
    }

    if (sender_ && eventType_[1])
    {
        eventData[Go_EndTimer::GO_SENDER] = (void*)sender_.Get(); // use (void ptr) for variant assignation to skip weakptr creation, we just need a number to keep track
        eventData[Go_EndTimer::GO_DATA1] = userData_[0];
        eventData[Go_EndTimer::GO_DATA2] = userData_[1];
        sender_->SendEvent(eventType_[1], eventData);
        eventType_[1] = 0;
        sender_.Reset();
    }

    if (objectType_ == NODE)
    {
        Node* node = static_cast<Node*>(object_.Get());
        if (removeState_ == POOLRESTORE)
            ObjectPool::Free(node);
        else if (removeState_ == FREEMEMORY)
            node->Remove();
        else if (removeState_ == ENABLE)
            node->SetEnabled(true);
        else if (removeState_ == DISABLE)
            node->SetEnabled(false);
        else if (removeState_ == REMOVEANIMATION)
        {
            node->SetAnimationTime(0.f);
            node->RemoveObjectAnimation();
        }
    }
    else if (objectType_ == COMPONENT)
    {
        Component* component = static_cast<Component*>(object_.Get());
//        URHO3D_LOGINFOF("TimerRemover() - Stop() : this=%u sender=%u componentptr=%u removestate=%s(%d)", this, sender_, node->GetID(), RemoveStateNames[removeState_], removeState_);
        if (removeState_ == ENABLE)
            component->SetEnabled(true);
        else if (removeState_ == DISABLE)
            component->SetEnabled(false);
    }
    else
    {
        UIElement* uielt = static_cast<UIElement*>(object_.Get());
//        URHO3D_LOGINFOF("TimerRemover() - Stop() : this=%u sender=%u uielt=%u removestate=%s(%d)...", this, sender_, uielt, RemoveStateNames[removeState_], removeState_);
        if (removeState_ == FREEMEMORY)
            uielt->Remove();
        else if (removeState_ == ENABLE)
        {
            uielt->SetEnabled(true);
            uielt->SetVisible(true);
        }
        else if (removeState_ == DISABLE)
        {
            uielt->SetEnabled(false);
            uielt->SetVisible(false);
        }
    }

//    URHO3D_LOGINFOF("TimerRemover() - Stop() : this=%u ... OK !", this);
    object_.Reset();
    pool_.Free(this);
}


