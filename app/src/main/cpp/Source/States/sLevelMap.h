#pragma once

#include "GameStateManager.h"

namespace Urho3D
{
    class UIElement;
    class Sprite;
    class Scene;
}


using namespace Urho3D;

enum PlanetMode
{
    PlanetMode_Disable,
    PlanetMode_Enable
};

class LevelMapState : public GameState
{
	URHO3D_OBJECT(LevelMapState, GameState);

public:

	LevelMapState(Context* context);
	~LevelMapState();

	virtual bool Initialize();
	virtual void Begin();
	virtual void End();

    virtual void OnAccessMenuOpenFrame(bool state);

	void StartLevel();
	void StartState(int action);
    void ReturnMainMenu();

    int GetPlanetMode() const { return planetMode_; }

private:
    bool HasCinematicAvailable(int levelid) const;

    bool ResetMap(bool purge=true);
    void ResetCamera();

    bool GenerateLevelMapScene(int zone);
    void SetMissionNodes(Node* root);
    void SetLinks();
    void UpdatLinkScaleY(float scaley);
    void SetPlanetNodes(Node* root);
    void SetPlanetLinkButtons();
    void UpdatePlanetSelection();
    void UpdateSceneRect();
    void UpdateScene();
    void UpdateStatics();
#if defined(TEST_NETWORK)
    void UpdatePeerOffers();
    void ReceivePeerOffers();
#endif
    bool CreateScene(bool reset=true);

    void SetEnableConstellation(bool enable);
    void SwitchPlanetMode();

    void CloseInteractiveFrames();
    void OnGainFrame(StringHash eventType, VariantMap& eventData);
    void OnGainFrameMessageAck(StringHash eventType, VariantMap& eventData);
    void OnGameNoMoreTries(StringHash eventType, VariantMap& eventData);
    void OnGameNoMoreTriesAck(StringHash eventType, VariantMap& eventData);

    void SetVisibleUI(bool state);
    void ResetUI();
	void CreateUI();
    void RemoveUI();

    void UnselectLevel();
    void GoToSelectedLevel();

    void UpdateDialogBox(int level);

    void Quit();

    void SubscribeToEvents();
    void UnsubscribeToEvents();

    void HandleInteractiveFrameStart(StringHash eventType, VariantMap& eventData);
    void HandleConfirmCinematicLaunch(StringHash eventType, VariantMap& eventData);
    void HandleChangePlanetMode(StringHash eventType, VariantMap& eventData);
    void HandleSceneUpdate(StringHash eventType, VariantMap& eventData);
    void HandleChangeSceneMode(StringHash eventType, VariantMap& eventData);
    void UpdateActionAnimations(int newaction);
    void HandleSelection(StringHash eventType, VariantMap& eventData);
    void HandleAccessMenu(StringHash eventType, VariantMap& eventData);
	void HandleKeyDown(StringHash eventType, VariantMap& eventData);
    void HandleScreenResized(StringHash eventType, VariantMap& eventData);

    void OnPostRenderUpdate(StringHash eventType, VariantMap& eventData);

    int firstMissionID_, lastMissionID_;
    int selectedLevelID_, lastSelectedLevelID_;
    int lastaction_;
    int planetMode_;
    bool constellationMode_;

    float cameraZoom_;
    unsigned cameraSmooth_;
    Vector3 cameraTargetPosition_;
    float cameraTargetZoom_;
};
