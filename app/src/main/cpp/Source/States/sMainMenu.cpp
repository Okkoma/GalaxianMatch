#include <Urho3D/Urho3D.h>

#include <Urho3D/Core/CoreEvents.h>
#include <Urho3D/Engine/Engine.h>

#include <Urho3D/IO/Log.h>

#include <Urho3D/Graphics/Octree.h>
#include <Urho3D/Graphics/Camera.h>
#include <Urho3D/Graphics/Graphics.h>
#include <Urho3D/Graphics/Renderer.h>
#include <Urho3D/Graphics/DebugRenderer.h>
#include <Urho3D/Graphics/GraphicsEvents.h>
#include <Urho3D/Graphics/Texture2D.h>

#include <Urho3D/Resource/Localization.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Resource/XMLFile.h>

#include <Urho3D/Scene/SceneEvents.h>
#include <Urho3D/Scene/Scene.h>

#include <Urho3D/Urho2D/Renderer2D.h>
#include <Urho3D/Urho2D/AnimatedSprite2D.h>
#include <Urho3D/Urho2D/StaticSprite2D.h>
#include <Urho3D/Urho2D/Text2D.h>

#include <Urho3D/Input/Input.h>

#include <Urho3D/UI/UI.h>
#include <Urho3D/UI/UIEvents.h>
#include <Urho3D/UI/Font.h>
#include <Urho3D/UI/Button.h>
#include <Urho3D/UI/Text.h>
#include <Urho3D/UI/Text3D.h>
#include <Urho3D/UI/Sprite.h>
#include <Urho3D/UI/MessageBox.h>

#include "Game.h"
#include "GameEvents.h"
#include "GameStatics.h"
#include "GameHelpers.h"
#include "GameOptions.h"

#include "DelayInformer.h"
#include "TimerRemover.h"

#include "sCinematic.h"

#include "sMainMenu.h"

const int NumSelection = 3;

WeakPtr<Node> titlescene_;
WeakPtr<UIElement> uimenu_;

int menustate_ = 0, lastmenustate_ = 0;
IntRect startRect_, optionsRect_, cinematicRect_;

bool drawDebug_;
extern const char* levelModeNames[];


MenuState::MenuState(Context* context) :
    GameState(context, "MainMenu")
{
    drawDebug_ = 0;
//    URHO3D_LOGINFO("MainMenu()");
}

MenuState::~MenuState()
{
    URHO3D_LOGINFO("~MainMenu()");
}

bool MenuState::Initialize()
{
	return GameState::Initialize();
}

void MenuState::Begin()
{
    if (IsBegun())
        return;

    URHO3D_LOGINFO("MainMenu() - ----------------------------------------");
    URHO3D_LOGINFO("MainMenu() - Begin ...                               -");
    URHO3D_LOGINFO("MainMenu() - ----------------------------------------");

//    // Reset controls
//    GetSubsystem<Input>()->ResetStates();

    GameStatics::camera_->SetZoom(GameStatics::uiScale_);
    GameStatics::UpdateViews();

    GameStatics::screenSceneRect_ = IntRect::ZERO;

    // Set Scene
    CreateScene(true);

    // Set UI
	CreateUI();

    // Play Theme
    if (GameStatics::currentMusic_ != MAINTHEME)
        GameStatics::currentMusic_ = MAINTHEME;

    GameHelpers::SetMusic(MAINMUSIC, 0.7f, GameStatics::currentMusic_, false);

    // Subscribers
    SubscribeToEvent(GAME_SCREENDELAYED, URHO3D_HANDLER(MenuState, HandleScreenResized));
    SubscribeToEvent(GAME_SCREENRESIZED, URHO3D_HANDLER(MenuState, HandleScreenResized));
    SubscribeToEvent(E_KEYDOWN, URHO3D_HANDLER(MenuState, HandleKeyDown));
    SubscribeToMenuEvents();
    Game::Get()->SubscribeToAccessMenuEvents();

    // Update Menu Collider Positions after entrance animation
    DelayInformer::Get(this, SWITCHSCREENTIME + 0.1f, GAME_SCREENDELAYED);

    GameStatics::SetMouseVisible(true, false);

    // Call base class implementation
	GameState::Begin();

    URHO3D_LOGINFO("MainMenu() - ----------------------------------------");
    URHO3D_LOGINFO("MainMenu() - Begin ...  OK !                       -");
    URHO3D_LOGINFO("MainMenu() - ----------------------------------------");
}

void MenuState::End()
{
    if (!IsBegun())
        return;

    URHO3D_LOGINFO("MainMenu() - ----------------------------------------");
    URHO3D_LOGINFO("MainMenu() - End ...                                  -");
    URHO3D_LOGINFO("MainMenu() - ----------------------------------------");

    // Go Out Animation
    if (!GameStatics::gameExit_)
    {
        GameHelpers::SetMoveAnimation(titlescene_, Vector3::ZERO, Vector3(-10.f, 0.f, 0.f), 0.f, SWITCHSCREENTIME);
        GameHelpers::SetMoveAnimationUI(uimenu_, IntVector2::ZERO, IntVector2(-GameStatics::graphics_->GetWidth()/2, 0), 0.f, SWITCHSCREENTIME);
    }

    // Rename Scene for Cleaner
    if (titlescene_)
        titlescene_->SetName("MainMenu");

    RemoveUI();

//    GameHelpers::StopMusic(GameStatics::rootScene_);

    Game::Get()->UnsubscribeFromAccessMenuEvents();
	UnsubscribeFromAllEvents();

//    // Reset controls
//    GetSubsystem<Input>()->ResetStates();

	// Call base class implementation
	GameState::End();

    URHO3D_LOGINFO("MainMenu() - ----------------------------------------");
    URHO3D_LOGINFO("MainMenu() - End ... OK !                           -");
    URHO3D_LOGINFO("MainMenu() - ----------------------------------------");
}

void MenuState::CreateScene(bool setscene)
{
    URHO3D_LOGINFOF("MainMenu() - CreateScene ... titlescene=%s", titlescene_ ? titlescene_->GetName().CString() : "NO");

    if (setscene && !titlescene_)
    {
        // Update Default Components for the Scene
        GameStatics::rootScene_->GetOrCreateComponent<Octree>(LOCAL);
        if (GameStatics::gameConfig_.debugRenderEnabled_)
            GameStatics::rootScene_->GetOrCreateComponent<DebugRenderer>(LOCAL);

        // Load MainMenu Scene Animation
        titlescene_ = GameStatics::rootScene_->GetChild("title") ? GameStatics::rootScene_->GetChild("title") : GameStatics::rootScene_->CreateChild("title", LOCAL);
        Node* mainscene = titlescene_->GetChild("MainScene") ? titlescene_->GetChild("MainScene") : titlescene_->CreateChild("MainScene", LOCAL);

        GameHelpers::LoadNodeXML(context_, mainscene, "UI/Menu.xml", LOCAL);
    }

    // Entrance Animation
    GameHelpers::SetMoveAnimation(titlescene_, Vector3(-10.f, 0.f, 0.f), Vector3::ZERO, 0.f, SWITCHSCREENTIME);
    GameHelpers::SetMoveAnimationUI(uimenu_, IntVector2(-GameStatics::graphics_->GetWidth()/2, 0), IntVector2(0, 0), 0.f, SWITCHSCREENTIME);

    GameStatics::cameraNode_->SetPosition(Vector3::ZERO);

    // Adjust Background To ScreenSize
//    GameHelpers::SetAdjustedToScreen(titlescene_->GetChild("MainScene")->GetChild("MenuFrame"), 1.25f, 1.f, false);
    GameHelpers::SetAdjustedToScreen(titlescene_->GetChild("MainScene")->GetChild("MenuFrame"), 1.25f, 1.25f, false);

    GameStatics::rootScene_->SetUpdateEnabled(true);

    menustate_ = 0;
    lastmenustate_ = 0;

    // Debug
//    GameStatics::DumpNode(rootScene_);

    URHO3D_LOGINFO("MainMenu() - CreateScene ... OK !");
}

void MenuState::CreateUI()
{
    if (!GameStatics::ui_)
        return;

    // Reset Focus, Keep Update restore the focus
	GameStatics::ui_->SetFocusElement(0);

//    if (GameStatics::gameConfig_.touchEnabled_)
//    {
//        GetSubsystem<Input>()->SetScreenJoystickVisible(GameStatics::gameConfig_.screenJoystickID_, true);
//
//        if (GameStatics::gameConfig_.screenJoysticksettingsID_ != -1)
//            GetSubsystem<Input>()->SetScreenJoystickVisible(GameStatics::gameConfig_.screenJoysticksettingsID_, false);
//    }

    // Create uimenu
    if (!uimenu_)
    {
        ResourceCache* cache = GetSubsystem<ResourceCache>();

        URHO3D_LOGINFO("MainMenu() - CreateUI ...");

        int width = GameStatics::ui_->GetRoot()->GetSize().x_;
        int height = GameStatics::ui_->GetRoot()->GetSize().y_;
        uimenu_ = GameStatics::ui_->GetRoot()->CreateChild<UIElement>("menurootui");
        uimenu_->SetPivot(0.5f, 0.5f);
        uimenu_->SetSize(width, height);
        uimenu_->SetPosition(width/2, height/2);

        URHO3D_LOGINFO("MainMenu() - CreateUI ... OK !");
    }

    Game::Get()->ShowHeader(uimenu_);

    // Test if access menu exists and remove it
//    UIElement* accessMenu = GameStatics::ui_->GetRoot()->GetChild(String("accessmenu"), true);
//    if (accessMenu)
//    {
//        UIElement* parent = accessMenu->GetParent();
//        URHO3D_LOGERRORF("MainMenu() - CreateUI ... accessmenu exists on parent=%s !", parent->GetName().CString());
//        parent->Remove();
//    }

    Game::Get()->CreateAccessMenu(uimenu_);

	// Load XML file containing MainMenu UI Layout
//	SharedPtr<UIElement> w = GameStatics::ui_->LoadLayout(GetSubsystem<ResourceCache>()->GetResource<XMLFile>("UI/mainui.xml"));
}

void MenuState::RemoveUI()
{
	if (GameStatics::ui_)
        GameStatics::ui_->SetFocusElement(0);

    // Remove Delayed for Go Out Animation
    TimerRemover::Get()->Start(uimenu_, SWITCHSCREENTIME + 0.05f);
}

void MenuState::BeginNewLevel(GameLevelMode mode, unsigned seed)
{
    URHO3D_LOGINFO("MenuState() - ---------------------------------------");
    URHO3D_LOGINFOF("MenuState() - BeginNewLevel : %s              -", levelModeNames[mode]);
    URHO3D_LOGINFO("MenuState() - ---------------------------------------");

    GameStatics::ResetGameStates();

//    if (seed)
//    {
//        PlayState* playstate = (PlayState*)stateManager_->GetState("Play");
//        if (playstate)
//            playstate->SetSeed(seed);
//    }

#ifdef ACTIVE_CINEMATICS
    if (!CinematicState::SetCinematic(CINEMATICSELECTIONMODE_INTRO_OUTRO, 0, 0, "LevelMap"))
        stateManager_->PushToStack("LevelMap");
#else
    stateManager_->PushToStack("LevelMap");
#endif
}

void MenuState::GoLevel(int level)
{
    URHO3D_LOGINFO("MenuState() - ---------------------------------------");
    URHO3D_LOGINFOF("MenuState() - GoLevel : %d             -", level);
    URHO3D_LOGINFO("MenuState() - ---------------------------------------");

    GameStatics::SetLevel(level);
    GameStatics::SetConsoleVisible(false);
    GameStatics::SetMouseVisible(false, false);

    stateManager_->PushToStack("Play");
}

bool MenuState::IsNetworkReady() const
{
    return true;
}

void MenuState::Quit()
{
#if defined(__ANDROID__) || defined(IOS) || defined(TVOS)
    return;
#endif

    if (!GameStatics::ui_)
    {
        GameStatics::Exit();
		return;
    }

    SharedPtr<MessageBox> messageBox = SharedPtr<MessageBox>(GameHelpers::AddMessageBox("gamequit", " ", false, "yes", "no", this, URHO3D_HANDLER(MenuState, HandleQuitMessageAck)));
	messageBox->AddRef();

//	Localization* l10n = GetSubsystem<Localization>();
//	SharedPtr<Urho3D::MessageBox> messageBox(new Urho3D::MessageBox(context_, " ", l10n->Get("gamequit")));
//
//	if (messageBox->GetWindow() != NULL)
//	{
//        UnsubscribeToMenuEvents();
//
//		Button* noButton = (Button*)messageBox->GetWindow()->GetChild("CancelButton", true);
//		((Text*)noButton->GetChild("Text", false))->SetText(l10n->Get("no"));
//		noButton->SetVisible(true);
//		noButton->SetFocus(true);
//		Button* yesButton = (Button*)messageBox->GetWindow()->GetChild("OkButton", true);
//		((Text*)yesButton->GetChild("Text", false))->SetText(l10n->Get("yes"));
//
//		SubscribeToEvent(messageBox, E_MESSAGEACK, URHO3D_HANDLER(MenuState, HandleQuitMessageAck));
//	}
//
//	messageBox->AddRef();
}

void MenuState::UpdateAnimButtons()
{
    if (!titlescene_)
        return;

    Node* mainscene = titlescene_->GetChild("MainScene");
    if (!mainscene)
        return;

    Node* node = mainscene->GetChild("ButtonStory");
    if (node)
    {
        node->GetComponent<AnimatedSprite2D>()->SetAnimation(menustate_ == 1 ? "on" : "off");
        node = node->GetChild(0U);
        node->SetScale2D(menustate_ == 1 ? Vector2(1.2f,1.2f) : Vector2::ONE);
        node->GetComponent<Text2D>()->SetColor(menustate_ == 1 ? Color::WHITE : Color::GRAY);
    }
    node = mainscene->GetChild("ButtonVersus");
    if (node)
    {
        node->GetComponent<AnimatedSprite2D>()->SetAnimation(menustate_ == 2 ? "on" : "off");
        node = node->GetChild(0U);
        node->SetScale2D(menustate_ == 2 ? Vector2(1.2f,1.2f) : Vector2::ONE);
        node->GetComponent<Text2D>()->SetColor(menustate_ == 2 ? Color::WHITE : Color::GRAY);
    }
}

void MenuState::SetMenuColliderPositions()
{
    if (!titlescene_)
        return;

    Node* mainscene = titlescene_->GetChild("MainScene");
    if (!mainscene)
        return;

    Node* node = mainscene->GetChild("ButtonStory");
    if (node)
    {
        AnimatedSprite2D* startbutton = node->GetComponent<AnimatedSprite2D>();
        startbutton->SetEnabled(false);  // need to update to get boundingbox
        startbutton->SetEnabled(true);
        GameHelpers::OrthoWorldToScreen(startRect_, startbutton->GetWorldBoundingBox());

//        URHO3D_LOGINFOF("MainMenu() - SetMenuColliderPositions : startrect=%s bbox=%s !",
//                        startRect_.ToString().CString(), startbutton->GetWorldBoundingBox().ToString().CString());
        if (GameStatics::gameConfig_.debugUI_)
        {
            UIElement* uielt = GameStatics::ui_->GetRoot()->GetChild("ButtonStartUIDebug", false);
            if (!uielt)
                uielt = GameStatics::ui_->GetRoot()->CreateChild<UIElement>("ButtonStartUIDebug");
            uielt->SetSize(IntVector2((float)startRect_.Size().x_ / GameStatics::uiScale_, (float)startRect_.Size().y_ / GameStatics::uiScale_));
            uielt->SetPosition((float)startRect_.left_ / GameStatics::uiScale_, (float)startRect_.top_ / GameStatics::uiScale_);
//            URHO3D_LOGINFOF("MainMenu() - SetMenuColliderPositions : startrect=%s uielt=%s %s visible=%s!",
//                            startRect_.ToString().CString(), uielt->GetPosition().ToString().CString(), uielt->GetSize().ToString().CString(), uielt->IsVisible()?"true":"false");
        }
    }

    node = mainscene->GetChild("ButtonVersus");
    if (node)
    {
        AnimatedSprite2D* optionsbutton = node->GetComponent<AnimatedSprite2D>();
        optionsbutton->SetEnabled(false);
        optionsbutton->SetEnabled(true);
        GameHelpers::OrthoWorldToScreen(optionsRect_, optionsbutton->GetWorldBoundingBox());

//        URHO3D_LOGINFOF("MainMenu() - SetMenuColliderPositions : optionsrect=%s bbox=%s !",
//                        optionsRect_.ToString().CString(), optionsbutton->GetWorldBoundingBox().ToString().CString());
        if (GameStatics::gameConfig_.debugUI_)
        {
            UIElement* uielt = GameStatics::ui_->GetRoot()->GetChild("ButtonOptionsUIDebug", false);
            if (!uielt)
                uielt = GameStatics::ui_->GetRoot()->CreateChild<UIElement>("ButtonOptionsUIDebug");
            uielt->SetSize(IntVector2((float)optionsRect_.Size().x_ / GameStatics::uiScale_, (float)optionsRect_.Size().y_ / GameStatics::uiScale_));
            uielt->SetPosition((float)optionsRect_.left_ / GameStatics::uiScale_, (float)optionsRect_.top_ / GameStatics::uiScale_);
        }
    }

}


void MenuState::SubscribeToMenuEvents()
{
    SubscribeToEvent(E_KEYUP, URHO3D_HANDLER(MenuState, HandleMenu));
    SubscribeToEvent(E_JOYSTICKBUTTONDOWN, URHO3D_HANDLER(MenuState, HandleMenu));
#ifndef __ANDROID__
    SubscribeToEvent(E_JOYSTICKAXISMOVE, URHO3D_HANDLER(MenuState, HandleMenu));
    SubscribeToEvent(E_JOYSTICKHATMOVE, URHO3D_HANDLER(MenuState, HandleMenu));
#endif
    SubscribeToEvent(E_TOUCHEND, URHO3D_HANDLER(MenuState, HandleMenu));
    SubscribeToEvent(E_TOUCHBEGIN, URHO3D_HANDLER(MenuState, HandleMenu));
    SubscribeToEvent(E_TOUCHMOVE, URHO3D_HANDLER(MenuState, HandleMenu));

    SubscribeToEvent(E_MOUSEMOVE, URHO3D_HANDLER(MenuState, HandleMenu));
    SubscribeToEvent(E_MOUSEBUTTONDOWN, URHO3D_HANDLER(MenuState, HandleMenu));
}

void MenuState::UnsubscribeToMenuEvents()
{
    UnsubscribeFromEvent(E_KEYUP);
    UnsubscribeFromEvent(E_JOYSTICKBUTTONDOWN);
#ifndef __ANDROID__
    UnsubscribeFromEvent(E_JOYSTICKAXISMOVE);
    UnsubscribeFromEvent(E_JOYSTICKHATMOVE);
#endif
    UnsubscribeFromEvent(E_TOUCHMOVE);
    UnsubscribeFromEvent(E_TOUCHEND);
    UnsubscribeFromEvent(E_TOUCHBEGIN);

    UnsubscribeFromEvent(E_MOUSEMOVE);
    UnsubscribeFromEvent(E_MOUSEBUTTONDOWN);
}

void MenuState::Launch(int selection)
{
    // for test
//    GoLevel(42);

    if (!selection)
        return;

    URHO3D_LOGINFOF("MenuState() - Launch %d", selection);

    if (uimenu_)
        Game::Get()->HideHeader(uimenu_);

    switch (selection)
    {
        // Story
    case 1:
        GameStatics::gameMode_ = 0;
        BeginNewLevel(LVL_NEW);
        break;

        // Versus Mode
    case 2:
        GameStatics::gameMode_ = 1;
        BeginNewLevel(LVL_NEW);
        break;
    }

    lastmenustate_ = menustate_ = -1;
}

void MenuState::HandleMenu(StringHash eventType, VariantMap& eventData)
{
    // touchEnabled KEY_SELECT Settings
    if (GetSubsystem<Input>()->GetKeyPress(KEY_SELECT) && GameStatics::gameConfig_.touchEnabled_)
    {
        if (GameStatics::gameConfig_.screenJoysticksettingsID_ == -1)
        {
            ResourceCache* cache = GetSubsystem<ResourceCache>();
            GameStatics::gameConfig_.screenJoysticksettingsID_ = GetSubsystem<Input>()->AddScreenJoystick(cache->GetResource<XMLFile>("UI/ScreenJoystickSettings2.xml"), cache->GetResource<XMLFile>("UI/DefaultStyle.xml"));
        }

        if (GameStatics::gameConfig_.screenJoysticksettingsID_ != -1)
            GetSubsystem<Input>()->SetScreenJoystickVisible(GameStatics::gameConfig_.screenJoysticksettingsID_, !GetSubsystem<Input>()->IsScreenJoystickVisible(GameStatics::gameConfig_.screenJoysticksettingsID_));

        return;
    }

    if (GameStatics::ui_->GetFocusElement())
        return;

    bool launch = false;
    int move = 0;

    /// Set Mouse Mouvements in Menu Items

    if (eventType == E_MOUSEMOVE || eventType == E_TOUCHEND || eventType == E_TOUCHMOVE || eventType == E_TOUCHBEGIN)
    {
        IntVector2 position;

        if (eventType == E_MOUSEMOVE)
        {
            GameHelpers::GetInputPosition(position.x_, position.y_);
//            URHO3D_LOGINFOF("MainMenu() - HandleMenu : mousemove=%s !", position.ToString().CString());
        }
        else if (eventType == E_TOUCHEND)
        {
            position.x_ = eventData[TouchEnd::P_X].GetInt();
            position.y_ = eventData[TouchEnd::P_Y].GetInt();
        }
        else if (eventType == E_TOUCHMOVE)
        {
            position.x_ = eventData[TouchMove::P_X].GetInt();
            position.y_ = eventData[TouchMove::P_Y].GetInt();
        }
        else if (eventType == E_TOUCHBEGIN)
        {
            position.x_ = eventData[TouchBegin::P_X].GetInt();
            position.y_ = eventData[TouchBegin::P_Y].GetInt();
//            URHO3D_LOGINFOF("MainMenu() - HandleMenu : E_TOUCHBEGIN=%s !", position.ToString().CString());
        }

        if (startRect_.IsInside(position) == INSIDE)
        {
            if (menustate_ != 1)
            {
                menustate_ = 1;
                move = 1;
            }
            launch = (lastmenustate_ == menustate_ && eventType == E_TOUCHEND);// && eventData[TouchBegin::P_PRESSURE].GetFloat() > 0.7f);
            if (eventType == E_TOUCHBEGIN) lastmenustate_ = menustate_;
        }
        else if (optionsRect_.IsInside(position) == INSIDE)
        {
            if (menustate_ != 2)
            {
                menustate_ = 2;
                move = 1;
            }
            launch = (eventType == E_TOUCHEND);// && eventData[TouchBegin::P_PRESSURE].GetFloat() > 0.7f);
            if (eventType == E_TOUCHBEGIN) lastmenustate_ = menustate_;
        }
        /*
        else if (cinematicRect_.IsInside(position) == INSIDE)
        {
            if (menustate_ != 3)
            {
                menustate_ = 3;
                move = 1;
            }
            launch = (eventType == E_TOUCHEND);// && eventData[TouchBegin::P_PRESSURE].GetFloat() > 0.7f);
            if (eventType == E_TOUCHBEGIN) lastmenustate_ = menustate_;
        }
        */
        else if (menustate_ != 0 && (eventType == E_MOUSEMOVE || eventType == E_TOUCHMOVE))
        {
            menustate_ = 0;
            move = 1;
            lastmenustate_ = menustate_;
        }
    }

    /// Set KeyBoard/JoyPad Mouvements in Menu Items

    else if (eventType == E_KEYUP || eventType == E_JOYSTICKAXISMOVE || eventType == E_JOYSTICKHATMOVE)
    {
        if (eventType == E_KEYUP)
        {
            URHO3D_LOGINFOF("MainMenu() - HandleMenu : keyup test hasfocus=%s !", GetSubsystem<Input>()->HasFocus() ? "true" : "false");

            int key = eventData[KeyUp::P_KEY].GetInt();
            if (key == KEY_UP || key == KEY_DOWN)
            {
                move = (key == KEY_UP) ? -1 : 1;
                SetMenuColliderPositions();
            }

            launch = (key == KEY_SPACE || key == KEY_RETURN);
        }

        else if (eventType == E_JOYSTICKAXISMOVE)
        {
            float motion = eventData[JoystickAxisMove::P_POSITION].GetFloat();
            int joyid = eventData[JoystickAxisMove::P_JOYSTICKID].GetInt();
            int axis = eventData[JoystickAxisMove::P_AXIS].GetInt();

            JoystickState* joystick = GameStatics::input_->GetJoystick(joyid);
            if (!joystick)
                return;

            move = abs(motion) >= 0.9f ? (motion < 0.f ? -1 : 1) : 0;
        }

        else if (eventType == E_JOYSTICKHATMOVE)
        {
            float motion = eventData[JoystickHatMove::P_POSITION].GetFloat();

            move = motion != 0.f ? (motion < 3.f ? -1 : 1) : 0;
        }

        if (move != 0)
        {
            menustate_ = menustate_ + move;

            if (menustate_ <= 0)
                menustate_ = NumSelection;
            else if (menustate_ > NumSelection)
                menustate_ = 1;
        }
    }

    /// Set Item Selection

    else if (eventType == E_JOYSTICKBUTTONDOWN)
    {
        int joyid = eventData[JoystickButtonDown::P_JOYSTICKID].GetInt();
        JoystickState* joystick = GameStatics::input_->GetJoystick(joyid);
        if (!joystick)
            return;

        launch = eventData[JoystickButtonDown::P_BUTTON] == SDL_CONTROLLER_BUTTON_A;  // PS4controller = X
    }

    else if (eventType == E_MOUSEBUTTONDOWN)
        launch = (eventData[MouseButtonDown::P_BUTTON].GetInt() == MOUSEB_LEFT);

    /// Animate Items

    if (move)
        UpdateAnimButtons();

    /// Launch Selected Item

    if (launch)
        Launch(menustate_);
}

void MenuState::HandleKeyDown(StringHash eventType, VariantMap& eventData)
{
	using namespace KeyDown;

	switch (eventData[P_KEY].GetInt())
    {
    case KEY_ESCAPE:
        Quit();
        break;
    case KEY_G:
        drawDebug_ = !drawDebug_;
        URHO3D_LOGINFOF("MenuState() - HandleKeyDown : KEY_G : Debug=%s", drawDebug_ ? "ON":"OFF");
        if (drawDebug_) SubscribeToEvent(E_POSTRENDERUPDATE, URHO3D_HANDLER(MenuState, OnPostRenderUpdate));
        else UnsubscribeFromEvent(E_POSTRENDERUPDATE);
        break;
    case KEY_PAGEUP :
        if (GameStatics::camera_)
        {
            GameStatics::camera_->SetZoom(GameStatics::camera_->GetZoom() * 1.01f);
            SetMenuColliderPositions();
        }
        break;
    case KEY_PAGEDOWN :
        if (GameStatics::camera_)
        {
            GameStatics::camera_->SetZoom(GameStatics::camera_->GetZoom() * 0.99f);
            SetMenuColliderPositions();
        }
        break;
    }
}

void MenuState::HandleScreenResized(StringHash eventType, VariantMap& eventData)
{
    if (eventType == GAME_SCREENRESIZED)
    {
        // need a delay to set the button collider positions. (don't know why)
        DelayInformer::Get(this, 0.1f, GAME_SCREENDELAYED);
        SubscribeToEvent(GAME_SCREENDELAYED, URHO3D_HANDLER(MenuState, HandleScreenResized));
    }
    else if (eventType == GAME_SCREENDELAYED)
    {
        UnsubscribeFromEvent(GAME_SCREENDELAYED);
        SetMenuColliderPositions();
    }

    UpdateAnimButtons();

    if (uimenu_)
    {
        const int width  = GameStatics::ui_->GetRoot()->GetSize().x_;
        const int height = GameStatics::ui_->GetRoot()->GetSize().y_;
        uimenu_->SetSize(width, height);
        uimenu_->SetPosition(width/2, height/2);

        URHO3D_LOGINFOF("MenuState() - HandleScreenResized : ui size=%d %d", width, height);
    }

//    GameHelpers::SetAdjustedToScreen(titlescene_->GetChild("MainScene")->GetChild("MenuFrame"), 1.25f, 1.f, false);
    GameHelpers::SetAdjustedToScreen(titlescene_->GetChild("MainScene")->GetChild("MenuFrame"), 1.25f, 1.25f, false);
}

void MenuState::HandleQuitMessageAck(StringHash eventType, VariantMap& eventData)
{
	using namespace MessageACK;

	if (eventData[P_OK].GetBool())
    {
        GameStatics::Exit();
		return;
    }

//    // Initialize controls
//    GetSubsystem<Input>()->ResetStates();

    SubscribeToMenuEvents();
}

void MenuState::OnAccessMenuOpenFrame(bool state)
{
    if (state)
        UnsubscribeToMenuEvents();
    else
        SubscribeToMenuEvents();
}

void MenuState::OnPostRenderUpdate(StringHash eventType, VariantMap& eventData)
{
    if (GameStatics::rootScene_)
    {
        DebugRenderer* debug = GameStatics::rootScene_->GetComponent<DebugRenderer>();

//        Octree* tree = GameStatics::rootScene_->GetComponent<Octree>();
//        tree->DrawDebugGeometry(true);
//        if (GameStatics::gameConfig_.debugLights_)
//        {
//            PODVector<Light*> lights;
//            GameStatics::rootScene_->GetComponents<Light>(lights,true);
////            URHO3D_LOGINFOF("nb Light=%d",lights.Size());
//            for (unsigned i = 0; i < lights.Size(); ++i)
//                lights[i]->DrawDebugGeometry(debug,true);
//        }
        if (GameStatics::gameConfig_.debugAnimatedSprite2D)
        {
            PODVector<AnimatedSprite2D*> drawables;
            GameStatics::rootScene_->GetComponents<AnimatedSprite2D>(drawables, true);
//            URHO3D_LOGINFOF("nb AnimatedSprite2D=%d", drawables.Size());
            for (unsigned i = 0; i < drawables.Size(); ++i)
                drawables[i]->DrawDebugGeometry(debug, true);
        }
        {
            PODVector<StaticSprite2D*> drawables;
            GameStatics::rootScene_->GetComponents<StaticSprite2D>(drawables, true);
            for (unsigned i = 0; i < drawables.Size(); ++i)
                drawables[i]->DrawDebugGeometry(debug, true);
        }
//        if (GameStatics::gameConfig_.physics2DEnabled_ && GameStatics::gameConfig_.debugPhysics_)
//        {
//            PhysicsWorld2D* physicsWorld2D = GameStatics::rootScene_->GetComponent<PhysicsWorld2D>();
//            if (physicsWorld2D) physicsWorld2D->DrawDebugGeometry();
//        }
    }

    if (GameStatics::gameConfig_.debugUI_)
    {
        GameStatics::ui_->DebugDraw(GameStatics::ui_->GetRoot());

//        PODVector<UIElement*> children;
//        GameStatics::ui_->GetRoot()->GetChildren(children, true);
//        for (PODVector<UIElement*>::ConstIterator it=children.Begin(); it!=children.End(); ++it)
//        {
//            UIElement* element = *it;
//            if (element->IsInternal())
//                continue;
//            if (element->HasTag("Console"))
//                continue;
//
//            GameStatics::ui_->DebugDraw(element);
//        }

        UIElement* uielt = GameStatics::ui_->GetRoot()->GetChild("ButtonStartUIDebug", false);
        if (uielt)
            GameStatics::ui_->DebugDraw(uielt);
        uielt = GameStatics::ui_->GetRoot()->GetChild("ButtonOptionsUIDebug", false);
        if (uielt)
            GameStatics::ui_->DebugDraw(uielt);
    }
}
