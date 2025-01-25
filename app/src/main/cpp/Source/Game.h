#pragma once

#include <Urho3D/Engine/Application.h>

#include "DefsJNI.h"

using namespace Urho3D;

namespace Urho3D
{
    class Context;
    class UIElement;
}

struct GameConfig;
class UIDialog;

class Game : public Application
{
	URHO3D_OBJECT(Game, Application);

public:
	Game(Context* context);
    virtual ~Game();

	virtual void Setup();
	virtual void Start();
	virtual void Stop();

    void CreateMenuButton(UIElement* uiroot);
    void CreateAccessMenu(UIElement* uiroot);
    void SetAccessMenuButtonVisible(int child, bool visible);

    void ShowHeader(UIElement* uiroot);
    void HideHeader(UIElement* uiroot);

    void SubscribeToAccessMenuEvents();
    void UnsubscribeFromAccessMenuEvents();

    UIElement* GetAccessMenu() const;

    UIDialog* GetCompanion() const;
    void SetCompanionMessages();

    static Game* Get() { return game_; }

private:
    bool LoadGameConfig(const char* fileName, GameConfig* config);

    void SetupDirectories();
    void SetupControllers();
	void SetupSubSystems();
    void ResetScreen();

    void HandleAppPaused(StringHash eventType, VariantMap& eventData);
    void HandleAppResumed(StringHash eventType, VariantMap& eventData);
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

    float delayCompanion_;
};
