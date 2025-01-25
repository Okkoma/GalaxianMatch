#include <Urho3D/Urho3D.h>

#include <Urho3D/Core/Object.h>
#include <Urho3D/Core/CoreEvents.h>
#include <Urho3D/Core/Context.h>

#include <Urho3D/IO/Log.h>
#include <Urho3D/Input/Input.h>
#include <Urho3D/Container/List.h>

#include <Urho3D/Resource/ResourceCache.h>

#include "GameEvents.h"
#include "GameHelpers.h"
#include "GameStatics.h"

#include "DelayInformer.h"

#include "GameStateManager.h"



const char* levelModeNames[] =
{
    "LVL_NEW",
    "LVL_LOAD",
    "LVL_ARENA",
    "LVL_TEST",
    "LVL_CREATE",
    "LVL_CHANGE",
};

/// Game Scene Cleaner Implementation

void GameSceneCleaner::SetState(GameState* state)
{
    Clear();
    state_ = state;

    sceneDirty_ = false;
}

void GameSceneCleaner::AddStep(const String& sceneName, int phase, bool launch)
{
    steps_.Resize(steps_.Size()+1);

    URHO3D_LOGINFOF("GameSceneCleaner() - AddStep : phase=%d name=%s", phase, sceneName.CString());

    CleaningSceneStep& step = steps_.Back();
    step.sceneStr_ = sceneName;
    step.phase_ = phase;
    step.launch_ = launch;
    sceneDirty_ = true;
}

bool GameSceneCleaner::Execute()
{
    if (sceneDirty_)
    {
        if (currentstep_ >= steps_.Size())
        {
            sceneDirty_ = false;
            //currentstep_ = 0;
            return true;
        }

        CleaningSceneStep& step = steps_[currentstep_];

        if (step.launch_)
        {
            state_->Launch();
            // Reset to not launch at each iteration
            step.launch_ = false;
        }

        if (!locked_)
        {
            if (!step.sceneStr_.Empty())
                GameHelpers::CleanScene(GameStatics::rootScene_, step.sceneStr_, step.phase_);

            currentstep_++;
        }

        return false;
    }

    return true;
}

void GameSceneCleaner::Clear()
{
    steps_.Clear();
    currentstep_ = 0;
}


/// Game State Implementation

GameState::GameState(Context* context) : Object(context)
{
    // Initialize variables to sensible defaults
    stateManager_ = 0;
    stateSuspended_ = false;
    begun_ = false;
    cleanLocked_ = true;
}

GameState::GameState(Context* context, const String& stateId) :
    Object(context)
{
    // Initialize variables to sensible defaults
    stateManager_ = 0;
    stateSuspended_ = false;
    begun_ = false;
    cleanLocked_ = true;

    SetStateId(stateId);
}

GameState::~GameState()
{
    // Clean up
    Dispose();
}

void GameState::Dispose()
{
    // End the state if it is begun.
    if (begun_ == true)
        End();

    // Clear variables
    stateManager_ = 0;
    stateSuspended_ = false;

    begun_ = false;

    URHO3D_LOGINFOF("GameState() - Dispose : id=%s",stateId_.CString());
}

bool GameState::IsActive() const
{
    if (!stateManager_) return false;
    return (this == stateManager_->GetActiveState());
}

bool GameState::IsSuspended() const
{
    return stateSuspended_;
}

const String & GameState::GetStateId() const
{
    return stateId_;
}

unsigned GameState::GetStateHash() const
{
    return stateHash_;
}

GameStateManager* GameState::GetManager() const
{
    return stateManager_;
}


bool GameState::Initialize()
{
    URHO3D_LOGINFOF("GameState() - Initialize : id=%s",stateId_.CString());
    // Success!
    return true;
}

void GameState::Begin()
{
    // Clean Scene

    GameSceneCleaner& scenecleaner = stateManager_->GetSceneCleaner();
    if (scenecleaner.GetState() != this)
    {
        GameState* lastState = stateManager_->GetLastActiveState();
        if (lastState)
        {
            const String& laststateId = lastState->GetStateId();

            URHO3D_LOGINFOF("GameState() - Begin : clean previous state = %s", laststateId.CString());

            scenecleaner.SetState(this);
            scenecleaner.AddStep(laststateId, 0, true);
            scenecleaner.AddStep(laststateId, 1);

            // Lock Cleaner : Wait for Event GAME_SCENECLEANERUNLOCK
            scenecleaner.SetLocked(cleanLocked_);

            // Clean laststate only after SWITCHSCREENTIME delay
            if (cleanLocked_)
                DelayInformer::Get(this, SWITCHSCREENTIME, GAME_SCENECLEANERUNLOCK);
        }
    }
    else
    {
        // Clean self
        URHO3D_LOGINFOF("GameState() - Begin : clean self = %s", GetStateId().CString());
        scenecleaner.AddStep(GetStateId(), 0, true);
        scenecleaner.SetLocked(false);
    }

    stateManager_->StartSceneCleaner();

    // Reset controls
    GameStatics::input_->ResetStates();

    begun_ = true;
    SendEvent(GAME_STATEBEGIN);
    URHO3D_LOGINFOF("GameState() - Begin : id=%s", stateId_.CString());
}

void GameState::End()
{
    // Notify manager that the state ended
    stateManager_->StateEnded(this);

    // We are no longer attached to anything (simply being stored in
    // the registered state list maintained by the manager).
    stateSuspended_ = false;
    begun_ = false;

    SendEvent(GAME_STATEEND);
    URHO3D_LOGINFOF("GameState() - End : id=%s",stateId_.CString());
}

void GameState::Launch()
{
    ;
}

void GameState::Suspend()
{
    // Is this a no-op?
    if (stateSuspended_)
        return;

    // Suspend
    stateSuspended_ = true;
    URHO3D_LOGINFOF("GameState() - Suspend : id=%s",stateId_.CString());
}

void GameState::Resume()
{
    // Is this a no-op?
    if (!stateSuspended_)
        return;

    // Resume
    stateSuspended_ = false;
    URHO3D_LOGINFOF("GameState() - Resume : id=%s",stateId_.CString());
}

void GameState::SetStateId(const String & stateId)
{
    stateId_ = stateId;
    stateHash_ = StringHash(stateId).Value();
}


/// Game State Manager

GameStateManager* GameStateManager::stateManager_ = 0;

GameStateManager::GameStateManager(Context* context) :
    Object(context)
{
    // Initialize variables to sensible defaults
    activeState_ = lastActiveState_ = 0;
    stackmove_ = 1;

    if (!stateManager_)
        stateManager_ = this;
}

GameStateManager::~GameStateManager()
{
    URHO3D_LOGINFO("~GameStateManager() ...");

    // End all currently running states
    Stop();

    // Reset variables
    activeState_.Reset();

    stateStack_.Clear();

    // Destroy any registered states
    URHO3D_LOGINFO("~GameStateManager() - Clear States ...");
    registeredStates_.Clear();
    URHO3D_LOGINFO("~GameStateManager() - Clear States ... OK !");

    if (stateManager_ == this)
        stateManager_ = 0;

    URHO3D_LOGINFO("~GameStateManager() ... OK ! ");
}

bool GameStateManager::RegisterState(GameState* pGameState)
{
    HashMap<String, SharedPtr<GameState> >::Iterator itState;

    // Validate requirements
    if (!pGameState) return false;

    // State already exists in the state map?
    itState = registeredStates_.Find(pGameState->GetStateId());
    if (itState != registeredStates_.End()) return false;

    // If it doesn't exist, add it to the list of available state types
    registeredStates_[pGameState->GetStateId()] = pGameState;

    // Set up state properties
    pGameState->stateManager_ = this;

    // Initialize the state and return result
    return pGameState->Initialize();
}

GameState* GameStateManager::GetActiveState()
{
    return activeState_;
}

GameState* GameStateManager::GetLastActiveState() const
{
    return lastActiveState_;
}

bool GameStateManager::SetActiveState(const String & stateId)
{
    HashMap<String, SharedPtr<GameState> >::Iterator itState;

    // First find the requested state
    itState = registeredStates_.Find(stateId);
    if (itState == registeredStates_.End()) return false;

    // The state was found, end any currently executing states
    if (activeState_)
        activeState_->End();

    // Link this new state
    activeState_ = itState->second_;

    // Start it running.
    if (activeState_->IsBegun() == false)
    {
        activeState_->Begin();
    }

    // If activeState can't begin, get last active state
    if (activeState_->IsBegun() == false)
    {
        activeState_ = lastActiveState_;
        if (activeState_->IsBegun() == false)
        {
            activeState_->Begin();
        }
        return false;
    }

    return true;
}

GameState* GameStateManager::GetState(const String & stateId)
{
    // First retrieve the details about the state specified, if none
    // was found this is clearly an invalid state identifier.
    HashMap<String, SharedPtr<GameState> >::Iterator itState = registeredStates_.Find(stateId);
    if (itState == registeredStates_.End())
        return 0;
    return itState->second_;
}

void GameStateManager::Stop()
{
    URHO3D_LOGINFO("GameStateManager() - Stop");

    UnsubscribeFromEvent(E_POSTUPDATE);

    // Stop all active states
    if (activeState_)
    {
        URHO3D_LOGINFOF("GameStateManager() - Stop : End activeState = %s ...", activeState_->GetStateId().CString());
        activeState_->End();
    }
}

void GameStateManager::StateEnded(GameState * pState)
{
    if (pState == 0) return;

    lastActiveState_ = activeState_;

    // If this was at the root of the state history, we're all out of states
    if (pState == activeState_)
        activeState_.Reset();
}

void GameStateManager::ClearStack()
{
    stateStack_.Clear();
}

bool GameStateManager::StartStack()
{
    URHO3D_LOGINFOF("GameStateManager() - StartStack ...");

    if (stateStack_.Size())
    {
        DumpStack();
        GameState* state = stateStack_.Back();
        lastActiveState_ = 0;

        if (state)
            bool b = SetActiveState(state->GetStateId());

        return true;
    }

    URHO3D_LOGERRORF("GameStateManager() - StartStack : stack empty !");
    return false;
}

bool GameStateManager::AddToStack(const String& stateId)
{
	WeakPtr<GameState> state(GetState(stateId));

	if (state)
	{
        if ((stateStack_.Size() && stateStack_.Back().Get() != state.Get()) || stateStack_.Empty())
        {
            URHO3D_LOGINFOF("GameStateManager() - AddToStack : stateId=%s", stateId.CString());
            stateStack_.Push(state);
            DumpStack();
            return true;
        }
	}

	return false;
}

bool GameStateManager::PushToStack(const String& stateId)
{
    stackmove_ = 1;

    bool b = SetActiveState(stateId);

    URHO3D_LOGINFOF("GameStateManager() - PushToStack : stateId=%s Status=%s lastState=%s", stateId.CString(), b ? "OK" : "NOK Returned to", lastActiveState_->GetStateId().CString());
    stateStack_.Push(activeState_);

    DumpStack();

    return b;
}

bool GameStateManager::ReplaceState(const String& stateId)
{
    if (stateStack_.Size() != 0)
        stateStack_.Pop();

    return PushToStack(stateId);
}

const String& GameStateManager::PopStack(bool activeprevstate)
{
    if (stateStack_.Size() != 0)
    {
        lastActiveState_ = stateStack_.Back();

        // pop active state
        stateStack_.Pop();

        GameState* prevState = 0;
        if (stateStack_.Size() != 0)
        {
            // get the last active state
            prevState = stateStack_.Back();
            stackmove_ = -1;
            URHO3D_LOGINFOF("GameStateManager() - PopStack : stateId=%s lastState=%s !", prevState ? prevState->GetStateId().CString() : "EmptyStack", lastActiveState_->GetStateId().CString());
        }

        DumpStack();

        if (prevState)
        {
            if (activeprevstate)
                SetActiveState(prevState->GetStateId());
            return prevState->GetStateId();
        }
    }

    URHO3D_LOGINFO("StateStack Empty!");

    return String::EMPTY;
}

void GameStateManager::DumpStack() const
{
    if (stateStack_.Size() != 0)
    {
        for (List<WeakPtr<GameState> >::ConstIterator it=stateStack_.Begin(); it != stateStack_.End(); it++)
        {
            const WeakPtr<GameState>& gamestate = *it;
            URHO3D_LOGINFOF("DumpStack : %s ", gamestate->GetStateId().CString());
        }
    }
    else
        URHO3D_LOGINFO("DumpStack : StateStack Empty!");
}

void GameStateManager::StartSceneCleaner()
{
    SubscribeToEvent(E_POSTUPDATE, URHO3D_HANDLER(GameStateManager, OnCleanerExecute));
    SubscribeToEvent(GAME_SCENECLEANERUNLOCK, URHO3D_HANDLER(GameStateManager, OnCleanerUnlock));
}

void GameStateManager::OnCleanerExecute(StringHash eventType, VariantMap& eventData)
{
    if (sceneCleaner_.Execute())
    {
        SendEvent(GAME_SCENECLEANERFINISHED);

        UnsubscribeFromEvent(E_POSTUPDATE);
        UnsubscribeFromEvent(GAME_SCENECLEANERUNLOCK);
    }
}

void GameStateManager::OnCleanerUnlock(StringHash eventType, VariantMap& eventData)
{
    sceneCleaner_.SetLocked(false);
}
