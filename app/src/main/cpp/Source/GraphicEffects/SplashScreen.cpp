#include <Urho3D/Urho3D.h>

#include <Urho3D/Engine/Engine.h>

#include <Urho3D/IO/Log.h>

#include <Urho3D/Graphics/Graphics.h>
#include <Urho3D/Graphics/Texture2D.h>

#include <Urho3D/Resource/ResourceCache.h>

#include <Urho3D/Input/Input.h>

#include <Urho3D/UI/Sprite.h>
#include <Urho3D/UI/UI.h>

#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Scene/ObjectAnimation.h>
#include <Urho3D/Scene/ValueAnimation.h>

#include "DelayInformer.h"

#include "GameEvents.h"
#include "GameStatics.h"
#include "GameHelpers.h"
#include "SplashScreen.h"



SplashScreen* SplashScreen::splashScreen_ = 0;

WeakPtr<UIElement> uisplashscreen_;
const float SPLASHSCREEN_KEEPSTATEDELAY = 1.f;
const float SPLASHSCREEN_MOVEDELAY = 0.5f;

enum
{
    SPLASHSCREEN_WAITING = 0,
    SPLASHSCREEN_CLOSING,
    SPLASHSCREEN_OPENING
};

SplashScreen::SplashScreen(Context* context, const StringHash& beginTrigger, const StringHash& finishTrigger, const char * fileName, float delay)
    : Object(context)
{
    URHO3D_LOGINFO("SplashScreen() - ---------------------------------------");
    URHO3D_LOGINFOF("SplashScreen() - %u : beginTrigger=%u finishTrigger=%u", this, beginTrigger.Value(), finishTrigger.Value());
    URHO3D_LOGINFO("SplashScreen() - ---------------------------------------");

    beginTrigger_  = beginTrigger;
    finishTrigger_ = finishTrigger;

    uisplashscreen_ = GameStatics::ui_->GetRoot()->CreateChild<UIElement>("splashscreenrootui");
    uisplashscreen_->SetSize(GameStatics::ui_->GetRoot()->GetSize().x_, GameStatics::ui_->GetRoot()->GetSize().y_);
    uisplashscreen_->SetPriority(M_MAX_INT-1);

    delay_ = delay;//Max(SPLASHSCREEN_KEEPSTATEDELAY+SPLASHSCREEN_MOVEDELAY, delay);
    texture = GetSubsystem<ResourceCache>()->GetResource<Texture2D>(fileName);

    // Create Sprites
    splashUI_bottom = uisplashscreen_->CreateChild<Sprite>();
    splashUI_bottom->SetTexture(texture);
    splashUI_bottom->SetOpacity(1.f);
    splashUI_bottom->SetBringToFront(true);

    splashUI_top = uisplashscreen_->CreateChild<Sprite>();
    splashUI_top->SetTexture(texture);;
    splashUI_top->SetOpacity(1.f);
    splashUI_top->SetRotation(180.f);
    splashUI_top->SetBringToFront(true);

    splashUI_bottom->SetVisible(false);
    splashUI_top->SetVisible(false);

    state_ = SPLASHSCREEN_WAITING;

    splashScreen_ = this;

    // Subscribe To Events
    SubscribeToEvents();
}

SplashScreen::~SplashScreen()
{
    if (uisplashscreen_)
        uisplashscreen_->Remove();

    if (splashScreen_ == this)
        splashScreen_ = 0;

    URHO3D_LOGINFO("~SplashScreen() - ---------------------------------------");
    URHO3D_LOGINFO("~SplashScreen() - End");
    URHO3D_LOGINFO("~SplashScreen() - ---------------------------------------");
}

void SplashScreen::SubscribeToEvents()
{
    // Subscribe To Events
    UnsubscribeFromAllEvents();

    SubscribeToEvent(beginTrigger_, URHO3D_HANDLER(SplashScreen, HandleCloseSplash));
    SubscribeToEvent(finishTrigger_, URHO3D_HANDLER(SplashScreen, HandleOpenSplash));
    SubscribeToEvent(SPLASHSCREEN_STOP, URHO3D_HANDLER(SplashScreen, HandleStop));

    SubscribeToEvent(this, SPLASHSCREEN_FINISHOPEN, URHO3D_HANDLER(SplashScreen, HandleVisibility));
}

void SplashScreen::SetOpenClosePositions()
{
    const float uiscale = GameStatics::uiScale_;
    const float width = GameStatics::ui_->GetRoot()->GetSize().x_;
    const float height = GameStatics::ui_->GetRoot()->GetSize().y_;
    const float twidth = splashUI_bottom->GetTexture()->GetWidth();
    const float theight = splashUI_bottom->GetTexture()->GetHeight();
    const float oversize = 1.2f;

    IntVector2 size(width, height);
    GameHelpers::ApplySizeRatio(twidth, theight, size);

    splashUI_bottom->SetSize(size.x_ * oversize, size.y_ * oversize);
    splashUI_bottom->SetHotSpot(0.5f * splashUI_bottom->GetSize().x_, splashUI_bottom->GetSize().y_);

    splashUI_top->SetSize(splashUI_bottom->GetSize());
    splashUI_top->SetHotSpot(0.5f * splashUI_top->GetSize().x_, splashUI_top->GetSize().y_);

    openposition_bottom  = Vector2(0.5f * width, height + splashUI_bottom->GetSize().y_);
    closeposition_bottom = Vector2(0.5f * width, height * 0.5f + splashUI_bottom->GetSize().y_ * 0.745f);

    openposition_top     = Vector2(0.5f * width, -splashUI_top->GetSize().y_);
    closeposition_top    = Vector2(0.5f * width, height * 0.5f - splashUI_top->GetSize().y_ * 0.745f);

    splashUI_bottom->SetPosition(closeposition_bottom);
    splashUI_top->SetPosition(closeposition_top);

    URHO3D_LOGINFOF("SplashScreen() - SetOpenClosePositions : %u - bottomelement (uirootsize=%f,%f ; textsize=%f,%f ; size=%s ; hotspot=%s ; position=%s), scale=%f",
                    this, width, height, twidth, theight, splashUI_bottom->GetSize().ToString().CString(),
                    splashUI_bottom->GetHotSpot().ToString().CString(),
                    splashUI_bottom->GetPosition().ToString().CString(), uiscale);
}

void SplashScreen::HandleCloseSplash(StringHash eventType, VariantMap& eventData)
{
    if (state_ == SPLASHSCREEN_CLOSING)
        return;

    state_ = SPLASHSCREEN_CLOSING;

    SetOpenClosePositions();

    keepDelay_ = SPLASHSCREEN_KEEPSTATEDELAY;
    if (eventData.Contains(SplashScreen_Events::KEEPSTATEDELAY))
        keepDelay_ = eventData[SplashScreen_Events::KEEPSTATEDELAY].GetFloat();

    float delay = Max(keepDelay_+SPLASHSCREEN_MOVEDELAY, delay_);

    // Reset controls
    GameStatics::input_->ResetStates();

    SendEvent(SPLASHSCREEN_STARTCLOSE);

    splashUI_bottom->SetVisible(true);
    splashUI_top->SetVisible(true);

    // Create close move
    {
        splashUI_bottom->SetPosition(openposition_bottom);
        SharedPtr<ObjectAnimation> objectAnimation(new ObjectAnimation(context_));
        SharedPtr<ValueAnimation> moveAnimation(new ValueAnimation(context_));
        moveAnimation->SetKeyFrame(0.f, openposition_bottom);
        moveAnimation->SetKeyFrame(keepDelay_, openposition_bottom);
        moveAnimation->SetKeyFrame(delay, closeposition_bottom);
        objectAnimation->AddAttributeAnimation("Position", moveAnimation, WM_ONCE);
        splashUI_bottom->SetObjectAnimation(objectAnimation);
    }

    {
        splashUI_top->SetPosition(openposition_top);
        SharedPtr<ObjectAnimation> objectAnimation(new ObjectAnimation(context_));
        SharedPtr<ValueAnimation> moveAnimation(new ValueAnimation(context_));
        moveAnimation->SetKeyFrame(0.f, openposition_top);
        moveAnimation->SetKeyFrame(keepDelay_, openposition_top);
        moveAnimation->SetKeyFrame(delay, closeposition_top);
        objectAnimation->AddAttributeAnimation("Position", moveAnimation, WM_ONCE);
        splashUI_top->SetObjectAnimation(objectAnimation);
    }

    splashUI_bottom->SetAnimationEnabled(true);
    splashUI_top->SetAnimationEnabled(true);

    DelayInformer::Get(this, delay+0.1f, SPLASHSCREEN_FINISHCLOSE);

    URHO3D_LOGINFO("SplashScreen() - ---------------------------------------");
    URHO3D_LOGINFOF("SplashScreen() - HandleCloseSplash : %u ! ", this);
    URHO3D_LOGINFO("SplashScreen() - ---------------------------------------");
}

void SplashScreen::HandleOpenSplash(StringHash eventType, VariantMap& eventData)
{
    if (state_ == SPLASHSCREEN_OPENING)
        return;

    keepDelay_ = SPLASHSCREEN_KEEPSTATEDELAY;
    if (eventData.Contains(SplashScreen_Events::KEEPSTATEDELAY))
        keepDelay_ = eventData[SplashScreen_Events::KEEPSTATEDELAY].GetFloat();

    float delay = Max(keepDelay_+SPLASHSCREEN_MOVEDELAY, delay_);

    state_ = SPLASHSCREEN_OPENING;

    // Create open move
    SendEvent(SPLASHSCREEN_STARTOPEN);

    {
        splashUI_bottom->SetPosition(closeposition_bottom);
        SharedPtr<ObjectAnimation> objectAnimation(new ObjectAnimation(context_));
        SharedPtr<ValueAnimation> moveAnimation(new ValueAnimation(context_));
        moveAnimation->SetKeyFrame(0.f, closeposition_bottom);
        moveAnimation->SetKeyFrame(keepDelay_, closeposition_bottom);
        moveAnimation->SetKeyFrame(delay, openposition_bottom);
        objectAnimation->AddAttributeAnimation("Position", moveAnimation, WM_ONCE);
        splashUI_bottom->SetObjectAnimation(objectAnimation);
    }

    {
        splashUI_top->SetPosition(closeposition_top);
        SharedPtr<ObjectAnimation> objectAnimation(new ObjectAnimation(context_));
        SharedPtr<ValueAnimation> moveAnimation(new ValueAnimation(context_));
        moveAnimation->SetKeyFrame(0.f, closeposition_top);
        moveAnimation->SetKeyFrame(keepDelay_, closeposition_top);
        moveAnimation->SetKeyFrame(delay, openposition_top);
        objectAnimation->AddAttributeAnimation("Position", moveAnimation, WM_ONCE);
        splashUI_top->SetObjectAnimation(objectAnimation);
    }

    DelayInformer::Get(this, delay+0.1f, SPLASHSCREEN_FINISHOPEN);

    URHO3D_LOGINFO("SplashScreen() - ---------------------------------------");
    URHO3D_LOGINFOF("SplashScreen() - HandleOpenSplash : %u ! ", this);
    URHO3D_LOGINFO("SplashScreen() - ---------------------------------------");
}

void SplashScreen::Stop()
{
    URHO3D_LOGINFO("SplashScreen() - Stop !");

    if (uisplashscreen_)
    {
        splashUI_bottom->SetVisible(false);
        splashUI_top->SetVisible(false);
    }

    state_ = SPLASHSCREEN_WAITING;

    SubscribeToEvents();

    SendEvent(SPLASHSCREEN_FINISHOPEN);
}

void SplashScreen::HandleVisibility(StringHash eventType, VariantMap& eventData)
{
    if (uisplashscreen_)
    {
        splashUI_bottom->SetVisible(false);
        splashUI_top->SetVisible(false);
    }
}

void SplashScreen::HandleStop(StringHash eventType, VariantMap& eventData)
{
    Stop();
}
