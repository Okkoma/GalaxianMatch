#include <iostream>
#include <cstdio>

#include <Urho3D/ThirdParty/PugiXml/pugixml.hpp>

#include <Urho3D/Urho3D.h>

#include <Urho3D/Core/CoreEvents.h>
#include <Urho3D/Core/Timer.h>

#include <Urho3D/Engine/Console.h>
#include <Urho3D/Engine/DebugHud.h>
#include <Urho3D/Engine/Engine.h>
#include <Urho3D/Engine/EngineEvents.h>

#include <Urho3D/IO/Log.h>
#include <Urho3D/IO/FileSystem.h>

#include <Urho3D/Graphics/Graphics.h>
#include <Urho3D/Graphics/GraphicsEvents.h>
#include <Urho3D/Graphics/Renderer.h>
#include <Urho3D/Graphics/DebugRenderer.h>
#include <Urho3D/Graphics/Camera.h>
#include <Urho3D/Graphics/Texture2D.h>

#include <Urho3D/Resource/Localization.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Resource/XMLFile.h>

#include <Urho3D/Input/Input.h>
#include <Urho3D/Input/InputEvents.h>

#include <Urho3D/Scene/SceneEvents.h>

#include <Urho3D/UI/UI.h>
#include <Urho3D/UI/UIEvents.h>
#include <Urho3D/UI/Cursor.h>
#include <Urho3D/UI/Button.h>
#include <Urho3D/UI/Font.h>
#include <Urho3D/UI/Sprite.h>
#include <Urho3D/UI/Text.h>
#include <Urho3D/UI/Menu.h>
#include <Urho3D/UI/Window.h>

#include <Urho3D/Urho2D/Renderer2D.h>

#include "GameOptions.h"
#include "GameAttributes.h"
#include "GameLibrary.h"
#include "GameEvents.h"
#include "GameStatics.h"
#include "GameHelpers.h"
#include "GameCommands.h"
#include "GameUI.h"
#include "GameTest.h"

#include "Network.h"

#include "InteractiveFrame.h"
#include "DelayAction.h"

#include "MAN_Matches.h"
#ifdef ACTIVE_SPLASHUI
#include "SplashScreen.h"
#endif
#include "sOptions.h"
#include "sPlay.h"
#include "sCinematic.h"

#include <SDL/SDL.h>

#include "Game.h"

URHO3D_DEFINE_APPLICATION_MAIN(Game);



extern int UISIZE[NUMUIELEMENTSIZE];
static bool engineConfigApplied_;

WeakPtr<UIMenu> accessMenu_;
WeakPtr<UIElement> headerHolder_;
SharedPtr<UIDialog> dialogInfo_;
SharedPtr<UIDialog> companionBox_;
SharedPtr<SplashScreen> splashScreen_;

const float DELAY_INACTIVECURSOR = 10.f;
float timerInactiveCursor_ = 0.f;

const unsigned NumMaxClicksEnd = 1U;
int numClicksOutsideCompanionBox_ = 0;

OptionState* options_;

Game* Game::game_;


Game::Game(Context* context) :
    Application(context),
    delayCompanion_(0.f)
{
    URHO3D_LOGINFO("Game() - ----------------------------------------");
    URHO3D_LOGINFOF("Game() - Game SpaceMatch ver %d rev.%d dat.%d -", gameVersion_, gameVersionMinor_, gameDataVersion_);
    URHO3D_LOGINFO("Game() - ----------------------------------------");

    game_ = this;

    SetRandomSeed(Time::GetSystemTime());
}

Game::~Game()
{
    URHO3D_LOGINFO("Game() - ----------------------------------------");
    URHO3D_LOGINFO("Game() - ~Game                                  -");
    URHO3D_LOGINFO("Game() - ----------------------------------------");
}

UIElement* Game::GetAccessMenu() const { return accessMenu_; }

UIDialog* Game::GetCompanion() const { return companionBox_; }

void Game::Setup()
{
    GameConfig& config = GameStatics::gameConfig_;

//    engineConfigApplied_ = false;
    engineConfigApplied_ = LoadGameConfig(Engine::GetParameter(engineParameters_, "GameConfig", String("engine_config.xml")).GetString().CString(), &config);

    if (!engineConfigApplied_)
    {
        std::cout << "Apply default configuration..." << std::endl;

        engineParameters_["WindowTitle"] = GameStatics::GAMENAME;
        engineParameters_["WindowIcon"] = "Textures/icone.png";
        engineParameters_["ResourcePaths"] = "CoreData;Data";
        if (!engineParameters_.Contains("LogName"))
            engineParameters_["LogName"] = GameStatics::GAMENAME + ".log";

//    #ifdef DESKTOP_GRAPHICS
        engineParameters_["WindowWidth"] = GameStatics::targetwidth_;
        engineParameters_["WindowHeight"] = GameStatics::targetheight_;
//    #else
//        engineParameters_["WindowWidth"] = 0;
//        engineParameters_["WindowHeight"] = 0;
//    #endif
        engineParameters_["Orientations"] = "Portrait PortraitUpsideDown";
        engineParameters_["WorkerThreads"] = true;
        engineParameters_["FullScreen"]  = false;
        engineParameters_["WindowResizable"]  = true;
        engineParameters_["Headless"] = false;
        engineParameters_["Shadows"] = false;
        engineParameters_["TripleBuffer"] = true;
        engineParameters_["VSync"] = true;

        /// LOGQUIET = false for DEBUG
        engineParameters_["LogQuiet"] = false;
        engineParameters_["LogLevel"] = 0;      // 0:LOGDEBUG 1: LOGINFO+LOGWARNING+LOGERROR 2: LOGWARNING+LOGERROR  3: LOGERROR ONLY

        engineParameters_["MaterialQuality"] = 0;
        engineParameters_["TextureQuality"] = 2;
        engineParameters_["TextureFilterMode"] = 1;
        engineParameters_["MultiSample"] = 1;

        engineParameters_["SoundBuffer"] = 100;
        engineParameters_["SoundMixRate"] = 44100;
        engineParameters_["SoundStereo"] = true;
        engineParameters_["SoundInterpolation"] = true;

        if (config.splashviewed_)
            config.initState_ = "MainMenu";
        else
            config.splashviewed_ = true;

        config.networkMode_ = "local";
        config.language_ = 0;
        config.touchEnabled_ = false;
        config.forceTouch_ = false;

        config.HUDEnabled_ = false;
        config.ctrlCameraEnabled_ = false;
        config.physics3DEnabled_ = false;
        config.physics2DEnabled_ = true;
        config.enlightScene_ = false;

        config.debugRenderEnabled_ = false;
        config.debugPhysics_ = false;
        config.debugLights_ = false;
        config.debugUI_ = false;

        config.frameLimiter_ = 0;
    }

    if (config.frameLimiter_)
    {
        engineParameters_["FrameLimiter"] = true;
        engineParameters_["MaxFPS"] = config.frameLimiter_;
    }

    SDL_SetHint(SDL_HINT_ACCELEROMETER_AS_JOYSTICK, "0");
}


bool Game::LoadGameConfig(const char* fileName, GameConfig* config)
{
    String filename;
//
//#if defined(__ANDROID__)
//    filename = "/apk/" + filename;
//#endif

    FileSystem* fs = context_->GetSubsystem<FileSystem>();
    if (!fs)
    {
        config->logString += ToString("Apply engine_config.xml... No FileSystem ... \n");
        return false;
    }

    filename = fs->GetProgramDir() + fileName;

    // Doc File
    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_file(filename.CString());
//    pugi::xml_parse_result result = doc.load_buffer_inplace_own(buffer, sizeFile);

    if (result.status != pugi::status_ok)
    {
        config->logString += ToString("Apply engine_config.xml... Can't find %s file ... \n", filename.CString());
        std::cout << result.description()  << std::endl;
        return false;
    }

    config->logString += ToString("Apply engine_config.xml... \n\n");
    std::cout << config->logString.CString();
//    std::cout << "Apply engine_config.xml... "<< std::endl << std::endl;
    // Root Elem
    pugi::xml_node root = doc.first_child();

    for (pugi::xml_node paramElem = root.child("parameter"); paramElem; paramElem = paramElem.next_sibling("parameter"))
    {
        const String& name = paramElem.attribute("name").value();
        if (engineParameters_.Contains(name)) continue;

        if (name == "WindowTitle" || name == "WindowIcon" || name == "ResourcePaths"
            || name == "LogName")
        {
            engineParameters_[name] = paramElem.attribute("value").value();
            config->logString += ToString("  engineParameters_[%s] = %s \n", name.CString(),paramElem.attribute("value").value());
            std::cout << config->logString.CString();
//            std::cout << "  engineParameters_[" << name.CString() << "] = " << paramElem.attribute("value").value() << std::endl;
        }
        else if (name == "WindowWidth" || name == "WindowHeight" || name == "LogLevel" || name == "MaterialQuality" || name == "TextureQuality"
                 || name == "MultiSample" || name == "TextureFilterMode")
        {
            engineParameters_[name] = paramElem.attribute("value").as_int();
            config->logString += ToString("  engineParameters_[%s] = %d \n", name.CString(),paramElem.attribute("value").as_int());
            std::cout << config->logString.CString();
//            std::cout << "  engineParameters_[" << name.CString() << "] = " << paramElem.attribute("value").as_int() << std::endl;
        }
        else
        {
            engineParameters_[name] = paramElem.attribute("value").as_bool();
            config->logString += ToString("  engineParameters_[%s] = %s \n", name.CString(),paramElem.attribute("value").as_bool()==1?"true":"false");
            std::cout << config->logString.CString();
//            std::cout << "  engineParameters_[" << name.CString() << "] = " << (paramElem.attribute("value").as_bool()==1?"true":"false") << std::endl;
        }
    }
    std::cout << std::endl;
    for (pugi::xml_node varElem = root.child("variable"); varElem; varElem = varElem.next_sibling("variable"))
    {
        const String& name = varElem.attribute("name").value();
        if (name == "language_" || name == "frameLimiter_" || name == "networkServerPort_" || name == "testscenario_")
        {
            int value = varElem.attribute("value").as_int();
            if (name == "language_")
                config->language_ = value;
            else if (name == "frameLimiter_")
                config->frameLimiter_ = value;
            else if (name == "networkServerPort_")
                config->networkServerPort_ = value;
            else if (name == "testscenario_")
                GameStatics::playTest_ = value;

            config->logString += ToString("  (Int) %s = %d \n", name.CString(), value);
            std::cout << config->logString.CString();
        }
        else if (name == "initState_" || name == "sceneToLoad_" || name == "networkServerIP_" || name == "networkMode_")
        {
            const String& value = varElem.attribute("value").value();

            if (name == "sceneToLoad_")
                ;
                //GameStatics::sceneToLoad_ = varElem.attribute("value").value();
            else if (name == "initState_")
                config->initState_ = value;
            else if (name == "networkServerIP_")
                config->networkServerIP_ = value;
            else if (name == "networkMode_")
                config->networkMode_ = value;

            config->logString += ToString("  (String) %s = %s \n", name.CString(), value.CString());
            std::cout << config->logString.CString();
        }
        else
        {
            bool value = varElem.attribute("value").as_bool();
            if (name == "touchEnabled_") config->touchEnabled_ = value;
            else if (name == "forceTouch_") config->forceTouch_ = value;
            else if (name == "HUDEnabled_") config->HUDEnabled_ = value;
            else if (name == "ctrlCameraEnabled_") config->ctrlCameraEnabled_ = value;
            else if (name == "debugRenderEnabled_") config->debugRenderEnabled_ = value;
            else if (name == "physics3DEnabled_") config->physics3DEnabled_ = value;
            else if (name == "physics2DEnabled_") config->physics2DEnabled_ = value;
            else if (name == "enlightScene_") config->enlightScene_ = value;
            else if (name == "debugPhysics_") config->debugPhysics_ = value;
            else if (name == "debugLights_") config->debugLights_ = value;
            else if (name == "debugUI_") config->debugUI_ = value;
            else if (name == "debugAnimatedSprite2D") config->debugAnimatedSprite2D = value;

            config->logString += ToString("  (bool) %s = %s \n", name.CString(), value ? "true":"false");
            std::cout << config->logString.CString();
//            std::cout << "  (bool) " << name.CString() << " = " << (value ? "true":"false") << std::endl;
        }
    }
    config->logString += ToString("\n");
    std::cout << std::endl;

    return true;
}

void Game::Start()
{
	URHO3D_LOGINFO("Game() - ----------------------------------------");
	URHO3D_LOGINFO("Game() - Start                                  -");
	URHO3D_LOGINFO("Game() - ----------------------------------------");

	URHO3D_LOGINFOF("Game() - engineConfigApplied_ = %s", engineConfigApplied_ ? "true" : "false");

	//engine_->RegisterApplication(this);

    if (engineConfigApplied_)
        URHO3D_LOGINFOF("%s", GameStatics::gameConfig_.logString.CString());
    else
        URHO3D_LOGWARNINGF("%s", GameStatics::gameConfig_.logString.CString());

    URHO3D_LOGINFOF("Game() - Graphics API %s -", context_->GetSubsystem<Graphics>()->GetApiName().CString());

    GameHelpers::SetGameLogFilter(GAMELOG_PRELOAD|GAMELOG_MAPPRELOAD|GAMELOG_MAPCREATE|GAMELOG_MAPUNLOAD|GAMELOG_WORLDUPDATE|GAMELOG_WORLDVISIBLE|GAMELOG_PLAYER);
//    GameHelpers::SetGameLogFilter(GAMELOG_MAPPRELOAD|GAMELOG_MAPCREATE|GAMELOG_MAPUNLOAD|GAMELOG_WORLDUPDATE|GAMELOG_WORLDVISIBLE|GAMELOG_PLAYER);

    URHO3D_LOGINFOF("Game() - LogFilter = %u", GameHelpers::GetGameLogFilter());

    SetupDirectories();

	RegisterGameLibrary(context_);

	// Create All Statics : Camera, Scene
	GameStatics::Initialize(context_);

    URHO3D_LOGINFOF("Game() - texture capacity = %d", GameStatics::graphics_->GetMaxTextureSize());

	// Detect Controls (Touch, Joystick)
	SetupControllers();

    GameStatics::cameraNode_->SetPosition(Vector3::ZERO);

	// Network, HUD, Localization
	SetupSubSystems();

    SubscribeToEvent(E_APPPAUSED, URHO3D_HANDLER(Game, HandleAppPaused));
    SubscribeToEvent(E_APPRESUMED, URHO3D_HANDLER(Game, HandleAppResumed));

	// Preload Ressources
    SubscribeToEvent(E_SCENEUPDATE, URHO3D_HANDLER(Game, HandlePreloadResources));

    SubscribeToEvent(E_POSTUPDATE, URHO3D_HANDLER(Game, HandleRewardsEvents));
    SubscribeToEvent(GAME_EARNSTARS, URHO3D_HANDLER(Game, HandleRewardsEvents));

    // Companion
    SubscribeToEvent(GAME_STATEEND, URHO3D_HANDLER(Game, HandleUpdateCompanion));
    SubscribeToEvent(GAME_STATEREADY, URHO3D_HANDLER(Game, HandleUpdateCompanion));
    SubscribeToEvent(COMPANION_SHOW, URHO3D_HANDLER(Game, HandleUpdateCompanion));
    SubscribeToEvent(COMPANION_HIDE, URHO3D_HANDLER(Game, HandleUpdateCompanion));
    SubscribeToEvent(E_CLICK, URHO3D_HANDLER(Game, HandleUpdateCompanion));
    SubscribeToEvent(E_MOUSEBUTTONUP, URHO3D_HANDLER(Game, HandleUpdateCompanion));
    SubscribeToEvent(E_TOUCHEND, URHO3D_HANDLER(Game, HandleUpdateCompanion));

    // Handle Screen Resize
	SubscribeToEvent(E_SCREENMODE, URHO3D_HANDLER(Game, HandleScreenResized));

	// Start Managers
    GameStatics::Start();

    options_ = ((OptionState*)GameStatics::stateManager_->GetState("Options"));

    // Splashscreen
#ifdef ACTIVE_SPLASHUI
    splashScreen_ = SharedPtr<SplashScreen>(new SplashScreen(context_, GAME_LEVELTOLOAD, GAME_LEVELREADY, "UI/splash02.webp", 0.f));
#endif

	URHO3D_LOGINFO("Game() - ----------------------------------------");
	URHO3D_LOGINFO("Game() - Start .... OK !                        -");
	URHO3D_LOGINFO("Game() - ----------------------------------------");
}

void Game::Stop()
{
    URHO3D_LOGINFO("Game() - ----------------------------------------");
    URHO3D_LOGINFO("Game() - Stop ...                               -");
    URHO3D_LOGINFO("Game() - ----------------------------------------");

    //Application::Stop();

    UnsubscribeFromAllEvents();

    if (GameStatics::gameConfig_.touchEnabled_)
        GameStatics::input_->RemoveScreenJoystick(GameStatics::gameConfig_.screenJoystickID_);

    GameStatics::Stop();

	UnRegisterGameLibrary(context_);

    dialogInfo_.Reset();
    companionBox_.Reset();
    splashScreen_.Reset();

    GetSubsystem<UI>()->GetRoot()->RemoveAllChildren();

#ifdef DUMP_UI
    PODVector<UIElement*> children;
    GetSubsystem<UI>()->GetRoot()->GetChildren(children, true);
    for (PODVector<UIElement*>::ConstIterator it=children.Begin();it!=children.End();++it)
        URHO3D_LOGINFOF(" UI child = %s",(*it)->GetName().CString());
#endif

    URHO3D_LOGINFO("Game() - ----------------------------------------");
    URHO3D_LOGINFO("Game() - Stop ...       OK !                     -");
    URHO3D_LOGINFO("Game() - ----------------------------------------");

#ifdef DUMP_RESOURCES
    engine_->DumpResources(true);
    //engine_->DumpMemory();
    engine_->DumpProfiler();
#endif

    Network::Remove();

    /// GAME EXIT
}

void Game::SetupDirectories()
{
    GameConfig* config = &GameStatics::gameConfig_;

    URHO3D_LOGINFO("Game() - SetupDirectories ...");
    FileSystem* fs = context_->GetSubsystem<FileSystem>();
    if (!fs)
    {
        URHO3D_LOGERROR("Game() - SetupDirectories : No FileSystem Found");
        return;
    }

    String saveDir = fs->GetAppPreferencesDir("OkkomaStudio", GameStatics::GAMENAME);
    if (saveDir == String::EMPTY)
    {
        saveDir = fs->GetUserDocumentsDir();
        if (saveDir == String::EMPTY)
        {
            URHO3D_LOGERROR("RegisterWorldPath : Can not Set Directories !");
            GameStatics::Exit();
            return;
        }
        else
        {
            saveDir += GameStatics::GAMENAME + "/";
            fs->CreateDir(saveDir);
        }
    }

    if (!fs->DirExists(saveDir + "Save/"))
        fs->CreateDir(saveDir + "Save/");

    if (!fs->DirExists(saveDir + "Data/"))
        fs->CreateDir(saveDir + "Data/");

    if (!fs->DirExists(saveDir + "Data/Levels/"))
        fs->CreateDir(saveDir + "Data/Levels/");

    config->saveDir_ = saveDir;
    config->appDir_ = fs->GetProgramDir();

    URHO3D_LOGINFOF("Game() - SetupDirectories : saveDir_=%s appDir_=%s ... OK !", saveDir.CString(), config->appDir_.CString());
}

void Game::SetupControllers()
{
    URHO3D_LOGINFO("Game() - ----------------------------------------");
    URHO3D_LOGINFO("Game() - SetupControllers ...                   -");
    URHO3D_LOGINFO("Game() - ----------------------------------------");

    GameConfig& config = GameStatics::gameConfig_;
    int numjoysticks = GameStatics::input_->GetNumJoysticks();
    URHO3D_LOGINFOF("Game() - SetupControllers : NumJoysticks = %d", numjoysticks);

    // Set Joysticks
    {
        #ifdef ACTIVE_SDLMAPPINGJOYSTICK_DB
        if (GameStatics::input_->GetNumJoysticks() > 0)
        {
            int numMappedControllers = SDL_GameControllerAddMappingsFromFile("Data/Input/gamecontrollerdb.txt");
            if (numMappedControllers == -1)
                URHO3D_LOGERROR("Game() - DetectControllers : ... SDL Controller Mapping Error ! ");
            else
                URHO3D_LOGINFOF("Game() - DetectControllers : ... SDL Controller Mapping numControllers=%d... OK ! ", numMappedControllers);

            for (unsigned i = 0; i < SDL_NumJoysticks(); ++i)
            {
                if (SDL_IsGameController(i))
                {
                    char *mapping = SDL_GameControllerMapping(SDL_GameControllerOpen(i));
                    URHO3D_LOGINFOF("Game() - DetectControllers : GameController(%u) : name=%s mapped=%s", i, SDL_GameControllerNameForIndex(i), mapping);
                    SDL_free(mapping);
                }
                else
                    URHO3D_LOGERRORF("Game() - DetectControllers : GameController(%u) : unknown controller !", i);
            }
        }
        #endif
    }

    // Set Touch Control
    {
        if (GetPlatform() == "Android" || GetPlatform() == "iOS")
        {
            URHO3D_LOGINFO("Game() - SetupControllers : Android or IOS : InitTouchInput ");

            GameStatics::InitTouchInput(context_);
        }

        else if (!GameStatics::input_->GetNumJoysticks() || config.forceTouch_)
        {
            if (!config.touchEnabled_ && !config.forceTouch_)
                SubscribeToEvent(E_TOUCHBEGIN, URHO3D_HANDLER(Game, HandleTouchBegin));

            else if (config.forceTouch_)
            {
                GameStatics::InitTouchInput(context_);
                GameStatics::input_->SetTouchEmulation(true);
                URHO3D_LOGINFO("Game() - SetupControllers : Desktop force TouchEmulation !");
            }

            URHO3D_LOGINFOF("Game() - SetupControllers : touchEnabled_=%s forceTouch_=%s",
                            config.touchEnabled_ ? "true":"false", config.forceTouch_ ? "true":"false");
        }
    }

    // Dump Joysticks
    if (GameStatics::input_->GetNumJoysticks())
    {
        URHO3D_LOGINFOF("Game() - Dump Joysticks : numJoysticks=%u", GameStatics::input_->GetNumJoysticks());
        for (unsigned i=0; i < GameStatics::input_->GetNumJoysticks();i++)
        {
            JoystickState* joystate = GameStatics::input_->GetJoystickByIndex(i);
            if (joystate)
            {
                URHO3D_LOGINFOF("Game() - Joystick[%u] : ID=%d name=%s screenjoy=%u", i, joystate->joystickID_, joystate->name_.CString(), joystate->screenJoystick_);
            }
            else
                URHO3D_LOGINFOF("Game() - Joystick[%u] : no State", i);

        }
    }

	// Subscribe Cursor Mouse Move
	if (!config.touchEnabled_)
        GameStatics::InitMouse(context_);


    URHO3D_LOGINFO("Game() - ----------------------------------------");
    URHO3D_LOGINFO("Game() - SetupControllers ... OK!               -");
    URHO3D_LOGINFO("Game() - ----------------------------------------");
}

void Game::SetupSubSystems()
{
    GameConfig* config = &GameStatics::gameConfig_;

    // Set Localization
	Localization* l10n = GetSubsystem<Localization>();
	l10n->LoadJSONFile("Texts/UI_messages.json");
	l10n->LoadJSONFile("Texts/UI_dialogues.json");

	// Set Local Language
	SDL_Locale* locale = SDL_GetPreferredLocales();
	if (locale)
    {
        int lang = l10n->GetLanguageIndex(String(SDL_GetPreferredLocales()->language));
        if (lang != -1 && lang != GameStatics::playerState_->language_)
            GameStatics::playerState_->language_ = lang;
    }
	l10n->SetLanguage(GameStatics::playerState_->language_);

	UI* ui = GetSubsystem<UI>();
	if (ui)
	{
        ResourceCache* cache = GetSubsystem<ResourceCache>();

	    if (config->HUDEnabled_)
        {
            // Get default style
            XMLFile* xmlFile = cache->GetResource<XMLFile>("UI/DefaultStyle.xml");
            if (xmlFile)
            {
                // Create console
                Console* console = engine_->CreateConsole();
                if (console)
                {
                    console->SetDefaultStyle(xmlFile);
                    console->SetNumRows(10);
                    console->SetNumBufferedRows(200);
        #ifdef ACTIVE_CONSOLECOMMAND
                    console->SetCommandInterpreter(GetTypeName());
                    SubscribeToEvent(E_CONSOLECOMMAND, URHO3D_HANDLER(Game, HandleConsoleCommand));
        #endif
                }

                // Create debug HUD.
                DebugHud* debugHud = engine_->CreateDebugHud();
                if (debugHud)
                {
                    debugHud->SetDefaultStyle(xmlFile);
                    SubscribeToEvent(E_KEYDOWN, URHO3D_HANDLER(Game, HandleKeyDown));
                }

                // Default style on root
                ui->GetRoot()->SetDefaultStyle(xmlFile);
            }
        }
	}
}

void Game::ResetScreen()
{
    URHO3D_LOGINFO("Game() - ResetScreen ...");

//    GameStatics::rootScene_->GetOrCreateComponent<Renderer2D>(LOCAL);

    GameStatics::SetGameScale();

//    GameStatics::ui_->GetRoot()->SetSize(GameStatics::graphics_->GetWidth(), GameStatics::graphics_->GetHeight());

    // Set Camera
    GameStatics::camera_->SetNearClip(DEFAULT_NEARCLIP);
    GameStatics::camera_->SetFarClip(DEFAULT_FARCLIP);
    GameStatics::camera_->SetFov(DEFAULT_CAMERA_FOV);
    GameStatics::camera_->SetOrthographic(true);
    GameStatics::camera_->SetOrthoSize((float)GameStatics::graphics_->GetHeight()*PIXEL_SIZE);
    GameStatics::camera_->SetZoom(GameStatics::uiScale_);
    GetSubsystem<Renderer>()->SetCullMode(CULL_NONE, GameStatics::camera_);

    GameStatics::fixedCamera_->SetNearClip(DEFAULT_NEARCLIP);
    GameStatics::fixedCamera_->SetFarClip(DEFAULT_FARCLIP);
    GameStatics::fixedCamera_->SetFov(DEFAULT_CAMERA_FOV);
    GameStatics::fixedCamera_->SetOrthographic(true);
    GameStatics::fixedCamera_->SetOrthoSize((float)GameStatics::graphics_->GetHeight()*PIXEL_SIZE);
    GameStatics::fixedCamera_->SetZoom(GameStatics::uiScale_);

    const int border = 10 * GameStatics::uiScale_;

    if (headerHolder_)
    {
        headerHolder_->SetSize(GameStatics::ui_->GetRoot()->GetSize().x_, 128);
        headerHolder_->GetChild(1)->SetPosition(GameStatics::ui_->GetRoot()->GetSize().x_ - headerHolder_->GetChild(1)->GetSize().x_ - border, border);
    }

    if (accessMenu_)
    {
        const int height = GameStatics::uiScale_ < 1.f ? (int)floor((float)GameStatics::graphics_->GetHeight() / GameStatics::uiScale_) : GameStatics::ui_->GetRoot()->GetSize().y_;
        accessMenu_->SetMinOffset(IntVector2(border, height-UISIZE[MBUTTONUISIZE]-border));
        accessMenu_->SetMaxOffset(IntVector2(border+UISIZE[MBUTTONUISIZE], height-border));
        accessMenu_->SetPosition(border, height - UISIZE[MBUTTONUISIZE] - border);
    }

    URHO3D_LOGINFOF("Game() - ResetScreen ... orthosize=%f gameScale=%s OK !",
                     GameStatics::camera_ ->GetOrthoSize(), GameStatics::gameScale_.ToString().CString());
}


void Game::CreateMenuButton(UIElement* uiroot)
{
    if (!uiroot->GetChild(String("menubutton")))
    {
        const int width = GameStatics::ui_->GetRoot()->GetSize().x_;
        const int height = GameStatics::uiScale_ < 1.f ? (int)floor((float)GameStatics::graphics_->GetHeight() / GameStatics::uiScale_) : GameStatics::ui_->GetRoot()->GetSize().y_;
        const int border = 10 * GameStatics::uiScale_;
        Button* button = uiroot->CreateChild<Button>();
        button->SetName("menubutton");
        button->SetTexture(GetSubsystem<ResourceCache>()->GetResource<Texture2D>("UI/accessmenu.webp"));
        button->SetImageRect(IntRect(0, 0, 128, 128));
        button->SetEnableAnchor(true);
        button->SetMinOffset(IntVector2(border, height-UISIZE[MBUTTONUISIZE]-border));
        button->SetMaxOffset(IntVector2(border+UISIZE[MBUTTONUISIZE], height-border));
        button->SetOpacity(0.9f);
        button->SetHoverOffset(0, 128);
        button->SetLayoutMode(LM_FREE);
        button->SetPosition(border, height-UISIZE[MBUTTONUISIZE] - border);

        URHO3D_LOGINFOF("Game() - CreateMenuButton ... on %s !", uiroot->GetName().CString());
    }
}

void Game::SetAccessMenuButtonVisible(int child, bool visible)
{
    UIElement* popup = accessMenu_->GetPopup();
    if (popup)
    {
        popup->GetChild(child)->SetVisible(visible);
        // todo : calculate the num of visible children
        int numvisiblechildren = visible ? 4 : 3;
        popup->SetSize(numvisiblechildren * UISIZE[MBUTTONUISIZE], UISIZE[MBUTTONUISIZE]);
    }
}

void Game::CreateAccessMenu(UIElement* uiroot)
{
    if (!uiroot->GetChild(String("accessmenu")))
    {
        ResourceCache* cache = GetSubsystem<ResourceCache>();
        Texture2D* texture = cache->GetResource<Texture2D>("UI/accessmenu.webp");

        // popup menu window
        Window* popup = uiroot->CreateChild<Window>();
        {
            // return mainmenu
            Button* button1 = popup->CreateChild<Button>("back");
            button1->SetTexture(texture);
            button1->SetImageRect(IntRect(128, 0, 256, 128));
            button1->SetHoverOffset(0, 128);
            // access parameters/options
            Button* button2 = popup->CreateChild<Button>("options");
            button2->SetTexture(texture);
            button2->SetImageRect(IntRect(256, 0, 384, 128));
            button2->SetHoverOffset(0, 128);
            // access shop
            Button* button3 = popup->CreateChild<Button>("shop");
            button3->SetTexture(texture);
            button3->SetImageRect(IntRect(384, 0, 512, 128));
            button3->SetHoverOffset(0, 128);
            // access cinematic
            Button* button4 = popup->CreateChild<Button>("cinematic");
            button4->SetTexture(texture);
            button4->SetImageRect(IntRect(512, 0, 640, 128));
            button4->SetHoverOffset(0, 128);
            button4->SetVisible(false);

            popup->SetLayoutMode(LM_HORIZONTAL);
            popup->SetOpacity(0.9f);
            popup->SetColor(Color(0.f,0.f,0.f,0.f));
            popup->SetSize(3*UISIZE[MBUTTONUISIZE], UISIZE[MBUTTONUISIZE]);
        }

        // menu button
        const int width = GameStatics::ui_->GetRoot()->GetSize().x_;
        const int height = GameStatics::uiScale_ < 1.f ? (int)floor((float)GameStatics::graphics_->GetHeight() / GameStatics::uiScale_) : GameStatics::ui_->GetRoot()->GetSize().y_;
        const int border = 10 * GameStatics::uiScale_;
        accessMenu_ = uiroot->CreateChild<UIMenu>();
        accessMenu_->SetName("accessmenu");
        accessMenu_->SetTexture(texture);
        accessMenu_->SetImageRect(IntRect(0, 0, 128, 128));
        accessMenu_->SetEnableAnchor(true);
        accessMenu_->SetMinOffset(IntVector2(border, height-UISIZE[MBUTTONUISIZE]-border));
        accessMenu_->SetMaxOffset(IntVector2(border+UISIZE[MBUTTONUISIZE], height-border));
        accessMenu_->SetOpacity(0.9f);
        accessMenu_->SetHoverOffset(0, 128);
        accessMenu_->SetPopup(popup);
        accessMenu_->SetPopupOffset(UISIZE[MBUTTONUISIZE]/2, -10);
        accessMenu_->SetLayoutMode(LM_FREE);
        accessMenu_->SetPosition(border, height-UISIZE[MBUTTONUISIZE] - border);
        accessMenu_->SetPriority(1000);
        URHO3D_LOGINFOF("Game() - CreateAccessMenu ... on %s !", uiroot->GetName().CString());
    }
}

void Game::ShowHeader(UIElement* uiroot)
{
    headerHolder_ = uiroot->GetChild(String("HeaderHolder"));

    if (!headerHolder_)
    {
        URHO3D_LOGINFOF("Game() - ShowHeader : ... ");

        const int SPRITE_UISIZE = 72;
        const int FONT_UISIZE = 20;

        ResourceCache* cache = context_->GetSubsystem<ResourceCache>();
        SharedPtr<XMLFile> xmlfile(cache->GetResource<XMLFile>("UI/header.xml"));
        uiroot->LoadChildXML(xmlfile->GetRoot());

        headerHolder_ = uiroot->GetChild(String("HeaderHolder"));
        if (headerHolder_)
        {
            headerHolder_->SetSize(GameStatics::graphics_->GetWidth() / GameStatics::uiScale_, 128);

            Button* triesButton = static_cast<Button*>(headerHolder_->GetChild(0)->GetChild(0));
            static_cast<Text*>(triesButton->GetChild(0))->SetText(String(GameStatics::tries_));
            SubscribeToEvent(triesButton, E_FOCUSED, URHO3D_HANDLER(Game, HandleUpdateHeader));
            SubscribeToEvent(triesButton, E_DEFOCUSED, URHO3D_HANDLER(Game, HandleUpdateHeader));

            Button* coinsButton = static_cast<Button*>(headerHolder_->GetChild(0)->GetChild(1));
            static_cast<Text*>(coinsButton->GetChild(0))->SetText(String(GameStatics::coins_));
            SubscribeToEvent(coinsButton, E_FOCUSED, URHO3D_HANDLER(Game, HandleUpdateHeader));
            SubscribeToEvent(coinsButton, E_DEFOCUSED, URHO3D_HANDLER(Game, HandleUpdateHeader));

            GameStatics::gameState_.UpdateStoryItems();

            int numitemsvisible = 0;
            for (int i=1; i <= MAXZONEOBJECTS; i++)
            {
                bool visible = GameStatics::gameState_.storyitems_[MAXZONEOBJECTS-i];
                UIElement* itembutton = headerHolder_->GetChild(1)->GetChild(i-1);
                itembutton->SetVisible(visible);
                if (visible)
                {
                    SubscribeToEvent(itembutton, E_FOCUSED, URHO3D_HANDLER(Game, HandleUpdateHeader));
                    SubscribeToEvent(itembutton, E_DEFOCUSED, URHO3D_HANDLER(Game, HandleUpdateHeader));
                    numitemsvisible++;
                }
            }
            headerHolder_->GetChild(1)->SetPosition(GameStatics::graphics_->GetWidth() / GameStatics::uiScale_ - (numitemsvisible * (64 + 10)), 10);

            URHO3D_LOGINFOF("Game() - ShowHeader : ... numitemsvisible=%u", numitemsvisible);
        }
    }

    SubscribeToEvent(GAME_TRYADDED, URHO3D_HANDLER(Game, HandleUpdateHeader));
    SubscribeToEvent(GAME_COINUPDATED, URHO3D_HANDLER(Game, HandleUpdateHeader));

    GameHelpers::SetMoveAnimationUI(headerHolder_, IntVector2(0, -96), IntVector2::ZERO, 1.f, SWITCHSCREENTIME/2);
}

void Game::HideHeader(UIElement* uiroot)
{
    UIElement* holder = uiroot->GetChild(String("HeaderHolder"));
    if (headerHolder_)
    {
        GameHelpers::SetMoveAnimationUI(holder, IntVector2::ZERO, IntVector2(0, -96), 0.f, SWITCHSCREENTIME/2);

        if (holder == headerHolder_)
        {
            UnsubscribeFromEvent(GAME_TRYADDED);
            UnsubscribeFromEvent(GAME_COINUPDATED);
        }
    }
}


void Game::SubscribeToAccessMenuEvents()
{
    game_->SubscribeToEvent(GAME_STATEBACK, URHO3D_HANDLER(Game, HandleGoBack));

    if (accessMenu_)
    {
        UIElement* popup = accessMenu_->GetPopup();
        UIElement* button;
        button = popup->GetChild(String("back"));
        if (button)
            game_->SubscribeToEvent(button, E_RELEASED, URHO3D_HANDLER(Game, HandleGoBack));

        button = popup->GetChild(String("options"));
        if (button)
            game_->SubscribeToEvent(button, E_RELEASED, URHO3D_HANDLER(Game, HandleShowOptions));

        button = popup->GetChild(String("shop"));
        if (button)
            game_->SubscribeToEvent(button, E_RELEASED, URHO3D_HANDLER(Game, HandleShowShop));

        button = popup->GetChild(String("cinematic"));
        if (button)
            game_->SubscribeToEvent(button, E_RELEASED, URHO3D_HANDLER(Game, HandleWatchCinematic));
    }
}

void Game::UnsubscribeFromAccessMenuEvents()
{
    game_->UnsubscribeFromEvent(GAME_STATEBACK);

    if (accessMenu_)
    {
        UIElement* popup = accessMenu_->GetPopup();
        UIElement* button;
        button = popup->GetChild(String("back"));
        if (button)
            game_->UnsubscribeFromEvent(button, E_RELEASED);
        button = popup->GetChild(String("options"));
        if (button)
            game_->UnsubscribeFromEvent(button, E_RELEASED);
        button = popup->GetChild(String("shop"));
        if (button)
            game_->UnsubscribeFromEvent(button, E_RELEASED);
        button = popup->GetChild(String("cinematic"));
        if (button)
            game_->UnsubscribeFromEvent(button, E_RELEASED);
    }
}


void Game::HandleRewardsEvents(StringHash eventType, VariantMap& eventData)
{
    if (eventType == E_POSTUPDATE)
    {
        if (GameStatics::IsPreloading())
            return;

        if (GameStatics::stateManager_->GetActiveState())
        {
            const String& stateid = GameStatics::stateManager_->GetActiveState()->GetStateId();
            if (stateid == "Splash" || stateid == "Cinematic")
                return;
        }

        if (GameStatics::rewardEventStack_.Size())
        {
            URHO3D_LOGINFOF("Game() - HandleRewardsEvents : have rewards in the stack qty=%d !", GameStatics::rewardEventStack_.Size());

            // unpause the game
            SendEvent(GAME_UNPAUSE);

            InteractiveFrame* frame = 0;
            for (unsigned i=0; i < GameStatics::rewardEventStack_.Size(); i++)
            {
                RewardEventItem& item = GameStatics::rewardEventStack_[i];
                if (item.interactive_)
                {
                    if (!frame)
                    {
                        frame = InteractiveFrame::Get("UI/InteractiveFrame/BonusFrame.xml");
                        Graphics* graphics = GetSubsystem<Graphics>();
                        frame->SetScreenPositionEntrance(IntVector2(-300, graphics->GetHeight()/2));
                        frame->SetScreenPosition(IntVector2(graphics->GetWidth()/2, graphics->GetHeight()/2));
                        frame->SetScreenPositionExit(0, IntVector2(-300, graphics->GetHeight()/2));
                        frame->SetScreenPositionExit(1, IntVector2(graphics->GetWidth()+300, graphics->GetHeight()/2));
                        frame->SetScreenPositionEntranceForBonus(IntVector2(graphics->GetWidth()/2, -200));
                    }

                    frame->AddBonus(item.GetSlot());
                }
                else
                {
                    GameStatics::AddBonus(item.GetSlot(), this);
                }
            }

            if (frame)
            {
                GameStatics::AddBonuses(frame->GetBonuses(), this);
                frame->Start(false, false);
                SubscribeToEvent(frame, E_MESSAGEACK, URHO3D_HANDLER(Game, OnBonusFrameMessageAck));
            }

//            if (!GameStatics::tries_)
//                SendEvent(GAME_OVER);

            GameStatics::rewardEventStack_.Clear();

            GameStatics::gameState_.Save();
        }
    }
    else if (eventType == GAME_EARNSTARS)
    {
        int numstars = Min(eventData[Game_EarnStars::NUMSTARS].GetInt(), MAX_ADDINGSTARS);
        URHO3D_LOGINFOF("Game() - HandleRewardsEvents : GAME_EARNSTARS : qty=%d !", numstars);

        GameStatics::rewardEventStack_.Resize(GameStatics::rewardEventStack_.Size()+1);
        RewardEventItem& item = GameStatics::rewardEventStack_.Back();
        item.rewardid_ = numstars;
        item.interactive_ = true;
    }
}

void Game::OnBonusFrameMessageAck(StringHash eventType, VariantMap& eventData)
{
    URHO3D_LOGINFO("Game() - OnBonusFrameMessageAck  !");

    InteractiveFrame* stoppedframe = static_cast<InteractiveFrame*>(context_->GetEventSender());
    if (stoppedframe)
    {
        UnsubscribeFromEvent(stoppedframe, E_MESSAGEACK);
        stoppedframe->Stop();
    }

    GameHelpers::SetMusic(MAINMUSIC, 1.f);
}

void Game::HandleAppPaused(StringHash eventType, VariantMap& eventData)
{
    URHO3D_LOGINFO("Game() - HandleAppPaused !");
    GameStatics::gameState_.Save();
}

void Game::HandleAppResumed(StringHash eventType, VariantMap& eventData)
{
    URHO3D_LOGINFO("Game() - HandleAppResumed !");
    GameStatics::CheckTimeForEarningStars();
}


void Game::HandlePreloadResources(StringHash eventType, VariantMap& eventData)
{
    if (GameStatics::PreloadResources())
    {
        UnsubscribeFromEvent(E_SCENEUPDATE);
        SubscribeToEvent(GAME_PRELOADINGFINISHED, URHO3D_HANDLER(Game, HandlePreloadFinished));
        SendEvent(GAME_PRELOADINGFINISHED);
    }
}

void Game::HandlePreloadFinished(StringHash eventType, VariantMap& eventData)
{
    UnsubscribeFromEvent(GAME_PRELOADINGFINISHED);

#ifdef ACTIVE_GAMELOOPTESTING
    // Launch Test if defined (in Android GameStatics::playTest_ is setted with JNI cf GameTest)

    if (GameStatics::playTest_ > 0)
    {
        URHO3D_LOGINFOF("Game() - HandlePreloadFinished : Launch Test=%d .... OK ! ", GameStatics::playTest_);

        if (!PlayInputRecordTest(GameStatics::playTest_))
        {
        #ifdef __ANDROID__
            Android_FinishTest();
        #else
            GameStatics::Exit();
        #endif
            return;
        }
    }
    else
    {
        URHO3D_LOGINFOF("Game() - HandlePreloadFinished : Test must be >= 1 to be launched ... ");
    #ifdef __ANDROID__
        Android_FinishTest();
        return;
    #endif
    }
#endif

	SubscribeToEvent(E_SCENEUPDATE, URHO3D_HANDLER(Game, HandleSceneUpdate));
}

void Game::HandleGoBack(StringHash eventType, VariantMap& eventData)
{
    URHO3D_LOGINFO("Game() - HandleGoBack !");

    if (accessMenu_)
        accessMenu_->ShowPopup(false);

    options_->CloseFrame();

    if (GameStatics::stateManager_->GetStackSize() > 1)
        GameStatics::stateManager_->PopStack();
}

void Game::HandleShowOptions(StringHash eventType, VariantMap& eventData)
{
    URHO3D_LOGINFO("Game() - HandleShowOptions !");

    accessMenu_->ShowPopup(false);
    options_->SwitchParameterFrame();
}

void Game::HandleShowShop(StringHash eventType, VariantMap& eventData)
{
    URHO3D_LOGINFO("Game() - HandleShowShop !");

    accessMenu_->ShowPopup(false);
    options_->SwitchShopFrame();
}

void Game::HandleWatchCinematic(StringHash eventType, VariantMap& eventData)
{
    URHO3D_LOGINFO("Game() - HandleWatchCinematic !");

    accessMenu_->ShowPopup(false);
    CinematicState::LaunchCinematicParts(CINEMATICSELECTIONMODE_REPLAY);
}

void Game::HandleTouchBegin(StringHash eventType, VariantMap& eventData)
{
    // On some platforms like Windows the presence of touch input can only be detected dynamically
    GameStatics::InitTouchInput(context_);
    UnsubscribeFromEvent("TouchBegin");
}

void Game::HandleKeyDown(StringHash eventType, VariantMap& eventData)
{
	using namespace KeyDown;

	int scancode = eventData[P_SCANCODE].GetInt();
    int keycode = eventData[P_KEY].GetInt();

	// Close console (if open)
	if (scancode == SCANCODE_ESCAPE)
	{
		Console* console = GetSubsystem<Console>();
		if (console)
            if (console->IsVisible())
                console->SetVisible(false);
	}
    // Toggle console with F1
    else if (scancode == SCANCODE_F1)
    {
        Console* console = GetSubsystem<Console>();
        if (console)
            console->Toggle();
    }
    // Toggle debug HUD with F2
    else if (scancode == SCANCODE_F2)
    {
        DebugHud* debugHud = GetSubsystem<DebugHud>();
        if (debugHud)
        {
            if (debugHud->GetMode() == DEBUGHUD_SHOW_NONE)
                debugHud->SetMode(DEBUGHUD_SHOW_FPS);
            else if (debugHud->GetMode() == DEBUGHUD_SHOW_FPS)
                debugHud->SetMode(DEBUGHUD_SHOW_ALL);
            else if (debugHud->GetMode() == DEBUGHUD_SHOW_ALL)
                debugHud->SetMode(DEBUGHUD_SHOW_ALL_MEMORY);
            else if (debugHud->GetMode() == DEBUGHUD_SHOW_ALL_MEMORY)
                debugHud->SetMode(DEBUGHUD_SHOW_NONE);
        }
    }
#ifdef ACTIVE_GAMELOOPTESTING
    // Switch Start/Stop Record Test
    else if (scancode == SCANCODE_F3)
    {
        InputRecorder* recorder = InputRecorder::Get();

        if (recorder->IsRecording())
            recorder->Stop();
        else
            recorder->Start();
    }
    // Switch Start/Stop Replay Test
    else if (scancode == SCANCODE_F4)
    {
        InputPlayer* player = InputPlayer::Get();

        if (player)
        {
            if (player->IsPlaying())
                player->Stop();
            else
            {
                if (!GameStatics::playTest_ || GameStatics::playTest_ >= player->GetLastRecordFile())
                    GameStatics::playTest_ = 1;

                bool ok = PlayInputRecordTest(GameStatics::playTest_);
            }
        }
    }
    // Switch Start/Stop Random Test
    else if (scancode == SCANCODE_F5)
    {
        InputPlayer* player = InputPlayer::Get();
        if (player)
        {
            if (player->IsPlaying())
                player->Stop();
            else
            {
                player->SetNumRandomEntries(100);
                player->Start();
            }
        }
    }
#endif
#ifdef ACTIVE_TESTMODE
    // Toggle TestMode
    else if (scancode == SCANCODE_F6)
    {
        MatchesManager::SwitchTestMode();
    }
#endif
#ifdef ACTIVE_TIPS
    else if (scancode == SCANCODE_E)
    {
        GameStatics::AddEarnStars(1);
    }
#endif
    else if (scancode == SCANCODE_F7)
    {
        InputPlayer* player = InputPlayer::Get();
        if (player)
        {
            if (player->IsPlaying())
                player->Stop();
            else
                //player->StartFile("didacticiel_shopacces");
                player->StartFile("didacticiel_play01");
        }
    }
	// Common rendering quality controls, only when UI has no focused element
	else if (!GetSubsystem<UI>()->GetFocusElement())
	{
		Renderer* renderer = GetSubsystem<Renderer>();
        if (!renderer) return;
        // Texture quality
        else if (scancode == SCANCODE_1)
        {
            int quality = renderer->GetTextureQuality();
            ++quality;
            if (quality > QUALITY_HIGH)
                quality = QUALITY_LOW;
            renderer->SetTextureQuality(quality);
        }
        // Material quality
        else if (scancode == SCANCODE_2)
        {
            int quality = renderer->GetMaterialQuality();
            ++quality;
            if (quality > QUALITY_HIGH)
                quality = QUALITY_LOW;
            renderer->SetMaterialQuality(quality);
        }
        // Specular lighting
        else if (scancode == SCANCODE_3)
            renderer->SetSpecularLighting(!renderer->GetSpecularLighting());
        // Shadow rendering
        else if (scancode == SCANCODE_4)
            renderer->SetDrawShadows(!renderer->GetDrawShadows());
        // Shadow map resolution
        else if (scancode == SCANCODE_5)
        {
            int shadowMapSize = renderer->GetShadowMapSize();
            shadowMapSize *= 2;
            if (shadowMapSize > 2048)
                shadowMapSize = 512;
            renderer->SetShadowMapSize(shadowMapSize);
        }
        // Shadow depth and filtering quality
        else if (scancode == SCANCODE_6)
        {
            ShadowQuality quality = renderer->GetShadowQuality();
            quality = (ShadowQuality)(quality + 1);
            if (quality > SHADOWQUALITY_BLUR_VSM)
                quality = SHADOWQUALITY_SIMPLE_16BIT;
            renderer->SetShadowQuality(quality);
        }
        // Occlusion culling
        else if (scancode == SCANCODE_7)
        {
            bool occlusion = renderer->GetMaxOccluderTriangles() > 0;
            occlusion = !occlusion;
            renderer->SetMaxOccluderTriangles(occlusion ? 5000 : 0);
        }
        // Instancing
        else if (scancode == SCANCODE_8)
            renderer->SetDynamicInstancing(!renderer->GetDynamicInstancing());
        // Take screenshot
        else if (scancode == SCANCODE_9)
        {
            Graphics* graphics = GetSubsystem<Graphics>();
            Image screenshot(context_);
            graphics->TakeScreenShot(screenshot);
            // Here we save in the Data folder with date and time appended
            screenshot.SavePNG(GetSubsystem<FileSystem>()->GetProgramDir() + "Data/Screenshot_" +
                Time::GetTimeStamp().Replaced(':', '_').Replaced('.', '_').Replaced(' ', '_') + ".png");
        }
#ifdef ACTIVE_TIPS
        // Tips Coins and Stars
        else if (keycode == KEY_M)
        {
            URHO3D_LOGINFOF("Game() - HandleKeyDown : Key M pressed => Add a move");
            SendEvent(GAME_MOVEADDED);
        }
        else if (scancode == SCANCODE_S)
        {
            URHO3D_LOGINFOF("Game() - HandleKeyDown : Key S pressed => Add a Try");
            GameStatics::AddBonus(Slot(COT::STARS, StringHash("star1"), 1), this);
        }
        else if (scancode == SCANCODE_C)
        {
            URHO3D_LOGINFOF("Game() - HandleKeyDown : Key C pressed => Add a Coin");
            GameStatics::AddBonus(Slot(COT::COINS, StringHash("coin1"), 1), this);
        }
        else if (scancode == SCANCODE_T)
        {
            URHO3D_LOGINFOF("Game() - HandleKeyDown : Key T pressed => Check Time for winning stars");
            GameStatics::CheckTimeForEarningStars();
        }
#endif
	}
}

void Game::HandleScreenResized(StringHash eventType, VariantMap& eventData)
{
    URHO3D_LOGINFO("Game() - HandleScreenResized !");

    ResetScreen();

    if (dialogInfo_)
        dialogInfo_->Resize();
    if (companionBox_)
        companionBox_->Resize();

    SendEvent(GAME_SCREENRESIZED);
}

void Game::HandleConsoleCommand(StringHash eventType, VariantMap& eventData)
{
    using namespace ConsoleCommand;
    if (eventData[P_ID].GetString() != GetTypeName())
        return;

    GameCommands::Launch(context_, eventData[P_COMMAND].GetString());
}

void Game::HandleUpdateHeader(StringHash eventType, VariantMap& eventData)
{
    if (!headerHolder_)
        return;

    if (eventType == GAME_TRYADDED)
    {
        URHO3D_LOGINFOF("Game() - HandleUpdateHeader : try Added => tries=%d !", GameStatics::tries_);

        Text* triesText = static_cast<Text*>(headerHolder_->GetChild(0)->GetChild(0)->GetChild(0));
        triesText->SetText(String(GameStatics::tries_));

        GameHelpers::AddText3DFadeAnim(GameStatics::rootScene_, "+1", triesText, Vector3(0.5f, -0.5f, 0.f) * GameStatics::uiScale_, 1.f, 8.f);

        GameHelpers::SpawnSound(GameStatics::rootScene_, SND_ADDTRY);
    }
    else if (eventType == GAME_COINUPDATED)
    {
        URHO3D_LOGINFOF("Game() - HandleUpdateHeader : coin Updated => coin=%d !", GameStatics::coins_);

        Text* coinsText = static_cast<Text*>(headerHolder_->GetChild(0)->GetChild(1)->GetChild(0));
        coinsText->SetText(String(GameStatics::coins_));

        GameHelpers::AddText3DFadeAnim(GameStatics::rootScene_, "+1", coinsText, Vector3(0.5f, -0.5f, 0.f) * GameStatics::uiScale_, 1.f, 8.f);
        GameHelpers::SpawnSound(GameStatics::rootScene_, SND_ADDCOIN);
    }
    else if (eventType == E_FOCUSED)
    {
        Button* button = static_cast<Button*>(context_->GetEventSender());
        URHO3D_LOGINFOF("Game() - HandleUpdateHeader : button = %s pressed !", button->GetName().CString());

        if (!dialogInfo_)
            dialogInfo_ = new UIDialog(context_);

        if (!dialogInfo_->GetFrame())
            dialogInfo_->Set(headerHolder_, "UI/dialogboxinfo.xml");

        dialogInfo_->SetMessage(button->GetName());
        dialogInfo_->StartMessage(true);
    }
    else if (eventType == E_DEFOCUSED)
    {
        if (dialogInfo_)
            dialogInfo_->StopMessage(true);
    }
}

void Game::SetCompanionMessages()
{
    delayCompanion_ = 0;

    bool alwaysactive = false;
#ifdef TESTMODE
        alwaysactive = true;
#endif

    if (!companionBox_)
    {
        companionBox_ = new UIDialog(context_);
        companionBox_->SetArchiveViewedMessages(!alwaysactive);
    }

    if (!companionBox_->GetFrame())
        companionBox_->Set(GameStatics::ui_->GetRoot(), "UI/companion.xml");

    companionBox_->Clear(true);

    // condition d'activation d'un tip de GameState
    if (GameStateManager::Get()->GetActiveState()->GetStateHash() == STATE_LEVELMAP)
    {
        URHO3D_LOGINFOF("Game() - SetCompanionMessages : Add Companion Messages for STATE_LEVELMAP !");

        if (GameStatics::playerState_->zone == 1)
        {
            if (GameStatics::playerState_->missionstates[2-1].state_ == GameStatics::MissionState::MISSION_LOCKED || alwaysactive)
            {
                companionBox_->AddMessage(STATE_LEVELMAP, false, "story_captured", "capi", 0.f);
                companionBox_->AddMessage(STATE_LEVELMAP, false, "story_hello1", "capi");
                companionBox_->AddMessage(STATE_LEVELMAP, false, "tuto_startmission", "capi");
            }
            if (GameStatics::playerState_->missionstates[1-1].state_ == GameStatics::MissionState::MISSION_COMPLETED &&
                GameStatics::playerState_->missionstates[2-1].state_ != GameStatics::MissionState::MISSION_COMPLETED)
            {
                Message* message = companionBox_->AddMessage(STATE_LEVELMAP, false, "tuto_powers", "capi");
                if (message)
                {
                    message->AddEventAction(DelayAction::Get(MESSAGE_START, UIMENU_SHOWCONTENT, 1.00f, "UI/Companion/animatedcursors.scml", "cursor_arrow_ontop", accessMenu_, Vector2(0.f, -accessMenu_->GetSize().y_)));
                    UIElement* shopaccess = accessMenu_->GetPopup()->GetChild(2);
                    message->AddEventAction(DelayAction::Get(UIMENU_SHOWCONTENT, E_CLICK, 0.25f, "UI/Companion/animatedcursors.scml", "cursor_arrow_ontop", shopaccess, Vector2(-shopaccess->GetSize().x_ / 2, 0.f)));
                    UIElement* poweraccess = options_->GetShop()->GetChild("Switch2", true);
                    message->AddEventAction(DelayAction::Get(UIMENU_SHOWCONTENT, E_CLICK, 0.25f, "UI/Companion/animatedcursors.scml", "cursor_arrow_ontop", poweraccess, Vector2(-poweraccess->GetSize().x_/2, -poweraccess->GetSize().y_)));
                }
            }
            if (GameStatics::playerState_->missionstates[3-1].state_ == GameStatics::MissionState::MISSION_COMPLETED &&
                GameStatics::playerState_->missionstates[4-1].state_ == GameStatics::MissionState::MISSION_UNLOCKED &&
                GameStatics::playerState_->missionstates[5-1].state_ == GameStatics::MissionState::MISSION_UNLOCKED)
            {
                companionBox_->AddMessage(STATE_LEVELMAP, false, "tuto_zoom", "capi", 0.f, "UI/Companion/animatedcursors.scml", "cursor_zoom", GameStatics::ui_->GetRoot());
                companionBox_->AddMessage(STATE_LEVELMAP, false, "story_branch1", "capi", 0.f);
            }

            if (GameStatics::playerState_->zonestates[GameStatics::playerState_->zone] == GameStatics::ZONE_UNBLOCKED)
            {
                Message* message = companionBox_->AddMessage(STATE_LEVELMAP, false, "tuto_constellation1", "capi");
                Node* nextZoneButton = GameStatics::rootScene_->GetChild("CurrentLevelMap")->GetChild("UILevelMapScene")->GetChild("Zone_next");
                if (message && nextZoneButton)
                    message->AddEventAction(DelayAction::Get(MESSAGE_START, MESSAGE_STOP, 1.00f, "UI/Companion/animatedcursors.scml", "cursor_arrow_ontop", nextZoneButton));

                if (GameStatics::GetNextUnlockedMission(false) != -1)
                    companionBox_->AddMessage(STATE_LEVELMAP, false, "story_allmissions1", "capi", 0.f);
            }
        }
        else if (GameStatics::playerState_->zone == 2)
        {
            if (GameStatics::playerState_->missionstates[10-1].state_ == GameStatics::MissionState::MISSION_UNLOCKED || alwaysactive)
            {
                companionBox_->AddMessage(STATE_LEVELMAP, false, "story_hello2_1", "capi", 0.f);
                companionBox_->AddMessage(STATE_LEVELMAP, false, "story_hello2_2", "rosi", 0.f);
                companionBox_->AddMessage(STATE_LEVELMAP, false, "story_hello2_3", "rosi", 0.f);
                companionBox_->AddMessage(STATE_LEVELMAP, false, "story_hello2_4", "rosi", 0.f);
            }
            if (GameStatics::playerState_->missionstates[16-1].state_ == GameStatics::MissionState::MISSION_COMPLETED &&
                GameStatics::playerState_->missionstates[17-1].state_ == GameStatics::MissionState::MISSION_UNLOCKED &&
                GameStatics::playerState_->missionstates[23-1].state_ == GameStatics::MissionState::MISSION_UNLOCKED)
            {
                companionBox_->AddMessage(STATE_LEVELMAP, false, "story_branch1", "rosi", 0.f);
                companionBox_->AddMessage(STATE_LEVELMAP, false, "story_allmissions1", "capi", 0.f);
            }
        }
        else if (GameStatics::playerState_->zone == 3)
        {
            if (GameStatics::playerState_->missionstates[25-1].state_ == GameStatics::MissionState::MISSION_UNLOCKED || alwaysactive)
            {
                companionBox_->AddMessage(STATE_LEVELMAP, false, "story_hello3_1", "jona", 0.f);
                companionBox_->AddMessage(STATE_LEVELMAP, false, "story_hello3_2", "jona", 0.f);
            }
        }
    }
    else if (GameStateManager::Get()->GetActiveState()->GetStateHash() == STATE_PLAY)
    {
        URHO3D_LOGINFOF("Game() - SetCompanionMessages : Add Companion Messages for STATE_PLAY !");

        if (GameStatics::playerState_->zone == 1)
        {
            if (GameStatics::playerState_->missionstates[1-1].state_ == GameStatics::MissionState::MISSION_UNLOCKED || alwaysactive)
            {
                companionBox_->AddMessage(STATE_PLAY, false, "tuto_match3", "capi");
                Message* message = companionBox_->AddMessage(STATE_PLAY, false, "tuto_objective", "capi");
                if (message)
                {
                    UIElement* objective0 = GameStatics::ui_->GetRoot()->GetChild(String("playrootui"))->GetChild(String("objectivezone"), true)->GetChild(String("objectivebutton_0"));
                    message->AddEventAction(DelayAction::Get(MESSAGE_START, MESSAGE_STOP, 1.00f, "UI/Companion/animatedcursors.scml", "cursor_arrow_onbottom", objective0, Vector2::ZERO));
                }
                companionBox_->AddMessage(STATE_PLAY, false, "tuto_msquare", "capi");
            }
            if ((GameStatics::playerState_->missionstates[1-1].state_ == GameStatics::MissionState::MISSION_COMPLETED &&
                 GameStatics::playerState_->missionstates[2-1].state_ != GameStatics::MissionState::MISSION_COMPLETED) || alwaysactive)
            {
                companionBox_->AddMessage(STATE_PLAY, false, "tuto_powers2", "capi");

                Message* message = companionBox_->AddMessage(STATE_PLAY, false, "tuto_powers", "capi");
                if (message)
                {
                    message->AddEventAction(DelayAction::Get(MESSAGE_START, UIMENU_SHOWCONTENT, 0.f, "UI/Companion/animatedcursors.scml", "cursor_arrow_ontop", accessMenu_, Vector2(0.f, -accessMenu_->GetSize().y_)));
                    UIElement* shopaccess = accessMenu_->GetPopup()->GetChild(2);
                    message->AddEventAction(DelayAction::Get(UIMENU_SHOWCONTENT, E_CLICK, 0.f, "UI/Companion/animatedcursors.scml", "cursor_arrow_ontop", shopaccess, Vector2(-shopaccess->GetSize().x_ / 2, 0.f)));
                    UIElement* poweraccess = options_->GetShop()->GetChild("Switch2", true);
                    message->AddEventAction(DelayAction::Get(UIMENU_SHOWCONTENT, E_CLICK, 0.f, "UI/Companion/animatedcursors.scml", "cursor_arrow_ontop", poweraccess, Vector2(-poweraccess->GetSize().x_/2, -poweraccess->GetSize().y_)));
                }
            }
            if (GameStatics::playerState_->level == 9 && GameStatics::playerState_->missionstates[9-1].state_ == GameStatics::MissionState::MISSION_UNLOCKED)
            {
                companionBox_->AddMessage(STATE_PLAY, false, "story_boss01", "capi", 12.f);
                companionBox_->AddMessage(STATE_PLAY, false, "tuto_boss01", "capi", 1.f);
            }
        }
        else if (GameStatics::playerState_->zone == 3)
        {
            if (GameStatics::playerState_->level == 42 && GameStatics::playerState_->missionstates[42-1].state_ == GameStatics::MissionState::MISSION_UNLOCKED)
            {
                companionBox_->AddMessage(STATE_PLAY, false, "tuto_boss03_1", "jona", 12.f);
            }
        }
    }

    companionBox_->Dump();
}

void Game::HandleUpdateCompanion(StringHash eventType, VariantMap& eventData)
{
    if (!GameStatics::playerState_->tutorialEnabled_)
        return;

    if (eventType == GAME_STATEEND)
    {
        delayCompanion_ = 0;
        if (companionBox_)
        {
            companionBox_->StopMessage(false);
            companionBox_->Clear(true);

            companionBox_->Dump();
        }
    }
    else if (eventType == GAME_STATEREADY)
    {
        SetCompanionMessages();
    }
    else if (eventType == COMPANION_SHOW)
    {
        if (companionBox_ && !companionBox_->IsStarted())
        {
            delayCompanion_ = 0;
            numClicksOutsideCompanionBox_ = 0;

            if (accessMenu_ && accessMenu_->GetShowPopup())
                return;
            if (options_ && options_->GetShop()->IsVisible())
                return;
            if (InteractiveFrame::HasRunningInstances())
                return;

            if (!companionBox_->GetFrame())
                companionBox_->Set(GameStatics::ui_->GetRoot(), "UI/companion.xml");

            if (!eventData[Companion_Show::CONTENT].GetString().Empty())
                companionBox_->SetMessage(eventData[Companion_Show::CONTENT].GetString());

            URHO3D_LOGINFOF("COMPANION_SHOW delay=%F !", companionBox_->GetCurrentMessageDelay());

            companionBox_->StartMessage(true);
        }
    }
    else if (eventType == COMPANION_HIDE)
    {
        if (companionBox_ && companionBox_->IsStarted())
        {
            companionBox_->StopMessage(true);
            delayCompanion_ = 0;
            numClicksOutsideCompanionBox_ = 0;
        }
    }
    else if (eventType == E_CLICK)
    {
        UIElement* elt =static_cast<UIElement*>(eventData[Click::P_ELEMENT].GetPtr());

//        URHO3D_LOGINFOF("COMPANION_CLICK on UIElement=%s !", elt ? elt->GetName().CString():"none");

        if (companionBox_ && !companionBox_->IsBusy())
        {
            if (accessMenu_ && accessMenu_->GetShowPopup())
                return;
            if (options_ && options_->GetShop()->IsVisible())
                return;

            if (companionBox_->IsStarted() && companionBox_->HasMessagesToShow() && delayCompanion_ > companionBox_->GetNextMessageDelay())
                companionBox_->StartMessage(false);
            else
                companionBox_->StopMessage(true);

            delayCompanion_ = 0;
            numClicksOutsideCompanionBox_ = 0;
        }
    }
    else if (eventType == E_TOUCHEND || eventType == E_MOUSEBUTTONUP)
    {
//        URHO3D_LOGINFOF("COMPANION_BUTTONUP !");

        if (companionBox_)
        {
            if (accessMenu_ && accessMenu_->GetShowPopup())
                return;
            if (options_ && options_->GetShop()->IsVisible())
                return;

            numClicksOutsideCompanionBox_++;
            if (numClicksOutsideCompanionBox_ > NumMaxClicksEnd)
            {
                if (companionBox_->IsStarted() && companionBox_->HasMessagesToShow() && delayCompanion_ > companionBox_->GetNextMessageDelay())
                    companionBox_->StartMessage(false);
                else
                    companionBox_->StopMessage(true);

                delayCompanion_ = 0;
                numClicksOutsideCompanionBox_ = 0;
            }
        }
    }
}

void Game::HandleSceneUpdate(StringHash eventType, VariantMap& eventData)
{
    // Autohide cursor
#ifdef ACTIVE_HANDLECURSORVISIBILITY
    if (GameStatics::gameConfig_.autoHideCursorEnable_ && GameStatics::input_->HasFocus())
    {
        if (GameStatics::cursor_)
        {
            if (GameStatics::cursor_->IsVisible())
            {
                timerInactiveCursor_ += eventData[SceneUpdate::P_TIMESTEP].GetFloat();
                if (timerInactiveCursor_ > DELAY_INACTIVECURSOR)
                    GameStatics::cursor_->SetVisible(false);
            }
            else if (GameStatics::input_->GetMouseMove() != IntVector2::ZERO)
            {
                timerInactiveCursor_ = 0.f;
                GameStatics::cursor_->SetVisible(true);
            }
        }
    }
#endif
    // Handle companion
    if (companionBox_)
    {
        if (GameStatics::playerState_->tutorialEnabled_)
        {
            delayCompanion_ += eventData[SceneUpdate::P_TIMESTEP].GetFloat();

            if (InteractiveFrame::HasRunningInstances())
            {
                delayCompanion_ = 0;
                return;
            }

            if (companionBox_ && !companionBox_->IsStarted() && companionBox_->HasMessagesToShow() &&
                delayCompanion_ > companionBox_->GetCurrentMessageDelay())
            {
                delayCompanion_ = 0;
                SendEvent(COMPANION_SHOW);
            }
        }
        else
        {
            companionBox_.Reset();
        }
    }
}
