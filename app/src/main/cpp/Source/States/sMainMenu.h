#pragma once

#include "GameStateManager.h"

namespace Urho3D
{
    class UIElement;
    class Sprite;
    class Scene;
}


using namespace Urho3D;


class MenuState : public GameState
{
	URHO3D_OBJECT(MenuState, GameState);

public:

	MenuState(Context* context);
	~MenuState();

	virtual bool Initialize();
	virtual void Begin();
	virtual void End();

    virtual void OnAccessMenuOpenFrame(bool state);

    void GoLevel(int level);
	void BeginNewLevel(GameLevelMode mode=LVL_NEW, unsigned seed=0);

private:
    bool IsNetworkReady() const;

    void CreateScene(bool setscene);
	void CreateUI();
    void RemoveUI();

    void Quit();
    void SetMenuColliderPositions();
    void UpdateAnimButtons();

    void SubscribeToMenuEvents();
    void UnsubscribeToMenuEvents();

    void Launch(int selection);

    void HandleMenu(StringHash eventType, VariantMap& eventData);
	void HandleKeyDown(StringHash eventType, VariantMap& eventData);

	void HandleQuitMessageAck(StringHash eventType, VariantMap& eventData);
    void HandleScreenResized(StringHash eventType, VariantMap& eventData);

    void OnPostRenderUpdate(StringHash eventType, VariantMap& eventData);
};
