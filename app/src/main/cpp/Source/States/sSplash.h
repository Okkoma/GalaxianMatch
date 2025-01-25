#pragma once

#include "GameStateManager.h"

namespace Urho3D
{
class UIElement;
class Sprite;
}

using namespace Urho3D;


class SplashState : public GameState
{
    URHO3D_OBJECT(SplashState, GameState);

public:

    SplashState(Context* context);
    ~SplashState();

    virtual bool Initialize();
    virtual void Begin();
    virtual void End();

protected:
    void Create();
    void ResizeScreen();

    void HandleStop(StringHash eventType, VariantMap& eventData);
    void HandleScreenResized(StringHash eventType, VariantMap& eventData);

    Vector<Sprite*> sprites_;
};
