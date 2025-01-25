#pragma once

namespace Urho3D
{
    class Node;
    class Component;
    class UIElement;
}

#include "Pool.h"

URHO3D_EVENT(GO_STARTTIMER, Go_StartTimer)
{
    URHO3D_PARAM(GO_SENDER, GoSender);
    URHO3D_PARAM(GO_DATA1, GoData1);
    URHO3D_PARAM(GO_DATA2, GoData2);
}

URHO3D_EVENT(GO_ENDTIMER, Go_EndTimer)
{
    URHO3D_PARAM(GO_SENDER, GoSender);
    URHO3D_PARAM(GO_DATA1, GoData1);
    URHO3D_PARAM(GO_DATA2, GoData2);
}

enum RemoveState
{
    POOLRESTORE,
    FASTPOOLRESTORE,
    FREEMEMORY,
    DISABLE,
    ENABLE,
    NOREMOVESTATE,
    REMOVEANIMATION,
};

enum RemoveObjectType
{
    NODE,
    COMPONENT,
    UIELEMENT,
};

class TimerRemover : public Object
{
    URHO3D_OBJECT(TimerRemover, Object);

public :
    static void Reset(int size=0);
    static TimerRemover* Get();

    TimerRemover(Context* context);
    TimerRemover();
    TimerRemover(const TimerRemover& timer);
    ~TimerRemover();

    void SetSendEvents(Object* sender=0, const StringHash& eventType1=GO_STARTTIMER, const StringHash& eventType2=StringHash::ZERO);

    void Start(Node* object, float delay, RemoveState state=FREEMEMORY, float delayBeforeStart = 0.f, unsigned userdata1=0, unsigned userdata2=0);
    void Start(Component* object, float delay, RemoveState state=FREEMEMORY, float delayBeforeStart = 0.f, unsigned userdata1=0, unsigned userdata2=0);
    void Start(UIElement* object, float delay, RemoveState state=FREEMEMORY, float delayBeforeStart = 0.f, unsigned userdata1=0, unsigned userdata2=0);

    void Stop(VariantMap& eventData);

private :
    void HandleUpdate(StringHash eventType, VariantMap& eventData);

    RemoveObjectType objectType_;
    RemoveState removeState_;
    WeakPtr<Animatable> object_;
    WeakPtr<Object> sender_;
    float timer_;
    float startTime_;
	float expirationTime_;
	unsigned userData_[2];
    StringHash eventType_[2];
    bool startok_;

    static Pool<TimerRemover> pool_;
};
