#include <Urho3D/Urho3D.h>

#include <Urho3D/Core/Object.h>
#include <Urho3D/Core/CoreEvents.h>

#include <Urho3D/Graphics/Graphics.h>
//#include <Urho3D/Graphics/GraphicsEvents.h>
#include <Urho3D/Graphics/Renderer.h>
#include <Urho3D/Graphics/DebugRenderer.h>
#include <Urho3D/Graphics/Viewport.h>
#include <Urho3D/Graphics/Camera.h>
#include <Urho3D/Graphics/Texture2D.h>

#include <Urho3D/Engine/Engine.h>
#include <Urho3D/IO/Log.h>

#include <Urho3D/Resource/Localization.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Resource/XMLFile.h>

#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Scene/SceneEvents.h>

#include <Urho3D/Urho2D/Sprite2D.h>
#include <Urho3D/Urho2D/StaticSprite2D.h>

#include <Urho3D/Input/Input.h>

#include <Urho3D/UI/UI.h>
#include <Urho3D/UI/UIEvents.h>
#include <Urho3D/UI/Button.h>
#include <Urho3D/UI/DropDownList.h>
#include <Urho3D/UI/Text.h>
#include <Urho3D/UI/MessageBox.h>

#include "Game.h"
#include "GameAttributes.h"
#include "GameHelpers.h"
#include "GameOptions.h"
#include "GameStatics.h"
#include "GameEvents.h"

#include "InteractiveFrame.h"

#include "DelayInformer.h"
#include "TimerRemover.h"

#include "MAN_Matches.h"

#include "sOptions.h"

//const String OPTIONS_XML("UI/Options1.xml);
//const String SHOP_XML("UI/Shop.xml");
const String OPTIONS_XML("UI/Options2.xml");
const String SHOP_XML("UI/ShopNoMoney.xml");

const String SHOP_ROOTUI("shopui");
const String OPTION_ROOTUI("optionrootui");
const String OPTION_GAMEDIFFICULTIES("gamedifficulties");
const String OPTION_GRAPHICEFFECTS("graphiceffects");
const String OPTION_MUSIC("music");
const String OPTION_SOUNDS("sounds");
const String OPTION_TUTORIAL("tutorial");
const String OPTION_LANGUAGE("gamelanguage");
const String OPTION_GAMERESET("gamereset");
const String OPTION_GAMEVERSION("gameversion");

extern int UISIZE[NUMUIELEMENTSIZE];
extern bool drawDebug_;

WeakPtr<UIElement> uioptions_;
WeakPtr<UIElement> uioptionsframe_;
WeakPtr<UIElement> uishopframe_;
WeakPtr<UIElement> closebutton_;
SharedPtr<MessageBox> MessageBox_;

OptionState::OptionState(Context* context) :
    GameState(context, "Options"),
    framed_(false)
{
//    URHO3D_LOGINFO("OptionState()");
}

OptionState::~OptionState()
{
    URHO3D_LOGINFO("~OptionState()");
}


bool OptionState::Initialize()
{
	return GameState::Initialize();
}

void OptionState::Begin()
{
    if (IsBegun())
        return;

    framed_ = false;

    GameStatics::screenSceneRect_ = IntRect::ZERO;

    URHO3D_LOGINFO("OptionState() - ----------------------------------------");
    URHO3D_LOGINFO("OptionState() - Begin ...                             -");
    URHO3D_LOGINFO("OptionState() - ----------------------------------------");

    // Set Scene
    if (!CreateScene())
    {
        URHO3D_LOGINFO("OptionState() - ----------------------------------------");
        URHO3D_LOGINFO("OptionState() - Begin ... Scene NOK !                     -");
        URHO3D_LOGINFO("OptionState() - ----------------------------------------");

        ReturnMenu();
        return;
    }

    // Reset Focus, Keep Update restore the focus
    UI* ui = GetSubsystem<UI>();
    if (ui)
        ui->SetFocusElement(0);

    // Set UI
    if (!CreateParameterUI())
    {
        URHO3D_LOGINFO("OptionState() - ----------------------------------------");
        URHO3D_LOGINFO("OptionState() - Begin ... UI NOK !                     -");
        URHO3D_LOGINFO("OptionState() - ----------------------------------------");

        ReturnMenu();
        return;
    }

    Game::Get()->CreateMenuButton(uioptions_);

    ReadOptions();

//    // Reset controls
//    GetSubsystem<Input>()->ResetStates();

    GameStatics::UpdateViews();

    GameStatics::SetMouseVisible(true, false);

    ResizeUI();

    SubscribeToEvents();

    // Call base class implementation
	GameState::Begin();

    GameHelpers::SetMoveAnimation(GameStatics::rootScene_->GetChild("Options"), Vector3(10.f * stateManager_->GetStackMove(), 0.f, 0.f), Vector3::ZERO, 0.f, SWITCHSCREENTIME);
    GameHelpers::SetMoveAnimationUI(uioptions_, IntVector2(-uioptions_->GetSize().x_, 0), IntVector2::ZERO, 0.f, SWITCHSCREENTIME);

    URHO3D_LOGINFO("OptionState() - ----------------------------------------");
    URHO3D_LOGINFO("OptionState() - Begin ... OK !                      -");
    URHO3D_LOGINFO("OptionState() - ----------------------------------------");
}

void OptionState::End()
{
    if (!IsBegun())
        return;

    URHO3D_LOGINFO("OptionState() - ----------------------------------------");
    URHO3D_LOGINFO("OptionState() - End ...                               -");
    URHO3D_LOGINFO("OptionState() - ----------------------------------------");

	UnsubscribeToEvents();

//    // Reset controls
//    GetSubsystem<Input>()->ResetStates();

    GameHelpers::SetMoveAnimation(GameStatics::rootScene_->GetChild("Options"), Vector3::ZERO, Vector3(-10.f * stateManager_->GetStackMove(), 0.f, 0.f), 0.f, SWITCHSCREENTIME);
    GameHelpers::SetMoveAnimationUI(uioptions_, IntVector2::ZERO, IntVector2(-uioptions_->GetSize().x_, 0), 0.f, SWITCHSCREENTIME);
    UIElement* menubutton = Game::Get()->GetAccessMenu();
    if (menubutton)
        menubutton->SetVisible(false);

    RemoveUI(FREEMEMORY);

	// Call base class implementation
	GameState::End();

    URHO3D_LOGINFO("OptionState() - ----------------------------------------");
    URHO3D_LOGINFO("OptionState() - End ... OK !                           -");
    URHO3D_LOGINFO("OptionState() - ----------------------------------------");
}

void OptionState::ReturnMenu()
{
    URHO3D_LOGINFO("OptionState() - ReturnMenu !");

    WriteOptions();

    stateManager_->PopStack();
}

bool OptionState::CreateScene()
{
    URHO3D_LOGINFO("OptionState() - CreateScene ... ");

    Node* optionscene = GameStatics::rootScene_->GetChild("Options", LOCAL);
    if (!optionscene)
        optionscene = GameStatics::rootScene_->CreateChild("Options", LOCAL);

    // add starsky
    {
        Node* node = GameHelpers::AddStaticSprite2D(optionscene, LOCAL, "background", "Textures/Background/starsky2.webp", Vector3::ZERO, Vector3::ONE, 0);
        GameHelpers::SetAdjustedToScreen(node);
        GameHelpers::AddAnimation(node, "Alpha", 0.f, 1.f, SWITCHSCREENTIME);
    }

    GameStatics::cameraNode_->SetPosition(Vector3::ZERO);

    GameStatics::rootScene_->SetUpdateEnabled(true);

    // Debug
//    GameStatics::DumpNode(rootScene_);

    URHO3D_LOGINFO("OptionState() - CreateScene ... OK !");

    return true;
}

bool OptionState::CreateParameterUI()
{
    URHO3D_LOGINFO("OptionState() - CreateParameterUI ...");

    ResourceCache* cache = GetSubsystem<ResourceCache>();

    // Set up global UI style into the root UI element
    XMLFile* style = cache->GetResource<XMLFile>("UI/DefaultStyle.xml");
    GameStatics::ui_->GetRoot()->SetDefaultStyle(style);

    // Create Root
    uioptions_ = GameStatics::ui_->GetRoot()->GetChild(OPTION_ROOTUI);
    if (!uioptions_)
        uioptions_ = GameStatics::ui_->GetRoot()->CreateChild<UIElement>(OPTION_ROOTUI);

    // Load UI content
    if (!uioptionsframe_)
    {
        SharedPtr<UIElement> uioptionsframe = GameStatics::ui_->LoadLayout(GetSubsystem<ResourceCache>()->GetResource<XMLFile>(OPTIONS_XML));
//        GameHelpers::SetUIScale(uioptionsframe, GameStatics::uiScale_ * Vector2::ONE);

        uioptions_->AddChild(uioptionsframe);
        uioptionsframe_ = uioptionsframe->GetChild(0);
        uioptionsframe_->SetName(OPTION_ROOTUI);
    }

    Button* gamereset = uioptions_->GetChildStaticCast<Button>(OPTION_GAMERESET, true);
    if (gamereset)
        gamereset->GetParent()->SetVisible(true);

    URHO3D_LOGINFO("OptionState() - CreateParameterUI ... OK !");

    return true;
}

unsigned numShopTabs_ = 0;
Color shopFrontWindowColor_, shopBackWindowColor_;
StringHash purchaseItemCat_;
String purchaseItemType_;
unsigned purchaseItemCoins_;
unsigned purchaseItemQty_;

void OptionState::CreateShopUI()
{
    URHO3D_LOGINFO("OptionState() - CreateShopUI ...");

    ResourceCache* cache = GetSubsystem<ResourceCache>();

    // Set up global UI style into the root UI element
    XMLFile* style = cache->GetResource<XMLFile>("UI/DefaultStyle.xml");
    GameStatics::ui_->GetRoot()->SetDefaultStyle(style);

    // Create Root
    uioptions_ = GameStatics::ui_->GetRoot()->GetChild(OPTION_ROOTUI);
    if (!uioptions_)
        uioptions_ = GameStatics::ui_->GetRoot()->CreateChild<UIElement>(OPTION_ROOTUI);

    // Load UI Content
    if (!uishopframe_)
    {
        SharedPtr<UIElement> uishopframe = GameStatics::ui_->LoadLayout(GetSubsystem<ResourceCache>()->GetResource<XMLFile>(SHOP_XML));
//        GameHelpers::SetUIScale(uishopframe, GameStatics::uiScale_ * Vector2::ONE);

        uioptions_->AddChild(uishopframe);
        uishopframe_ = uishopframe->GetChild(0);
        uishopframe_->SetName(SHOP_ROOTUI);

        numShopTabs_ = uishopframe_->GetChild(String("TabbedWindow"))->GetChild(String("TabSwitches"))->GetNumChildren();

        // Set TabbedWindows Colors and switch to the first Tab
        SetShopWindowColors(Color(0.5f, 0.5f, 0.5f, 1.f), Color(0.3f, 0.3f, 0.3f, 1.f), 0.8f);
    }

    UpdateShopItems();

    URHO3D_LOGINFO("OptionState() - CreateShopUI ... OK !");
}

UIElement* OptionState::GetShop()
{
    if (!uishopframe_)
    {
        CreateShopUI();
        uishopframe_->SetVisible(false);
    }

    return uishopframe_;
}

void OptionState::RemoveUI(int state)
{
    UnsubscribeToEvents();

    if (uishopframe_)
        UnsubscribeShopEvents();

    TimerRemover::Get()->Start(uioptions_, SWITCHSCREENTIME + 0.05f, (RemoveState)state);
}

void OptionState::ResizeUI()
{
    if (uioptions_)
    {
        uioptions_->SetSize(GameStatics::ui_->GetRoot()->GetSize());

        if (uioptionsframe_)
        {
            uioptionsframe_->GetParent()->SetSize(uioptions_->GetSize().x_, uioptions_->GetSize().y_);
            uioptionsframe_->GetParent()->SetPivot(0, 0);
            uioptionsframe_->GetParent()->SetPosition(IntVector2::ZERO);
            uioptionsframe_->SetPivot(0.5f, 0.5f);
            uioptionsframe_->SetPosition(uioptions_->GetSize().x_/2, uioptions_->GetSize().y_/2);
        }

        if (uishopframe_)
        {
            uishopframe_->GetParent()->SetSize(uioptions_->GetSize().x_, uioptions_->GetSize().y_);
            uishopframe_->GetParent()->SetPivot(0, 0);
            uishopframe_->GetParent()->SetPosition(IntVector2::ZERO);
            uishopframe_->SetPivot(0.5f, 0.5f);
            uishopframe_->SetPosition(uioptions_->GetSize().x_/2, uioptions_->GetSize().y_/2);
        }
    }
}

void OptionState::SetShopWindowColors(const Color& color1, const Color& color2, float opacity)
{
    shopFrontWindowColor_ = color1;
    shopBackWindowColor_ = color2;

    uishopframe_->SetOpacity(opacity);

//    for (unsigned i=1; i <= numShopTabs_; i++)
//        uishopframe_->GetChild(String("TabbedWindow"))->GetChild(ToString("Tab%d", i))->GetChild(String("TabContainer"))->SetColor(shopFrontWindowColor_);

    SwitchShopTab(uishopframe_->GetChild(String("TabbedWindow"))->GetChild(String("TabSwitches"))->GetChild(0));
}

void OptionState::SwitchShopTab(UIElement* uielement)
{
    UIElement* parent = uielement->GetParent();

    unsigned tabid = ToUInt(uielement->GetName().Substring(String("Switch").Length()));

//    URHO3D_LOGINFOF("OptionState() - SwitchShopTab : Switch to tabid=%u !", tabid);

    UIElement* windowtab = parent->GetParent()->GetChild(ToString("Tab%u", tabid));

    // Hide Tab Containers
    for (unsigned i=1; i <= numShopTabs_; i++)
    {
        UIElement* window = parent->GetParent()->GetChild(ToString("Tab%u", i));
        if (window && window != windowtab)
        {
            window->SetEnabled(false);
            window->SetVisible(false);
        }

        UIElement* button = parent->GetChild(ToString("Switch%u", i));
        button->SetColor(shopBackWindowColor_);
        button->GetChild(0)->SetColor(shopBackWindowColor_);
    }

    // Show the clicked Tab
    if (windowtab)
    {
        windowtab->SetEnabled(true);
        windowtab->BringToFront();
        windowtab->SetVisible(true);

        purchaseItemCat_ = StringHash(windowtab->GetChild(String("TabContainer"))->GetChild(0)->GetName());
    }

    uielement->SetColor(shopFrontWindowColor_);
    uielement->GetChild(0)->SetColor(shopFrontWindowColor_);
}

void OptionState::UpdateShopItems()
{
    URHO3D_LOGINFO("OptionState() - UpdateShopItems ... !");

    // Update Coin Text
    Text* coins = static_cast<Text*>(uishopframe_->GetChild(String("Coins"), true));
    if (coins)
        coins->SetText(String(GameStatics::coins_));

    // Update Items Access
    UIElement* shoproot = uishopframe_->GetChild(String("TabbedWindow"));

    for (unsigned i=1; i <= numShopTabs_; i++)
    {
        UIElement* tabcontainer = shoproot->GetChild(ToString("Tab%d", i))->GetChild(String("TabContainer"))->GetChild(0);
        bool checkunlockpower = (StringHash(tabcontainer->GetName()) == COT::POWERS);
        const Vector<SharedPtr<UIElement> >& buttons = tabcontainer->GetChildren();
        for (unsigned j=0; j < buttons.Size(); j++)
        {
            UIElement* textvalue = buttons[j]->GetChild(String("value"));
            if (!textvalue)
                continue;

            String valuestr = static_cast<Text*>(textvalue)->GetText();

            if (checkunlockpower)
            {
                int indexincat = COT::IsInCategory(StringHash(buttons[j]->GetName()), COT::POWERS);
                bool unlocked = indexincat != -1 && GameStatics::playerState_->powers_[indexincat].enabled_ == 1;
                SetUnlockedShopItem(buttons[j], unlocked);

                if (!unlocked)
                    continue;
            }
//            URHO3D_LOGINFO("OptionState() - UpdateShopItems ... valuestr=" + valuestr);

            // item without value like "showads"
            if (valuestr.Empty())
                SetEnableShopItem(buttons[j], true);

            // can be buy with coins
            else if (valuestr[valuestr.Length()-1] == 'c')
                SetEnableShopItem(buttons[j], ToUInt(valuestr.Substring(0, valuestr.Length()-1)) <= GameStatics::coins_);
        }
    }
}

void OptionState::SetUnlockedShopItem(UIElement* itembutton, bool unlocked)
{
    const Color& color = unlocked ? Color::WHITE : Color::BLACK;

    itembutton->SetEnabled(unlocked);
    itembutton->SetColor(color);

    const Vector<SharedPtr<UIElement> >& children = itembutton->GetChildren();
    for (unsigned i=0; i < children.Size(); i++)
    {
        children[i]->SetColor(color);
        children[i]->SetVisible(unlocked);
    }
}

void OptionState::SetEnableShopItem(UIElement* itembutton, bool enabled)
{
    const Color& color = enabled ? Color::WHITE : Color::WHITE*0.4f;

    itembutton->SetEnabled(enabled);
    itembutton->SetColor(color);

    const Vector<SharedPtr<UIElement> >& children = itembutton->GetChildren();
    for (unsigned i=0; i < children.Size(); i++)
    {
        children[i]->SetColor(color);
        children[i]->SetVisible(true);
    }
}


void OptionState::SetCloseButton(int frame, bool state)
{
    if (frame == 0 && uioptionsframe_)
        closebutton_ = uioptionsframe_->GetChild(String("closebutton"));

    if (frame == 1 && uishopframe_)
        closebutton_ = uishopframe_->GetChild(String("closebutton"));

    if (closebutton_)
    {
        closebutton_->SetVisible(state);
        URHO3D_LOGINFOF("OptionState() - SetCloseButton ... close button is %s !", closebutton_->IsVisible() ? "visible" : "novisible");
    }
}

bool OptionState::IsParameterOpened() const
{
    return closebutton_ && closebutton_->GetParent() && closebutton_->GetParent() == uioptionsframe_;
}

bool OptionState::IsShopOpened() const
{
    return uishopframe_->IsVisible();
}

void OptionState::OpenParameterFrame()
{
    if (!framed_)
    {
        stateManager_->GetActiveState()->OnAccessMenuOpenFrame(true);

        framed_ = true;

        CreateParameterUI();
        ResizeUI();

        SetCloseButton(0, true);

        Button* gamereset = uioptions_->GetChildStaticCast<Button>(OPTION_GAMERESET, true);
        if (gamereset)
            gamereset->GetParent()->SetVisible(false);

        ReadOptions();

        SubscribeToEvents();

        GameHelpers::SetMoveAnimationUI(uioptionsframe_, IntVector2(-uioptionsframe_->GetSize().x_, uioptions_->GetSize().y_/2),
                                        IntVector2(uioptions_->GetSize().x_/2, uioptions_->GetSize().y_/2), 0.f, SWITCHSCREENTIME);
    }
}

void OptionState::OpenShopFrame()
{
    if (!framed_)
    {
        stateManager_->GetActiveState()->OnAccessMenuOpenFrame(true);

        framed_ = true;

        CreateShopUI();
        ResizeUI();

        SetCloseButton(1, true);

        SubscribeToShopEvents();
        SubscribeToEvents();

        GameHelpers::SetMoveAnimationUI(uishopframe_, IntVector2(-uishopframe_->GetSize().x_, uioptions_->GetSize().y_/2),
                                        IntVector2(uioptions_->GetSize().x_/2, uioptions_->GetSize().y_/2), 0.f, SWITCHSCREENTIME);
    }
}

void OptionState::CloseFrame()
{
    if (framed_)
    {
        if (uioptionsframe_)
            WriteOptions();

        UnsubscribeToEvents();

        framed_ = false;

        if (uioptionsframe_)
        {
            SetCloseButton(0, false);
            GameHelpers::SetMoveAnimationUI(uioptionsframe_, IntVector2(uioptions_->GetSize().x_/2, uioptions_->GetSize().y_/2),
                        IntVector2(-uioptionsframe_->GetSize().x_, uioptions_->GetSize().y_/2), 0.f, SWITCHSCREENTIME);
            TimerRemover::Get()->Start(uioptionsframe_, SWITCHSCREENTIME + 0.05f, DISABLE);
        }

        if (uishopframe_)
        {
            SetCloseButton(1, false);
            GameHelpers::SetMoveAnimationUI(uishopframe_, IntVector2(uioptions_->GetSize().x_/2, uioptions_->GetSize().y_/2),
                        IntVector2(-uishopframe_->GetSize().x_, uioptions_->GetSize().y_/2),0.f, SWITCHSCREENTIME);

            UnsubscribeShopEvents();
            TimerRemover::Get()->Start(uishopframe_, SWITCHSCREENTIME + 0.05f, DISABLE);
        }

        stateManager_->GetActiveState()->OnAccessMenuOpenFrame(false);
    }
}

void OptionState::SwitchParameterFrame()
{
    if (uishopframe_)
        CloseFrame();

    if (!framed_)
        OpenParameterFrame();
    else
    {
        CloseFrame();
        RemoveUI(DISABLE);
    }
}

void OptionState::SwitchShopFrame()
{
    if (uioptionsframe_)
        CloseFrame();

    if (!framed_)
        OpenShopFrame();
    else
    {
        CloseFrame();
        RemoveUI(DISABLE);
    }
}


void OptionState::ReadOptions()
{
    // Game Difficulties
    DropDownList* gdlist = uioptions_->GetChildStaticCast<DropDownList>(OPTION_GAMEDIFFICULTIES, true);

    // Graphic Effects
    DropDownList* gelist = uioptions_->GetChildStaticCast<DropDownList>(OPTION_GRAPHICEFFECTS, true);

    // Music
    DropDownList* musiclist = uioptions_->GetChildStaticCast<DropDownList>(OPTION_MUSIC, true);
    if (musiclist)
        musiclist->SetSelection(GameStatics::playerState_->musicEnabled_ ? 0 : 1);

    // Sound Effects
    DropDownList* soundlist = uioptions_->GetChildStaticCast<DropDownList>(OPTION_SOUNDS, true);
    if (soundlist)
        soundlist->SetSelection(GameStatics::playerState_->soundEnabled_ ? 0 : 1);

    // Tutorial
    DropDownList* tutoriallist = uioptions_->GetChildStaticCast<DropDownList>(OPTION_TUTORIAL, true);
    if (tutoriallist)
        tutoriallist->SetSelection(GameStatics::playerState_->tutorialEnabled_ ? 0 : 1);

    // Language
    DropDownList* languagelist = uioptions_->GetChildStaticCast<DropDownList>(OPTION_LANGUAGE, true);
    if (languagelist)
    {
        languagelist->SetSelection(GameStatics::playerState_->language_);
        Localization* l10n = GetSubsystem<Localization>();
        l10n->SetLanguage(GameStatics::playerState_->language_);
    }

    Text* gameVersion = uioptions_->GetChildStaticCast<Text>(OPTION_GAMEVERSION, true);
    // Game version
    if (gameVersion)
    {
        String version;
        version.AppendWithFormat("rev.%d.%d", gameVersion_, gameVersionMinor_);
        gameVersion->SetText(version);
    }
}

void OptionState::WriteOptions()
{
    DropDownList* list;

    // Game Difficulties
    list = uioptions_->GetChildStaticCast<DropDownList>(OPTION_GAMEDIFFICULTIES, true);

    // Graphic Effects
    list = uioptions_->GetChildStaticCast<DropDownList>(OPTION_GRAPHICEFFECTS, true);

    // Music
    list = uioptions_->GetChildStaticCast<DropDownList>(OPTION_MUSIC, true);
    if (list) GameStatics::playerState_->musicEnabled_ = list->GetSelection() == 0 ? 1 : 0;

    // Sound Effects
    list = uioptions_->GetChildStaticCast<DropDownList>(OPTION_SOUNDS, true);
    if (list) GameStatics::playerState_->soundEnabled_ = list->GetSelection() == 0 ? 1 : 0;

    // Tutorial
    list = uioptions_->GetChildStaticCast<DropDownList>(OPTION_TUTORIAL, true);
    if (list) GameStatics::playerState_->tutorialEnabled_ = list->GetSelection() == 0 ? 1 : 0;

    // Language
    list = uioptions_->GetChildStaticCast<DropDownList>(OPTION_LANGUAGE, true);
    if (list) GameStatics::playerState_->language_ = list->GetSelection();
}


void OptionState::SubscribeToEvents()
{
    InteractiveFrame::SetAllowInputs(false);

    SubscribeToEvent(GAME_SCREENRESIZED, URHO3D_HANDLER(OptionState, HandleScreenResized));
    SubscribeToEvent(E_KEYDOWN, URHO3D_HANDLER(OptionState, HandleKeyDown));

    DropDownList* list = uioptions_->GetChildStaticCast<DropDownList>(OPTION_MUSIC, true);
    if (list)
        SubscribeToEvent(list, E_ITEMSELECTED, URHO3D_HANDLER(OptionState, HandleOptionChanged));

    list = uioptions_->GetChildStaticCast<DropDownList>(OPTION_SOUNDS, true);
    if (list)
        SubscribeToEvent(list, E_ITEMSELECTED, URHO3D_HANDLER(OptionState, HandleOptionChanged));

    list = uioptions_->GetChildStaticCast<DropDownList>(OPTION_TUTORIAL, true);
    if (list)
        SubscribeToEvent(list, E_ITEMSELECTED, URHO3D_HANDLER(OptionState, HandleOptionChanged));

    list = uioptions_->GetChildStaticCast<DropDownList>(OPTION_LANGUAGE, true);
    if (list)
        SubscribeToEvent(list, E_ITEMSELECTED, URHO3D_HANDLER(OptionState, HandleOptionChanged));

    Button* gamereset = uioptions_->GetChildStaticCast<Button>(OPTION_GAMERESET, true);
    if (gamereset)
        SubscribeToEvent(gamereset, E_RELEASED, URHO3D_HANDLER(OptionState, HandleGameReset));

    if (framed_)
    {
        if (closebutton_)
            SubscribeToEvent(closebutton_, E_RELEASED, URHO3D_HANDLER(OptionState, HandleCloseFrame));
    }
    else
    {
        UIElement* menubutton = uioptions_->GetChild(String("menubutton"));
        if (menubutton)
            SubscribeToEvent(menubutton, E_RELEASED, URHO3D_HANDLER(OptionState, HandleReturnMenu));
    }
}

void OptionState::UnsubscribeToEvents()
{
    InteractiveFrame::SetAllowInputs(true);

    UnsubscribeFromEvent(GAME_SCREENRESIZED);
    UnsubscribeFromEvent(E_KEYDOWN);

    DropDownList* list = uioptions_->GetChildStaticCast<DropDownList>(OPTION_MUSIC, true);
    if (list)
        UnsubscribeFromEvent(list, E_ITEMSELECTED);

    list = uioptions_->GetChildStaticCast<DropDownList>(OPTION_TUTORIAL, true);
    if (list)
        UnsubscribeFromEvent(list, E_ITEMSELECTED);

    list = uioptions_->GetChildStaticCast<DropDownList>(OPTION_LANGUAGE, true);
    if (list)
        UnsubscribeFromEvent(list, E_ITEMSELECTED);

    Button* gamereset = uioptions_->GetChildStaticCast<Button>(OPTION_GAMERESET, true);
    if (gamereset)
        UnsubscribeFromEvent(gamereset, E_RELEASED);

    if (framed_)
    {
        if (closebutton_)
            UnsubscribeFromEvent(closebutton_, E_RELEASED);
    }
    else
    {
        UIElement* menubutton = uioptions_->GetChild(String("menubutton"));
        if (menubutton)
            UnsubscribeFromEvent(menubutton, E_RELEASED);
    }
}

void OptionState::SubscribeToShopEvents()
{
    UIElement* shoproot = uishopframe_->GetChild(String("TabbedWindow"));

    SubscribeToEvent(GAME_COINUPDATED, URHO3D_HANDLER(OptionState, HandleCoinsUpdated));

    // Subscribe to Tab Switches
    const Vector<SharedPtr<UIElement> >& switches = shoproot->GetChild(String("TabSwitches"))->GetChildren();
    for (unsigned i=0; i < switches.Size(); i++)
    {
        if (switches[i])
            SubscribeToEvent(switches[i].Get(), E_RELEASED, URHO3D_HANDLER(OptionState, HandleShopSwitchTab));
    }

    // Subscribe to Tab Buttons
    for (unsigned i=1; i <= switches.Size(); i++)
    {
        UIElement* tabcontainer = shoproot->GetChild(ToString("Tab%d", i))->GetChild(String("TabContainer"))->GetChild(0);
        const Vector<SharedPtr<UIElement> >& buttons = tabcontainer->GetChildren();
        for (unsigned j=0; j < buttons.Size(); j++)
        {
            if (buttons[j])
                SubscribeToEvent(buttons[j].Get(), E_RELEASED, URHO3D_HANDLER(OptionState, HandleShopButtonClick));
        }
    }
}

void OptionState::UnsubscribeShopEvents()
{
    UIElement* shoproot = uishopframe_->GetChild(String("TabbedWindow"));

    UnsubscribeFromEvent(GAME_COINUPDATED);

    // Unsubscribe from Tab Switches
    const Vector<SharedPtr<UIElement> >& switches = shoproot->GetChild(String("TabSwitches"))->GetChildren();
    for (unsigned i=0; i < switches.Size(); i++)
    {
        if (switches[i])
            UnsubscribeFromEvent(switches[i].Get(), E_RELEASED);
    }

    // Unsubscribe to from Buttons
    for (unsigned i=1; i <= switches.Size(); i++)
    {
        UIElement* tabcontainer = shoproot->GetChild(ToString("Tab%d", i))->GetChild(String("TabContainer"))->GetChild(0);
        const Vector<SharedPtr<UIElement> >& buttons = tabcontainer->GetChildren();
        for (unsigned j=0; j < buttons.Size(); j++)
        {
            if (buttons[j])
                UnsubscribeFromEvent(buttons[j].Get(), E_RELEASED);
        }
    }
}


void OptionState::HandleShopSwitchTab(StringHash eventType, VariantMap& eventData)
{
    SwitchShopTab(static_cast<UIElement*>(eventData[Released::P_ELEMENT].GetPtr()));
}

void OptionState::HandleShopButtonClick(StringHash eventType, VariantMap& eventData)
{
    UIElement* uielement = static_cast<UIElement*>(eventData[Released::P_ELEMENT].GetPtr());

    URHO3D_LOGINFO("OptionState() - HandleShopButtonClick ... button clicked !");

    Localization* l10n = GetSubsystem<Localization>();
    Text* text;

    // get Type
    purchaseItemType_ = uielement->GetName();

    // Get Value
    purchaseItemCoins_ = 0;

    text = static_cast<Text*>(uielement->GetChild(String("value")));
    if (!text)
        return;

    String valuestr(text->GetText());

    if (valuestr.Empty())
    {
        // no value
        ;
    }
    else if (valuestr[valuestr.Length()-1] == 'c')
    {
        // value in shop currency (coins)
        purchaseItemCoins_ = ToUInt(valuestr.Substring(0, valuestr.Length()-1));
    }
    else
    {
        // value in currency (euros)

        /// TODO
    }

    // Get Quantity
    purchaseItemQty_ = 1;
    text = static_cast<Text*>(uielement->GetChild(String("qty")));
    if (!text)
        return;

    purchaseItemQty_ = ToUInt(text->GetText());

    if (!valuestr.Empty())
    {
        // Open Validation Dialogue
        String question = ToString("%s (%u)", !l10n->Get(purchaseItemType_).Empty() ? l10n->Get(purchaseItemType_).CString() : purchaseItemType_.CString(), purchaseItemQty_);
        MessageBox_ = GameHelpers::AddMessageBox("purchase", question, false, "yes", "no", this, URHO3D_HANDLER(OptionState, OnShopMessageAck), GetSubsystem<ResourceCache>()->GetResource<XMLFile>("UI/ShopMessageBox.xml"));
        MessageBox_->AddRef();
    }
    else
    {
        // no value : launch action directly
        /// TODO
        URHO3D_LOGINFO("LevelMapState() - HandleShopButtonClick : Show Ad !");
    #if defined(__ANDROID__)
        bool ok = Android_ShowAds();
    #endif
    }
}

void OptionState::OnShopMessageAck(StringHash eventType, VariantMap& eventData)
{
	using namespace MessageACK;

    UnsubscribeFromEvent(E_MESSAGEACK);
    MessageBox_.Reset();

	if (eventData[P_OK].GetBool())
    {
        GameStatics::coins_ = Max(0, GameStatics::coins_ - (int)purchaseItemCoins_);
        SendEvent(GAME_COINUPDATED);
        URHO3D_LOGINFOF("OptionState() - OnShopMessageAck : accept %s qty=%u value=%u wallet=%d",
                        purchaseItemType_.CString(), purchaseItemQty_, purchaseItemCoins_, GameStatics::coins_);

        GameStatics::AddBonus(Slot(purchaseItemCat_, StringHash(purchaseItemType_), purchaseItemQty_, true), this);

        UpdateShopItems();
    }
	else
    {
        URHO3D_LOGINFO("OptionState() - OnShopMessageAck : decline !");
    }
}

void OptionState::OnGameResetMessageAck(StringHash eventType, VariantMap& eventData)
{
	using namespace MessageACK;

    UnsubscribeFromEvent(E_MESSAGEACK);
    MessageBox_.Reset();

	if (eventData[P_OK].GetBool())
    {
        URHO3D_LOGINFOF("OptionState() - OnGameResetMessageAck : reset the game !");
        GameStatics::gameState_.Reset();
        GameStatics::gameState_.Save();

        ReadOptions();
    }
	else
    {
        URHO3D_LOGINFO("OptionState() - OnGameResetMessageAck : decline !");
    }
}

void OptionState::HandleGameReset(StringHash eventType, VariantMap& eventData)
{
    Button* gamereset = uioptions_->GetChildStaticCast<Button>(OPTION_GAMERESET, true);

    if (gamereset)
    {
        // Open Confirm Dialogue
        MessageBox_ = GameHelpers::AddMessageBox("reset", "gamereset", true, "yes", "no", this, URHO3D_HANDLER(OptionState, OnGameResetMessageAck), GetSubsystem<ResourceCache>()->GetResource<XMLFile>("UI/ShopMessageBox.xml"));
        MessageBox_->AddRef();
    }
}

void OptionState::HandleOptionChanged(StringHash eventType, VariantMap& eventData)
{
    UIElement* uielement = static_cast<UIElement*>(eventData[ItemSelected::P_ELEMENT].GetPtr());

    DropDownList* music = uioptions_->GetChildStaticCast<DropDownList>(OPTION_MUSIC, true);
    DropDownList* sounds = uioptions_->GetChildStaticCast<DropDownList>(OPTION_SOUNDS, true);
    DropDownList* tutorial = uioptions_->GetChildStaticCast<DropDownList>(OPTION_TUTORIAL, true);
    DropDownList* language = uioptions_->GetChildStaticCast<DropDownList>(OPTION_LANGUAGE, true);

    if (uielement == music)
    {
        bool enablemusic = music->GetSelection() == 0 ? 1 : 0;
        if (enablemusic != GameStatics::playerState_->musicEnabled_)
        {
            GameStatics::playerState_->musicEnabled_ = enablemusic;
            if (GameStatics::playerState_->musicEnabled_)
            {
                GameHelpers::SetMusic(MAINMUSIC, 1.f, GameStatics::currentMusic_, framed_);
                GameHelpers::SetMusicVolume(0.7f);
            }
            else
            {
                GameHelpers::StopMusic(MAINMUSIC);
                GameHelpers::SetMusicVolume(0.f);
            }
        }
    }
    else if (uielement == sounds)
    {
        GameStatics::playerState_->soundEnabled_ = sounds->GetSelection() == 0 ? 1 : 0;
        GameHelpers::SetSoundVolume(GameStatics::playerState_->soundEnabled_ ? 1.f : 0.f);
    }
    else if (uielement == tutorial)
    {
        GameStatics::playerState_->tutorialEnabled_ = tutorial->GetSelection() == 0 ? 1 : 0;

        if (GameStatics::playerState_->tutorialEnabled_)
            GameStatics::gameState_.ResetTutorialState();
    }
    else if (uielement == language)
    {
        GameStatics::playerState_->language_ = language->GetSelection();
        Localization* l10n = GetSubsystem<Localization>();
        l10n->SetLanguage(GameStatics::playerState_->language_);
    }
}

void OptionState::HandleCoinsUpdated(StringHash eventType, VariantMap& eventData)
{
    UpdateShopItems();
}

void OptionState::HandleReturnMenu(StringHash eventType, VariantMap& eventData)
{
    ReturnMenu();
}

void OptionState::HandleCloseFrame(StringHash eventType, VariantMap& eventData)
{
    CloseFrame();
}

void OptionState::HandleKeyDown(StringHash eventType, VariantMap& eventData)
{
    if (framed_)
        return;

    using namespace KeyDown;

    switch (eventData[P_KEY].GetInt())
    {
    case KEY_G:
        drawDebug_ = !drawDebug_;
        URHO3D_LOGINFOF("OptionState() - HandleKeyDown : KEY_G : Debug=%s", drawDebug_ ? "ON":"OFF");
        if (drawDebug_) SubscribeToEvent(E_POSTRENDERUPDATE, URHO3D_HANDLER(OptionState, OnPostRenderUpdate));
        else UnsubscribeFromEvent(E_POSTRENDERUPDATE);
        break;
    case KEY_PAGEUP :
        if (GameStatics::camera_)
            GameStatics::camera_->SetZoom(GameStatics::camera_->GetZoom() * 1.01f);
        break;
    case KEY_PAGEDOWN :
        if (GameStatics::camera_)
            GameStatics::camera_->SetZoom(GameStatics::camera_->GetZoom() * 0.99f);
        break;
    }

    // do escape here only (not with eventdata => prevent residual events from last state)
    if (GetSubsystem<Input>()->GetKeyPress(KEY_ESCAPE))
    {
        ReturnMenu();
    }
}

void OptionState::HandleScreenResized(StringHash eventType, VariantMap& eventData)
{
    Node* optionscene = GameStatics::rootScene_->GetChild("Options");
    if (optionscene)
        GameHelpers::SetAdjustedToScreen(optionscene->GetChild("background"), 1.25f, 1.25f, false);

    ResizeUI();
}

void OptionState::OnPostRenderUpdate(StringHash eventType, VariantMap& eventData)
{
    if (GameStatics::gameConfig_.debugUI_)
    {
//        GameStatics::ui_->DebugDraw(GameStatics::ui_->GetRoot());

//        PODVector<UIElement*> children;
//        GameStatics::ui_->GetRoot()->GetChildren(children, true);
//        for (PODVector<UIElement*>::ConstIterator it=children.Begin(); it!=children.End(); ++it)
//        {
//            UIElement* element = *it;
//            if (element->IsInternal())
//                continue;
//            if (element->HasTag("Console"))
//                continue;
            if (uioptions_)
                GameStatics::ui_->DebugDraw(uioptions_);
            if (uioptionsframe_)
                GameStatics::ui_->DebugDraw(uioptionsframe_);
            if (uioptionsframe_ && uioptionsframe_->GetParent())
                GameStatics::ui_->DebugDraw(uioptionsframe_->GetParent());
//        }
    }
}
