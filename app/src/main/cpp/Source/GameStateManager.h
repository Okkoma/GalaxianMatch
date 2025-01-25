#pragma once


namespace Urho3D
{
    class Object;
    class Context;
}

using namespace Urho3D;

#include "DelayInformer.h"

URHO3D_EVENT(GAME_SCENECLEANERFINISHED, Game_SceneCleanerFinished) { }
URHO3D_EVENT(GAME_SCENECLEANERUNLOCK, Game_SceneCleanerUnlock) { }

const float SWITCHSCREENTIME = 0.25f;

enum GameLevelMode
{
    LVL_NEW = 0,
    LVL_LOAD,
    LVL_ARENA,
    LVL_TEST,
    LVL_CREATE,
    LVL_CHANGE
};

class GameStateManager;
class GameState;

class GameSceneCleaner
{
public :
    GameSceneCleaner() : state_(0), sceneDirty_(false), locked_(true), currentstep_(0) { }

    void SetState(GameState* currentState);
    void AddStep(const String& sceneName, int phase, bool launch=false);
    void SetLocked(bool state) { locked_ = state; }
    void Clear();

    bool Execute();

    GameState* GetState() const { return state_; }

private :
    struct CleaningSceneStep
    {
        String sceneStr_;
        int phase_;
        bool launch_;
    };

    int currentstep_;
    Vector<CleaningSceneStep> steps_;
    GameState* state_;
    bool sceneDirty_, locked_;
};

class GameState : public Object
{
	URHO3D_OBJECT(GameState, Object);

	friend class GameStateManager;

public:

	GameState(Context* context);
	GameState(Context* context, const String & stateId);
	virtual ~GameState();

	void SetStateId(const String & stateId);
	void SetCleanerLocked(bool state) { cleanLocked_ = state; }

	const String& GetStateId() const;
	unsigned GetStateHash() const;
	bool IsActive() const;
	bool IsSuspended() const;
	GameStateManager* GetManager() const;

	virtual bool Initialize();
	virtual void Begin();
	virtual void End();
    virtual void Launch();

	// State Management
	virtual void Suspend();
	virtual void Resume();

	// Query Routines
	virtual bool IsBegun() const { return begun_; }
	// delete all resources
	virtual void Dispose();

    virtual void OnAccessMenuOpenFrame(bool state) { }

protected :

    inline void ResetUnlocker();

	GameStateManager* stateManager_;
	String stateId_;
	unsigned stateHash_;
	bool stateSuspended_;
	bool begun_;
	bool cleanLocked_;
};

class GameStateManager : public Object
{
	URHO3D_OBJECT(GameStateManager, Object);

	friend class GameState;

public:
	GameStateManager(Context* context);
	virtual ~GameStateManager();

	static GameStateManager* Get() { return stateManager_; }

	void Stop();

    void StartSceneCleaner();

	//  Getters & Setters
	bool RegisterState(GameState* pGameState);
	GameState* GetState(const String & stateId);
	bool SetActiveState(const String & stateId);

	GameState* GetActiveState();
    GameState* GetLastActiveState() const;
    GameSceneCleaner& GetSceneCleaner() { return sceneCleaner_; }
    int GetStackMove() const { return stackmove_; }
    unsigned GetStackSize() const { return stateStack_.Size(); }

	// Stack Management
	/// clear the history stack of the states
	void ClearStack();
	/// start manually the state from the back of the stack (added manually with AddToStack)
    bool StartStack();
	/// add state to the stack manually without activating this state
	bool AddToStack(const String & stateId);
	/// add a state to the state stack and SetActiveState. The active state will be stopped (Stop function call)
	/// and the new state will be started (Begin function call)
	/// returns true if state was found and started
	bool PushToStack(const String & stateId);
	/// the active state will be stopped (Stop function call) and the last state pushed
	/// to the stack will be started (Begin function call)
	/// returns the new active state name/id or empty string
	const String& PopStack(bool activeprevstate=true);

    bool ReplaceState(const String& stateId);

	void DumpStack() const;

private:

	void StateEnded(GameState* state);

    void OnCleanerExecute(StringHash eventType, VariantMap& eventData);
    void OnCleanerUnlock(StringHash eventType, VariantMap& eventData);

	/// All state types that have been registered with the system
	HashMap<String, SharedPtr<GameState> > registeredStates_;

	/// The currently active state that the game is currently in (restorable states will be available via its parent)
	WeakPtr<GameState> activeState_;
    WeakPtr<GameState> lastActiveState_;

	/// The State Stack
	List<WeakPtr<GameState> > stateStack_;
    int stackmove_;

    /// The Scene Cleaner
    GameSceneCleaner sceneCleaner_;

	static GameStateManager* stateManager_;
};


