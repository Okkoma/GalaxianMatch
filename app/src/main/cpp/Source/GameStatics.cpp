#include <iostream>
#include <cstdio>

#include <Urho3D/Urho3D.h>

#include <Urho3D/Core/Context.h>
#include <Urho3D/Core/Timer.h>

#include <Urho3D/Container/Ptr.h>

#include <Urho3D/Engine/Engine.h>
#include <Urho3D/Engine/Console.h>

#include <Urho3D/IO/Log.h>
#include <Urho3D/IO/FileSystem.h>

#include <Urho3D/Graphics/Graphics.h>
#include <Urho3D/Graphics/GraphicsEvents.h>
#include <Urho3D/Graphics/Renderer.h>
#include <Urho3D/Graphics/DebugRenderer.h>
#include <Urho3D/Graphics/Viewport.h>
#include <Urho3D/Graphics/Octree.h>
#include <Urho3D/Graphics/Camera.h>
#include <Urho3D/Graphics/Light.h>
#include <Urho3D/Graphics/Material.h>
#include <Urho3D/Graphics/Model.h>
#include <Urho3D/Graphics/StaticModel.h>
#include <Urho3D/Graphics/Texture.h>
#include <Urho3D/Graphics/Texture2D.h>

#include <Urho3D/Resource/Localization.h>
#include <Urho3D/Resource/Resource.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Resource/XMLFile.h>

#include <Urho3D/Input/Input.h>

#include <Urho3D/UI/UI.h>
#include <Urho3D/UI/UIElement.h>
#include <Urho3D/UI/Sprite.h>
#include <Urho3D/UI/Font.h>
#include <Urho3D/UI/Text.h>
#include <Urho3D/UI/Cursor.h>
#include <Urho3D/UI/Menu.h>
#include <Urho3D/UI/Window.h>

#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Scene/ObjectAnimation.h>
#include <Urho3D/Scene/ValueAnimation.h>

#include <Urho3D/Urho2D/Renderer2D.h>
#include <Urho3D/Urho2D/AnimatedSprite2D.h>
#include <Urho3D/Urho2D/AnimationSet2D.h>
#include <Urho3D/Urho2D/ParticleEmitter2D.h>
#include <Urho3D/Urho2D/ParticleEffect2D.h>
#include <Urho3D/Urho2D/Sprite2D.h>
#include <Urho3D/Urho2D/StaticSprite2D.h>
#include <Urho3D/Urho2D/CollisionCircle2D.h>
#include <Urho3D/Urho2D/RigidBody2D.h>
#include <Urho3D/Urho2D/SpriteSheet2D.h>
#include <Urho3D/Urho2D/PhysicsWorld2D.h>

#include "GameOptions.h"
#include "GameAttributes.h"
#include "GameRand.h"
#include "GameHelpers.h"
#include "GameEvents.h"

#include "GameStateManager.h"
#include "sSplash.h"
#include "sMainMenu.h"
#include "sLevelMap.h"
#include "sPlay.h"
#include "sCinematic.h"
#include "sOptions.h"

#include "ObjectPool.h"
#include "DelayAction.h"
#include "DelayInformer.h"
#include "TimerRemover.h"
#include "TextMessage.h"
#include "InteractiveFrame.h"

#include "Matches.h"
#include "MAN_Matches.h"

#include "GameStatics.h"


int UISIZE[NUMUIELEMENTSIZE];

const char* gameStatusNames[] =
{
    "MENUSTATE = 0",
    "LEVELSTATE",
    "LEVELSTATE_CHANGEZONE",
    "LEVELSTATE_CONSTELLATION",
    "CINEMATICSTATE",
    "PLAYSTATE_INITIALIZING",
    "PLAYSTATE_LOADING",
    "PLAYSTATE_FINISHLOADING",
    "PLAYSTATE_CLIENT_LOADING_SERVEROBJECTS",
    "PLAYSTATE_CLIENT_LOADING_REPLICATEDNODES",
    "PLAYSTATE_SYNCHRONIZING",
    "PLAYSTATE_READY",
    "PLAYSTATE_STARTGAME",
    "PLAYSTATE_RUNNING",
    "PLAYSTATE_ENDGAME",

    "MAX_GAMESTATUS"
};


/// Static Slots

Slot::Slot(const StringHash& cat, const StringHash& type, int qty, bool setindex) :
    cat_(cat.Value()),
    type_(type.Value()),
    qty_(qty)
{
    if (setindex)
        SetIndex();
}

Slot::Slot(const StringHash& cat, int index, int qty, bool settype) :
    cat_(cat.Value()),
    qty_(qty)
{
    indexcat_ = index != -1 ? index : GameRand::GetRand(OBJRAND, COT::GetObjectsInCategory(cat).Size());

    if (settype)
        type_ = COT::GetTypeFromCategory(cat, indexcat_).Value();
}

void Slot::Reset()
{
    cat_ = type_ = 0U;
    qty_ = 0;
    indexcat_ = -1;
}

void Slot::SetIndex()
{
    indexcat_ = COT::IsInCategory(StringHash(type_), StringHash(cat_));
}

String Slot::GetString() const
{
    return ToString("cat=%u type=%u qty=%d indexcat=%d", cat_, type_, qty_, indexcat_);
}

Slot Slot::ITEMS_2MOVES = Slot(COT::MOVES, StringHash("move2"), 2);
Slot Slot::ITEMS_4MOVES = Slot(COT::MOVES, StringHash("move4"), 4);
Slot Slot::ITEMS_2COINS = Slot(COT::COINS, StringHash("coin2"), 2);

/// Game Config

GameConfig::GameConfig() :
    language_(0),
    frameLimiter_(0),
    entitiesLimiter_(-1),
    touchEnabled_(false),
    forceTouch_(false),  // force show touch Emulation
    HUDEnabled_(true),
	networkMode_("auto"),
    ctrlCameraEnabled_(false),
    debugRenderEnabled_(true),
    physics3DEnabled_(false),
    physics2DEnabled_(true),
    enlightScene_(false),
    debugPhysics_(true),
    debugLights_(false),
    debugUI_(false),
    debugAnimatedSprite2D(false),
    debugMatches_(true),
    initState_(String::EMPTY),
    saveDir_(String::EMPTY),
    screenJoystickID_(-1),
    screenJoysticksettingsID_(-1),
    autoHideCursorEnable_(true),
    splashviewed_(false)
{ }


const int GameStatics::targetwidth_ = 540;
const int GameStatics::targetheight_ = 960;
GameConfig GameStatics::gameConfig_;
bool GameStatics::gameExit_ = false;
bool GameStatics::preloading_;
Sprite* GameStatics::preloaderIcon_ = 0;
int GameStatics::preloaderState_ = 0;
const long long GameStatics::preloadDelayUsec_ = 15000;
long long GameStatics::preloadtime_ = 0;

/// Game Scene Context
Context* GameStatics::context_;
Graphics* GameStatics::graphics_;
Input* GameStatics::input_;
UI* GameStatics::ui_;
Viewport* GameStatics::viewport_;
SharedPtr<Viewport> GameStatics::fixedViewport_;
Camera* GameStatics::camera_ = 0;
Camera* GameStatics::fixedCamera_ = 0;
SharedPtr<Scene> GameStatics::rootScene_;
Renderer2D* GameStatics::renderer2d_ = 0;
WeakPtr<Node> GameStatics::soundNodes_[NUMSOUNDCHANNELS];
int GameStatics::currentMusic_;
SharedPtr<Node> GameStatics::cameraNode_;
SharedPtr<Node> GameStatics::fixedCameraNode_;
WeakPtr<Node> GameStatics::controllablesNode_;

IntRect GameStatics::screenSceneRect_;
Vector2 GameStatics::fScreenSize_;
Vector2 GameStatics::gameScale_ = Vector2::ONE;
float GameStatics::uiScale_ = 1.f;
Cursor* GameStatics::cursor_ = 0;
int GameStatics::playTest_ = 0;
bool GameStatics::playingTest_ = false;
bool GameStatics::switchScaleXY_ = false;

/// Game States
GameStatus GameStatics::gameStatus_ = MENUSTATE;
GameStatus GameStatics::prevGameStatus_ = MENUSTATE;

int GameStatics::numRemainObjectives_ = 0;
bool GameStatics::allowInputs_ = false;
const String GameStatics::GAMENAME               = String("GalaxianMatch");
const char* GameStatics::saveDir_                = "Save/";
const char* GameStatics::savedGameFile_          = "Save/Save";
const char* GameStatics::savedPlayerFile_        = "Save/Player.xml";
const char* GameStatics::savedPlayerMissionFile_ = "Save/Missions_0";

/// Game's State to save
GameStatics::GameState GameStatics::gameState_ =
{
    {
        0, 0, { 0 }, 0, 0, 0, initialLevel, initialZone, initialCoins, initialTries, initialMoves, 0, { 0,0,0,0,0,0 },
    }
};

GameStateManager* GameStatics::stateManager_ = 0;

GameStatics::PlayerState* GameStatics::playerState_ = &GameStatics::gameState_.pstate_;

PODVector<RewardEventItem> GameStatics::rewardEventStack_;


/// Level's States
int GameStatics::gameMode_ = 0;
bool GameStatics::peerConnected_ = false;
String GameStatics::netidentity_;
const unsigned GameStatics::MAXZONES = NBMAXZONE;
float GameStatics::CameraZ_ = 10.f;
const Plane GameStatics::GroundPlane_ = Plane(Vector3(0.0f, 0.0f, 1.0f), Vector3(0.0f, 0.0f, 0.0f));
Rect GameStatics::MapBounds_  = Rect();
int& GameStatics::currentLevel_ = GameStatics::gameState_.pstate_.level;
int& GameStatics::currentZone_ = GameStatics::gameState_.pstate_.zone;
int GameStatics::currentZoneFirstMissionId_ = 0;
int GameStatics::currentZoneLastMissionId_ = 0;
int& GameStatics::tries_ = GameStatics::gameState_.pstate_.tries;
int& GameStatics::coins_ = GameStatics::gameState_.pstate_.coins;
int& GameStatics::moves_ = GameStatics::gameState_.pstate_.moves;

const unsigned NumLevelsByZone_[GameStatics::MAXZONES] =
{
    9, 15, 18, 4, 12, 17, 6, 9, 12, 6, 12, 18, 21
};

const unsigned BossLevel_[GameStatics::MAXZONES] =
{
    9, 24, 42, 44, 58, 75, 79, 85, 98, 108, 117, 138, 159
};

const LevelInfo classicLevelInfo1(CLASSICLEVELMODE, 3, SHAKE, true);
const LevelInfo bossLevelInfo1(BOSSMODE, 3, SHAKE, false);
const LevelInfo bossLevelInfo2(CLASSICLEVELMODE, 3, LOOSELIFE, true);

const LevelInfo* StdLevelInfos_[3] =
{
    &classicLevelInfo1, &bossLevelInfo1, &bossLevelInfo2
};

LevelInfo GameStatics::levelInfos_[NBMAXLVL];

LevelData* GameStatics::currentLevelDatas_;

const char* GameStatics::Musics[NUMMUSICS] =
{
    "Music/theme.ogg",
    "Music/win1.ogg",
    "Music/win2.ogg",
    "Music/loose.ogg",
    "Music/cinematic1.ogg",
    "Music/cinematic2.ogg",
    "Music/levelmap1.ogg",
    "Music/levelmap2.ogg",
    "Music/levelmap3.ogg",
    "Music/play.ogg",
    "Music/play.ogg",
    "Music/play.ogg",
    "Music/boss1.ogg",
    "Music/boss2.ogg",
    "Music/boss3.ogg",
    "Music/constellation1.ogg",
    "Music/constellation2.ogg",
    "Music/constellation3.ogg"
};

const char* GameStatics::Sounds[NUMSOUNDS] =
{
    "Sounds/blg_energy_field_11a.wav",
    "Sounds/blg_a_robo_09.wav",
    "Sounds/blg_beam_01a.wav",
    "Sounds/blg_beam_02a.wav",
    "Sounds/blg_energy_field_01a.wav",
    "Sounds/blg_energy_field_01a.wav",
    "Sounds/warning.ogg",
    "Sounds/blg_energy_field_03.wav",
    "Sounds/blg_energy_field_09.wav",
    "Sounds/blg_energy_field_09.wav",
};

/// Functions

void GameStatics::Initialize(Context* context)
{
    URHO3D_LOGINFO("GameStatics() - ----------------------------------------");
    URHO3D_LOGINFO("GameStatics() - Initialize  ....                       -");
    URHO3D_LOGINFO("GameStatics() - ----------------------------------------");

    GameStatics::context_ = context;
    GameStatics::graphics_ = context->GetSubsystem<Graphics>();
    GameStatics::input_ = context->GetSubsystem<Input>();
    GameStatics::ui_ = context->GetSubsystem<UI>();

    TimerRemover::Reset(500);
    DelayInformer::Reset(500);
    DelayAction::Reset(500);
    TextMessage::Reset(20);

    // Set default UI style
    UIElement* uiroot = GameStatics::ui_->GetRoot();
    uiroot->SetDefaultStyle(context->GetSubsystem<ResourceCache>()->GetResource<XMLFile>("UI/DefaultStyle.xml"));

	// Create Camera
    GameStatics::cameraNode_ = new Node(context);
    GameStatics::fixedCameraNode_ = new Node(context);
    GameStatics::camera_ = GameStatics::cameraNode_->CreateComponent<Camera>(LOCAL);
    GameStatics::fixedCamera_ = GameStatics::fixedCameraNode_->CreateComponent<Camera>(LOCAL);

    // Create Root Scene
    GameStatics::rootScene_ = new Scene(context);
    GameStatics::rootScene_->SetName("RootScene");
    GameStatics::rootScene_->CreateComponent<Octree>(LOCAL);
    GameStatics::renderer2d_ = GameStatics::rootScene_->GetOrCreateComponent<Renderer2D>(LOCAL);

    // Create Sound Channels
    InitSoundChannels();

    URHO3D_LOGINFOF("GameStatics() - Initialize() : ... rootSceneId=%u cameraNodeId=%u cameraId=%u",
                    GameStatics::rootScene_->GetID(), GameStatics::cameraNode_->GetID(), GameStatics::camera_->GetID());

    fixedViewport_ = SharedPtr<Viewport>(new Viewport(context, GameStatics::rootScene_, GameStatics::fixedCamera_));

    SharedPtr<Viewport> viewport(new Viewport(context, GameStatics::rootScene_, GameStatics::camera_));
    context->GetSubsystem<Renderer>()->SetViewport(0, viewport);
    GameStatics::viewport_ = context->GetSubsystem<Renderer>()->GetViewport(0);

    if (gameConfig_.debugRenderEnabled_)
    {
        viewport->SetDrawDebug(true);
        GameStatics::rootScene_->GetOrCreateComponent<DebugRenderer>(LOCAL);
    }

    SetGameScale();

    // Create Game State Manager
	stateManager_ = new GameStateManager(context);
	stateManager_->RegisterState(new SplashState(context));
	stateManager_->RegisterState(new MenuState(context));
	stateManager_->RegisterState(new LevelMapState(context));
	stateManager_->RegisterState(new PlayState(context));
    stateManager_->RegisterState(new CinematicState(context));
    stateManager_->RegisterState(new OptionState(context));

    stateManager_->AddToStack("MainMenu");

//    if (!GameStatics::playerState_->firstCinematicPlayed)
//    {
//        CinematicState* cinematicstate = (CinematicState*)stateManager_->GetState("Cinematic");
//        cinematicstate->SetSceneFile("Data/Cinematic/Intro/test4.xml");
//        stateManager_->AddToStack("Cinematic");
//        GameStatics::playerState_->firstCinematicPlayed = true;
//    }

    if (gameConfig_.initState_.Empty() && !GameStatics::playTest_)
    {
        URHO3D_LOGINFO("GameStatics() - Initialize ... : Add Default State : Splash ...");
        stateManager_->AddToStack("Splash");
    }
    else
        URHO3D_LOGINFO("GameStatics() - Initialize ... : Add Default State : MainMenu ...");

    URHO3D_LOGINFO("GameStatics() - ----------------------------------------");
    URHO3D_LOGINFO("GameStatics() - Initialize ....     OK !               -");
    URHO3D_LOGINFO("GameStatics() - ----------------------------------------");
}

void GameStatics::CreateUICursors()
{
    if (cursor_)
        return;

    // Create Cursor, initialize to invisible
    Image* image = context_->GetSubsystem<ResourceCache>()->GetResource<Image>("UI/fireball.png");
    if (image)
    {
        Cursor* cursor = new Cursor(context_);
		cursor->DefineShape(CS_NORMAL, image, IntRect(0,0,24,24), IntVector2(12,12));
        GameStatics::ui_->SetCursor(cursor);

        GameStatics::cursor_ = GameStatics::ui_->GetCursor();
        GameStatics::cursor_->SetUseSystemShapes(false);
        GameStatics::cursor_->SetPosition(GameStatics::input_->GetMousePosition());
        GameStatics::cursor_->SetVisible(false);
        GameStatics::cursor_->SetColor(Color(1.f,1.f,1.f,0.7f));
    }
}

void GameStatics::InitMouse(Context* context)
{
    if (gameConfig_.touchEnabled_)
        return;

    CreateUICursors();

    GameStatics::input_->SetMouseMode(MM_FREE);
    GameStatics::input_->SetMouseVisible(false);
    GameStatics::input_->SetMouseGrabbed(false);
}

bool savedTouchEnabled_ = false;

void GameStatics::InitTouchInput(Context* context)
{
    gameConfig_.touchEnabled_ = true;

#ifdef ACTIVE_TOUCHTIPS
    ResourceCache* cache = context->GetSubsystem<ResourceCache>();

    XMLFile* layout = cache->GetResource<XMLFile>("UI/ScreenJoystick.xml");
    gameConfig_.screenJoystickID_ = input_->AddScreenJoystick(layout, cache->GetResource<XMLFile>("UI/DefaultStyle.xml"));
    gameConfig_.screenJoysticksettingsID_ = -1;

    input_->SetScreenJoystickVisible(gameConfig_.screenJoystickID_, false);

    UIElement* uiscreenjoy = input_->GetJoystick(gameConfig_.screenJoystickID_)->screenJoystick_;
    uiscreenjoy->SetSize(graphics_->GetWidth(), graphics_->GetHeight());

    URHO3D_LOGINFOF("GameStatics() - InitTouchInput : screenJoystickID_=%d ", gameConfig_.screenJoystickID_);
#endif
}

void GameStatics::SetTouchEmulation(bool enable)
{
    if (input_->GetTouchEmulation() != enable)
    {
        URHO3D_LOGINFOF("GameStatics - SetTouchEmulation : enable = %s", enable ? "true":"false");

        input_->SetTouchEmulation(enable);

        if (enable)
            savedTouchEnabled_ = gameConfig_.touchEnabled_;
        else
            enable = savedTouchEnabled_;

        gameConfig_.touchEnabled_ = enable;

        SetMouseVisible(!enable, false);
    }
}

void GameStatics::InitSoundChannels()
{
    if (!rootScene_)
        return;

    for (int i=0; i < NUMSOUNDCHANNELS; i++)
    {
        soundNodes_[i] = rootScene_->CreateChild(String("SndChannel")+String(i), LOCAL);
    }
}

void GameStatics::ResetCamera()
{
    if (!cameraNode_)
    {
        cameraNode_ = new Node(context_);
        camera_ = cameraNode_->CreateComponent<Camera>(LOCAL);
    }

    ChangeCameraTo(cameraNode_, true);
}

void GameStatics::ChangeCameraTo(Node* node, bool ortho)
{
    if (!node)
        return;

    Camera* camera = node->GetComponent<Camera>();
    if (camera)
    {
        context_->GetSubsystem<Renderer>()->GetViewport(0)->SetCamera(camera);
        camera->SetOrthographic(ortho);

        if (rootScene_)
            rootScene_->GetComponent<Renderer2D>()->UpdateFrustumBoundingBox(camera);
    }
}

void GameStatics::AllowInputs(bool state)
{
    if (state != allowInputs_)
    {
        URHO3D_LOGINFOF("GameStatics() - AllowInputs : enable = %s !", state?"true":"false");
        allowInputs_ = state;
    }
}

void GameStatics::UpdateViews()
{
    URHO3D_LOGINFOF("GameStatics() - UpdateViews ... ");

    Renderer* renderer = camera_->GetSubsystem<Renderer>();
//    Viewport* viewport = renderer->GetViewport(0);
//    if (!viewport->GetView())
//        viewport->AllocateView();

    Renderer2D* renderer2d = rootScene_->GetOrCreateComponent<Renderer2D>(LOCAL);

//    using namespace BeginViewRender;
//    VariantMap& eventData = context_->GetEventDataMap();
//    eventData[P_VIEW] = viewport->GetView();
//    eventData[P_SCENE] = rootScene_;
//    eventData[P_CAMERA] = camera_;
//    renderer->SendEvent(E_BEGINVIEWUPDATE, eventData);
    renderer->SendEvent(E_SCREENMODE);

    renderer2d->UpdateFrustumBoundingBox(camera_);

    URHO3D_LOGINFOF("GameStatics() - UpdateViews ... OK !");
}

void GameStatics::CreatePreloaderIcon()
{
    Texture2D* iconTexture = GameStatics::context_->GetSubsystem<ResourceCache>()->GetResource<Texture2D>("UI/Textures/loadericon.png");

    if (!iconTexture)
    {
        URHO3D_LOGERRORF("No Loader Icon Texture !");
        return;
    }

    preloaderIcon_ = GameStatics::context_->GetSubsystem<UI>()->GetRoot()->CreateChild<Sprite>();
    preloaderIcon_->SetTexture(iconTexture);
    preloaderIcon_->SetSize(floor((float)iconTexture->GetWidth() * GameStatics::uiScale_), floor((float)iconTexture->GetHeight() * GameStatics::uiScale_));
    preloaderIcon_->SetHotSpot(preloaderIcon_->GetSize()/2);
    float maxoffset = (float)Max(preloaderIcon_->GetWidth(), preloaderIcon_->GetHeight()) / 2.f;
    preloaderIcon_->SetPosition(maxoffset, GameStatics::context_->GetSubsystem<Graphics>()->GetHeight()-maxoffset);
    preloaderIcon_->SetOpacity(0.98f);
    preloaderIcon_->SetPriority(1000);

    SharedPtr<ObjectAnimation> objectAnimation(new ObjectAnimation(GameStatics::context_));
    SharedPtr<ValueAnimation> rotateAnimation(new ValueAnimation(GameStatics::context_));
    rotateAnimation->SetKeyFrame(0.f, 0.f);
    rotateAnimation->SetKeyFrame(0.5f, 360.f);
    objectAnimation->AddAttributeAnimation("Rotation", rotateAnimation, WM_LOOP);
    preloaderIcon_->SetObjectAnimation(objectAnimation);

    preloaderIcon_->SetVisible(true);

    URHO3D_LOGINFOF("GameStatics() - CreateLoaderIcon ... OK !");
}

bool GameStatics::PreloadResources()
{
#ifdef ACTIVE_PRELOADER
    static HiresTimer timer;
    timer.Reset();

    bool ok = false;

    if (preloaderState_ == 0)
    {
        if (!GameStatics::rootScene_)
        {
            URHO3D_LOGERROR("GameStatics() - PreLoadResources : no scene !");
            return true;
        }

        URHO3D_LOGINFO("GameStatics() - --------------------------------------------------------");
        URHO3D_LOGINFO("GameStatics() - PreLoadResources ...                                   -");
        URHO3D_LOGINFO("GameStatics() - --------------------------------------------------------");

        GameStatics::rootScene_->SetAsyncLoadingMs(preloadDelayUsec_/1000);

        GameHelpers::SetGameLogEnable(GameStatics::rootScene_->GetContext(), GAMELOG_PRELOAD, false);

        GOT::LoadJSONFile(GameStatics::rootScene_->GetContext(), "Data/Objects/GOTPackage1.json");

        Slot::ITEMS_2MOVES.SetIndex();
        Slot::ITEMS_4MOVES.SetIndex();
        Slot::ITEMS_2COINS.SetIndex();

//        CreatePreloaderIcon();

        preloading_ = true;
        preloadtime_ = 0;
        preloaderState_++;

        long long dt = timer.GetUSec(false);
        preloadtime_ += dt;
        URHO3D_LOGINFOF("GameStatics() - PreLoadResources : STEP0 ... GOTPackage1.json ... time=%d (dt=%d) ", preloadtime_, dt);

        if (dt > preloadDelayUsec_)
            return false;
    }

    if (preloaderState_ == 1)
    {
        Node* preloader = GameStatics::rootScene_->GetChild("PreLoad");
        if (!preloader)
        {
            preloader = GameStatics::rootScene_->CreateChild("PreLoad", LOCAL);

            if (GameHelpers::LoadNodeXML(GameStatics::rootScene_->GetContext(), preloader, "Data/Scenes/PreLoader.xml", LOCAL))
                preloader->SetEnabled(false);
        }

        preloaderState_++;

        long long dt = timer.GetUSec(false);
        URHO3D_LOGINFOF("GameStatics() - PreLoadResources : STEP1 ... PreLoader.xml ... time=%d (dt=%d) ", preloadtime_, dt);
        if (dt > preloadDelayUsec_)
            return false;
    }

    if (preloaderState_ == 2)
    {
        Node* preloaderGOT = GameStatics::rootScene_->GetChild("PreLoadGOT");
        ok = preloaderGOT != 0;

        if (!ok)
        {
            URHO3D_LOGINFOF("GameStatics() - PreLoadResources : PreLoadGOT ...");
            preloaderGOT = GameStatics::rootScene_->CreateChild("PreLoadGOT", LOCAL);
            preloaderState_++;
        }

        long long dt = timer.GetUSec(false);
        preloadtime_ += dt;
        URHO3D_LOGINFOF("GameStatics() - PreLoadResources : STEP2 ... PreLoadGOT ... time=%d (dt=%d) ", preloadtime_, dt);
        if (!ok && dt > preloadDelayUsec_)
            return false;
    }

    if (preloaderState_ > 2)
    {
    #ifdef ACTIVE_POOL
        ok = GOT::PreLoadObjects(preloaderState_, &timer, preloadDelayUsec_, GameStatics::rootScene_->GetChild("PreLoadGOT"), true);
    #else
        ok = GOT::PreLoadObjects(preloaderState_, &timer, preloadDelayUsec_, GameStatics::rootScene_->GetChild("PreLoadGOT"), false);
    #endif

        if (!ok)
        {
            long long dt = timer.GetUSec(false);
            preloadtime_ += dt;
            URHO3D_LOGINFOF("GameStatics() - PreLoadResources : STEP3 ... GOT::PreLoadObjects ... time=%d (dt=%d) ", preloadtime_, dt);
            return false;
        }
    }

    if (ok)
    {
        GameHelpers::SetGameLogEnable(GameStatics::rootScene_->GetContext(), GAMELOG_PRELOAD, true);

    #ifdef DUMP_RESOURCES
        URHO3D_LOGINFO("GameStatics() - ---------------------------------------------------------------");
        URHO3D_LOGINFO("GameStatics() - PreLoadResources ... Dump ...                                 -");
        URHO3D_LOGINFO("GameStatics() - ---------------------------------------------------------------");

        GOT::DumpAll();
        COT::DumpAll();

        if (ObjectPool::Get())
            ObjectPool::Get()->DumpCategories();
    #endif

        const Vector<StringHash>& bosses = COT::GetObjectsInCategory(COT::BOSSES);
        NBMAXBOSSES = bosses.Size();

        URHO3D_LOGINFO("GameStatics() - ---------------------------------------------------------------");
        URHO3D_LOGINFOF("GameStatics() - PreLoadResources ... time=%fs ... numbosses=%d OK !          -", (float)preloadtime_*0.000001f, NBMAXBOSSES);
        URHO3D_LOGINFO("GameStatics() - ---------------------------------------------------------------");

        GameStatics::rootScene_->GetChild("PreLoadGOT")->SetEnabled(false);
        preloaderState_ = 0;
        if (preloaderIcon_)
        {
            preloaderIcon_->SetVisible(false);
            preloaderIcon_->SetAnimationEnabled(false);
        }
        preloading_ = false;
        return true;
    }
#endif

    return false;
}

bool GameStatics::UnloadResources()
{
    URHO3D_LOGINFO("GameStatics() - ----------------------------------------");
    URHO3D_LOGINFO("GameStatics() - UnloadRessources  ....                 -");
    URHO3D_LOGINFO("GameStatics() - ----------------------------------------");

    GameHelpers::SetGameLogEnable(GameStatics::rootScene_->GetContext(), GAMELOG_PRELOAD, false);

    if (GameStatics::rootScene_)
    {
        Node* preloaderGOT = GameStatics::rootScene_->GetChild("PreLoadGOT");
        if (preloaderGOT)
        {
            URHO3D_LOGINFO("UnloadRessources ... -> PreLoaderGOT To Remove ...");

            GOT::UnLoadObjects(preloaderGOT);

            URHO3D_LOGINFO("UnloadRessources ... -> PreLoaderGOT Removed OK !");
        }

        Node* preloader = GameStatics::rootScene_->GetChild("PreLoad");
        if (preloader)
        {
            URHO3D_LOGINFO("UnloadRessources ... -> PreLoader To Remove ...");

            preloader->Remove();

            URHO3D_LOGINFO("UnloadRessources ... -> PreLoader Removed OK !");
        }
    }

    GameHelpers::SetGameLogEnable(GameStatics::rootScene_->GetContext(), GAMELOG_PRELOAD, true);

    URHO3D_LOGINFO("GameStatics() - ----------------------------------------");
    URHO3D_LOGINFO("GameStatics() - UnloadRessources  .... OK !            -");
    URHO3D_LOGINFO("GameStatics() - ----------------------------------------");

    return true;
}

void GameStatics::SetLevelInfo(int level, int levelInfoID)
{
    levelInfos_[level-1] = *StdLevelInfos_[levelInfoID];
}

void GameStatics::SetLevelInfos()
{
    // Classic Level Infos
    for (int i=0; i < NBMAXLVL; i++)
        levelInfos_[i] = classicLevelInfo1;

    // Boss Level Infos
    for (int i=0; i < MAXZONES; i++)
        levelInfos_[BossLevel_[i]-1] = bossLevelInfo1;

    // Hybrid Level Infos
    levelInfos_[BossLevel_[2]-1] = bossLevelInfo2;

    /// TODO : Playable Boss Levels but to Custom
    for (int i=4; i < MAXZONES; i++)
        levelInfos_[BossLevel_[i]-1] = bossLevelInfo2;

    // Unlock powers
    levelInfos_[0].newpowers_[0] = 0;
    levelInfos_[0].newpowers_[1] = 1;
    levelInfos_[BossLevel_[0]-1].newpowers_[0] = 2;
    levelInfos_[BossLevel_[2]-1].newpowers_[0] = 3;
    levelInfos_[BossLevel_[4]-1].newpowers_[0] = 4;
    levelInfos_[BossLevel_[6]-1].newpowers_[0] = 5;
}

void GameStatics::GameState::Load()
{
	String filename(gameConfig_.saveDir_ + String(savedGameFile_) + String(gameDataVersion_) + String(".bin"));

    if (!GameHelpers::LoadData(context_, filename.CString(), &pstate_))
        gameState_.Reset();

    UpdateStoryItems();
//    Dump();
}

void GameStatics::GameState::Save()
{
#if !defined(TESTMODE) && defined(ACTIVE_SERIALIZEGAMESTATE)
    // Save GameData
    unsigned newtime = context_->GetSubsystem<Time>()->GetTimeSinceEpoch();
    GameHelpers::SaveData(context_, &pstate_, sizeof(PlayerState), String(gameConfig_.saveDir_ + String(savedGameFile_) + String(gameDataVersion_) + String(".bin")).CString());

    URHO3D_LOGINFOF("GameStatics::GameState() - Save : time=%u saveplaytime=%u", newtime, pstate_.lastplaytime);

//    Dump();
#endif
}

void GameStatics::GameState::Reset()
{
    // Player States
    pstate_.lastplaytime = 0;
    pstate_.score = 0;
    for (int i=0; i < NBMAXZONE; i++)
        pstate_.cinematicShown_[i] = CINEMATIC_NONE;
    pstate_.musicEnabled_ = 1;
    pstate_.soundEnabled_ = 1;
    pstate_.tutorialEnabled_ = 1;
    pstate_.level = initialLevel;
    pstate_.zone = initialZone;
    pstate_.tries = initialTries;
    pstate_.moves = initialMoves;
    pstate_.coins = initialCoins;

    for (unsigned i=0; i < MAXABILITIES; i++)
        pstate_.powers_[i].state_ = 0;

    ResetTutorialState();

    // Mission States
    for (int i=0; i < NBMAXLVL;i++)
    {
        MissionState& mstate = pstate_.missionstates[i];
        mstate.state_ = MissionState::MISSION_LOCKED;
        mstate.score_ = 0;
        // Mission Bonuses
        for (int j=0; j < MAXMISSIONBONUSES; j++)
            mstate.bonuses_[j].Reset();
        // Mission Links
        for (int j=0; j < MAXMISSIONBONUSES; j++)
            mstate.linkedmissions_[j] = 0;
    }

    // Zone States
    for (int i=0; i < NBMAXZONE; i++)
        pstate_.zonestates[i] = ZONE_BLOCKED;

#ifdef TESTMODE
    const int maxlvl = NBMAXLVL;
    const int maxzone = NBMAXZONE;
    // Mission States
    for (int i=0; i < maxlvl; i++)
    {
        MissionState& mstate = pstate_.missionstates[i];
        mstate.state_ = MissionState::MISSION_COMPLETED;
        mstate.score_ = 0;
    }
    // Just Unlock Bosses
    for (int i=0; i < maxzone; i++)
        pstate_.missionstates[BossLevel_[i]-1].state_ = MissionState::MISSION_UNLOCKED;
    // Unlock Zones
    for (int i=0; i < maxzone; i++)
        pstate_.zonestates[i] = ZONE_UNBLOCKED;
    // Unlock powers
    PODVector<int> unlockedpowers;
    for (int i=0; i < maxlvl; i++)
        if (pstate_.missionstates[i].state_ == MissionState::MISSION_COMPLETED)
            UnlockPowers(i, unlockedpowers);
//    for (int i=0; i < maxzone-1; i++)
//        pstate_.cinematicShown_[i] = CINEMATIC_BOSSDEFEAT;
#elif defined(ACTIVE_TESTMODE)
    const int maxlvl  = GameStatics::GetMinLevelId(initialZone);
    const int maxzone = initialZone;
    // Cinematics
    for (int i=0; i < maxzone; i++)
        pstate_.cinematicShown_[i] = CINEMATIC_BOSSDEFEAT;
    // Mission States
    for (int i=0; i < maxlvl; i++)
    {
        MissionState& mstate = pstate_.missionstates[i];
        mstate.state_ = MissionState::MISSION_COMPLETED;
        mstate.score_ = 0;
    }
    for (int i=0; i < maxzone; i++)
        pstate_.missionstates[BossLevel_[i]-1].state_ = MissionState::MISSION_COMPLETED;
    // Unlock Zones
    for (int i=0; i < maxzone-1; i++)
        pstate_.zonestates[i] = ZONE_UNBLOCKED;
    // Unlock powers
    PODVector<int> unlockedpowers;
    for (int i=0; i < maxlvl; i++)
        if (pstate_.missionstates[i].state_ == MissionState::MISSION_COMPLETED)
            UnlockPowers(i, unlockedpowers);
#endif

    pstate_.zonestates[pstate_.zone-1] = ZONE_UNBLOCKED;

    for (int i=0; i < MAXZONEOBJECTS; i++)
        storyitems_[i] = false;

    UpdateStoryItems();

//    gameState_.Dump();
}

void GameStatics::GameState::SetMissionState(int missionid, const GameStatics::MissionState& mission)
{
    pstate_.missionstates[missionid-1] = mission;
}

void GameStatics::GameState::GetMissionState(int missionid, GameStatics::MissionState& mission)
{
    mission = pstate_.missionstates[missionid-1];
}

unsigned GameStatics::GameState::GetNextLevel(int currentlevel) const
{
    if (!currentlevel)
    {
        if (pstate_.zonestates[GameStatics::currentZone_] == CONSTELLATION_UNBLOCKED)
            return 0;

        currentlevel = GameStatics::currentLevel_;
    }

    int nextlevel = 0;
    int unlockedMissions = 0;
    Vector<int> completedMissions;

    for (unsigned i = 0; i < 4; i++)
    {
        int linkedmissionid = pstate_.missionstates[currentlevel-1].linkedmissions_[i];
        if (linkedmissionid > 0)
        {
            int state = pstate_.missionstates[linkedmissionid-1].state_;
            if (state == GameStatics::MissionState::MISSION_UNLOCKED)
            {
                unlockedMissions++;
                if (unlockedMissions == 1)
                    nextlevel = linkedmissionid;
            }
            else if (state == GameStatics::MissionState::MISSION_COMPLETED)
                completedMissions.Push(linkedmissionid);
        }
    }

    // several unlocked missions, player has to do a choice
    if (unlockedMissions > 1)
        nextlevel = 0;
    // find a better nextlevel if exists
    else if (!unlockedMissions)
        nextlevel = Max(0, GameStatics::GetNextUnlockedMission(false));

    URHO3D_LOGINFOF("GameStatics::GameState() - GetNextLevel : currentLevel=%d unlockedsize=%u completedsize=%u nextlevel=%d",
                    currentlevel, unlockedMissions, completedMissions.Size(), nextlevel);

    return nextlevel;
}

int GameStatics::GameState::UpdateMissionScores(int missionid)
{
    MissionState& mstate = pstate_.missionstates[missionid-1];

    // Get Metrics
    unsigned numObjectives = MatchesManager::GetObjectives().Size();
    for (unsigned i=0; i < MAXOBJECTIVES; i++)
    {
        if (i < MatchesManager::GetObjectives().Size())
            mstate.objectives_[i] = MatchesManager::GetObjectives()[i];
        else
            mstate.objectives_[i].Reset();
    }
    mstate.numMovesUsed_ = MatchesManager::GetNumMovesUsed();
    mstate.elapsedTime_ = MatchesManager::GetElapsedTime();

    // Calculate the score : 0...3 stars
    mstate.score_ = 0;

    // critere1 : coeff 2 - ratio de mouvements = nombre de mouvements utilisés / nombre des mouvements à réalisés pour faire des match3 et gagner
    float sumScoreObjectives = 0.f;
    for (unsigned i=0; i < numObjectives; i++)
        sumScoreObjectives += mstate.objectives_[i].target_;
    float crit1 = sumScoreObjectives ? mstate.numMovesUsed_ / (sumScoreObjectives / 3) : 10.f;
    URHO3D_LOGINFOF("GameStatics::GameState() - UpdateMissionScores : mission=%d crit1=%F", missionid, crit1);
    if (crit1 <= 0.25f)
        crit1 = 3.f;
    else if (crit1 <= 0.5f)
        crit1 = 2.f;
    else if (crit1 <= 1.f)
        crit1 = 1.f;
    else
        crit1 = 0.f;

    // critere2 : coeff 1 - ratio d'équilibre = les scores des objectifs sont équilibrés => equilibre = moyenne des ecarts entre objectifs.
    float crit2 = 0.f;
    if (numObjectives > 1)
    {
        for (unsigned i=0; i < numObjectives; i++)
            crit2 += (float)(mstate.objectives_[i].count_ - mstate.objectives_[i].target_) / mstate.objectives_[i].target_;
        crit2 /= numObjectives;

        URHO3D_LOGINFOF("GameStatics::GameState() - UpdateMissionScores : mission=%d crit2=%F", missionid, crit2);
        // depassement inf à +50%
        if (crit2 <= 0.5f)
            crit2 = 3.f;
        // depassement inf à +100%
        else if (crit2 <= 1.5f)
            crit2 = 2.f;
        // depassement inf à +200%
        else if (crit2 <= 3.f)
            crit2 = 1.f;
        else
            crit2 = 0.f;
    }

    mstate.score_ = Clamp((int)Round((crit1 * 2 + crit2) / 3), 0, 3);

    URHO3D_LOGINFOF("GameStatics::GameState() - UpdateMissionScores : mission=%d nummoves=%d score=%d score1=%F score2=%F", missionid, mstate.numMovesUsed_, mstate.score_, crit1, crit2);

    return mstate.score_;
}

bool GameStatics::GameState::UpdateMission(int missionid, int state)
{
    MissionState& m = pstate_.missionstates[missionid-1];

    bool changestate = m.state_ != state && state != -1;

    if (changestate)
    {
        URHO3D_LOGINFOF("GameStatics::GameState() - UpdateMission : mission=%d oldstate=%d newstate=%d", missionid, m.state_, state);
        m.state_ = state;
    }

    if (m.state_ == GameStatics::MissionState::MISSION_COMPLETED)
    {
        for (int j=0; j < 4; j++)
        {
            int linkedmissionid = m.linkedmissions_[j];
            if (linkedmissionid > 0)
            {
                MissionState& linkedmstate = pstate_.missionstates[linkedmissionid-1];
                if (linkedmstate.state_ == GameStatics::MissionState::MISSION_LOCKED)
                    linkedmstate.state_ = GameStatics::MissionState::MISSION_UNLOCKED;
            }
        }
    }

    URHO3D_LOGINFOF("GameStatics::GameState() - UpdateMission : ... OK !");

    return changestate;
}

void GameStatics::GameState::UnlockPowers(int missionid, PODVector<int>& unlockedpowers)
{
	if (!missionid)
		return;

    MissionState& m = pstate_.missionstates[missionid-1];

    for (int i=0; i < MAXNEWPOWERSBYLVL; i++)
    {
        int index = levelInfos_[missionid-1].newpowers_[i];

        if (index != -1 && index < MAXABILITIES)
        {
            if (!pstate_.powers_[index].enabled_)
            {
                unlockedpowers.Push(index);

                pstate_.powers_[index].enabled_ = 1;

                URHO3D_LOGINFOF("GameStatics::GameState() - UnlockPowers : mission=%d unlock power=%d", missionid, index);
            }
        }
    }
}

void GameStatics::GameState::UpdateStoryItems()
{
    // all items are removed at constellation 12
    const int tictactoebosslevelid = GameStatics::GetBossLevelId(11);
    const bool constellation12reached = (pstate_.missionstates[tictactoebosslevelid-1].state_ == MissionState::MISSION_COMPLETED);

    // item 1 - plume : constellation 8 unlocked (cancer finished)
    const int cancerbosslevelid = GameStatics::GetBossLevelId(7);
    storyitems_[0] = (pstate_.missionstates[cancerbosslevelid-1].state_ == MissionState::MISSION_COMPLETED && constellation12reached == false);
    // item 2 - singe : constellation 9 unlocked (lion finished)
    const int lionbosslevelid = GameStatics::GetBossLevelId(8);
    storyitems_[1] = (pstate_.missionstates[lionbosslevelid-1].state_ == MissionState::MISSION_COMPLETED && constellation12reached == false);
    // item 3 - oeuf : constellation 10 unlocked (virgo finished)
    const int virgobosslevelid = GameStatics::GetBossLevelId(9);
    storyitems_[2] = (pstate_.missionstates[virgobosslevelid-1].state_ == MissionState::MISSION_COMPLETED && constellation12reached == false);

    storyitems_[3] = false;

#ifdef TESTMODE
    storyitems_[0] = storyitems_[1] = storyitems_[2] = true;
#endif

    URHO3D_LOGINFOF("GameStatics::GameState() - UpdateStoryItems : ... OK !");
}

void GameStatics::GameState::ResetTutorialState()
{
    for (unsigned i=0; i < MAXABILITIES; i++)
        pstate_.powers_[i].shown_ = 0;

    for (unsigned i=0; i < NBMAXMESSAGES; i++)
        pstate_.archivedmessages_[i] = 0U;
}


void GameStatics::GameState::Dump() const
{
    URHO3D_LOGINFOF("GameStatics::GameState() - Dump : ...");
    URHO3D_LOGINFOF("GameStatics::GameState() - Dump : lastplay=%u score=%u level=%d zone=%d coins=%d tries=%d moves=%d powers=%u,%u,%u,%u",
                    pstate_.lastplaytime, pstate_.score, pstate_.level, pstate_.zone, pstate_.coins, pstate_.tries, pstate_.moves,
                    pstate_.powers_[0].qty_, pstate_.powers_[1].qty_, pstate_.powers_[2].qty_, pstate_.powers_[3]);

    for (int i=0; i < NBMAXLVL; i++)
    {
        const MissionState& mstate = pstate_.missionstates[i];

        URHO3D_LOGINFOF("GameStatics::GameState() - Dump : mission=%d state=%d bonuses={slot1=%s, slot2=%s, slot3=%s, slot4=%s } linkedmissions=%d,%d,%d,%d",
                        i+1, mstate.state_,
                        mstate.bonuses_[0].GetString().CString(), mstate.bonuses_[1].GetString().CString(),
                        mstate.bonuses_[2].GetString().CString(), mstate.bonuses_[3].GetString().CString(),
                        mstate.linkedmissions_[0], mstate.linkedmissions_[1], mstate.linkedmissions_[2], mstate.linkedmissions_[3]);
    }

    for (int i=0; i < NBMAXZONE; i++)
        URHO3D_LOGINFOF("GameStatics::GameState() - Dump : zone%d=%d", i+1, pstate_.zonestates[i]);

    URHO3D_LOGINFOF("GameStatics::GameState() - Dump : ... OK !");
}

void GameStatics::CheckTimeForEarningStars()
{
    // Earn 1 try for each EARNSTAR_DELAY passed (max 4 tries)
    if (playerState_->lastplaytime)
    {
        unsigned newtime = context_->GetSubsystem<Time>()->GetTimeSinceEpoch();
        int deltatime = newtime - playerState_->lastplaytime;
        if (deltatime > 0)
        {
            int numNewTries = Min(Min(deltatime / EARNSTAR_DELAY, EARNSTAR_MAXAUTHORIZEDSTARS - tries_), MAX_ADDINGSTARS);

            URHO3D_LOGINFOF("GameStatics() - CheckTimeForEarningStars : time-lastvisit=%d EARNSTAR_DELAY=%d", deltatime, EARNSTAR_DELAY);

            if (numNewTries)
            {
                URHO3D_LOGINFOF("GameStatics() - CheckTimeForEarningStars : Earn %d Stars", numNewTries);

                playerState_->lastplaytime = newtime;

                AddEarnStars(numNewTries);
            }
        }
    }
    else
    {
        playerState_->lastplaytime = context_->GetSubsystem<Time>()->GetTimeSinceEpoch();
    }
}

void GameStatics::AddEarnStars(int qty)
{
    VariantMap& eventdata = context_->GetEventDataMap();
    eventdata[Game_EarnStars::NUMSTARS] = qty;
    rootScene_->SendEvent(GAME_EARNSTARS, eventdata);
}

void GameStatics::Start()
{
    URHO3D_LOGINFO("GameStatics() - ----------------------------------------");
    URHO3D_LOGINFO("GameStatics() - Start  ....                            -");
    URHO3D_LOGINFO("GameStatics() - ----------------------------------------");

    GameStatics::gameExit_ = false;

    GameStatics::SetLevelInfos();

    // Load GameData (if not exist create new default file)
    {
        // Load
#if !defined(TESTMODE) && defined(ACTIVE_SERIALIZEGAMESTATE)
        gameState_.Load();
#else
        gameState_.Reset();
#endif

        gameState_.Save();
    }

    // Change Localization after loading gameState_
    Localization* l10n = context_->GetSubsystem<Localization>();
	l10n->SetLanguage(GameStatics::playerState_->language_);

    CheckTimeForEarningStars();

#ifdef DUMP_SIMLATE_FIRST100LEVELS
    // Dump Leveling Simulation : 100first levels
    {
        URHO3D_LOGINFO("GameStatics() - Simulate 100 first levels params (based on 4 enemies max) :");

        for (int lvl=1; lvl < 101; lvl++)
        {
            int layoutshape = -1;
            int layoutsize = -1;
            int numobjectives = -1;
            int totalitems;
            GameStatics::SetLevelParameters(0, lvl, 4, layoutshape, layoutsize, numobjectives, totalitems);
        }
    }
#endif // DUMP_SIMLATE_FIRST100LEVELS

    // Start GameState
    if (!GameStatics::stateManager_->StartStack())
        GameStatics::Exit();

    URHO3D_LOGINFO("GameStatics() - ----------------------------------------");
    URHO3D_LOGINFO("GameStatics() - Start  .... OK !                       -");
    URHO3D_LOGINFO("GameStatics() - ----------------------------------------");
}

void GameStatics::SetMouseVisible(bool uistate, bool osstate)
{
    if (gameConfig_.touchEnabled_)
    {
        uistate = false;
        osstate = false;
    }

    if (cursor_)
    {
        input_->SetMouseVisible(osstate);
        cursor_->SetVisible(uistate);
    }
    else
    {
        input_->SetMouseVisible(uistate);
    }

    URHO3D_LOGINFOF("GameStatics() - SetMouseVisible : uicursor=%s oscursor=%s touchenabled=%s",
                    uistate ? "true":"false", osstate ? "true":"false", gameConfig_.touchEnabled_ ? "true":"false");
}

void GameStatics::Stop()
{
    URHO3D_LOGINFO("GameStatics() - ----------------------------------------");
    URHO3D_LOGINFO("GameStatics() - Stop  ....                             -");
    URHO3D_LOGINFO("GameStatics() - ----------------------------------------");

    GameStatics::gameExit_ = true;

    gameState_.Save();

    InteractiveFrame::Reset();

    // Stop current State (Menu, Play etc...) and delete Manager
    if (stateManager_)
        stateManager_->Stop();

    if (rootScene_)
        rootScene_->CleanupNetwork();

    if (rootScene_)
        GameHelpers::StopMusics();

    #ifdef ACTIVE_PRELOADER
    UnloadResources();
    #endif

    #ifdef DUMP_SCENE_BEFORERESET
    URHO3D_LOGINFO("GameStatics() - Stop  .... Dump Scene Before Reset ...");
    GameHelpers::DumpNode(rootScene_, 2, true);
    GameHelpers::DumpObject(rootScene_);
    #endif // DUMP_SCENE

    cameraNode_.Reset();
    fixedViewport_.Reset();
    fixedCameraNode_.Reset();
    rootScene_.Reset();

    #ifdef DUMP_SCENE_AFTERRESET
    URHO3D_LOGINFO("GameStatics() - Stop  .... Dump Scene After Reset ...");
    GameHelpers::DumpObject(rootScene_);
    #endif // DUMP_SCENE

	TimerRemover::Reset();
    DelayInformer::Reset();
    DelayAction::Reset();
    TextMessage::Reset();

    URHO3D_LOGINFO("GameStatics() - ----------------------------------------");
    URHO3D_LOGINFO("GameStatics() - Stop  .... OK !                        -");
    URHO3D_LOGINFO("GameStatics() - ----------------------------------------");
}

#include <SDL/SDL.h>
void GameStatics::Exit()
{
    GameStatics::gameExit_ = true;

    if (context_)
        context_->GetSubsystem<Engine>()->Exit();

    atexit(SDL_Quit);
}

void GameStatics::SetStatus(GameStatus status)
{
    if (status >= MAX_GAMESTATUS)
    {
        URHO3D_LOGERRORF("GameStatics() - SetStatus : ERROR of gamestatus >= MAX_GAMESTATUS");
        return;
    }

    prevGameStatus_ = gameStatus_;
    gameStatus_ = status;

    URHO3D_LOGINFO("------------------------------------------------------------");
    URHO3D_LOGINFOF("- GameStatics() - SetStatus : New Status = %s(%d)      -", gameStatusNames[gameStatus_], gameStatus_);
    URHO3D_LOGINFO("------------------------------------------------------------");
}

void GameStatics::SetGameScale()
{
    fScreenSize_.x_ = (float)graphics_->GetWidth() * PIXEL_SIZE;
    fScreenSize_.y_ = (float)graphics_->GetHeight() * PIXEL_SIZE;

    gameScale_.x_ = (float)graphics_->GetWidth() / (float)targetwidth_;
    gameScale_.y_ = (float)graphics_->GetHeight() / (float)targetheight_;

    uiScale_ = Min(gameScale_.x_, gameScale_.y_);

    ui_->SetScale(uiScale_);

    for (unsigned i=0; i < NUMUIELEMENTSIZE; i++)
        UISIZE[i] = (int)floor((float)UIDEFAULTSIZE[i]);
//        UISIZE[i] = (int)floor((float)UIDEFAULTSIZE[i] * uiScale_);

#ifdef ACTIVE_TOUCHTIPS
    if (gameConfig_.screenJoystickID_ != -1)
    {
        UIElement* uiscreenjoy = input_->GetJoystick(gameConfig_.screenJoystickID_)->screenJoystick_;
        uiscreenjoy->SetSize(graphics_->GetWidth() / uiScale_, graphics_->GetHeight() / uiScale_);
    }
#endif

    URHO3D_LOGINFOF("GameStatics() - SetGameScale  uiScale_=%f gameScale_=%f %f", uiScale_, gameScale_.x_, gameScale_.y_);
}

void GameStatics::SetConsoleVisible(bool state)
{
    Console* console = GameStatics::rootScene_->GetSubsystem<Console>();
    if (console)
        console->SetVisible(state);
}

void GameStatics::ResetGameStates()
{

}

void GameStatics::SetLevelParameters(unsigned seed, int lvl, int objmax, int& layoutshape, int& layoutsize, int& numobjectives, int& totalitems)
{
    GameRand::SetSeedRand(ALLRAND, seed+lvl);

    const int MINSIZE = 5;
    const int MAXSIZE = 9;
    const int MIDSIZE = (MAXSIZE+MINSIZE)/2;
    const int MIDOBJECTIVES = MAXOBJECTIVES/2;
    const int MATCHBASE = 3;
    const int MOVEBASE = 20;
    const int NUMLVLBYRANK = 10;
    const int NUMRANKS = 50;
    const int ITEMSBYRANK = 2;

    float rank = 1.f + (lvl / NUMLVLBYRANK);

    // define num objectives
    if (numobjectives == -1)
        numobjectives = Min(objmax, GameRand::GetRand(MAPRAND, 1, MAXOBJECTIVES));

    // define layout shape
    if (layoutshape == -1)
        layoutshape = GameRand::GetRand(MAPRAND, L_MAXSTANDARDLAYOUT);

    if (layoutsize == -1)
        layoutsize = GameRand::GetRand(MAPRAND, MINSIZE, MAXSIZE);

    // define base total items for the objectives
    int baseitems = MATCHBASE * MOVEBASE; // 60
    // Add items based on level rank
    int lvlitems = floor(float(lvl+MATCHBASE) / NUMLVLBYRANK) * ITEMSBYRANK; // (0->10)*2 => 0->20
    // Add some items for more randomness
    int randitems = rank/2 + GameRand::GetRand(MAPRAND, Max(1, floor(rank/2))); // 0-49 (with 4 objectives and rank 50)

    totalitems = baseitems+lvlitems+randitems; // MAX = 60+24+49 = 133

    int reduceitems1 = 0;
    int reduceitems2 = 0;
    int additems = 0;

    // reduce items if numerous items and a few objectives
    if (totalitems/numobjectives > 50 && numobjectives <= MIDOBJECTIVES)
        reduceitems1 = Max(0, floor(15 * float(NUMRANKS-rank) / NUMRANKS)); //

    // reduce items if complex layout
    if (LayoutDifficulty[layoutshape] >= 1.f)
        reduceitems2 = Max(0, floor(0.15f * (MAXSIZE - layoutsize+1) * (totalitems-reduceitems1))); // 15% of reduce by unit size < MidSize

    // add items if easy (big size + lot of objectives)
    if (layoutsize > MIDSIZE && numobjectives > MIDOBJECTIVES)
        additems = floor(0.1f * (MAXSIZE-MIDSIZE + numobjectives-MIDOBJECTIVES) * (totalitems-reduceitems1)); // 10% of adding by unit size and by unit objective

    totalitems += additems - (reduceitems1+reduceitems2);

    // reduce items for the first levels
    if (lvl < 5 && totalitems > lvl * 4 * MIDOBJECTIVES)
        totalitems = lvl * 4 * MIDOBJECTIVES;

//    totalitems = Clamp(totalitems, MATCHBASE*4, MATCHBASE*40);

    URHO3D_LOGINFOF("GameStatics() - SetLevelParameters : lvl=%d rank=%F shp=%d sz=%d objs=%d items=%d(%d+%d+%d+%d-%d-%d) ibyobjs=%d",
                        lvl, rank, layoutshape,  layoutsize, numobjectives, totalitems,
                        baseitems, lvlitems, randitems, additems, reduceitems1, reduceitems2, totalitems/numobjectives);
}

int GameStatics::SetLevel(int level)
{
    if (level > 0)
    {
        currentLevel_ = level;

        // update current zone
        int num = 0;
        int i = 0;

        for (;;)
        {
            num += NumLevelsByZone_[i];

            if (num >= currentLevel_)
                break;

            i++;

            if (i >= MAXZONES)
                break;
        }

        currentZone_ = i+1;
    }

    return currentLevel_;
}

void GameStatics::UpdateLevel()
{
    unsigned nextlevel = gameState_.GetNextLevel();
    if (nextlevel)
        currentLevel_ = nextlevel;
    else
        currentLevel_++;

    SetLevel(currentLevel_);
}

void GameStatics::AddBonus(const Slot& slot, Object* sender, float time)
{
    StringHash eventType;
    VariantMap& eventData = context_->GetEventDataMap();

    URHO3D_LOGINFOF("GameStatics - AddBonus() - slot=%s", slot.GetString().CString());

    if (slot.cat_ == COT::ITEMS.Value())
    {
        URHO3D_LOGERRORF("GameStatics - AddBonus() - category ITEMS NOT IMPLEMENTED");
        return;
    }
    else if (slot.cat_ == COT::POWERS.Value())
    {
        if (slot.indexcat_ != -1 && slot.indexcat_ < MAXABILITIES)
        {
            // Update Ability Qty in playerstate
            URHO3D_LOGINFOF("GameStatics - AddBonus() - category POWERS index=%d add+%d", slot.indexcat_, slot.qty_);
            playerState_->powers_[slot.indexcat_].qty_ = Max(playerState_->powers_[slot.indexcat_].qty_ + slot.qty_, 0);
            eventType = GAME_ABILITYADDED;
            eventData[Game_AbilityAdded::ABILITY_ID] = slot.indexcat_+1;
        }
    }
    else if (slot.cat_ == COT::MOVES.Value())
    {
        URHO3D_LOGINFOF("GameStatics - AddBonus() - category MOVES qty=%d", slot.qty_);
        // Add +3 more than the event which add +1 move
        moves_ += (slot.qty_-1);
        eventType = GAME_MOVEADDED;
    }
    else if (slot.cat_ == COT::STARS.Value())
    {
        URHO3D_LOGINFOF("GameStatics - AddBonus() - category STARS qty=%d", slot.qty_);
        if (!tries_)
            moves_ = initialMoves;
        tries_ += slot.qty_;
        eventType = GAME_TRYADDED;
    }
    else if (slot.cat_ == COT::COINS.Value())
    {
        URHO3D_LOGINFOF("GameStatics - AddBonus() - category COINS qty=%d", slot.qty_);
        coins_ += slot.qty_;
        eventType = GAME_COINUPDATED;
    }
    else if (slot.cat_ == COT::SPECIALS.Value())
    {
        /// TODO : ex RemoveAds
        URHO3D_LOGERRORF("GameStatics - AddBonus() - SPECIALS NOT IMPLEMENTED");
        return;
    }
    else
        return;

    DelayInformer::Get(sender, time, eventType, eventData);
}

void GameStatics::AddBonuses(const Vector<Slot >& slots, Object* sender)
{
    float time = 0.5f;
    for (unsigned i=0; i < slots.Size(); ++i)
    {
        AddBonus(slots[i], sender, time);
        time += 0.5f;
    }
}

int GameStatics::GetNextUnlockedMission(bool checkunblockzone)
{
    if (checkunblockzone && playerState_->zonestates[currentZone_-1] == CONSTELLATION_UNBLOCKED)
        return -1;

    int firstMissionId = GetMinLevelId(currentZone_);
    int lastMissionId = firstMissionId + NumLevelsByZone_[currentZone_-1];
    for (int i=firstMissionId; i < lastMissionId; i++)
    {
        MissionState& mstate = playerState_->missionstates[i-1];
        if (mstate.state_ == MissionState::MISSION_UNLOCKED)
            return i;
    }

    return -1;
}

int GameStatics::GetCurrentBoss()
{
    int bossid = -1;

    if (BossLevel_[currentZone_-1] == currentLevel_)
        bossid = currentZone_ > NBMAXBOSSES ? (currentZone_-1) % NBMAXBOSSES + 1 : currentZone_;

    return bossid;
}

bool GameStatics::IsBossLevel()
{
    return BossLevel_[currentZone_-1] == currentLevel_;
}

unsigned GameStatics::GetBossLevelId(int zone)
{
    return BossLevel_[zone-1];
}

bool GameStatics::CanUnblockConstellation(int zone)
{
    if (playerState_->zonestates[zone-1] == CONSTELLATION_UNBLOCKED && GameStatics::playerState_->cinematicShown_[zone-1] >= CINEMATIC_BOSSDEFEAT)
        return false;

    int firstMissionId = GetMinLevelId(zone);
    int lastMissionId = firstMissionId + NumLevelsByZone_[zone-1];
    int missionscompleted = 0;

    for (int i=firstMissionId; i < lastMissionId; i++)
    {
        MissionState& mstate = playerState_->missionstates[i-1];
        if (mstate.state_ == MissionState::MISSION_COMPLETED)
            missionscompleted++;
    }

    URHO3D_LOGINFOF("GameStatics() - CanUnblockConstellation : zone=%d minlevel=%d maxlevel=%d constellationok=%s",
                    zone, firstMissionId, lastMissionId-1, missionscompleted == NumLevelsByZone_[zone-1] ? "true" : "false");

    return missionscompleted == NumLevelsByZone_[zone-1];
}

int GameStatics::GetMinLevelId(int zone)
{
    zone-=2;
    int level = 1;

    while (zone >= 0)
    {
        level += NumLevelsByZone_[zone--];
    }

    return level;
}

int GameStatics::GetMaxLevelId(int zone)
{
    return GetMinLevelId(zone) + NumLevelsByZone_[zone-1] -1;
}

void GameStatics::GetMissionBonuses(int missionid, Vector<Slot >& bonuses)
{
    MissionState& mstate = playerState_->missionstates[missionid-1];

    for (int j=0; j < MAXMISSIONBONUSES; j++)
        if (mstate.bonuses_[j].type_ != 0)
            bonuses.Push(mstate.bonuses_[j]);
}

void GameStatics::Dump()
{
    gameState_.Dump();
}
