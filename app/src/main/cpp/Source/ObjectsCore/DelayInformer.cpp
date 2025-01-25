#include <Urho3D/Urho3D.h>

#include <Urho3D/Core/Context.h>
#include <Urho3D/Core/CoreEvents.h>

#include "GameStatics.h"

#include "DelayInformer.h"


Pool<DelayInformer> DelayInformer::pool_;

void DelayInformer::Reset(int size)
{
    pool_.Resize(GameStatics::context_, size);
}

DelayInformer* DelayInformer::Get()
{
    return pool_.Get();
}

DelayInformer* DelayInformer::Get(Object* object, float delay, StringHash eventType)
{
    return Get()->Set(object, delay, eventType);
}

DelayInformer* DelayInformer::Get(Object* object, float delay, StringHash eventType, const VariantMap& eventData)
{
    return Get()->Set(object, delay, eventType, eventData);
}


DelayInformer::DelayInformer(Context* context) : Object(context) { }

DelayInformer::DelayInformer() : Object(0) { }

DelayInformer::DelayInformer(const DelayInformer& timer) : Object(timer.GetContext()) { }

DelayInformer::~DelayInformer() { }

DelayInformer* DelayInformer::Set(Object* object, float delay, StringHash eventType)
{
    object_ = object;
    timer_ = 0.0f;
    expirationTime_ = delay;
    eventType_ = eventType;
    eventData_.Clear();

    SubscribeToEvent(E_UPDATE, URHO3D_HANDLER(DelayInformer, HandleUpdate));
    return this;
}

DelayInformer* DelayInformer::Set(Object* object, float delay, StringHash eventType, const VariantMap& eventData)
{
    object_ = object;
    timer_ = 0.0f;
    expirationTime_ = delay;
    eventType_ = eventType;
    eventData_ = eventData;

    SubscribeToEvent(E_UPDATE, URHO3D_HANDLER(DelayInformer, HandleUpdate));
    return this;
}

bool DelayInformer::Expired() const
{
    return timer_ > expirationTime_;
}

void DelayInformer::Free()
{
    object_.Reset();
    UnsubscribeFromEvent(E_UPDATE);

    pool_.Free(this);
}

void DelayInformer::HandleUpdate(StringHash eventType, VariantMap& eventData)
{
    timer_ += eventData[Update::P_TIMESTEP].GetFloat();

    if (timer_ > expirationTime_)
    {
        UnsubscribeFromEvent(E_UPDATE);

        if (object_)
        {
            if (eventData_.Size())
                object_->SendEvent(eventType_, eventData_);
            else
                object_->SendEvent(eventType_);
        }

        Free();
    }
}

