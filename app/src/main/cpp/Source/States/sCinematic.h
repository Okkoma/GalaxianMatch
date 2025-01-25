#pragma once

#include <Urho3D/Core/Timer.h>

#include "GameStateManager.h"

namespace Urho3D
{
    class UIElement;
    class Sprite;
}

using namespace Urho3D;

enum
{
    CINEMATICSELECTIONMODE_INTRO_OUTRO = 0,
    CINEMATICSELECTIONMODE_LEVELSELECTED,
    CINEMATICSELECTIONMODE_REPLAY,
    CINEMATICSELECTIONMODE_TOBECONTINUED
};

class CinematicState : public GameState
{
	URHO3D_OBJECT(CinematicState, GameState);

public:

	CinematicState(Context* context);
	~CinematicState();

	static bool SetCinematic(int mode, int firstlevel=0, int selectedlevel=0, const String& nextState=String::EMPTY);
    static bool GetCinematicParts(int mode, int firstlevel, int selectedlevel);
    static void LaunchCinematicParts(int mode, const String& nextState=String::EMPTY);

	virtual bool Initialize();
	virtual void Begin();
	virtual void End();

	virtual void OnAccessMenuOpenFrame(bool state);

	void AddSceneFileToPlay(const String& filename);
    void ChangeCameraTo(Node* node);
    void RestoreCamera();
    void ChangeToAnimation(int index);
    void Stop();
    void SetPaused(bool state);

protected:
    bool LaunchSceneFile();

    void SubscribeToEvents();
    void UnsubscribeToEvents();

    void HandleLaunchCinematic(StringHash eventType, VariantMap& eventData);
    void HandleDelayedStart(StringHash eventType, VariantMap& eventData);
    void HandleSceneUpdate(StringHash eventType, VariantMap& eventData);
    void HandleAccessMenu(StringHash eventType, VariantMap& eventData);
    void HandleInput(StringHash eventType, VariantMap& eventData);
	void HandleStop(StringHash eventType, VariantMap& eventData);

	Vector<String> scenefilenames_;
	int scenefileindex_;
	bool paused_, stopped_;
	Timer timer_;
	float animationDuration_, animationSpeed_;
};
