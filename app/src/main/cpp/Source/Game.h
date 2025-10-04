#pragma once

#include "GameUI.h"

using namespace Urho3D;

namespace Urho3D
{
    class Context;
    class UIElement;
}

struct GameConfig;

class Game : public Application
{
	URHO3D_OBJECT(Game, Application);

public:
	Game(Context* context);
    virtual ~Game();

	void Setup() override;
	void Start() override;
	void Stop() override;

    void CreateMenuButton(UIElement* uiroot);
    void CreateAccessMenu(UIElement* uiroot);
    void SetAccessMenuButtonVisible(int child, bool visible);

    void ShowHeader(UIElement* uiroot);
    void HideHeader(UIElement* uiroot);

    void SubscribeToAccessMenuEvents();
    void UnsubscribeFromAccessMenuEvents();

    UIElement* GetAccessMenu() const { return accessMenu_.Get(); }
    UIDialog* GetCompanion() const { return companionBox_.Get(); }

    void SetCompanionMessages();

    static Game* Get() { return game_; }

private:
    void SetupDirectories();
    void SetupControllers();
	void SetupSubSystems();
    void ResetScreen();

    void HandleInputFocus(StringHash eventType, VariantMap& eventData);
    void HandleRewardsEvents(StringHash eventType, VariantMap& eventData);
    void OnBonusFrameMessageAck(StringHash eventType, VariantMap& eventData);

    void HandlePreloadResources(StringHash eventType, VariantMap& eventData);
    void HandlePreloadFinished(StringHash eventType, VariantMap& eventData);

    void HandleGoBack(StringHash eventType, VariantMap& eventData);
    void HandleShowOptions(StringHash eventType, VariantMap& eventData);
    void HandleShowShop(StringHash eventType, VariantMap& eventData);
    void HandleWatchCinematic(StringHash eventType, VariantMap& eventData);

    void HandleTouchBegin(StringHash eventType, VariantMap& eventData);
    void HandleKeyDown(StringHash eventType, VariantMap& eventData);
    void HandleScreenResized(StringHash eventType, VariantMap& eventData);
    void HandleConsoleCommand(StringHash eventType, VariantMap& eventData);

    void HandleSceneUpdate(StringHash eventType, VariantMap& eventData);

    void HandleUpdateHeader(StringHash eventType, VariantMap& eventData);
    void HandleUpdateCompanion(StringHash eventType, VariantMap& eventData);

    static Game* game_;

    WeakPtr<UIMenu> accessMenu_;    
    SharedPtr<UIDialog> companionBox_;

    bool debugCameraWithMouse_{false};
    float cameraYaw_{0.f};
    float cameraPitch_{0.f};
    float timerInactiveCursor_{0.f};

    int numClicksOutsideCompanionBox_ = 0;
    float delayCompanion_;
};
