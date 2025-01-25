#include <Urho3D/Urho3D.h>

#include <Urho3D/Core/Object.h>
#include <Urho3D/Core/CoreEvents.h>

#include <Urho3D/Graphics/Renderer.h>
#include <Urho3D/Graphics/Viewport.h>
#include <Urho3D/Graphics/Camera.h>
#include <Urho3D/Graphics/Graphics.h>

#include <Urho3D/Engine/Engine.h>
#include <Urho3D/IO/Log.h>
#include <Urho3D/IO/FileSystem.h>

#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Resource/XMLFile.h>

#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Scene/SceneEvents.h>
#include <Urho3D/Scene/SplinePath.h>

#include <Urho3D/UI/UI.h>
#include <Urho3D/UI/UIEvents.h>
#include <Urho3D/UI/Menu.h>

#include <Urho3D/Urho2D/Renderer2D.h>
#include <Urho3D/Urho2D/AnimatedSprite2D.h>
#include <Urho3D/Urho2D/StaticSprite2D.h>

#include <Urho3D/Input/Input.h>

#include "Game.h"
#include "GameEvents.h"
#include "GameHelpers.h"
#include "GameOptions.h"
#include "GameStatics.h"
#include "GameAttributes.h"

#include "SplashScreen.h"
#include "SceneAnimation2D.h"

#include "DelayInformer.h"

#include "sCinematic.h"


#define DEFAULTANIMATIONDURATION 3.f

const StringHash CINEMATIC_STOP = StringHash("Cinematic_Stop");

const char* CinematicPartStr[] =
{
    "intro",
    "branch",
    "boss",
    "outro",
    "tobecontinued"
};

Vector<int> cinematicParts_;
int cinematicMode_;
String nextStateAfterCinematic_;

WeakPtr<Node> localScene_;
WeakPtr<Component> animation_;
PODVector<Component*> animations_;
unsigned animationIndex_;

CinematicState::CinematicState(Context* context) :
    GameState(context, "Cinematic"),
    animationDuration_(DEFAULTANIMATIONDURATION)
{
//    URHO3D_LOGINFO("CinematicState()");
//    SetCleanerLocked(false);
}

CinematicState::~CinematicState()
{
    URHO3D_LOGINFO("~CinematicState()");
}

bool CinematicState::GetCinematicParts(int mode, int firstlevel, int selectedlevel)
{
    CinematicState* cinematicstate = (CinematicState*)GameStateManager::Get()->GetState("Cinematic");
    if (!cinematicstate)
        return false;

    if (GameStatics::graphics_->GetMaxTextureSize() < 2048)
        return false;

    const int zone = GameStatics::playerState_->zone;
    const int currentCinematicPartShown = GameStatics::playerState_->cinematicShown_[zone-1];
    const int bosslevel = GameStatics::GetBossLevelId(zone);

    Vector<int> parts;
    cinematicParts_.Clear();

    if (mode == CINEMATICSELECTIONMODE_LEVELSELECTED)
    {
        if (currentCinematicPartShown < CINEMATIC_BOSSDEFEAT && selectedlevel == bosslevel &&
            GameStatics::playerState_->missionstates[bosslevel-1].state_ == GameStatics::MissionState::MISSION_UNLOCKED)
            parts.Push(CINEMATIC_BOSSINTRO);
    //    else if (currentCinematicPartShown != CINEMATIC_BRANCH && selectedlevel != -1 && GameStatics::playerState_->missionstates[selectedlevel-1].state_ == GameStatics::MissionState::MISSION_UNLOCKED)
    //        parts.Push(CINEMATIC_BRANCH);
    }
    else if (mode == CINEMATICSELECTIONMODE_INTRO_OUTRO)
    {
        if (currentCinematicPartShown == CINEMATIC_NONE)
            parts.Push(CINEMATIC_INTRO);
        if (currentCinematicPartShown < CINEMATIC_BOSSDEFEAT && GameStatics::playerState_->missionstates[bosslevel-1].state_ == GameStatics::MissionState::MISSION_COMPLETED)
            parts.Push(CINEMATIC_BOSSDEFEAT);
    }
    else if (mode == CINEMATICSELECTIONMODE_REPLAY)
    {
        if (selectedlevel == firstlevel)
        {
            parts.Push(CINEMATIC_INTRO);
        }
        else if (selectedlevel == bosslevel)
        {
            parts.Push(CINEMATIC_BOSSINTRO);
            if (GameStatics::playerState_->missionstates[bosslevel-1].state_ == GameStatics::MissionState::MISSION_COMPLETED)
                parts.Push(CINEMATIC_BOSSDEFEAT);
        }
    }
    else if (mode == CINEMATICSELECTIONMODE_TOBECONTINUED)
    {
        parts.Push(CINEMATIC_TOBECONTINUED);
    }

    // Check if the cinematic files exist
    for (unsigned i = 0; i < parts.Size(); i++)
    {
        String scenefile;
        scenefile.AppendWithFormat("Cinematic/constellation%d-%s.xml", zone, CinematicPartStr[parts[i]]);
        if (GameStatics::context_->GetSubsystem<ResourceCache>()->GetResource<XMLFile>(scenefile))
            cinematicParts_.Push(parts[i]);
    }

    URHO3D_LOGINFOF("CinematicState() - GetCinematicParts : numcinematics=%u !", cinematicParts_.Size());

    return cinematicParts_.Size() > 0;
}

void CinematicState::LaunchCinematicParts(int mode, const String& nextState)
{
    CinematicState* cinematicstate = (CinematicState*)GameStateManager::Get()->GetState("Cinematic");
    if (!cinematicstate)
        return;

    cinematicMode_ = mode;
    nextStateAfterCinematic_ = nextState;

    // Launch SplashScreen
    VariantMap& eventData = GameStatics::context_->GetEventDataMap();
    eventData[SplashScreen_Events::KEEPSTATEDELAY] = 0.f;
    cinematicstate->SendEvent(GAME_LEVELTOLOAD, eventData);

    // Wait FinishClose Before Start to Load cinematics
    cinematicstate->SubscribeToEvent(SPLASHSCREEN_FINISHCLOSE, new Urho3D::EventHandlerImpl<CinematicState>(cinematicstate, &CinematicState::HandleLaunchCinematic));
}

bool CinematicState::SetCinematic(int mode, int firstlevel, int selectedlevel, const String& nextState)
{
    if (GameStatics::gameMode_)
        return false;

    if (GetCinematicParts(mode, firstlevel, selectedlevel))
    {
        LaunchCinematicParts(mode, nextState);
        return true;
    }

    return false;
}

void CinematicState::HandleLaunchCinematic(StringHash eventType, VariantMap& eventData)
{
    UnsubscribeFromEvent(SPLASHSCREEN_FINISHCLOSE);

    Vector<int> parts(cinematicParts_);
    cinematicParts_.Clear();

    URHO3D_LOGINFOF("CinematicState() - HandleLaunchCinematic : show cinematic : numparts=%u ", parts.Size());

    const int zone = GameStatics::playerState_->zone;
    const String basename = "Cinematic/constellation" + String(zone) + "-";
    for (unsigned i=0; i < parts.Size(); i++)
        AddSceneFileToPlay(basename + CinematicPartStr[parts[i]] + ".xml");

    if (cinematicMode_ != CINEMATICSELECTIONMODE_REPLAY)
        GameStatics::playerState_->cinematicShown_[zone-1] = parts.Front();

    if (!nextStateAfterCinematic_.Empty())
        stateManager_->AddToStack(nextStateAfterCinematic_);

    stateManager_->PushToStack("Cinematic");
}


bool CinematicState::Initialize()
{
	return GameState::Initialize();
}

void CinematicState::Begin()
{
    if (IsBegun())
        return;

    URHO3D_LOGINFO("CinematicState() - ----------------------------------------");
    URHO3D_LOGINFO("CinematicState() - Begin ...                             -");
    URHO3D_LOGINFO("CinematicState() - ----------------------------------------");

    SubscribeToEvent(GAME_SCENECLEANERFINISHED, URHO3D_HANDLER(CinematicState, HandleDelayedStart));

	// Call base class implementation
	GameState::Begin();
    stopped_ = false;

    URHO3D_LOGINFO("CinematicState() - ----------------------------------------");
    URHO3D_LOGINFO("CinematicState() - Begin ...  OK !                       -");
    URHO3D_LOGINFO("CinematicState() - ----------------------------------------");
}

void CinematicState::End()
{
    if (!IsBegun())
        return;

    URHO3D_LOGINFO("CinematicState() - ----------------------------------------");
    URHO3D_LOGINFO("CinematicState() - End ...                               -");
    URHO3D_LOGINFO("CinematicState() - ----------------------------------------");

    Stop();

	UnsubscribeFromAllEvents();

//    // Reset controls
//    GetSubsystem<Input>()->ResetStates();

	// Call base class implementation
	GameState::End();

    URHO3D_LOGINFO("CinematicState() - ----------------------------------------");
    URHO3D_LOGINFO("CinematicState() - End ... OK !                           -");
    URHO3D_LOGINFO("CinematicState() - ----------------------------------------");

    if (nextStateAfterCinematic_ == "Play")
        SendEvent(GAME_LEVELTOLOAD);
}

void CinematicState::AddSceneFileToPlay(const String& filename)
{
    URHO3D_LOGINFOF("CinematicState() - AddSceneFileToPlay filename=%s ...", filename.CString());

    scenefilenames_.Push(filename);
}

void CinematicState::SubscribeToEvents()
{
    if (localScene_->GetScene())
        SubscribeToEvent(localScene_->GetScene(), E_SCENEPOSTUPDATE, URHO3D_HANDLER(CinematicState, HandleSceneUpdate));
    SubscribeToEvent(E_KEYDOWN, URHO3D_HANDLER(CinematicState, HandleInput));
    SubscribeToEvent(E_MOUSEBUTTONDOWN, URHO3D_HANDLER(CinematicState, HandleInput));
    SubscribeToEvent(E_TOUCHEND, URHO3D_HANDLER(CinematicState, HandleInput));

    Game::Get()->SubscribeToAccessMenuEvents();

    UIElement* accessmenu = Game::Get()->GetAccessMenu();
    if (accessmenu)
    {
        SubscribeToEvent(accessmenu, UIMENU_SHOWCONTENT, URHO3D_HANDLER(CinematicState, HandleAccessMenu));
        SubscribeToEvent(accessmenu, UIMENU_HIDECONTENT, URHO3D_HANDLER(CinematicState, HandleAccessMenu));
    }
}

void CinematicState::UnsubscribeToEvents()
{
    if (localScene_ && localScene_->GetScene())
        UnsubscribeFromEvent(localScene_->GetScene(), E_SCENEPOSTUPDATE);
    UnsubscribeFromEvent(E_KEYDOWN);
    UnsubscribeFromEvent(E_MOUSEBUTTONDOWN);
    UnsubscribeFromEvent(E_TOUCHEND);

    UnsubscribeFromEvent(this, CINEMATIC_STOP);

    Game::Get()->UnsubscribeFromAccessMenuEvents();

    UIElement* accessmenu = Game::Get()->GetAccessMenu();
    if (accessmenu)
    {
        UnsubscribeFromEvent(accessmenu, UIMENU_SHOWCONTENT);
        UnsubscribeFromEvent(accessmenu, UIMENU_HIDECONTENT);
    }
}

bool CinematicState::LaunchSceneFile()
{
    Timer timer;

    if (scenefileindex_ >= scenefilenames_.Size())
    {
        URHO3D_LOGERROR("CinematicState() - LaunchSceneFile : Error scenefileindex_ > scenefilenames_.Size() !");
        return false;
    }

    if (localScene_)
        localScene_->RemoveAllChildren();
    else
        localScene_ = GameStatics::rootScene_->CreateChild("LocalScene", LOCAL);

    GameHelpers::LoadNodeXML(context_, localScene_, scenefilenames_[scenefileindex_]);
    if (!localScene_)
    {
        URHO3D_LOGERRORF("CinematicState() - LaunchSceneFile : Error with %s !", scenefilenames_[scenefileindex_].CString());
        return false;
    }

    GameStatics::SetStatus(CINEMATICSTATE);

    float cinematicLength = 0.f;
    animationSpeed_ = 1.f;

    // Standard Mode
    SubscribeToEvent(this, CINEMATIC_STOP, URHO3D_HANDLER(CinematicState, HandleStop));

    // try to get all SceneAnimation2D
    animations_.Clear();
    PODVector<SceneAnimation2D*> sceneAnimations;
    localScene_->GetComponents<SceneAnimation2D>(sceneAnimations, true);
    if (sceneAnimations.Size())
    {
        for (unsigned i=0; i < sceneAnimations.Size(); i++)
        {
            SceneAnimation2D* animation = sceneAnimations[i];
            animation->SetUpdateEventMask(USE_UPDATE);
            animation->GetNode()->SetEnabledRecursive(false);
            animations_.Push(animation);
        }
    }
    // no SceneAnimation2D, try with SCML animations or static images
    else
    {
        PODVector<StaticSprite2D*> staticSprites;
        localScene_->GetDerivedComponents<StaticSprite2D>(staticSprites, true);

        for (unsigned i=0; i < staticSprites.Size(); i++)
        {
            StaticSprite2D* sprite = staticSprites[i];
            if (sprite->GetType() == AnimatedSprite2D::GetTypeStatic())
            {
                AnimatedSprite2D* animatedsprite = static_cast<AnimatedSprite2D*>(sprite);
                if (!animatedsprite->GetAnimationSet())
                    URHO3D_LOGINFOF("CinematicState() - LaunchSceneFile ! Remove animation = %s", animatedsprite->GetNode()->GetName().CString());
                else
                {
                    animations_.Push(sprite);
                    cinematicLength += animatedsprite->GetCurrentAnimationLength();
                }
            }
            else
            {
                cinematicLength += sprite->GetNode()->GetVar(GOA::DURATION).GetFloat();
                animations_.Push(sprite);
            }

            sprite->GetNode()->SetEnabledRecursive(false);
        }

        GameStatics::currentMusic_ = cinematicLength < 10.f ? CINEMATICTHEME1 : CINEMATICTHEME2;
        animationSpeed_ = cinematicLength / (GameStatics::currentMusic_ == CINEMATICTHEME1 ? 8.f : 16.f);
    }

    GameHelpers::SetMusic(MAINMUSIC, 0.25f, GameStatics::currentMusic_, false);

    URHO3D_LOGINFOF("CinematicState() - LaunchSceneFile ! LoadTime = %u currentmusic=%d animationSpeed_=%F cinematicLength=%F",
                    timer.GetMSec(false), GameStatics::currentMusic_, animationSpeed_, cinematicLength);

#ifdef ACTIVE_SPLASHUI
    SendEvent(GAME_LEVELREADY);
#endif

    ChangeToAnimation(0);

    URHO3D_LOGINFOF("CinematicState() - LaunchSceneFile ! Play animation = %s", animation_ ? animation_->GetNode()->GetName().CString() : "null");

    if (animation_)
    {
        paused_ = false;

        if (GameStatics::ui_)
        {
            String cinematicui = "cinematicrootui";
            UIElement* uiroot = GameStatics::ui_->GetRoot()->GetChild(cinematicui) ? GameStatics::ui_->GetRoot()->GetChild(cinematicui) : GameStatics::ui_->GetRoot()->CreateChild<UIElement>(cinematicui, LOCAL);
            if (uiroot)
            {
                uiroot->SetSize(GameStatics::graphics_->GetWidth(), GameStatics::graphics_->GetHeight());
                Game::Get()->CreateAccessMenu(uiroot);
            }
        }

        SubscribeToEvents();

        GameStatics::rootScene_->GetComponent<Renderer2D>()->orthographicMode_ = false;

        URHO3D_LOGINFO("CinematicState() - LaunchSceneFile : ... OK !");
        return true;
    }

    return false;
}

void CinematicState::HandleDelayedStart(StringHash eventType, VariantMap& eventData)
{
//    float delay = 0.5f;

    GameHelpers::ResetCamera();

    if (!scenefilenames_.Size())
    {
        URHO3D_LOGERROR("CinematicState() - HandleDelayedStart : Empty scenefilenames_ !");

        if (SplashScreen::Get())
            SplashScreen::Get()->Stop();

        stateManager_->ReplaceState("LevelMap");
        return;
    }

    scenefileindex_ = -1;

    while (++scenefileindex_ < scenefilenames_.Size() && !LaunchSceneFile()) { }
	if (scenefileindex_ >= scenefilenames_.Size())
    {
        URHO3D_LOGERROR("CinematicState() - HandleDelayedStart : Not OK ... Stop !");
        SendEvent(CINEMATIC_STOP);
    }
}

void CinematicState::ChangeToAnimation(int index)
{
    if (animation_)
        animation_->GetNode()->SetEnabledRecursive(false);

    animationIndex_ = index;
    if (animationIndex_ < animations_.Size())
        animation_ = animations_[animationIndex_];
    else
        animation_.Reset();

    if (animation_)
    {
        animation_->GetNode()->SetEnabledRecursive(true);

        if (animation_->GetType() == AnimatedSprite2D::GetTypeStatic())
        {
            AnimatedSprite2D* animatedSprite = static_cast<AnimatedSprite2D*>(animation_.Get());
            animatedSprite->SetSpeed(animationSpeed_);
            animationDuration_ = animatedSprite->GetCurrentAnimationLength();
        }
        else
        {
            float length = animation_->GetNode()->GetVar(GOA::DURATION).GetFloat();
            if (length == 0.f)
                length = DEFAULTANIMATIONDURATION;

            animation_->GetNode()->SetVar(GOA::DURATION, length * animationSpeed_);
            animationDuration_ = length * animationSpeed_ * 1000.f ;
        }

        GameStatics::ChangeCameraTo(animation_->GetNode()->GetChild("cam1"), false);
        float scale = Clamp(animation_->GetNode()->GetVar(GOA::BONUS).GetFloat(), 1.f, 2.f) / GameStatics::uiScale_;

        if (GameStatics::fScreenSize_.x_ < GameStatics::fScreenSize_.y_ * 0.75f)
            GameHelpers::SetAdjustedToScreen(animation_->GetNode(), scale * 0.65f);
        else
            GameHelpers::SetAdjustedToScreen(animation_->GetNode(), scale, 0.3f, false);

        URHO3D_LOGINFOF("CinematicState() - ChangeToAnimation filename=%s animation=%s duration=%f scale=%s !", scenefilenames_[scenefileindex_].CString(),
                        animation_->GetNode()->GetName().CString(), animationDuration_, animation_->GetNode()->GetWorldScale().ToString().CString());
    }

    timer_.Reset();
}

void CinematicState::Stop()
{
    scenefilenames_.Clear();

    UIElement* uicinematic = GameStatics::ui_->GetRoot()->GetChild(String("cinematicrootui"));
    if (uicinematic)
        uicinematic->Remove();

    if (!stopped_)
    {
        URHO3D_LOGINFO("CinematicState() - Stop !");

        if (localScene_)
            localScene_->SetEnabledRecursive(false);

        GameStatics::ResetCamera();
        GameStatics::rootScene_->GetComponent<Renderer2D>()->orthographicMode_ = true;
        stopped_ = true;
    }

    UnsubscribeToEvents();
}

void CinematicState::HandleSceneUpdate(StringHash eventType, VariantMap& eventData)
{
    if (!localScene_)
        return;

//    URHO3D_LOGINFO("CinematicState() - HandleSceneUpdate !");

    float timestep = eventData[SceneUpdate::P_TIMESTEP].GetFloat();
    unsigned completedpaths = 0;

    if (!paused_)
    {
        if (animation_)
        {
            if (animation_->GetType() == SceneAnimation2D::GetTypeStatic())
            {
                SceneAnimation2D* animation = static_cast<SceneAnimation2D*>(animation_.Get());
                animation->Update(timestep);
                if (animation->IsFinished())
                {
                    URHO3D_LOGINFO("CinematicState() - HandleSceneUpdate ! SceneAnimation2D Finished");
                    ChangeToAnimation(animationIndex_+1);
                }
            }
            else if (animation_->GetType() == AnimatedSprite2D::GetTypeStatic())
            {
                AnimatedSprite2D* animation = static_cast<AnimatedSprite2D*>(animation_.Get());
                if (animation->HasFinishedAnimation())
                {
                    URHO3D_LOGINFO("CinematicState() - HandleSceneUpdate ! AnimatedSprite2D Finished");
                    ChangeToAnimation(animationIndex_+1);
                }
            }
            else if (animation_->GetType() == StaticSprite2D::GetTypeStatic())
            {
                if (timer_.GetMSec(false) > animationDuration_)
                {
                    ChangeToAnimation(animationIndex_+1);
                }
            }
        }
    }

    if (!animation_)
    {
        URHO3D_LOGINFOF("CinematicState() - HandleSceneUpdate ! No More Animation ... Change sceneFile=%d", scenefileindex_+1);

        while (++scenefileindex_ < scenefilenames_.Size() && !LaunchSceneFile()) { }
        if (scenefileindex_ >= scenefilenames_.Size())
        {
            URHO3D_LOGERROR("CinematicState() - HandleSceneUpdate : No More SceneFile ... Stop !");
            SendEvent(CINEMATIC_STOP);
        }
    }
}

void CinematicState::HandleAccessMenu(StringHash eventType, VariantMap& eventData)
{
    URHO3D_LOGINFO("CinematicState() - HandleAccessMenu !");

    if (eventType == UIMENU_SHOWCONTENT)
    {
        SetPaused(true);
    }
    else if (eventType == UIMENU_HIDECONTENT)
    {
        SetPaused(false);
    }
}

void CinematicState::SetPaused(bool state)
{
    paused_ = state;

    GameStatics::rootScene_->SetUpdateEnabled(!paused_);

    if (animation_ && animation_->GetType() == SceneAnimation2D::GetTypeStatic())
        static_cast<SceneAnimation2D*>(animation_.Get())->SetRunning(!paused_);
}

void CinematicState::OnAccessMenuOpenFrame(bool state)
{
    SetPaused(state);
}

void CinematicState::HandleInput(StringHash eventType, VariantMap& eventData)
{
    using namespace KeyDown;
    if (eventType == E_KEYDOWN)
    {
        int scancode = eventData[KeyDown::P_SCANCODE].GetInt();
        // Close console (if open)
        if (scancode == SCANCODE_ESCAPE)
        {
            SendEvent(CINEMATIC_STOP);
        }
        else if (scancode == SCANCODE_SPACE)
        {
            URHO3D_LOGINFO("CinematicState() - HandleInput ! E_KEYDOWN SCANCODE_SPACE Next Animation");
            ChangeToAnimation(animationIndex_+1);
        }
    }
    else if (eventType == E_MOUSEBUTTONDOWN)
    {
        if (eventData[MouseButtonDown::P_BUTTON].GetInt() == MOUSEB_LEFT)
        {
            URHO3D_LOGINFO("CinematicState() - HandleInput ! E_MOUSEBUTTONDOWN MOUSEB_LEFT Next Animation");
            ChangeToAnimation(animationIndex_+1);
        }
    }
    else if (eventType == E_TOUCHEND)
    {
        URHO3D_LOGINFO("CinematicState() - HandleInput ! E_TOUCHEND Next Animation");
        ChangeToAnimation(animationIndex_+1);
    }
}

void CinematicState::HandleStop(StringHash eventType, VariantMap& eventData)
{
    Stop();

    stateManager_->PopStack();
}


