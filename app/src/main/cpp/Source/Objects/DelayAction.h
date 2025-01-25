#pragma once

#include "Pool.h"

namespace Urho3D
{
    class Animatable;
}

using namespace Urho3D;

enum
{
    PLAY_SOUND = 1,
    PLAY_ANIMATION = 2,
    PLAY_UI = 3,
    CLICK_UI = 4,
};

struct AnimatableParameters
{
    String animationfilename_;
    String parentname_;
    Vector2 position_;
    float angle_;
};

class DelayAction : public Object
{
    URHO3D_OBJECT(DelayAction, Object);

public :
    static void Reset(int size=0);
    static DelayAction* Get();
    static DelayAction* Get(const String& soundname, float delay);
    static DelayAction* Get(StringHash eventType, StringHash eventTypeStop, float delay, const String& animationfilename, const String& entityname, Animatable* parent, const Vector2& position=Vector2::ZERO, float angle=0.f);
    static DelayAction* Get(StringHash eventType, StringHash eventTypeStop, float delay, int actionType, const String& animationfilename, const String& entityname, const String& parentname, const Vector2& position=Vector2::ZERO, float angle=0.f);

    DelayAction(Context* context);
    ~DelayAction();

    DelayAction* Set(const String& soundname, float delay);
    DelayAction* Set(StringHash eventType, StringHash eventTypeStop, float delay, const String& animationfilename, const String& entityname, Animatable* parent, const Vector2& position=Vector2::ZERO, float angle=0.f);
    DelayAction* Set(StringHash eventType, StringHash eventTypeStop, float delay, int actionType, const String& animationfilename, const String& entityname, const String& parentname, const Vector2& position=Vector2::ZERO, float angle=0.f);

    bool IsRunning() const;

    void Start();
    void Stop();

    void Free();

private :
    void ExecuteAction();

    void HandleEvent(StringHash eventType, VariantMap& eventData);
    void HandleDelayedAction(StringHash eventType, VariantMap& eventData);
    void HandleStop(StringHash eventType, VariantMap& eventData);

    String name_;
    int actionType_;
    StringHash eventType_, eventTypeStop_;
    float delay_;
    SharedPtr<Animatable> object_;
    WeakPtr<Object> parent_;
    bool inpool_;
    bool running_;

    AnimatableParameters params_;

    static Pool<DelayAction> pool_;
};



