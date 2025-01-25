#pragma once

#include "GameStateManager.h"

namespace Urho3D
{
    class UIElement;
    class Sprite;
}

using namespace Urho3D;


class OptionState : public GameState
{
	URHO3D_OBJECT(OptionState, GameState);

public:

	OptionState(Context* context);
	~OptionState();

	virtual bool Initialize();
	virtual void Begin();
	virtual void End();

    bool IsFrameOpened() const { return framed_; }
	bool IsParameterOpened() const;
	bool IsShopOpened() const;

    void SwitchParameterFrame();
    void SwitchShopFrame();
	void CloseFrame();

    void ReturnMenu();

    UIElement* GetShop();

protected:

    bool CreateScene();
    bool CreateParameterUI();
	void CreateShopUI();
	void SetCloseButton(int frame, bool state);

    void ResizeUI();
    void RemoveUI(int state);

    void OpenParameterFrame();
    void OpenShopFrame();

    void ReadOptions();
    void WriteOptions();

    void SubscribeToEvents();
    void UnsubscribeToEvents();

    // Shop Panel
    void RemoveShop();
    void SetShopWindowColors(const Color& color1, const Color& color2, float opacity);
    void SubscribeToShopEvents();
    void UnsubscribeShopEvents();
    void SwitchShopTab(UIElement* uielement);
    void UpdateShopItems();
    void AcquireShopItems();
    void SetUnlockedShopItem(UIElement* itembutton, bool unlocked);
    void SetEnableShopItem(UIElement* itembutton, bool enabled);
    void HandleShopSwitchTab(StringHash eventType, VariantMap& eventData);
    void HandleShopButtonClick(StringHash eventType, VariantMap& eventData);
    void OnShopMessageAck(StringHash eventType, VariantMap& eventData);

    void OnGameResetMessageAck(StringHash eventType, VariantMap& eventData);
    void HandleGameReset(StringHash eventType, VariantMap& eventData);
    void HandleOptionChanged(StringHash eventType, VariantMap& eventData);
    void HandleCoinsUpdated(StringHash eventType, VariantMap& eventData);
    void HandleReturnMenu(StringHash eventType, VariantMap& eventData);
    void HandleCloseFrame(StringHash eventType, VariantMap& eventData);
//    void HandleAccessMenu(StringHash eventType, VariantMap& eventData);
    void HandleKeyDown(StringHash eventType, VariantMap& eventData);
    void HandleScreenResized(StringHash eventType, VariantMap& eventData);

    void OnPostRenderUpdate(StringHash eventType, VariantMap& eventData);

    bool framed_;
};
