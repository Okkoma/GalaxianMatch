#include <Urho3D/Urho3D.h>

#include <Urho3D/Core/StringUtils.h>
#include <Urho3D/Core/Object.h>
#include <Urho3D/Core/Timer.h>
#include <Urho3D/Core/Context.h>
#include <Urho3D/Core/CoreEvents.h>

#include <Urho3D/Core/Profiler.h>

#include <Urho3D/IO/Log.h>
#include <Urho3D/IO/FileSystem.h>
#include <Urho3D/IO/File.h>

#include <Urho3D/Engine/Console.h>
#include <Urho3D/Engine/DebugHud.h>
#include <Urho3D/Engine/Engine.h>

#include <Urho3D/DebugNew.h>

#include <Urho3D/Resource/Localization.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Resource/XMLFile.h>

#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Scene/SceneEvents.h>
#include <Urho3D/Scene/ObjectAnimation.h>
#include <Urho3D/Scene/ValueAnimation.h>

#include <Urho3D/Graphics/Camera.h>
#include <Urho3D/Graphics/Material.h>
#include <Urho3D/Graphics/Renderer.h>
#include <Urho3D/Graphics/DebugRenderer.h>
#include <Urho3D/Graphics/GraphicsEvents.h>
#include <Urho3D/Graphics/Graphics.h>
#include <Urho3D/Graphics/Light.h>
#include <Urho3D/Graphics/Texture2D.h>

#include <Urho3D/Graphics/Viewport.h>
#include <Urho3D/Graphics/Zone.h>
#include <Urho3D/Graphics/Octree.h>

#include <Urho3D/Urho2D/Renderer2D.h>
#include <Urho3D/Urho2D/Sprite2D.h>
#include <Urho3D/Urho2D/StaticSprite2D.h>
#include <Urho3D/Urho2D/AnimatedSprite2D.h>
#include <Urho3D/Urho2D/PhysicsWorld2D.h>
#include <Urho3D/Urho2D/RigidBody2D.h>
#include <Urho3D/Urho2D/CollisionShape2D.h>

#include <Urho3D/Input/Input.h>

#include <Urho3D/UI/Cursor.h>
#include <Urho3D/UI/Button.h>
#include <Urho3D/UI/CheckBox.h>
#include <Urho3D/UI/Sprite.h>
#include <Urho3D/UI/Font.h>
#include <Urho3D/UI/Text.h>
#include <Urho3D/UI/Text3D.h>
#include <Urho3D/UI/UI.h>
#include <Urho3D/UI/UIEvents.h>
#include <Urho3D/UI/MessageBox.h>
#include <Urho3D/UI/Window.h>
#include <Urho3D/UI/Menu.h>

#include <Urho3D/Audio/Audio.h>
#include <Urho3D/Audio/Sound.h>
#include <Urho3D/Audio/SoundListener.h>

#include "Game.h"
#include "GameAttributes.h"
#include "GameRand.h"
#include "GameEvents.h"
#include "GameStatics.h"
#include "GameHelpers.h"
#include "GameOptions.h"
#include "GameCommands.h"
#include "GameUI.h"

#include "Network.h"

#include "ObjectPool.h"
#include "InteractiveFrame.h"
#include "TextMessage.h"
#include "DelayAction.h"
#include "DelayInformer.h"
#include "TimerRemover.h"
#include "MAN_Matches.h"
#include "Tutorial.h"
#include "BossLogic.h"

#include "sCinematic.h"
#include "sOptions.h"

#include "sPlay.h"

#define ACTIVE_SCREENJOYSTICK

extern int UISIZE[NUMUIELEMENTSIZE];
extern const char* levelModeNames[];
extern const char* matchStateNames[];

DelayAction* action_ = 0;
SharedPtr<MessageBox> messageBox;

WeakPtr<InteractiveFrame> gainframe_, abilitypanel_, gameoverframe_, gameneedmorestarsframe_;

WeakPtr<UIElement> uiplay_;
WeakPtr<Node> boss_;
WeakPtr<BorderImage> triesSprite;
WeakPtr<Text> moveText, triesText, matchStateText;
WeakPtr<Text> objectiveText[MAXOBJECTIVES];
WeakPtr<Text> hiscoreText;

int numObjectives_;
unsigned objectives[MAXOBJECTIVES][2];
int initialtries_, initialmoves_;
bool tipWinLevelEnable_;
bool showHiScore_;
static unsigned hiScore = 0;



PlayState::PlayState(Context* context) :
    GameState(context, "Play"),
    drawDebug_(false),
    ctrlCameraWithMouse_(false)
{
//    URHO3D_LOGINFO("PlayState()");
#ifdef ACTIVE_SPLASHUI
    SetCleanerLocked(false);
#endif
}

PlayState::~PlayState()
{
    URHO3D_LOGINFO("~PlayState()");
}

bool PlayState::Initialize()
{
//    URHO3D_LOGINFO("PlayState() - Initialize");

    seed_ = 0;
	return GameState::Initialize();
}


void PlayState::Begin()
{
    if (IsBegun())
        return;

    GameStatics::SetStatus(PLAYSTATE_INITIALIZING);

    URHO3D_LOGINFO("PlayState() - ---------------------------------------");
    URHO3D_LOGINFO("PlayState() - Begin ...                                -");
    URHO3D_LOGINFO("PlayState() - ---------------------------------------");

    // Get the scene instantiate by Game
    rootScene_ = GameStatics::rootScene_;
    cameraNode_ = GameStatics::cameraNode_;
    scene_ = 0;

    hiScore = GameStatics::playerState_->score;
    showHiScore_ = false;

#ifdef ACTIVE_HISCORE
    SetHiScoreEnable(true);
#endif // ACTIVE_HISCORE

    tipWinLevelEnable_ = false;

    // New Managers Constructs here ...

    rootScene_->StopAsyncLoading();

    rootScene_->GetComponent<Renderer2D>()->orthographicMode_ = false;

    GameState::Begin();

    URHO3D_LOGINFO("PlayState() - ---------------------------------------");
    URHO3D_LOGINFO("PlayState() - Begin ... OK !                         -");
    URHO3D_LOGINFO("PlayState() - ---------------------------------------");
}

void PlayState::End()
{
    URHO3D_LOGINFO("PlayState() - ---------------------------------------");
    URHO3D_LOGINFO("PlayState() - End ...                                   -");
    URHO3D_LOGINFO("PlayState() - ---------------------------------------");

    rootScene_->GetComponent<Renderer2D>()->orthographicMode_ = true;

    UnsubscribeFromEvent(E_POSTUPDATE);

    GameStatics::playerState_->score = hiScore;

    EndScene();

	if (GetSubsystem<UI>())
	    RemoveUI();

	// .. Release Resource Save Files here



    // .. Remove Managers here


    // remove the peerconnection
    Network* network = Network::Get(false);
    if (network && network->GetState() == NetworkConnectionState::Connected)
        network->DisconnectAll();

    GameStatics::peerConnected_ = false;

	GameState::End();

    URHO3D_LOGINFO("PlayState() - ---------------------------------------");
    URHO3D_LOGINFO("PlayState() - End ... OK !                            -");
    URHO3D_LOGINFO("PlayState() - ---------------------------------------");
}

void PlayState::SetSeed(unsigned seed)
{
//    URHO3D_LOGINFOF("PlayState() - SetWorldSeed : %u -", seed);
    seed_ = seed;
}

unsigned PlayState::GetSeed() const
{
    return seed_;
}

InteractiveFrame* PlayState::GetAbilityPanel() const
{
     return abilitypanel_;
}

void PlayState::ResetGameStates()
{
    activeGameLogic_ = false;
    gameOver_ = false;
    paused_ = false;
    MatchesManager::Reset(GetContext());
    Tutorial::Reset(GetContext());
    initialtries_ = GameStatics::tries_;
    initialmoves_ = GameStatics::moves_;
    cameraYaw_ = 0.0f;
    cameraPitch_ = 0.0f;
    GameStatics::numRemainObjectives_ = 0;
}

void PlayState::SetLevelDatas()
{
	if (GameStatics::currentLevelDatas_)
    {
        URHO3D_LOGINFOF("PlayState() - SetLevelDatas : has GameStatics::currentLevelDatas_ !");

        MatchesManager::SetAuthorizedObjects(COT::ENEMIES, GameStatics::currentLevelDatas_->authorizedEnemies_);
        MatchesManager::SetAuthorizedObjects(COT::POWERS, GameStatics::currentLevelDatas_->authorizedPowers_);
        MatchesManager::SetAuthorizedObjects(COT::MOVES, GameStatics::currentLevelDatas_->authorizedMoves_);
        MatchesManager::SetAuthorizedObjects(COT::ROCKS, GameStatics::currentLevelDatas_->authorizedRocks_);
        Match::ROCKCHANCE = GameStatics::currentLevelDatas_->rockChance_;
        GridTile::WALLCHANCE = GameStatics::currentLevelDatas_->wallChance_;

        for (unsigned i=0; i < GameStatics::currentLevelDatas_->objectiveTypes_.Size() ; i++)
        {
            objectives[i][0] = GameStatics::currentLevelDatas_->objectiveQties_[i];
            objectives[i][1] = GameStatics::currentLevelDatas_->objectiveTypes_[i];
            MatchesManager::RegisterObjective(GameStatics::currentLevelDatas_->objectiveNames_[i], GameStatics::currentLevelDatas_->objectiveQties_[i]);
            GameStatics::numRemainObjectives_ += objectives[i][0];
        }

        MatchesManager::SetLayout(GameStatics::currentLevelDatas_->layoutSize_, (GridLayout)GameStatics::currentLevelDatas_->layoutShape_);
        GameStatics::SetLevelInfo(GameStatics::currentLevel_, GameStatics::currentLevelDatas_->levelInfoID_);
        GameStatics::currentMusic_ = GameStatics::currentLevelDatas_->musicThemeId_;
    }
    else
    {
        // Authorized Ennemy types
        {
            Vector<StringHash> enemies;
            enemies.Push(StringHash("Invader4"));
            enemies.Push(StringHash("Invader5"));
            enemies.Push(StringHash("Invader6"));
            enemies.Push(StringHash("Invader8"));
            MatchesManager::SetAuthorizedObjects(COT::ENEMIES, enemies);
        }
        // Authorized Power types
        {
            Vector<StringHash> powers;
            if (GameStatics::IsBossLevel())
            {
                // just enable bombs hv, h & v
                const StringHash POWERBOMBH("power_bombh");
                const StringHash POWERBOMBV("power_bombv");
                const StringHash POWERBOMBHV("power_bombhv");
                for (unsigned i=0; i < MAXABILITIES; i++)
                {
                    if (GameStatics::playerState_->powers_[i].enabled_)
                    {
                        const StringHash& power = COT::GetTypeFromCategory(COT::POWERS, i);
                        if (power == POWERBOMBH || power == POWERBOMBV || power == POWERBOMBHV)
                            powers.Push(power);
                    }
                }
            }
            else
            {
                for (unsigned i=0; i < MAXABILITIES; i++)
                {
                    if (GameStatics::playerState_->powers_[i].enabled_)
                    {
                        const StringHash& power = COT::GetTypeFromCategory(COT::POWERS, i);
                        if (power.Value() != 0)
                            powers.Push(power);
                    }
                }
            }

            MatchesManager::SetAuthorizedObjects(COT::POWERS, powers);
        }
        // Authorized Bonus Move types
        {
            Vector<StringHash> moves;
            moves.Push(StringHash("move2"));
            MatchesManager::SetAuthorizedObjects(COT::MOVES, moves);
        }
        // Authorized Rock types : cap to level 5 max
        {
            Vector<StringHash> rocks;
            rocks.Push(StringHash("Rock1"));
            rocks.Push(StringHash("Rock2"));
            rocks.Push(StringHash("Rock3"));
            MatchesManager::SetAuthorizedObjects(COT::ROCKS, rocks);

            Match::ROCKCHANCE = Max(0, Min(GameStatics::currentZone_-2, 5)) * 2;
        }
        // Wall Chance : cap to level 5 max
        {
            GridTile::WALLCHANCE = Max(0, Min(GameStatics::currentZone_-2, 5)) * 2;
        }

        numObjectives_ = -1;
        int layoutshape = -1;
        int layoutsize = -1;

        // define objectives
        if (GameStatics::IsBossLevel())
        {
            int bossid = GameStatics::GetCurrentBoss();

            layoutshape = L_MAXSTANDARDLAYOUT + bossid;
            numObjectives_ = 1;
            Node* templatenode = GOT::GetObject(StringHash(ToString("Boss%d", bossid)));
            objectives[0][0] = templatenode ? templatenode->GetDerivedComponent<BossLogic>()->GetLife() : 20;
            objectives[0][1] = 0;
            MatchesManager::RegisterObjective(ToString("Boss%d_Static", bossid), objectives[0][0]);
            GameStatics::numRemainObjectives_ = objectives[0][0];

            GameStatics::currentMusic_ = BOSSTHEME1 + bossid % MAXBOSSTHEMES;

            URHO3D_LOGINFOF("PlayState() - SetLevelDatas : Boss Level id = %d numRemainObjectives_ = %d !", bossid, GameStatics::numRemainObjectives_);
        }
        else
        {
            int totalitems;

            GameStatics::SetLevelParameters(seed_, GameStatics::currentLevel_, MatchesManager::GetAuthorizedObjects(COT::ENEMIES).Size(), layoutshape, layoutsize, numObjectives_, totalitems);

            for (int i=0; i < numObjectives_; i++)
            {
                // num items goal
                objectives[i][0] = totalitems / numObjectives_;

                // index type
                StringHash got = MatchesManager::GetAuthorizedObjectByIndex(COT::ENEMIES, i);
                objectives[i][1] = got.Value();
                MatchesManager::RegisterObjective(got, objectives[i][0]);
                GameStatics::numRemainObjectives_ += objectives[i][0];
            }

            GameStatics::currentMusic_ = PLAYTHEME1 + GameStatics::currentLevel_ % MAXPLAYTHEMES;

            URHO3D_LOGINFOF("PlayState() - SetLevelDatas : numRemainObjectives_ = %d !", GameStatics::numRemainObjectives_);
        }

        // set layout
        MatchesManager::SetLayout(layoutsize, (GridLayout)layoutshape);
    }
}

void PlayState::BeginNewLevel(GameLevelMode mode, unsigned seed)
{
    GameStatics::SetConsoleVisible(false);

    GameStatics::ResetGameStates();

    URHO3D_LOGINFO("PlayState() - ---------------------------------------");
    URHO3D_LOGINFOF("PlayState() - BeginNewLevel : %s seed=%u          -", levelModeNames[mode], seed);
    URHO3D_LOGINFO("PlayState() - ---------------------------------------");

    SetSeed(seed);

    CreateLevel(mode == LVL_CHANGE);
}

void PlayState::CreateLevel(bool updatelevel)
{
//    stateManager_->SetActiveState("Play");

    if (updatelevel)
    {
        if (GameStatics::currentLevel_ >= NBMAXLVL)
            return;

        GameStatics::UpdateLevel();
    }
    else
        GameStatics::SetLevel();

    GameState::Begin();

    //sceneCleaner_.AddStep(GetStateId(), 0, true);
}

void PlayState::Launch()
{
    URHO3D_PROFILE(Launch);

    GameStatics::SetStatus(PLAYSTATE_INITIALIZING);

    URHO3D_LOGINFOF("PlayState() - Launch : Level %d ...", GameStatics::currentLevel_);

    InitRandomizer();

    ResetGameStates();
    SetLevelDatas();

    if (!CreateScene())
    {
        SubscribeToEvent(E_UPDATE, URHO3D_HANDLER(PlayState, HandleStop));
        return;
    }

    // Stop the update of the physics (prevent crash)
    rootScene_->GetComponent<PhysicsWorld2D>()->SetUpdateEnabled(false);

    // Stop Delayed action
    if (action_)
    {
        action_->Free();
        action_ = 0;
    }

    // Play Theme
    GameHelpers::SetMusic(MAINMUSIC, 0.7f, GameStatics::currentMusic_, true);

    // Stop Residual Boss appears from previous level
    UnsubscribeFromEvent(GAME_BOSSAPPEARS);

    // Set UI
    TextMessage::FreeAll();
    CreateUI();

    URHO3D_LOGINFOF("PlayState() - Launch : Level %d ... OK !", GameStatics::currentLevel_);

    // Finish Initialize
    GameStatics::SetStatus(PLAYSTATE_LOADING);

    // Subscribe to SceneLoading
    SubscribeToEvent(E_UPDATE, URHO3D_HANDLER(PlayState, HandleSceneLoading));

    // Subscribe to SplashScreen
    // when sceneloading finishes => Scene Appears when splashscreen starts to open
    SubscribeToEvent(SPLASHSCREEN_STARTOPEN, URHO3D_HANDLER(PlayState, HandleSceneAppears));

    SendEvent(GAME_STATEREADY);
}

void PlayState::InitRandomizer()
{
    // Initialize Random System
    unsigned seed = 0;

    if (seed_)
        seed = seed_;
//    else if (GameStatics::randomMode_ == RANDOM_TIME)
//        seed = Time::GetSystemTime();
//    else if (GameStatics::randomMode_ == RANDOM_FIXED)
//        seed = GameStatics::gameseed_;

    URHO3D_LOGINFOF("PlayState() - InitRandomizer : seed=%u level=%d", seed, GameStatics::currentLevel_);

    srand(seed);
    GameRand::SetSeedRand(ALLRAND, seed + GameStatics::currentLevel_);
}

bool PlayState::CreateScene()
{
    URHO3D_LOGINFO("PlayState() - CreateScene");

//    GameStatics::cameraNode_->SetPosition(Vector3::ZERO);

//    LOGINFOF("PlayState() - CreateScene : Dump vars = %s", rootScene_->GetVarNamesAttr().CString());

    if (GameStatics::gameConfig_.debugRenderEnabled_)
        rootScene_->GetOrCreateComponent<DebugRenderer>(LOCAL);

    if (GameStatics::gameConfig_.physics2DEnabled_)
    {
        PhysicsWorld2D* physicsWorld2D_ = rootScene_->GetOrCreateComponent<PhysicsWorld2D>(LOCAL);
        if (physicsWorld2D_)
        {
            if (GameStatics::gameConfig_.debugRenderEnabled_)
            {
                physicsWorld2D_->SetDrawJoint(true); // Display the joints
//                physicsWorld2D_->SetDrawAabb(true);
                physicsWorld2D_->SetDrawShape(true);
//                physicsWorld2D_->SetDrawCenterOfMass(true);
            }
//            physicsWorld2D_->SetVelocityIterations(5);
//            physicsWorld2D_->SetPositionIterations(2);
            physicsWorld2D_->SetContinuousPhysics(true);
            physicsWorld2D_->SetSubStepping(false);
            physicsWorld2D_->SetWarmStarting(true);
            physicsWorld2D_->SetAutoClearForces(true);
            // no gravity
            physicsWorld2D_->SetGravity(Vector2::ZERO);
        }
        else
            URHO3D_LOGERROR("PlayState() - CreateScene : physicsWorld2D_ NOT CREATE !");
    }

    // Create Structure Nodes for the scene
    scene_ = rootScene_->CreateChild("Scene", LOCAL);
    // Create Pool Node for GoPools
    Node* nodePool = rootScene_->CreateChild("Pool", LOCAL);
    // staticScene is for collisionShape generated by MapCollider2D
    rootScene_->CreateChild("StaticScene", LOCAL);

    // ... Start Managers Here
    //GO_Pools::Start(nodePool);

    // Load Scene
//    URHO3D_LOGINFOF("PlayState() - CreateScene : ... Load Scene for level=%d", GameStatics::currentLevel_);
//    if (!GameHelpers::LoadSceneXML(context_, scene_, LOCAL))
//    {
//        URHO3D_LOGERROR("PlayState() - CreateScene NOK !");
//        return false;
//    }

    // Setting Nodes Scene / World Scene
    // Default Zone for the moment
    Node* zoneNode = scene_->GetChild("Zone");
    if (!zoneNode)
    {
        zoneNode = scene_->CreateChild("Zone", LOCAL);
        Zone* zone = zoneNode->CreateComponent<Zone>(LOCAL);
        zone->SetBoundingBox(BoundingBox(-1000.0f, 1000.0f));
        zone->SetAmbientColor(Color(0.02f, 0.05f, 0.05f));
        zone->SetFogColor(Color(0.09f, 0.09f, 0.1f));
        zone->SetFogStart(10.0f);
        zone->SetFogEnd(100.0f);
    }

    if (GameStatics::gameConfig_.touchEnabled_)
        GetSubsystem<Input>()->SetScreenJoystickVisible(GameStatics::gameConfig_.screenJoystickID_, true);

    URHO3D_LOGINFO("PlayState() - CreateScene : OK");
    return true;
}

void PlayState::EndScene()
{
    URHO3D_LOGINFO("PlayState() - EndScene ... ");

    if (GameStatics::gameConfig_.touchEnabled_)
        GetSubsystem<Input>()->SetScreenJoystickVisible(GameStatics::gameConfig_.screenJoystickID_, false);

    if (action_)
    {
        action_->Free();
        action_ = 0;
    }

    GameHelpers::StopMusics();

    UnsubscribeToEvents();

    SetVisibleUI(false);

    InteractiveFrame::FreeAll();
    TextMessage::FreeAll();

    // ... Stop Managers here

    MatchesManager::Clear();

    URHO3D_LOGINFO("PlayState() - EndScene ... Stop Events & Managers OK ! ...");

    GameHelpers::CleanScene(GameStatics::rootScene_, GetStateId(), 0);

//    GetSubsystem<Input>()->ResetStates();

    URHO3D_LOGINFO("PlayState() - EndScene ... OK !");

    GameStatics::SetStatus(PLAYSTATE_INITIALIZING);
}

void PlayState::Pause(bool enable, bool addmessagebox, bool autoUnpause)
{
    URHO3D_LOGINFOF("PlayState() - Pause : %s", enable ? "true":"false");

    if (!enable)
    {
        paused_ = false;
        rootScene_->SetUpdateEnabled(true);

        SetBossActive(enable);

        UnsubscribeFromEvent(messageBox, E_MESSAGEACK);
        UnsubscribeFromEvent(GAME_UNPAUSE);
        messageBox.Reset();

        GameHelpers::SetMusicVolume(1.f);
        GameHelpers::SetSoundVolume(1.f);
    }
    else
    {
        paused_ = true;
        rootScene_->SetUpdateEnabled(false);

        if (addmessagebox)
        {
            messageBox = SharedPtr<MessageBox>(GameHelpers::AddMessageBox("gamepause", "continue", true, "yes", "no", this, URHO3D_HANDLER(PlayState, OnPauseMessageAck)));
            messageBox->AddRef();
        }
        if (autoUnpause)
        {
            SubscribeToEvent(GAME_UNPAUSE, URHO3D_HANDLER(PlayState, HandlePause));
        }

        GameHelpers::SetMusicVolume(0.f);
        GameHelpers::SetSoundVolume(0.f);
    }
}

void PlayState::CheckStars(bool loosetry)
{
    if (MatchesManager::GetState() != NoMatchState)
        return;

    if (loosetry)
        GameStatics::tries_ = Max(0, GameStatics::tries_-1);

    if (!GameStatics::tries_)
    {
        GameStatics::SetStatus(PLAYSTATE_ENDGAME);

        URHO3D_LOGINFOF("PlayState() - CheckStars : No More Star !");

        GameStatics::moves_ = 0;
        SendEvent(GAME_MOVEREMOVED);
        gameOver_ = true;
        hiScore = 0;

        GameStatics::AllowInputs(false);
        activeGameLogic_ = false;
        Tutorial::Get()->SetEnabled(false);

        Localization* l10n = GetSubsystem<Localization>();
        GameHelpers::AddUIMessage(l10n->Get("notry"), false, "Fonts/BlueHighway.ttf", UISIZE[FONTSIZE_BIG], Color::WHITE,
                                  IntVector2(0, MESSAGEPOSITIONYRATIO * GameStatics::ui_->GetRoot()->GetSize().y_), 2.3f);

        // Need More Stars Frame
        SubscribeToEvent(this, GAME_UIFRAME_ADD, URHO3D_HANDLER(PlayState, OnGameNeedMoreStars));
        DelayInformer::Get(this, 2.5f, GAME_UIFRAME_ADD);
    }
    else
    {
        URHO3D_LOGINFOF("PlayState() - CheckStars !");

        gameOver_ = false;

        GameStatics::AllowInputs(true);
        activeGameLogic_ = true;
        Tutorial::Get()->SetEnabled(true);

        SendEvent(GAME_TRYREMOVED);
        GameStatics::moves_ = initialMoves-1;
        SendEvent(GAME_MOVEADDED);
    }
}

void PlayState::LevelWin()
{
    URHO3D_LOGINFO("PlayState() - LevelWin ! ");

    GameStatics::AllowInputs(false);
    activeGameLogic_ = false;
    Tutorial::Get()->SetEnabled(false);

    GameHelpers::SetMusic(MAINMUSIC, 0.1f);
    GameHelpers::SetMusic(SECONDARYMUSIC, 1.f, GameStatics::playerState_->level % 2 ? WINTHEME:WINTHEME2, false);
    GameHelpers::SetSoundVolume(0.1f);

    DelayInformer::Get(this, 0.4f, GAME_LEVELWIN);
    SubscribeToEvent(this, GAME_LEVELWIN, URHO3D_HANDLER(PlayState, OnLevelWin));
}

void PlayState::CreateUI()
{
    ResourceCache* cache = GetSubsystem<ResourceCache>();

    const int width = GameStatics::ui_->GetRoot()->GetSize().x_;
    const int height = GameStatics::ui_->GetRoot()->GetSize().y_;
    const int border = 20 * GameStatics::uiScale_;

	if (!uiplay_)
    {
        URHO3D_LOGINFO("PlayState() - CreateUI ... ");

        // Load XML file containing default UI style sheet
        XMLFile* style = cache->GetResource<XMLFile>("UI/DefaultStyle.xml");

        // Set the loaded style as default style
        UIElement* uiroot = GetSubsystem<UI>()->GetRoot();
        uiroot->SetDefaultStyle(style);

        uiplay_ = uiroot->CreateChild<UIElement>("playrootui");
        uiplay_->SetSize(width, height);
    }

    // Create UI Buttons
    UIElement* uibuttons = uiplay_->GetChild(String("uibuttons"));
    if (!uibuttons)
        uibuttons = uiplay_->CreateChild<UIElement>(String("uibuttons"));

    // Singe activated
    if (GameStatics::gameState_.storyitems_[1])
    {
        // Add Shake button
        Button* button = uibuttons->GetChildStaticCast<Button>(String("shake"));
        if (!button)
        {
            Button* button = uibuttons->CreateChild<Button>("shake");
            button->SetImage("2D/tiles.xml@shake");
            button->SetSize(IntVector2(80, 80));
            button->SetPriority(499);
            button->SetOpacity(0.9f);
            URHO3D_LOGINFOF("PlayState() - CreateUI : add singe shake button !");
        }
    }
    // Create Tries Button
    if (!triesSprite)
    {
        triesSprite = uibuttons->CreateChild<BorderImage>();
        triesSprite->SetImage("2D/tiles.xml@item_star");
        triesSprite->SetSize(UISIZE[TRIESUISIZE], UISIZE[TRIESUISIZE]);
        triesSprite->SetPosition(20.f, 1.f);
        triesSprite->SetOpacity(0.8f);
        triesSprite->SetPriority(499);
    }

    if (GameStatics::tries_ <= 1)
        triesSprite->SetVisible(false);

    // Create Move Button
    if (!uibuttons->GetChild(String("movebutton")))
    {
        Button* movebutton = uibuttons->CreateChild<Button>();
        movebutton->SetName("movebutton");
        movebutton->SetImage("2D/tiles.xml@button1");
        movebutton->SetEnableAnchor(true);
        movebutton->SetMinOffset(IntVector2(border, UISIZE[TRIESUISIZE]+5));
        movebutton->SetMaxOffset(IntVector2(border+UISIZE[BUTTONUISIZE], UISIZE[TRIESUISIZE]+5+UISIZE[BUTTONUISIZE]));
        movebutton->SetOpacity(0.5f);
        movebutton->SetPriority(499);
    }

    // Create objectives zone
    UIElement* objectivezone = uibuttons->GetChild(String("objectivezone"));
    if (objectivezone)
        objectivezone->RemoveAllChildren();
    else
        objectivezone = uibuttons->CreateChild<UIElement>("objectivezone");

    // Create Objectives
    const bool bosslevel = GameStatics::IsBossLevel();
    const int uisize = UISIZE[bosslevel ? BIGOBJECTIVEUISIZE : OBJECTIVEUISIZE];
    const int fontsize = UISIZE[FONTSIZE_NORMAL];
    for (int i=0; i < numObjectives_; i++)
    {
        Button* objbutton = objectivezone->CreateChild<Button>();
        objbutton->SetName("objectivebutton_"+String(i));
        objbutton->SetImage("2D/tiles.xml@button1");
        objbutton->SetEnableAnchor(true);
        objbutton->SetMinOffset(IntVector2(width - uisize * (numObjectives_-i), fontsize + 10));
        objbutton->SetMaxOffset(IntVector2(width - uisize * (numObjectives_-i) + uisize, fontsize + 10 + uisize));
        objbutton->SetOpacity(0.8f);
        objbutton->SetPriority(499);

        BorderImage* objectiveImage = objbutton->CreateChild<BorderImage>();
        objectiveImage->SetSize(uisize-4, uisize-4);
        objectiveImage->SetPivot(Vector2(0.5f, 0.5f));
        objectiveImage->SetPosition(objbutton->GetSize() / 2);
        objectiveImage->SetPriority(500);

        if (!bosslevel)
        {
            Node* objectivenode = GOT::GetObject(StringHash(objectives[i][1]));
            if (objectivenode)
                objectiveImage->SetImage(objectivenode->GetComponent<StaticSprite2D>()->GetSprite());
        }
        else
        {
            int bossid = GameStatics::GetCurrentBoss();
            if (bossid != -1)
                objectiveImage->SetImage(ToString("2D/bossestatics.xml@boss%d_static", bossid));
        }
    }
    if (bosslevel)
        objectivezone->SetOpacity(0.f);

    URHO3D_LOGINFOF("PlayState() - CreateUI - Objectives added !");

    // Create UI Texts
    UIElement* uitexts = uiplay_->GetChild(String("uitexts"));
    if (!uitexts)
        uitexts = uiplay_->CreateChild<UIElement>(String("uitexts"));

    Font* font = cache->GetResource<Font>("Fonts/BlueHighway.ttf");
    // Create Debug Match State
    if (!matchStateText)
    {
        matchStateText = uitexts->CreateChild<Text>();
        matchStateText->SetFont(font, UISIZE[FONTSIZE_SMALL]);
        matchStateText->SetEnableAnchor(true);
        matchStateText->SetMinOffset(IntVector2(width/2, 100));
        matchStateText->SetMaxOffset(IntVector2(width/2, 100));
        matchStateText->SetPivot(0.f, 0.f);
        matchStateText->SetTextAlignment(HA_CENTER);
        matchStateText->SetColor(Color::GREEN);
        matchStateText->SetPriority(501);
    }
    if (!triesText)
    {
        triesText = uitexts->CreateChild<Text>();
        triesText->SetFont(font, UISIZE[FONTSIZE_SMALL]);
        triesText->SetEnableAnchor(true);
        triesText->SetMinOffset(IntVector2(UISIZE[TRIESUISIZE]+border, UISIZE[TRIESUISIZE]/2-7));
        triesText->SetMaxOffset(IntVector2(UISIZE[TRIESUISIZE]+border, UISIZE[TRIESUISIZE]/2-7));
        triesText->SetTextAlignment(HA_CENTER);
        triesText->SetText(String(GameStatics::tries_-1));
        triesText->SetPriority(501);
    }

    if (GameStatics::tries_ <= 1)
        triesText->SetVisible(false);

    // Create Level Text
//    if (!uitexts->GetChild(String("level")))
//    {
//        Text* levelText = uitexts->CreateChild<Text>();
//        levelText->SetName("level");
//        levelText->SetFont(font, UISIZE[FONTSIZE_SMALL]);
//        levelText->SetEnableAnchor(true);
//        levelText->SetMinOffset(IntVector2(width-border, 5));
//        levelText->SetMaxOffset(IntVector2(width-border, 5));
//        levelText->SetPivot(0.75f, 0.25f);
//        levelText->SetTextAlignment(HA_RIGHT);
//
//        levelText->SetText("Mission "+String(GameStatics::currentLevel_));
//    }

    // Create Objectives Quantities texts
    for (int i=0; i < MAXOBJECTIVES; i++)
    {
        if (objectiveText[i])
            objectiveText[i]->SetVisible(false);
    }

    for (int i = 0; i < numObjectives_; i++)
    {
        WeakPtr<Text>& otext = objectiveText[i];
        if (!otext)
        {
            otext = uitexts->CreateChild<Text>();
            UIElement* button = objectivezone->GetChild(i);
            otext->SetFont(font, fontsize);
            otext->SetPosition(button->GetPosition().x_ + button->GetSize().x_ / 2, button->GetPosition().y_ + button->GetSize().y_ - fontsize/2);
            otext->SetPivot(Vector2(0.5f, 0.5f));
            otext->SetTextAlignment(HA_CENTER);
            otext->SetPriority(502);
            if (bosslevel)
                otext->SetOpacity(0.f);
        }

        otext->SetText(String(objectives[i][0]));
        otext->SetColor(Color::WHITE);
        otext->SetVisible(true);
    }

    if (!moveText)
    {
        Button* button = uibuttons->GetChildStaticCast<Button>(String("movebutton"));
        moveText = uitexts->CreateChild<Text>();
        moveText->SetFont(font, fontsize);
        moveText->SetPosition(button->GetPosition().x_ + button->GetSize().x_ / 2, button->GetPosition().y_ + button->GetSize().y_ / 2);
        moveText->SetPivot(Vector2(0.5f, 0.5f));
        moveText->SetTextAlignment(HA_CENTER);
        moveText->SetText(String(GameStatics::moves_));
        moveText->SetPriority(502);
    }

    // Create Score Text
    if (!hiscoreText)
    {
        hiscoreText = uitexts->CreateChild<Text>();
        hiscoreText->SetFont(font, fontsize);
        hiscoreText->SetEnableAnchor(true);
        hiscoreText->SetMinOffset(IntVector2(width/2, 5));
        hiscoreText->SetMaxOffset(IntVector2(width/2, 5));
        hiscoreText->SetPivot(0.f, 0.f);
        hiscoreText->SetTextAlignment(HA_CENTER);
        hiscoreText->SetColor(C_BOTTOMLEFT, Color(1, 1, 0.25));
        hiscoreText->SetColor(C_BOTTOMRIGHT, Color(1, 1, 0.25));
        hiscoreText->SetVisible(showHiScore_);
        hiscoreText->SetPriority(502);
    }

    // TEST NETWORK : Create Duo CheckBox
    CheckBox* duocheck = uibuttons->GetChildStaticCast<CheckBox>(String("duo"));
    if (!duocheck)
    {
        duocheck = uibuttons->CreateChild<CheckBox>();
        duocheck->SetName("duo");
        duocheck->SetTexture(GetSubsystem<ResourceCache>()->GetResource<Texture2D>("UI/accessmenu.webp"));
        duocheck->SetImageRect(IntRect(640, 0, 768, 128));
//        duocheck->SetHoverOffset(0, 128);
        duocheck->SetCheckedOffset(0, 128);
        duocheck->SetEnableAnchor(true);
        duocheck->SetMinOffset(IntVector2(border, height-UISIZE[MBUTTONUISIZE]-border));
        duocheck->SetMaxOffset(IntVector2(border+UISIZE[MBUTTONUISIZE], height-border));
        duocheck->SetOpacity(0.9f);
        duocheck->SetPriority(499);
    }
    // update counter texts
    if (showHiScore_)
        hiscoreText->SetText(String(hiScore));

    // Add Abilities panel
    if (!abilitypanel_)
    {
        abilitypanel_ = InteractiveFrame::Get("UI/AbilityPanel/AbilityPanel.xml");
        abilitypanel_->SetSelectionMode(SELECTIONONCLICK);
    }

    // Update Abilities panel
    abilitypanel_->SetBehavior(IB_ABILITIES);

    Game::Get()->CreateAccessMenu(uiplay_);

    ResizeUI(false);

    SetVisibleUI(false);
}

void PlayState::ResizeUI(bool instant)
{
    const int width = GameStatics::ui_->GetRoot()->GetSize().x_;
    const int height = GameStatics::ui_->GetRoot()->GetSize().y_;
    const int border = 20 * GameStatics::uiScale_;

    uiplay_ = GameStatics::ui_->GetRoot()->GetChild(String("playrootui"));
    uiplay_->SetSize(width, height);
    UIElement* uitexts = uiplay_->GetChild(String("uitexts"));
    UIElement* uibuttons = uiplay_->GetChild(String("uibuttons"));

//    Text* levelText = static_cast<Text*>(uitexts->GetChild(String("level")));
//    if (levelText)
//    {
//        levelText->SetMinOffset(IntVector2(width-border, 5));
//        levelText->SetMaxOffset(IntVector2(width-border, 5));
//    }

    if (GameStatics::gameState_.storyitems_[1])
    {
        // Resize Shake button
        Button* button = uibuttons->GetChildStaticCast<Button>(String("shake"));
        button->SetPosition(width - button->GetSize().x_ - border/2, height - button->GetSize().y_ - border/2);
        //button->SetSize(IntVector2(80 * GameStatics::uiScale_, 80 * GameStatics::uiScale_));
    }

    if (triesSprite)
        triesSprite->SetPosition(20.f, 1.f);

    Button* movebutton = uibuttons->GetChildStaticCast<Button>(String("movebutton"));
    if (movebutton)
    {
        movebutton->SetMinOffset(IntVector2(border, UISIZE[TRIESUISIZE]+5));
        movebutton->SetMaxOffset(IntVector2(border+UISIZE[BUTTONUISIZE], UISIZE[TRIESUISIZE]+5+UISIZE[BUTTONUISIZE]));
    }

    UIElement* objectivezone = uibuttons->GetChild(String("objectivezone"));
    const bool bosslevel = GameStatics::IsBossLevel();
    for (int i=0; i< numObjectives_; i++)
    {
        const int uisize = UISIZE[bosslevel ? BIGOBJECTIVEUISIZE : OBJECTIVEUISIZE];
        Button* button = objectivezone->GetChildStaticCast<Button>(i);
        button->SetMinOffset(IntVector2(width-uisize*(numObjectives_-i), UISIZE[FONTSIZE_SMALL]+10));
        button->SetMaxOffset(IntVector2(width-uisize*(numObjectives_-i) + uisize, UISIZE[FONTSIZE_SMALL] + 10 + uisize));
    }

    if (triesText)
    {
        triesText->SetMinOffset(IntVector2(UISIZE[TRIESUISIZE]+border, UISIZE[TRIESUISIZE]/2-7));
        triesText->SetMaxOffset(IntVector2(UISIZE[TRIESUISIZE]+border, UISIZE[TRIESUISIZE]/2-7));
    }

    if (moveText && movebutton)
        moveText->SetPosition(movebutton->GetPosition().x_ + movebutton->GetSize().x_ / 2, movebutton->GetPosition().y_ + movebutton->GetSize().y_ / 2);

    if (!hiscoreText)
    {
        hiscoreText->SetMinOffset(IntVector2(width/2, 5));
        hiscoreText->SetMaxOffset(IntVector2(width/2, 5));
    }

    for (int i=0; i< numObjectives_; i++)
    {
        Text* text = objectiveText[i];
        Button* button = objectivezone->GetChildStaticCast<Button>(i);
        text->SetPosition(button->GetPosition().x_ + button->GetSize().x_ / 2, button->GetPosition().y_ + button->GetSize().y_ - text->GetFontSize()/2);
    }

    // TEST NETWORK : Create Duo Button
    CheckBox* duocheck = uibuttons->GetChildStaticCast<CheckBox>(String("duo"));
    if (duocheck)
        duocheck->SetPosition(width - duocheck->GetSize().x_ - border/2, height - duocheck->GetSize().y_ - border/2);

    ResizeAbilityPanel(instant);
}

void PlayState::ResizeAbilityPanel(bool instant)
{
    // Update Abilities panel
    if (abilitypanel_)
    {
        Graphics* graphics = GetSubsystem<Graphics>();
        UIElement* menubutton = uiplay_->GetChild(String("accessmenu"));

        int menubuttonposition = (menubutton->GetPosition().y_) * GameStatics::uiScale_;
        int abilitysize = 60 * GameStatics::uiScale_;

        URHO3D_LOGINFOF("PlayState() - ResizeAbilityPanel : menubuttonposition=%d ", menubuttonposition);

        abilitypanel_->SetLayer(ABILITYLAYER);
        abilitypanel_->SetScreenPosition(IntVector2(graphics->GetWidth()/2, /*menubuttonposition*/graphics->GetHeight() - abilitysize), instant);
        abilitypanel_->SetScreenPositionEntrance(IntVector2(graphics->GetWidth()/2, graphics->GetHeight() + abilitysize));
        abilitypanel_->SetScreenPositionExit(0, IntVector2(graphics->GetWidth()/2, graphics->GetHeight() + abilitysize));
        abilitypanel_->SetAbilityMoveZone(-1, 1, 0.3f);

        // Update the ScreenSize parameter for the shader "FadeOutside" applied on the abilities
        Node* node = GOT::GetObject(StringHash("Abilities"));
        AnimatedSprite2D* sprite = node->GetComponent<AnimatedSprite2D>();
        Material* material = sprite->GetCustomMaterial();
        if (material)
        {
            material->SetShaderParameter("ScreenSize", Vector2(graphics->GetWidth(), graphics->GetHeight()));
            URHO3D_LOGINFOF("PlayState() - ResizeAbilityPanel : update screensize !");
        }
    }
}

void PlayState::SetVisibleUI(bool state)
{
    if (uiplay_)
    {
        uiplay_->SetVisible(state);
    }

    if (gainframe_)
    {
        if (!state)
            gainframe_->Stop();
    }
}

void PlayState::RemoveUI()
{
    URHO3D_LOGINFO("PlayState() - RemoveUI ... ");

    UIElement* root = GetSubsystem<UI>()->GetRoot();
    if (!root)
        return;

	if (uiplay_)
    {
        root->RemoveChild(uiplay_);
        uiplay_.Reset();
    }

    if (abilitypanel_)
    {
        abilitypanel_->Clean();
        abilitypanel_.Reset();
    }

    if (gainframe_)
    {
        gainframe_->Clean();
        gainframe_.Reset();
    }

    if (gameoverframe_)
    {
        gameoverframe_->Clean();
        gameoverframe_.Reset();
    }

    if (gameneedmorestarsframe_)
    {
        gameneedmorestarsframe_->Clean();
        gameneedmorestarsframe_.Reset();
    }

    root->DisableLayoutUpdate();

    // ...

    root->EnableLayoutUpdate();

    root->UpdateLayout();

    URHO3D_LOGINFO("PlayState() - RemoveUI ... OK !");
}

void PlayState::SetHiScoreEnable(bool enable)
{
    if (enable != showHiScore_)
    {
        showHiScore_ = enable;

        if (hiscoreText)
        {
            hiscoreText->SetVisible(enable);

            if (GameStatics::GetStatus() == PLAYSTATE_RUNNING)
            {
                if (enable)
                    SubscribeToEvent(GAME_SCORECHANGE, URHO3D_HANDLER(PlayState, HandleUpdateScores));
                else
                    UnsubscribeFromEvent(GAME_SCORECHANGE);
            }
        }
    }
}

void PlayState::SaveGame()
{
    URHO3D_LOGINFO("PlayState() - SaveGame : ...");



    URHO3D_LOGINFO("PlayState() - SaveGame : ... OK !");
}

void PlayState::SetBossActive(bool enable)
{
    if (!boss_)
        return;

    if (enable && !boss_->IsEnabled())
        boss_->SetEnabled(true);

    boss_->GetDerivedComponent<BossLogic>()->SetActive(boss_->IsEnabled() && GameStatics::allowInputs_, false);
}

void PlayState::AddRandomBossWarning()
{
    float appearDelay = 3.f;
    SubscribeToEvent(GAME_BOSSAPPEARS, URHO3D_HANDLER(PlayState, HandleBossAppears));

    if (GameStatics::GetCurrentBoss() == 1 || GameRand::GetRand(ALLRAND, 100) > 25)
    {
        GameHelpers::AddUIMessage("Warning", true, "Fonts/BlueHighway.ttf", UISIZE[FONTSIZE_BIG], Color::WHITE,
                                IntVector2(0, MESSAGEPOSITIONYRATIO * GameStatics::ui_->GetRoot()->GetSize().y_), 4.f, 2.5f);
        if (action_)
            action_->Free();

        action_ = DelayAction::Get(GameStatics::Sounds[SND_BOSSWARNING], 2.2f);
        appearDelay = 6.5f;
    }

    DelayInformer::Get(this, appearDelay, GAME_BOSSAPPEARS);
}

bool sDuoChecked_ = false;

void PlayState::SubscribeToEvents()
{
    URHO3D_LOGINFO("PlayState() - ---------------------------------------");
    URHO3D_LOGINFO("PlayState() - SubscribeToEvents ! ");
    URHO3D_LOGINFO("PlayState() - ---------------------------------------");

    // Subscribe HandleUpdate() function for processing update events
    SubscribeToEvent(E_UPDATE, URHO3D_HANDLER(PlayState, HandleUpdate));
    SubscribeToEvent(E_KEYDOWN, URHO3D_HANDLER(PlayState, HandlePause));
    SubscribeToEvent(GAME_PLAYEXIT, URHO3D_HANDLER(PlayState, HandleStop));
    SubscribeToEvent(GAME_SCREENRESIZED, URHO3D_HANDLER(PlayState, HandleScreenResized));

    if (showHiScore_)
        SubscribeToEvent(GAME_SCORECHANGE, URHO3D_HANDLER(PlayState, HandleUpdateScores));

    SubscribeToEvent(GAME_MOVEREMOVED, URHO3D_HANDLER(PlayState, HandleUpdateMoves));
    SubscribeToEvent(GAME_MOVEADDED, URHO3D_HANDLER(PlayState, HandleUpdateMoves));
    SubscribeToEvent(GAME_MOVERESTORED, URHO3D_HANDLER(PlayState, HandleUpdateMoves));

    SubscribeToEvent(GAME_TRYREMOVED, URHO3D_HANDLER(PlayState, HandleUpdateStars));
    SubscribeToEvent(GAME_TRYADDED, URHO3D_HANDLER(PlayState, HandleUpdateStars));
    SubscribeToEvent(GAME_TRYRESTORED, URHO3D_HANDLER(PlayState, HandleUpdateStars));

    SubscribeToEvent(GAME_COINUPDATED, URHO3D_HANDLER(PlayState, HandleUpdateCoins));

    SubscribeToEvent(GAME_OBJECTIVECHANGE, URHO3D_HANDLER(PlayState, HandleUpdateObjectives));
    SubscribeToEvent(GAME_OVER, URHO3D_HANDLER(PlayState, HandleGameOver));

    if (GameStatics::gameState_.storyitems_[1])
    {
        Button* button = static_cast<Button*>(uiplay_->GetChild(String("shake"), true));
        SubscribeToEvent(button, E_PRESSED, URHO3D_HANDLER(PlayState, HandleShake));
    }

    CheckBox* duocheckbox = uiplay_->GetChildStaticCast<CheckBox>(String("duo"), true);
    if (duocheckbox)
    {
        sDuoChecked_ = duocheckbox->IsChecked();
        SubscribeToEvent(duocheckbox, E_TOGGLED, URHO3D_HANDLER(PlayState, HandleDuoToggled));
    }

    SubscribeToDebugEvents(drawDebug_);

    Game::Get()->SubscribeToAccessMenuEvents();
}

void PlayState::UnsubscribeToEvents()
{
	UnsubscribeFromEvent(E_UPDATE);
    UnsubscribeFromEvent(E_KEYDOWN);
	UnsubscribeFromEvent(GAME_PLAYEXIT);
    UnsubscribeFromEvent(GAME_SCREENRESIZED);

	if (showHiScore_)
        UnsubscribeFromEvent(GAME_SCORECHANGE);

    UnsubscribeFromEvent(GAME_MOVEREMOVED);
    UnsubscribeFromEvent(GAME_MOVEADDED);
    UnsubscribeFromEvent(GAME_MOVERESTORED);

    UnsubscribeFromEvent(GAME_TRYREMOVED);
    UnsubscribeFromEvent(GAME_TRYADDED);
    UnsubscribeFromEvent(GAME_TRYRESTORED);

    UnsubscribeFromEvent(GAME_COINUPDATED);

    UnsubscribeFromEvent(GAME_OBJECTIVECHANGE);
    UnsubscribeFromEvent(GAME_OVER);

    UnsubscribeFromEvent(GAME_BOSSAPPEARS);

    SubscribeToDebugEvents(false);

    Game::Get()->UnsubscribeFromAccessMenuEvents();
}

void PlayState::SubscribeToDebugEvents(bool enabled)
{
    if (enabled)
    {
        if (matchStateText)
        {
            matchStateText->SetVisible(true);
            matchStateText->SetText(String(matchStateNames[MatchesManager::GetState()]));
        }
        SubscribeToEvent(GAME_MATCHSTATECHANGE, URHO3D_HANDLER(PlayState, HandleUpdateMatchState));
    }
    else
    {
        if (matchStateText)
            matchStateText->SetVisible(false);
        UnsubscribeFromEvent(GAME_MATCHSTATECHANGE);
    }

    if (enabled && rootScene_->GetOrCreateComponent<DebugRenderer>())
        SubscribeToEvent(E_POSTRENDERUPDATE, URHO3D_HANDLER(PlayState, OnPostRenderUpdate));
    else
        UnsubscribeFromEvent(E_POSTRENDERUPDATE);
}


void PlayState::HandleSceneLoading(StringHash eventType, VariantMap& eventData)
{
    if (GameStatics::GetStatus() == PLAYSTATE_LOADING)
    {
        // Here Asynchronous loading

        URHO3D_LOGINFO("------------------------------------------------------------");
        URHO3D_LOGINFO("- PlayState() - HandleSceneLoading :  ... OK !             -");
        URHO3D_LOGINFO("------------------------------------------------------------");

        GameStatics::SetStatus(PLAYSTATE_FINISHLOADING);
    }

    if (GameStatics::GetStatus() == PLAYSTATE_FINISHLOADING)
    {
        GameHelpers::SetMusic(MAINMUSIC, 0.7f, GameStatics::currentMusic_, true);
        GameStatics::SetStatus(PLAYSTATE_SYNCHRONIZING);
    }

    if (GameStatics::GetStatus() == PLAYSTATE_SYNCHRONIZING)
    {
        UnsubscribeFromEvent(E_UPDATE);

        rootScene_->StopAsyncLoading();
        // Set timescale to 1.f to prevent bug on client when a message for a scene node is received with a timescale=0.f
        rootScene_->SetTimeScale(1.f);
        // Restart the physics
        rootScene_->GetComponent<PhysicsWorld2D>()->SetUpdateEnabled(true);

        URHO3D_LOGINFO("------------------------------------------------------------");
        URHO3D_LOGINFO("- PlayState() - HandleSceneLoading : ... OK !              -");
        URHO3D_LOGINFO("------------------------------------------------------------");

        GameStatics::SetStatus(PLAYSTATE_READY);

        GameHelpers::ResetCamera();

        if (GameStatics::tries_)
            GameHelpers::AddUIMessage(ToString("Mission %u", GameStatics::currentLevel_), false, "Fonts/BlueHighway.ttf", UISIZE[FONTSIZE_HUGE],
                                      Color::YELLOW, IntVector2(0, MESSAGEPOSITIONYRATIO * GameStatics::ui_->GetRoot()->GetSize().y_), 2.f, 0.5f);

        SendEvent(GAME_LEVELREADY);

    #ifndef ACTIVE_SPLASHUI
        SendEvent(SPLASHSCREEN_STARTOPEN);
    #endif

        SetVisibleUI(true);
    }
}

/// When SplashScreen finished
void PlayState::HandleSceneAppears(StringHash eventType, VariantMap& eventData)
{
    URHO3D_LOGINFOF("PlayState() - HandleSceneAppears : ... touchEnabled_=%s screenJoystickID_=%d", GameStatics::gameConfig_.touchEnabled_ ? "true" : "false",
                    GameStatics::gameConfig_.screenJoystickID_);

    UnsubscribeFromEvent(SPLASHSCREEN_STARTOPEN);
    SubscribeToEvent(SPLASHSCREEN_FINISHOPEN, URHO3D_HANDLER(PlayState, HandleStart));

    Tutorial::Start();
    MatchesManager::Start();

    SubscribeToEvents();

    if (GameStatics::gameConfig_.touchEnabled_)
        GetSubsystem<Input>()->SetScreenJoystickVisible(GameStatics::gameConfig_.screenJoystickID_, true);

    GameStatics::SetMouseVisible(!GameStatics::gameConfig_.touchEnabled_, false);
//    GetSubsystem<Input>()->SetMouseVisible(!GameStatics::gameConfig_.touchEnabled_);

#ifndef ACTIVE_SPLASHUI
    // Entrance Animation
    GameHelpers::SetMoveAnimationUI(uiplay_, IntVector2(500 * stateManager_->GetStackMove(), 0), IntVector2(0, 0), 0.f, SWITCHSCREENTIME);
    GameHelpers::SetMoveAnimation(GameStatics::rootScene_->GetChild("Scene"), Vector3(10.f * stateManager_->GetStackMove(), 0.f, 0.f), Vector3::ZERO, 0.f, SWITCHSCREENTIME);

    SendEvent(SPLASHSCREEN_FINISHOPEN);
#endif

    URHO3D_LOGINFOF("PlayState() - HandleSceneAppears : ... coins=%d stars=%d moves=%d OK !", GameStatics::coins_, GameStatics::tries_, GameStatics::moves_);
}

void PlayState::HandleStart(StringHash eventType, VariantMap& eventData)
{
    if (abilitypanel_)
    {
        ResizeAbilityPanel(true);
        abilitypanel_->Start(false, false);
    }

    SetVisibleUI(true);

    UnsubscribeFromEvent(SPLASHSCREEN_FINISHOPEN);

    // UI Messages
	if (GameStatics::tries_ && GameStatics::moves_)
    {
        URHO3D_LOGINFOF("PlayState() - HandleStart : ... OK !");

        if (GameStatics::IsBossLevel())
            AddRandomBossWarning();

        gameOver_ = false;
        activeGameLogic_ = true;
    }
    else
    {
        URHO3D_LOGINFOF("PlayState() - HandleStart : ... stars=%d moves=%d ", GameStatics::tries_, GameStatics::moves_);

        GameStatics::moves_ = 0;
        SendEvent(GAME_MOVEREMOVED);
        activeGameLogic_ = false;
        gameOver_ = true;

        Localization* l10n = GetSubsystem<Localization>();
        GameHelpers::AddUIMessage(l10n->Get("notry"), false, "Fonts/BlueHighway.ttf", UISIZE[FONTSIZE_BIG], Color::WHITE,
                                  IntVector2(0, MESSAGEPOSITIONYRATIO * GameStatics::ui_->GetRoot()->GetSize().y_), 2.3f);

        CheckStars(false);
    }

    if (!gameOver_)
    {
        Tutorial::Get()->SetEnabled(true);
        GameStatics::SetStatus(PLAYSTATE_RUNNING);
    }
}

void PlayState::HandleBossAppears(StringHash eventType, VariantMap& eventData)
{
    int bossid = GameStatics::GetCurrentBoss();

    if (bossid == -1)
    {
        URHO3D_LOGERRORF("PlayState() - HandleBossAppears : ... noboss for this level=%d", GameStatics::currentLevel_);
        return;
    }

    URHO3D_LOGINFOF("PlayState() - HandleBossAppears : ... boss=%d", bossid);

    String bossname = ToString("Boss%d", bossid);
    boss_ = GameHelpers::SpawnGOtoNode(context_, StringHash(bossname), scene_);
    boss_->SetName(bossname);

    BossLogic* bosslogic = boss_->GetDerivedComponent<BossLogic>();
    bosslogic->SetBoss(bossid-1);
    boss_->SetEnabled(true);

    // At start check if Inputs are allowed (AccessMenu not in use) before activate boss
    bosslogic->SetActive(GameStatics::allowInputs_, false);

    UIElement* objectivezone = uiplay_->GetChild(String("objectivezone"), true);
    if (objectivezone)
    {
        SharedPtr<ObjectAnimation> objectAnimation(new ObjectAnimation(objectivezone->GetContext()));
        SharedPtr<ValueAnimation> opacityAnimation(new ValueAnimation(objectivezone->GetContext()));
        opacityAnimation->SetKeyFrame(0.f, 0.f);
        opacityAnimation->SetKeyFrame(2.f, 1.f);
        objectAnimation->AddAttributeAnimation("Opacity", opacityAnimation, WM_ONCE);
        objectivezone->SetObjectAnimation(objectAnimation);

        SharedPtr<ObjectAnimation> textAnimation(new ObjectAnimation(objectivezone->GetContext()));
        SharedPtr<ValueAnimation> textopacityAnimation(new ValueAnimation(objectivezone->GetContext()));
        textopacityAnimation->SetKeyFrame(0.f, 0.f);
        textopacityAnimation->SetKeyFrame(2.f, 1.f);
        textAnimation->AddAttributeAnimation("Opacity", textopacityAnimation, WM_ONCE);
        objectiveText[0]->SetObjectAnimation(textAnimation);
    }

    URHO3D_LOGINFOF("PlayState() - HandleBossAppears : ... Add Boss : node=%s(%u) OK !", boss_->GetName().CString(), boss_->GetID());
}

void PlayState::HandleUpdateScores(StringHash eventType, VariantMap& eventData)
{
    if (eventType == GAME_SCORECHANGE)
    {
        hiScore += MatchesManager::GetTurnScore();

        hiscoreText->SetText(String(hiScore));

        // Create "Add Score" Effect

        float fontzoom = Min(1.3f + (float)(MatchesManager::GetTurnScore() / 1000), 5.f);
//        GameHelpers::AddTextFadeAnim(uiplay_, "+"+String(MatchesManager::GetTurnScore()), hiscoreText, IntVector2(0, floor((float)30 * GameStatics::uiScale_)), 0.7f, fontzoom);
        GameHelpers::AddText3DFadeAnim(scene_, "+"+String(MatchesManager::GetTurnScore()), hiscoreText, Vector3(0.f, -0.3f, 0.f) * GameStatics::uiScale_, 0.7f, fontzoom);
    }
}

void PlayState::HandleUpdateMoves(StringHash eventType, VariantMap& eventData)
{
    if (eventType == GAME_MOVEREMOVED)
    {
        GameStatics::moves_ = Max(0, GameStatics::moves_-1);

        moveText->SetText(String(GameStatics::moves_));

        // Create "Add Move" Effect
//        GameHelpers::AddTextFadeAnim(uiplay_, String(GameStatics::moves_), moveText, IntVector2(floor((float)30 * GameStatics::uiScale_), floor((float)30 * GameStatics::uiScale_)), 1.f, 5.f);
        GameHelpers::AddText3DFadeAnim(scene_, String(GameStatics::moves_), moveText, Vector3(0.3f, -0.3f, 0.f) * GameStatics::uiScale_, 1.f, 5.f);

        URHO3D_LOGINFOF("PlayState() - HandleUpdateMoves : move removed => moves=%d !", GameStatics::moves_);

        if (!GameStatics::moves_ && GameStatics::tries_)
            CheckStars(true);
    }
    else if (eventType == GAME_MOVEADDED)
    {
        GameStatics::moves_++;

        URHO3D_LOGINFOF("PlayState() - HandleUpdateMoves : move Added => moves=%d !", GameStatics::moves_);

        moveText->SetText(String(GameStatics::moves_));

        // Create "Add Move" Effect
//        GameHelpers::AddTextFadeAnim(uiplay_, String(GameStatics::moves_), moveText, IntVector2(floor((float)30 * GameStatics::uiScale_), floor((float)30 * GameStatics::uiScale_)), 1.f, 5.f);
        GameHelpers::AddText3DFadeAnim(scene_, String(GameStatics::moves_), moveText, Vector3(0.3f, -0.3f, 0.f) * GameStatics::uiScale_, 1.f, 5.f);

        if (GameStatics::moves_ > 0 && GameStatics::numRemainObjectives_ > 0)
            GameStatics::AllowInputs(true);
    }
    else if (eventType == GAME_MOVERESTORED)
    {
        URHO3D_LOGINFOF("PlayState() - HandleUpdateMoves : move Restored => moves=%d !", GameStatics::moves_);
        moveText->SetText(String(GameStatics::moves_));
        GameHelpers::AddText3DFadeAnim(scene_, String(GameStatics::moves_), moveText, Vector3(0.3f, -0.3f, 0.f) * GameStatics::uiScale_, 1.f, 5.f);
    }
}

void PlayState::HandleUpdateStars(StringHash eventType, VariantMap& eventData)
{
    if (eventType == GAME_TRYREMOVED)
    {
        URHO3D_LOGINFOF("PlayState() - HandleUpdateStars : try Removed => tries=%d !", GameStatics::tries_);
        if (GameStatics::tries_ > 1)
        {
            triesSprite->SetVisible(true);
            triesText->SetVisible(true);
            triesText->SetText(String(GameStatics::tries_-1));
            GameHelpers::AddText3DFadeAnim(scene_, "-1", triesText, Vector3(0.2f, -0.1f, 0.f) * GameStatics::uiScale_, 1.5f, 8.f);
        }
        else
        {
            triesSprite->SetVisible(false);
            triesText->SetVisible(false);
        }

        GameHelpers::SpawnSound(GameStatics::rootScene_, SND_REMOVETRY);
    }
    else if (eventType == GAME_TRYADDED)
    {
        if (gameOver_)
        {
            URHO3D_LOGINFOF("PlayState() - HandleUpdateStars : start game !");

            gameOver_ = false;
            GameStatics::AllowInputs(true);
            activeGameLogic_ = true;
            Tutorial::Get()->SetEnabled(true);

            if (gameneedmorestarsframe_ && gameneedmorestarsframe_->IsStarted())
            {
                UnsubscribeFromEvent(gameneedmorestarsframe_, E_MESSAGEACK);
                gameneedmorestarsframe_->Stop();
                SubscribeToEvent(GAME_OVER, URHO3D_HANDLER(PlayState, HandleGameOver));

                ((OptionState*)GameStatics::stateManager_->GetState("Options"))->CloseFrame();

                if (!MatchesManager::GetNumPossibleMatches() && !MatchesManager::GetGrid().HasTileEntrances())
                    MatchesManager::ShakeMatches();

                SetBossActive(true);
            }

            GameStatics::moves_ = initialMoves-1;

            SendEvent(GAME_MOVEADDED);

            if (GameStatics::IsBossLevel() && !boss_)
                AddRandomBossWarning();
        }

        if (GameStatics::tries_ > 1)
        {
            triesSprite->SetVisible(true);
            triesText->SetVisible(true);
        }

        URHO3D_LOGINFOF("PlayState() - HandleUpdateStars : try Added => tries=%d !", GameStatics::tries_);

        triesText->SetText(String(GameStatics::tries_-1));
//        GameHelpers::AddTextFadeAnim(uiplay_, "+1", triesText, IntVector2(floor((float)50 * GameStatics::uiScale_), floor((float)50 * GameStatics::uiScale_)), 1.f, 8.f);
        GameHelpers::AddText3DFadeAnim(scene_, "+1", triesText, Vector3(0.5f, -0.5f, 0.f) * GameStatics::uiScale_, 1.f, 8.f);

        GameHelpers::SpawnSound(GameStatics::rootScene_, SND_ADDTRY);
    }
    else if (eventType == GAME_TRYRESTORED)
    {
        URHO3D_LOGINFOF("PlayState() - HandleUpdateStars : try Restored => tries=%d !", GameStatics::tries_);

        triesText->SetText(String(GameStatics::tries_-1));
        GameHelpers::AddText3DFadeAnim(scene_, String(GameStatics::tries_), triesText, Vector3(0.5f, -0.5f, 0.f) * GameStatics::uiScale_, 1.f, 8.f);

        GameHelpers::SpawnSound(GameStatics::rootScene_, SND_ADDTRY);
    }
}

void PlayState::HandleUpdateCoins(StringHash eventType, VariantMap& eventData)
{
    if (eventType == GAME_COINUPDATED)
    {
        URHO3D_LOGINFOF("PlayState() - HandleUpdateCoins : coin Added => coin=%d !", GameStatics::coins_);

        GameHelpers::AddText3DFadeAnim(scene_, "+1", triesText, Vector3(0.5f, -0.5f, 0.f) * GameStatics::uiScale_, 1.f, 8.f);

        GameHelpers::SpawnSound(GameStatics::rootScene_, SND_ADDCOIN);
    }
}

void PlayState::UpdateObjectives(bool init)
{
    GameStatics::numRemainObjectives_ = 0;

    UIElement* objectivezone = uiplay_->GetChild(String("uibuttons"))->GetChild(String("objectivezone"));

    for (int i=0; i < numObjectives_; i++)
    {
//        if (objectives[i][0] == 0 && !init)
//            continue;

        int remainqty = MatchesManager::GetObjectiveRemainingQty(i);

        int dps = objectives[i][0] - remainqty;
        if (dps != 0 || init)
        {
            if (boss_)
                boss_->GetDerivedComponent<BossLogic>()->Hit(dps, false);

            if (remainqty > 0)
            {
                objectiveText[i]->SetText(String(remainqty));
                objectiveText[i]->SetColor(Color::WHITE);
            }
            else
            {
                objectiveText[i]->SetText(boss_ ? "KO" : "OK");
                objectiveText[i]->SetColor(Color::GREEN);
            }

            if (!init)
                GameHelpers::AddText3DFadeAnim(scene_, String(dps), objectiveText[i], Vector3(0.f, -0.6f, 0.f), 1.f, 5.f);
            GameHelpers::SetScaleAnimationUI(objectivezone->GetChild(i)->GetChild(0), 1.f, 1.f, Clamp((float)dps * 0.35f, 1.2f, 1.75f), 0.f, 0.15f);
        }

        objectives[i][0] = remainqty;

        GameStatics::numRemainObjectives_ += objectives[i][0];
    }
}

void PlayState::HandleUpdateObjectives(StringHash eventType, VariantMap& eventData)
{
    URHO3D_LOGINFO("PlayState() - HandleUpdateObjectives !");
    UpdateObjectives();
}

void PlayState::HandleGameOver(StringHash eventType, VariantMap& eventData)
{
    URHO3D_LOGINFO("PlayState() - HandleGameOver !");

    UnsubscribeFromEvent(GAME_OVER);

    if (gameneedmorestarsframe_ && gameneedmorestarsframe_->IsStarted())
    {
        UnsubscribeFromEvent(gameneedmorestarsframe_, E_MESSAGEACK);
        gameneedmorestarsframe_->Stop();
        ((OptionState*)GameStatics::stateManager_->GetState("Options"))->CloseFrame();
    }

    if (!gameOver_)
    {
        GameHelpers::SetMusic(MAINMUSIC, 0.1f);
        GameHelpers::SetMusic(SECONDARYMUSIC, 1.f, LOOSETHEME, false);
        GameHelpers::SetSoundVolume(0.1f);

        gameOver_ = true;
        GameStatics::AllowInputs(false);
        activeGameLogic_ = false;
        Tutorial::Get()->SetEnabled(false);
    }

    SubscribeToEvent(this, GAME_UIFRAME_ADD, URHO3D_HANDLER(PlayState, OnGameOverFrame));
    DelayInformer::Get(this, 0.25f, GAME_UIFRAME_ADD);
}

void PlayState::HandleUpdateMatchState(StringHash eventType, VariantMap& eventData)
{
//    URHO3D_LOGINFO("PlayState() - HandleUpdateMatchState !");

    matchStateText->SetText(String(matchStateNames[MatchesManager::GetState()]));
}

void PlayState::CheckInputForDebug(Input* input)
{
//    if (input->GetMouseButtonDown(MOUSEB_LEFT))
    {
        int x = input->GetMousePosition().x_;
        int y = input->GetMousePosition().y_;

        if (GetSubsystem<UI>()->GetElementAt(x, y))
            return;

        Viewport* viewport = GetSubsystem<Renderer>()->GetViewport(0);
        Ray ray = viewport->GetScreenRay(x, y);

        Vector3 wpoint3 = viewport->ScreenToWorldPoint(x, y, ray.HitDistance(GameStatics::GroundPlane_));
        Vector2 wpoint = Vector2(wpoint3.x_, wpoint3.y_);

        RigidBody2D* body = 0;
        CollisionShape2D* shape = 0;
        rootScene_->GetComponent<PhysicsWorld2D>()->GetPhysicElements(wpoint, body, shape);

        if (body && body->GetNode())
        {
            Node* node = body->GetNode();

            URHO3D_LOGINFOF("HandleCreateMode_MouseButton() : Entity %s(%u) enabled=%s body=%u shape=%u(from node=%s(%u))",
                            node->GetName().CString(), node->GetID(), node->IsEnabled() ? "true":"false", body->GetID(),
                            shape->GetID(), shape->GetNode()->GetName().CString(), shape->GetNode()->GetID());

            GameCommands::Launch(GetContext(), String("node=")+String(node->GetID()));
        }
    }
}


void PlayState::HandleUpdate(StringHash eventType, VariantMap& eventData)
{
	using namespace Update;

	// Take the frame time step, which is stored as a float
	float timeStep = eventData[P_TIMESTEP].GetFloat();

	// Game Logic
	if (activeGameLogic_)
    {
        if (MatchesManager::GetState() == NoMatchState)
        {
            if (GameStatics::numRemainObjectives_ <= 0)
            {
                URHO3D_LOGINFOF("PlayState() - HandleUpdate : level win ...");
                LevelWin();
            }
            else if (GameStatics::moves_ == 0)
            {
                URHO3D_LOGINFOF("PlayState() - HandleUpdate : No more Moves => use a star ...");
                CheckStars(true);
            }
        }
    }

	Input* input = GetSubsystem<Input>();

    // touchEnabled KEY_SELECT Settings
    if (input->GetKeyPress(KEY_SELECT) && GameStatics::gameConfig_.touchEnabled_)
    {
        if (GameStatics::gameConfig_.screenJoysticksettingsID_ == -1)
        {
            ResourceCache* cache = GetSubsystem<ResourceCache>();
            GameStatics::gameConfig_.screenJoysticksettingsID_ = input->AddScreenJoystick(cache->GetResource<XMLFile>("UI/ScreenJoystickSettings2.xml"), cache->GetResource<XMLFile>("UI/DefaultStyle.xml"));
        }

        if (GameStatics::gameConfig_.screenJoysticksettingsID_ != -1)
            input->SetScreenJoystickVisible(GameStatics::gameConfig_.screenJoysticksettingsID_, !input->IsScreenJoystickVisible(GameStatics::gameConfig_.screenJoysticksettingsID_));

        return;
    }

    // Debug Stuff
    if (GameStatics::gameConfig_.debugRenderEnabled_)
    {
        if (rootScene_)
        {
            if (input->GetKeyPress(KEY_G))
            {
                drawDebug_ = !drawDebug_;
                SubscribeToDebugEvents(drawDebug_);
            }
        }
    }

    if (input->GetKeyDown(KEY_LCTRL))
        CheckInputForDebug(input);

    // Toggle Camera Control
    if (GameStatics::gameConfig_.ctrlCameraEnabled_)
    {
        if (input->GetMouseButtonDown(MOUSEB_LEFT))
        {
            ctrlCameraWithMouse_ = true;
            // Move camera with Mouse
            IntVector2 mouseMove = input->GetMouseMove();
            cameraYaw_ += 0.1f * mouseMove.x_;
            cameraPitch_ += 0.1f * mouseMove.y_;
            cameraPitch_ = Clamp(cameraPitch_, -90.0f, 90.0f);
            cameraNode_->SetRotation(Quaternion(cameraPitch_, cameraYaw_, 0.0f));
        }
        else if (ctrlCameraWithMouse_)
        {
            ctrlCameraWithMouse_ = false;
            // Reset camera's position
            cameraYaw_ = 0.0f;
            cameraPitch_ = 0.0f;
            cameraNode_->SetRotation(Quaternion(cameraPitch_, cameraYaw_, 0.0f));
        }
    }

    // Don't Check Tips if the UI has a focused element (the console)
    if (GetSubsystem<UI>()->GetFocusElement())
        return;

    // Tip for enable/disabled all lights
    if (input->GetKeyPress(KEY_E))
    {
        PODVector<Light*> lights;
        rootScene_->GetComponents<Light>(lights, true);
        for (unsigned i = 0; i < lights.Size(); ++i)
            lights[i]->SetEnabled(!lights[i]->IsEnabled());
    }
    // Tip Pause Without Window
    if (input->GetKeyPress(KEY_P))
    {
        rootScene_->SetUpdateEnabled(!rootScene_->IsUpdateEnabled());
        rootScene_->GetComponent<PhysicsWorld2D>()->SetUpdateEnabled(rootScene_->IsUpdateEnabled());
        GameHelpers::SetMusicVolume(rootScene_->IsUpdateEnabled() ? 1.f : 0.1f);
        GameHelpers::SetSoundVolume(rootScene_->IsUpdateEnabled() ? 1.f : 0.1f);

        URHO3D_LOGINFOF("PlayState() - HandleUpdate : Key P pressed => Scene Updated = %s", rootScene_->IsUpdateEnabled() ? "enable" : "disable");
    }
    if (input->GetKeyPress(KEY_J))
    {
        MatchesManager::ShakeMatches();
    }
    if (input->GetKeyPress(KEY_N))
    {
        CreateLevel(true);
    }
    if (input->GetKeyPress(KEY_W))
    {
        tipWinLevelEnable_ = true;
        LevelWin();
    }
    if (input->GetKeyPress(KEY_X))
    {
        bool bosslevel = GameStatics::IsBossLevel();

        if (bosslevel)
        {
            Node* boss = scene_->GetChild(ToString("Boss%d", GameStatics::GetCurrentBoss()));
            if (boss)
                boss->GetDerivedComponent<BossLogic>()->Hit(5);
        }
    }
    // Tip Dump Pool
    if (input->GetKeyPress(KEY_O))
    {
        if (ObjectPool::Get())
            ObjectPool::Get()->DumpCategories();
    }
    if (input->GetKeyDown(KEY_PAGEUP))
    {
        if (GameStatics::camera_)
            GameStatics::camera_->SetZoom(GameStatics::camera_->GetZoom() * 1.01f);
    }
    if (input->GetKeyDown(KEY_PAGEDOWN))
    {
        if (GameStatics::camera_)
            GameStatics::camera_->SetZoom(GameStatics::camera_->GetZoom() * 0.99f);
    }
    // Tip for generate an application crash
//    if (input->GetKeyPress(KEY_C))
//    {
//        // Crash Test
//        int a = 1 / 0;
//    }
}

void PlayState::HandlePause(StringHash eventType, VariantMap& eventData)
{
    // Auto Unpause
    if (eventType == GAME_UNPAUSE)
    {
//        if (paused_)
            Pause(false);
    }
    else
    {
        if (eventType == E_KEYDOWN && eventData[KeyDown::P_KEY].GetInt() == KEY_ESCAPE)
        {
            if (((OptionState *) stateManager_->GetState("Options"))->IsFrameOpened())
                ((OptionState *) stateManager_->GetState("Options"))->CloseFrame();
            else
                Pause(true, true);
        }
        else if (eventType == E_RELEASED)
        {
            Pause(true, true);
        }
    }
};

void PlayState::HandleStop(StringHash eventType, VariantMap& eventData)
{
    const unsigned bosslevel = GameStatics::GetBossLevelId(GameStatics::currentZone_);

    URHO3D_LOGINFOF("PlayState() - HandleStop : currentzone=%d currentlevel=%d bosslevel=%d ...", GameStatics::currentZone_, GameStatics::currentLevel_, bosslevel);

    gameOver_ = true;

    if (eventType == GAME_PLAYEXIT && bosslevel == GameStatics::currentLevel_)
    {
        URHO3D_LOGINFO("PlayState() - HandleStop : bosslevel !");

    #ifdef ACTIVE_CINEMATICS
        stateManager_->PopStack(false);
        if (!CinematicState::SetCinematic(CINEMATICSELECTIONMODE_INTRO_OUTRO, GameStatics::GetMinLevelId(GameStatics::currentZone_), GameStatics::currentLevel_))
            stateManager_->SetActiveState("LevelMap");
    #else
        stateManager_->PopStack(true);
    #endif
    }
    else
    {
        // Focus on bosslevel in levelmap state
        if (GameStatics::gameState_.GetNextLevel() == bosslevel)
        {
            URHO3D_LOGINFO("PlayState() - HandleStop : nextlevel = boss ... focus on it !");
            GameStatics::currentLevel_ = bosslevel;
        }

        stateManager_->PopStack(true);
    }
}

void PlayState::HandleNewLevel(StringHash eventType, VariantMap& eventData)
{
    URHO3D_LOGINFO("PlayState() - HandleNewLevel !");

    UnsubscribeFromEvent(this, GAME_NEXTLEVEL);

    CreateLevel(true);
}

void PlayState::HandleAccessMenu(StringHash eventType, VariantMap& eventData)
{
    URHO3D_LOGINFO("PlayState() - HandleAccessMenu !");
    if (eventType == UIMENU_SHOWCONTENT)
    {
        GameStatics::AllowInputs(false);
        SetBossActive(false);
    }
    else if (eventType == UIMENU_HIDECONTENT)
    {
        if (!static_cast<OptionState*>(stateManager_->GetState("Options"))->IsFrameOpened())
        {
            GameStatics::AllowInputs(true);
            SetBossActive(true);
        }
    }
}

void PlayState::HandleShake(StringHash eventType, VariantMap& eventData)
{
    URHO3D_LOGINFOF("PlayState() - HandleShake !");
    if (MatchesManager::Get() && GameStatics::numRemainObjectives_ > 0)
        MatchesManager::ShakeMatches();
}


bool firstserverpong_;
void PlayState::HandleDuoToggled(StringHash eventType, VariantMap& eventData)
{
    sDuoChecked_ = eventData[Toggled::P_STATE].GetBool();

    if (sDuoChecked_)
    {
        URHO3D_LOGINFOF("PlayState() - HandleDuoToggled : checked !");

        Network* network = Network::Get(false);
        if (!network)
        {
            NetworkPeer* networkpeer = static_cast<NetworkPeer*>(Network::Get(true, NetPeering));
            if (!networkpeer)
            {
                URHO3D_LOGERROR("PlayState() - HandleDuoToggled : No Network !");
                return;
            }

            if (GameStatics::netidentity_.Empty())
            {
                GameStatics::netidentity_ = GameHelpers::GetRandomString(10);
                URHO3D_LOGINFOF("PlayState() - HandleDuoToggled : create netidentity = %s", GameStatics::netidentity_.CString());
            }

            networkpeer->RegisterChannel("griddata");
            networkpeer->OnAvailablePeersUpdate(std::bind(&PlayState::OnNetworkAvailablePeersUpdate, this, std::placeholders::_1));
            networkpeer->OnConnectedPeersUpdate(std::bind(&PlayState::OnNetworkConnectedPeersUpdate, this, std::placeholders::_1));
            networkpeer->OnMessageReceived(std::bind(&PlayState::OnNetworkMessageReceived, this, std::placeholders::_1, std::placeholders::_2));

            network = static_cast<Network*>(networkpeer);
        }

        if (network && (!network->GetConnection() || !network->GetConnection()->IsConnected()))
        {
            network->Connect("ws://127.0.0.1:8080/", GameStatics::netidentity_);
            firstserverpong_ = true;
//            network->GetConnection()->SetAutoConnectPeers(1);
        }
    }
    else
    {
        URHO3D_LOGINFOF("PlayState() - HandleDuoToggled : unchecked !");

        OnDisconnectPeer();
    }
}

void PlayState::OnNetworkAvailablePeersUpdate(const StringVector* peers)
{
    if (peers && peers->Size())
    {
        URHO3D_LOGINFOF("PlayState() - OnNetworkAvailablePeersUpdate : peer size=%u ...", peers->Size());

        if (firstserverpong_)
        {
            for (unsigned i = 0; i < peers->Size(); i++)
            {
                if (peers->At(i) == GameStatics::netidentity_ && firstserverpong_)
                {
                    URHO3D_LOGINFOF("PlayState() - OnNetworkAvailablePeersUpdate : firstserverpong_ !");
                    firstserverpong_ = false;
                    Network::Get()->SendMessage("needoffer", String::EMPTY, "*");
                }
            }
        }
    }
}

void PlayState::OnNetworkConnectedPeersUpdate(const StringVector* peers)
{
    URHO3D_LOGINFOF("PlayState() - OnNetworkConnectedPeersUpdate : peers=%u peerSize=%u ...", peers, peers ? peers->Size() : 0);

    if (peers && peers->Size())
        OnConnectPeer(peers);

    else if (MatchesManager::GetNumGrids() == 2)
        OnDisconnectPeer();
}

void PlayState::OnConnectPeer(const StringVector* peers)
{
    Network* network = Network::Get(false);

    URHO3D_LOGINFOF("PlayState() - OnConnectPeer : connected with %s", peers->At(0).CString());
    GameStatics::peerConnected_ = true;

    // the peerconnection is established, so don't need websocket anymore
    if (network && network->GetConnection() && network->GetConnection()->GetTransport())
        network->GetConnection()->GetTransport()->Disconnect();

    if (MatchesManager::GetNumGrids() == 1)
    {
        MatchesManager::Stop();
        MatchesManager::SetNetPlayMod(NETPLAY_COLLAB);
        MatchesManager::AddGrid(NETREMOTE);
        SetLevelDatas();
        MatchesManager::Start();
        UpdateObjectives(true);

        URHO3D_LOGINFOF("PlayState() - OnConnectPeer : Send Local Grid to peer !");
        MatchesManager::GetGridInfo(NETLOCAL)->Net_SendGrid();
    }
}

void PlayState::OnDisconnectPeer()
{
    Network* network = Network::Get(false);

    URHO3D_LOGINFOF("PlayState() - OnDisconnectPeer ...");
    GameStatics::peerConnected_ = false;

    if (network && network->GetState() == NetworkConnectionState::Connected)
        network->DisconnectAll();

    if (MatchesManager::GetNumGrids() == 2)
    {
        MatchesManager::Stop();
        MatchesManager::RemoveGrid(NETREMOTE);
        SetLevelDatas();
        MatchesManager::Start();
        UpdateObjectives(true);
    }

    if (sDuoChecked_)
        uiplay_->GetChildStaticCast<CheckBox>(String("duo"), true)->SetChecked(false);
}

void PlayState::OnNetworkMessageReceived(NetworkTransport* transport, Vector<VectorBuffer >* packets)
{
    if (transport && packets)
    {
        MatchGridInfo* gridinfo = MatchesManager::GetGridInfo(NETREMOTE);
        if (gridinfo)
        {
            URHO3D_LOGINFOF("PlayState() - OnNetworkMessageReceived ... gridinfo=%u", gridinfo);

            for (Vector<VectorBuffer >::Iterator it = packets->Begin(); it != packets->End(); ++it)
                gridinfo->Net_ReceiveCommands(*it);

            transport->ClearIncomingPackets();
        }
    }
}


void PlayState::HandleScreenResized(StringHash eventType, VariantMap& eventData)
{
    URHO3D_LOGINFOF("PlayState() - HandleScreenResized !");

    // Update UI
    ResizeUI(true);

    // Update Grid Scene & Background
    MatchesManager::UpdateGridPositions();
    MatchesManager::ResizeBackGround();
}


void PlayState::OnAccessMenuOpenFrame(bool state)
{
    if (gameOver_)
        state = true;

    // desactivate inputs when opening the menu
    if (state)
        GameStatics::AllowInputs(false);
    // reactivate inputs when closing the menu
    else if (activeGameLogic_)
        GameStatics::AllowInputs(true);

    // set boss actived if accessmenu is not used
    SetBossActive(!state);
}

void PlayState::OnLevelWin(StringHash eventType, VariantMap& eventData)
{
    URHO3D_LOGINFO("PlayState() - OnLevelWin ...");

    Graphics* graphics = GetSubsystem<Graphics>();
    String framelayout;
    InteractiveFrame* frame;

    GameStatics::MissionState initialMissionState;
    GameStatics::gameState_.GetMissionState(GameStatics::currentLevel_, initialMissionState);

    int score = GameStatics::gameState_.UpdateMissionScores(GameStatics::currentLevel_);
    bool completedmission = GameStatics::gameState_.UpdateMission(GameStatics::currentLevel_, GameStatics::MissionState::MISSION_COMPLETED);
    bool unblockconstellation = GameStatics::CanUnblockConstellation(GameStatics::playerState_->zone);
    unsigned numitems = MatchesManager::GetAllItemsOnGrid();
    bool bosslevel = GameStatics::IsBossLevel();
    float starttime = (bosslevel ? 2.f : 0.25f) + numitems*0.3f;

#ifdef ACTIVE_RESTORESTARSONRETRY
    if (initialMissionState.state_ == GameStatics::MissionState::MISSION_COMPLETED && score <= initialMissionState.score_)
    {
        GameStatics::gameState_.SetMissionState(GameStatics::currentLevel_, initialMissionState);
        completedmission = false;
        GameStatics::tries_ = initialtries_;
        GameStatics::moves_ = initialmoves_;
        SendEvent(GAME_MOVERESTORED);
        SendEvent(GAME_TRYRESTORED);

        Localization* l10n = context_->GetSubsystem<Localization>();
        GameHelpers::AddUIMessage(l10n->Get("worsescore"), false, "Fonts/BlueHighway.ttf", 30, Color::WHITE, IntVector2(0, MESSAGEPOSITIONYRATIO * GameStatics::ui_->GetRoot()->GetSize().y_), 1.2f);
    }
    else
#endif
    // Story Item : Oeuf => Get All Powered Bonuses On Grid
    if (GameStatics::gameState_.storyitems_[2])
    {
        MatchesManager::GetPowerBonusesOnGrid(0.5f);
    }

    MatchesManager::SetPhysicsEnable(false);
    MatchesManager::Stop();
    Tutorial::Stop();
    if (Game::Get()->GetCompanion())
        Game::Get()->GetCompanion()->Clear();

    activeGameLogic_ = false;
    paused_ = true;

    if (bosslevel)
    {
        Node* boss = scene_->GetChild(ToString("Boss%d", GameStatics::GetCurrentBoss()));
        if (boss)
            boss->GetDerivedComponent<BossLogic>()->SetState(DIE);
    }

    // Add Unlock Powers Frame
    if (completedmission)
    {
        PODVector<int> unlockedpowers;
        GameStatics::gameState_.UnlockPowers(GameStatics::currentLevel_, unlockedpowers);

        if (unlockedpowers.Size())
        {
            framelayout = "UI/InteractiveFrame/GameTitleFrame.xml";

            for (int i=0; i < unlockedpowers.Size(); i++)
            {
                frame = InteractiveFrame::Get(framelayout, true);

                frame->SetScreenPositionEntrance(IntVector2(graphics->GetWidth()+300, graphics->GetHeight()/2));
                frame->SetScreenPosition(IntVector2(graphics->GetWidth()/2, graphics->GetHeight()/2));
                frame->SetScreenPositionExit(0, IntVector2(graphics->GetWidth()+300, graphics->GetHeight()/2));
                frame->SetScreenPositionExit(1, IntVector2(-300, graphics->GetHeight()/2));
                frame->SetScreenPositionEntranceForBonus(IntVector2(graphics->GetWidth()+300, graphics->GetHeight()/2));
                frame->SetText("Title", "unlockedpower", true);
                frame->SetAutoStart(starttime);
                frame->SetAutoStop(1.8f);
                frame->SetBreakInput(true);
                frame->AddBonus(Slot(COT::POWERS, unlockedpowers[i]));
                frame->SetBonusStart(0.f);
                frame->GetBonusNode()->SetScale(Vector3(2.f, 2.f, 1.f));
                starttime += 2.f;
            }
        }
    }

    // Add Gain Frame
    {
        framelayout = unblockconstellation || bosslevel ? "UI/InteractiveFrame/ZoneWinFrame.xml" : "UI/InteractiveFrame/GainFrame.xml";
        frame = GameHelpers::AddInteractiveFrame(framelayout, this, URHO3D_HANDLER(PlayState, OnWinMessageAck), false);
        frame->SetScreenPositionEntrance(IntVector2(-300, graphics->GetHeight()/2));
        frame->SetScreenPosition(IntVector2(graphics->GetWidth()/2, graphics->GetHeight()/2));
        frame->SetScreenPositionExit(0, IntVector2(-300, graphics->GetHeight()/2));
        frame->SetScreenPositionExit(1, IntVector2(graphics->GetWidth()+300, graphics->GetHeight()/2));
        frame->SetScreenPositionEntranceForBonus(IntVector2(graphics->GetWidth()/2, -200));
        frame->SetAutoStart(starttime);
        frame->SetBonusEnabled(true);

        // Update Score Animation
        AnimatedSprite2D* anim = frame->GetNode()->GetChild("Content", true)->GetComponent<AnimatedSprite2D>();
        if (anim)
            anim->SetEntity(String(score));

        URHO3D_LOGINFOF("PlayState() - OnLevelWin : ... mission %s", completedmission ? "completed":"not completed");

        // Add 4 moves
        if (completedmission && !tipWinLevelEnable_)
        {
            if (GameStatics::tries_ <= initialtries_ && GameStatics::moves_ < initialmoves_-4)
            {
                URHO3D_LOGINFO("PlayState() - OnLevelWin : Add bonus 4Moves");
                frame->AddBonus(Slot::ITEMS_4MOVES);
            }
            else
            {
                URHO3D_LOGINFO("PlayState() - OnLevelWin : Add bonus 2Moves");
                frame->AddBonus(Slot::ITEMS_2MOVES);
            }
        }

        if (completedmission)
        {
            Vector<Slot > bonuses;
            GameStatics::GetMissionBonuses(GameStatics::currentLevel_, bonuses);

            // Add Boss Random Bonus
            if (bosslevel)
            {
                const Vector<StringHash>& powertypes = MatchesManager::GetAuthorizedObjects(COT::POWERS);
                URHO3D_LOGINFOF("PlayState() - OnLevelWin : ... powertypesSize=%u", powertypes.Size());
                if (powertypes.Size())
                    bonuses.Push(Slot(COT::POWERS, powertypes[GameRand::GetRand(OBJRAND, powertypes.Size())], 1, true));
            }

            // Add Mission Bonus
            URHO3D_LOGINFOF("PlayState() - OnLevelWin : Add %u bonuses", bonuses.Size());
            for (int i=0; i < bonuses.Size(); i++)
                frame->AddBonus(bonuses[i]);
        }

        URHO3D_LOGINFOF("PlayState() - OnLevelWin ... OK !");

        // Reset Tip flag
        tipWinLevelEnable_ = false;

        if (gainframe_)
        {
            gainframe_->Stop();
            gainframe_.Reset();
        }

        gainframe_ = frame;
    }

    GameStatics::gameState_.Save();
}

void PlayState::OnWinMessageAck(StringHash eventType, VariantMap& eventData)
{
    UnsubscribeFromEvent(E_MESSAGEACK);

    const unsigned bosslevel = GameStatics::GetBossLevelId(GameStatics::currentZone_);
    const unsigned nextlevel = GameStatics::gameState_.GetNextLevel();
    if (!eventData[MessageACK::P_OK].GetBool() || nextlevel == 0 || nextlevel == bosslevel)
    {
        // it's a branch with multiple unexplored missions or it's a boss level
        // go to levelmap ; the player need to make a choice
        DelayInformer::Get(this, 0.4f, GAME_PLAYEXIT);
    }
    else
    {
        URHO3D_LOGINFO("PlayState() - Game Next Level !");
        SubscribeToEvent(this, GAME_NEXTLEVEL, URHO3D_HANDLER(PlayState, HandleNewLevel));
//        SetVisibleUI(false);
    #ifdef ACTIVE_SPLASHUI
        SendEvent(GAME_LEVELTOLOAD);
    #endif
        DelayInformer::Get(this, 1.5f, GAME_NEXTLEVEL);
    }

    if (gainframe_)
        gainframe_->Stop();

    if (abilitypanel_)
        abilitypanel_->Stop();

    GameHelpers::SetSoundVolume(1.f);
}

void PlayState::OnPauseMessageAck(StringHash eventType, VariantMap& eventData)
{
	using namespace MessageACK;

    UnsubscribeFromEvent(messageBox, E_MESSAGEACK);

    messageBox.Reset();

	if (eventData[P_OK].GetBool())
    {
        URHO3D_LOGINFO("PlayState() - Game UnPaused !");
		paused_ = false;
		rootScene_->SetUpdateEnabled(true);
    }
	else
    {
        URHO3D_LOGINFO("PlayState() - Play Exit !");
        SendEvent(GAME_PLAYEXIT);
    }

    GameHelpers::SetMusicVolume(1.f);
    GameHelpers::SetSoundVolume(1.f);
}

void PlayState::OnGameOverFrame(StringHash eventType, VariantMap& eventData)
{
    UnsubscribeFromEvent(this, GAME_UIFRAME_ADD);

    if (!gameoverframe_ || !gameoverframe_->IsStarted())
    {
        gameoverframe_ = InteractiveFrame::Get(GameStatics::tries_ ? "UI/InteractiveFrame/GameContinueFrame.xml" : "UI/InteractiveFrame/GameOverFrame.xml");

        Graphics* graphics = GetSubsystem<Graphics>();
        gameoverframe_->SetScreenPositionEntrance(IntVector2(-300, graphics->GetHeight()/2));
        gameoverframe_->SetScreenPosition(IntVector2(graphics->GetWidth()/2, graphics->GetHeight()/2));
        gameoverframe_->SetScreenPositionExit(0, IntVector2(-300, graphics->GetHeight()/2));
        gameoverframe_->SetScreenPositionExit(1, IntVector2(graphics->GetWidth()+300, graphics->GetHeight()/2));
        gameoverframe_->SetScreenPositionEntranceForBonus(IntVector2(graphics->GetWidth()/2, -200));

        gameoverframe_->Start(false, false);
    }

    SubscribeToEvent(gameoverframe_, E_MESSAGEACK, URHO3D_HANDLER(PlayState, OnGameOverAck));
}

void PlayState::OnGameOverAck(StringHash eventType, VariantMap& eventData)
{
    SubscribeToEvent(GAME_OVER, URHO3D_HANDLER(PlayState, HandleGameOver));

	if (eventData[MessageACK::P_OK].GetBool())
    {
        String action = eventData[TextEntry::P_TEXT].GetString();
        if (action == "Retry")
        {
            UnsubscribeFromEvent(gameoverframe_, E_MESSAGEACK);

            URHO3D_LOGINFO("PlayState() - OnGameOverAck : Retry !");

            // it's a Retry with at tries
            CheckStars(true);

            MatchesManager::ResetObjects();

            // Reset boss life and objective
//            Node* boss = scene_->GetChild(ToString("Boss%d", GameStatics::GetCurrentBoss()));
//            if (boss)
//            {
//                boss->GetDerivedComponent<BossLogic>()->ResetHitted();
//                objectives[0][0] = boss->GetDerivedComponent<BossLogic>()->GetLife();
//                UpdateObjectives();
//            }

            if (gameoverframe_)
                gameoverframe_->Stop();

            GameHelpers::SetMusic(MAINMUSIC, 0.7f);
            GameHelpers::SetMusicVolume(1.f);
            GameHelpers::SetSoundVolume(1.f);
        }
    }
	else
    {
        UnsubscribeFromEvent(gameoverframe_, E_MESSAGEACK);

        URHO3D_LOGINFO("PlayState() - OnGameOverAck : Quit !");
        DelayInformer::Get(this, 0.4f, GAME_PLAYEXIT);

        if (gameoverframe_)
            gameoverframe_->Stop();

        GameHelpers::SetMusic(MAINMUSIC, 0.7f);
        GameHelpers::SetMusicVolume(1.f);
        GameHelpers::SetSoundVolume(1.f);
    }
}

void PlayState::OnGameNeedMoreStars(StringHash eventType, VariantMap& eventData)
{
    UnsubscribeFromEvent(this, GAME_UIFRAME_ADD);

    if (!gameneedmorestarsframe_ || !gameneedmorestarsframe_->IsStarted())
    {
    #if defined(ACTIVE_ADS)
        gameneedmorestarsframe_ = InteractiveFrame::Get("UI/InteractiveFrame/GameNeedMoreStars_Ads.xml");
    #else
        gameneedmorestarsframe_ = InteractiveFrame::Get("UI/InteractiveFrame/GameNeedMoreStars.xml");
    #endif
        Graphics* graphics = GetSubsystem<Graphics>();
        gameneedmorestarsframe_->SetScreenPositionEntrance(IntVector2(-300, graphics->GetHeight()/2));
        gameneedmorestarsframe_->SetScreenPosition(IntVector2(graphics->GetWidth()/2, graphics->GetHeight()/2));
        gameneedmorestarsframe_->SetScreenPositionExit(0, IntVector2(-300, graphics->GetHeight()/2));
        gameneedmorestarsframe_->SetScreenPositionExit(1, IntVector2(graphics->GetWidth()+300, graphics->GetHeight()/2));
        gameneedmorestarsframe_->SetScreenPositionEntranceForBonus(IntVector2(graphics->GetWidth()/2, -200));

        gameneedmorestarsframe_->Start(false, false);
    }

    SubscribeToEvent(gameneedmorestarsframe_, E_MESSAGEACK, URHO3D_HANDLER(PlayState, OnGameNeedMoreStarsAck));
}

void PlayState::OnGameNeedMoreStarsAck(StringHash eventType, VariantMap& eventData)
{
    SubscribeToEvent(GAME_OVER, URHO3D_HANDLER(PlayState, HandleGameOver));

	if (eventData[MessageACK::P_OK].GetBool())
    {
        String action = eventData[TextEntry::P_TEXT].GetString();
        if (action == "ShowAd")
        {
            bool ok = false;
#if defined(ACTIVE_ADS)
        #if defined(__ANDROID__)
            Pause(true, false, true);
            URHO3D_LOGINFO("PlayState() - OnGameNeedMoreStarsAck : Show Ad !");
            ok = Android_ShowAds();
            if (!ok)
                Pause(false, false);
        #endif
#endif
            if (!ok)
                action = "GoShop";
        }
        if (action == "GoShop")
        {
            OptionState* optionState = (OptionState*)GameStatics::stateManager_->GetState("Options");
            if (!optionState->IsShopOpened())
            {
                URHO3D_LOGINFO("PlayState() - OnGameNeedMoreStarsAck : Show Shop ...");
                optionState->SwitchShopFrame();
            }
        }
    }
	else
    {
        UnsubscribeFromEvent(gameneedmorestarsframe_, E_MESSAGEACK);

        URHO3D_LOGINFO("PlayState() - OnGameNeedMoreStarsAck : Quit !");
        DelayInformer::Get(this, 0.4f, GAME_PLAYEXIT);

        if (gameneedmorestarsframe_)
        {
            gameneedmorestarsframe_->Stop();
        #if defined(__ANDROID__)
            gameneedmorestarsframe_->SendEvent(GAME_UIFRAME_REMOVE);
        #else
            gameneedmorestarsframe_->Stop();
        #endif
        }

        GameHelpers::SetMusic(MAINMUSIC, 0.7f);
        GameHelpers::SetMusicVolume(1.f);
        GameHelpers::SetSoundVolume(1.f);
    }
}

void PlayState::OnQuitMessageAck(StringHash eventType, VariantMap& eventData)
{
	using namespace MessageACK;

    UnsubscribeFromEvent(messageBox, E_MESSAGEACK);

    messageBox.Reset();

	if (eventData[P_OK].GetBool())
    {
        URHO3D_LOGINFO("PlayState() - OnQuitMessageAck : Play !");

        GameStatics::AllowInputs(true);
        activeGameLogic_ = true;

        gameOver_ = false;
        GameStatics::moves_--;
        SendEvent(GAME_TRYREMOVED);
        SendEvent(GAME_MOVEADDED);
    }
	else
    {
        URHO3D_LOGINFO("PlayState() - OnQuitMessageAck : Quit !");

        SendEvent(GAME_PLAYEXIT);
    }

    GameHelpers::SetMusicVolume(1.f);
    GameHelpers::SetSoundVolume(1.f);
}

void PlayState::OnDelayedActions()
{

}

void PlayState::OnDelayedActions_Local(StringHash eventType, VariantMap& eventData)
{
    OnDelayedActions();
}

void PlayState::OnPostRenderUpdate(StringHash eventType, VariantMap& eventData)
{
    if (rootScene_ && drawDebug_)
    {
        DebugRenderer* debugRenderer = rootScene_->GetComponent<DebugRenderer>();
        if (!debugRenderer)
            return;
        if (GameStatics::gameConfig_.debugLights_)
        {
            PODVector<Light*> lights;
            rootScene_->GetComponents<Light>(lights,true);
//            URHO3D_LOGINFOF("nb Light=%d",lights.Size());
            for (unsigned i = 0; i < lights.Size(); ++i)
                lights[i]->DrawDebugGeometry(debugRenderer,true);
        }
        if (GameStatics::gameConfig_.debugAnimatedSprite2D)
        {
            PODVector<AnimatedSprite2D*> drawables;
            rootScene_->GetComponents<AnimatedSprite2D>(drawables,true);
//            URHO3D_LOGINFOF("nb AnimatedSprite2D=%d",drawables.Size());
            for (unsigned i = 0; i < drawables.Size(); ++i)
                drawables[i]->DrawDebugGeometry(debugRenderer,true);
        }
        if (GameStatics::gameConfig_.physics2DEnabled_ && GameStatics::gameConfig_.debugPhysics_)
        {
            PhysicsWorld2D* physicsWorld2D = rootScene_->GetComponent<PhysicsWorld2D>();
            if (physicsWorld2D) physicsWorld2D->DrawDebugGeometry();
        }
        if (GameStatics::gameConfig_.debugUI_)
        {
            GameStatics::ui_->DebugDraw(GameStatics::ui_->GetRoot());
            PODVector<UIElement*> children;
            GameStatics::ui_->GetRoot()->GetChildren(children, true);
            for (PODVector<UIElement*>::ConstIterator it=children.Begin(); it!=children.End(); ++it)
            {
                UIElement* element = *it;
                if (element->IsInternal())
                    continue;
                if (element->HasTag("Console"))
                    continue;

                GameStatics::ui_->DebugDraw(element);
            }
        }
        if (GameStatics::gameConfig_.debugMatches_)
        {
            MatchesManager::Get()->DrawDebugGeometry(debugRenderer,true);
        }
//        {
//            PODVector<StaticSprite2D*> drawables;
//            rootScene_->GetComponents<StaticSprite2D>(drawables,true);
//            for (unsigned i = 0; i < drawables.Size(); ++i)
//                drawables[i]->DrawDebugGeometry(debugRenderer,true);
//        }

        if (abilitypanel_)
            abilitypanel_->DrawDebugGeometry(debugRenderer,true);
    }
}

