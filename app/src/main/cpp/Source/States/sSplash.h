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

    bool Initialize() override;
    void Begin() override;
    void End() override;

protected:
    void Create();
    void ResizeScreen();

    void HandleStop(StringHash eventType, VariantMap& eventData);
    void HandleScreenResized(StringHash eventType, VariantMap& eventData);

    Vector<Sprite*> sprites_;
};
