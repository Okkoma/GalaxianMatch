#pragma once

#include "DefsGame.h"
#include "Network.h"

#include "GameStateManager.h"


namespace Urho3D
{
    class Camera;
    class Connection;
    class UIElement;
    class Text;
}

using namespace Urho3D;


class InteractiveFrame;


class PlayState : public GameState
{
	URHO3D_OBJECT(PlayState, GameState);

friend struct SceneCleaner;

public:
	PlayState(Context* context);
	~PlayState();

	virtual bool Initialize();
	virtual void Begin();
	virtual void End();
    virtual void Launch();

    virtual void OnAccessMenuOpenFrame(bool state);

#if defined(TEST_NETWORK)
    // Network Callbacks
    void OnNetworkAvailablePeersUpdate(const StringVector* peers);
    void OnNetworkConnectedPeersUpdate(const StringVector* peers);
    void OnNetworkMessageReceived(NetworkTransport* transport, Vector<VectorBuffer >* packets);
#endif

public:
    void BeginNewLevel(GameLevelMode mode=LVL_NEW, unsigned seed=0);
    void LevelWin();
    void GameOver();

	void CheckStars(bool loosetry);

    void SetSeed(unsigned seed);
    unsigned GetSeed() const;

    InteractiveFrame* GetAbilityPanel() const;

private:
    void ResetGameStates();
    void SetLevelDatas();
    void InitRandomizer();

    void CreateLevel(bool updatelevel);

	bool CreateScene();
    void EndScene();

	void CreateUI();
	void ResizeUI(bool instant);
	void ResizeAbilityPanel(bool instant);
	void SetVisibleUI(bool ok);
    void RemoveUI();
    void SetHiScoreEnable(bool enable);

    void SaveGame();

    void SetBossActive(bool enable);
    void AddRandomBossWarning();

	void Pause(bool enable, bool addmessagebox=true, bool autoUnpause=false);

	void SubscribeToEvents();
    void UnsubscribeToEvents();
    void SubscribeToDebugEvents(bool enabled);

    void HandleSceneLoading(StringHash eventType, VariantMap& eventData);
    void HandleSceneAppears(StringHash eventType, VariantMap& eventData);
    void HandleStart(StringHash eventType, VariantMap& eventData);
    void HandleBossAppears(StringHash eventType, VariantMap& eventData);
    void HandleGameOver(StringHash eventType, VariantMap& eventData);

    void HandleUpdateScores(StringHash eventType, VariantMap& eventData);
    void HandleUpdateMoves(StringHash eventType, VariantMap& eventData);
    void HandleUpdateStars(StringHash eventType, VariantMap& eventData);
    void HandleUpdateCoins(StringHash eventType, VariantMap& eventData);

    void HandleUpdateObjectives(StringHash eventType, VariantMap& eventData);
    void HandleUpdateMatchState(StringHash eventType, VariantMap& eventData);

    void CheckInputForDebug(Input* input);
	void HandleUpdate(StringHash eventType, VariantMap& eventData);
	void HandlePause(StringHash eventType, VariantMap& eventData);
    void HandleStop(StringHash eventType, VariantMap& eventData);
    void HandleNewLevel(StringHash eventType, VariantMap& eventData);
    void HandleAccessMenu(StringHash eventType, VariantMap& eventData);
    void HandleShake(StringHash eventType, VariantMap& eventData);

#if defined(TEST_NETWORK)
    void HandleDuoToggled(StringHash eventType, VariantMap& eventData);
    void OnConnectPeer(const StringVector* peers);
    void OnDisconnectPeer();
#endif

    void HandleScreenResized(StringHash eventType, VariantMap& eventData);

    void UpdateObjectives(bool init=false);
    void PutBonusesInPlayerState(const Vector<int>& bonuses);

    void OnLevelWin(StringHash eventType, VariantMap& eventData);
    void OnWinMessageAck(StringHash eventType, VariantMap& eventData);
    void OnPauseMessageAck(StringHash eventType, VariantMap& eventData);
    void OnGameOverFrame(StringHash eventType, VariantMap& eventData);
    void OnGameOverAck(StringHash eventType, VariantMap& eventData);
    void OnGameNeedMoreStarsAck(StringHash eventType, VariantMap& eventData);
    void OnGameNeedMoreStars(StringHash eventType, VariantMap& eventData);
	void OnQuitMessageAck(StringHash eventType, VariantMap& eventData);
    void OnDelayedActions();
    void OnDelayedActions_Local(StringHash eventType, VariantMap& eventData);
    void OnPostRenderUpdate(StringHash eventType, VariantMap& eventData);

	Scene* rootScene_;
	Node* scene_;
	Node* cameraNode_;
	Camera* camera_;

    float cameraYaw_;
    float cameraPitch_;
    Vector3 cameraMotion_;
    float camMotionSpeed_;
    float currentCamMotionSpeed_;

    bool activeGameLogic_;
    bool initMode_;
    bool restartMode_;
    bool drawDebug_;
    bool ctrlCameraWithMouse_;
	bool gameOver_;
	bool paused_;
    bool toLoadGame_;

	Vector<UIElement* > playerStatusZone;

    unsigned seed_;

    GameSceneCleaner sceneCleaner_;
};
