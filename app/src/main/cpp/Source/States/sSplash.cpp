#include <Urho3D/Urho3D.h>

#include <Urho3D/Core/Object.h>
#include <Urho3D/Core/CoreEvents.h>

#include <Urho3D/Engine/Engine.h>
#include <Urho3D/IO/Log.h>

#include <Urho3D/Graphics/Graphics.h>
#include <Urho3D/Graphics/Renderer.h>
#include <Urho3D/Graphics/Texture2D.h>

#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Resource/XMLFile.h>

#include <Urho3D/Scene/ObjectAnimation.h>
#include <Urho3D/Scene/ValueAnimation.h>

#include <Urho3D/Input/Input.h>

#include <Urho3D/UI/Sprite.h>
#include <Urho3D/UI/UI.h>

#include <Urho3D/DebugNew.h>

#include "DelayInformer.h"

#include "GameStatics.h"
#include "GameHelpers.h"
#include "GameEvents.h"

#include "sSplash.h"

#define DURATION 4.f
#define INTERVAL_LOGOROTATE 1.5f
#define RATIO_FADEIN 0.6f
#define RATIO_FADEOUT 0.9f


const StringHash SPLASH_STOP = StringHash("Splash_Stop");


SplashState::SplashState(Context* context) :
    GameState(context,"Splash")
{
//    URHO3D_LOGINFO("SplashState()");
}

SplashState::~SplashState()
{
    URHO3D_LOGINFO("~SplashState()");
}


bool SplashState::Initialize()
{
    return GameState::Initialize();
}

void SplashState::ResizeScreen()
{
    UI* ui = GameStatics::ui_;

    for (unsigned i=0; i < sprites_.Size(); i++)
        sprites_[i]->SetAnimationEnabled(false);

    float width = ui->GetRoot()->GetSize().x_;
    float height = ui->GetRoot()->GetSize().y_;

    Sprite* sprite;
    Texture2D* texture;

    // Set BackGround
    sprite = sprites_[0];
    if (sprite->GetSize().x_ != width || sprite->GetSize().y_ != height)
    {
        texture = static_cast<Texture2D*>(sprite->GetTexture());
        IntVector2 size(width, height);
        GameHelpers::ApplySizeRatioMaximized(texture->GetWidth(), texture->GetHeight(), size);
        sprite->SetSize(size);
        sprite->SetHotSpot(size.x_ / 2.f, size.y_ / 2.f);
        sprite->SetPosition(width/ 2.f, height / 2.f);
    }
    // Set Background Effect
    sprite = sprites_[1];
    if (sprite->GetSize().x_ != width || sprite->GetSize().y_ != height)
    {
        texture = static_cast<Texture2D*>(sprite->GetTexture());
        IntVector2 size(width, height);
        GameHelpers::ApplySizeRatio(texture->GetWidth(), texture->GetHeight(), size);
        sprite->SetSize(size);
        sprite->SetHotSpot(size.x_ / 2.f, size.y_ / 2.f);
        sprite->SetPosition(width/ 2.f, height / 2.f);
    }
    // Set Okko Logo
    sprite = sprites_[2];
    if (sprite->GetSize().x_ != (int)(width * 0.6f) || sprite->GetSize().y_ != (int)(height* 0.6f))
    {
        texture = static_cast<Texture2D*>(sprite->GetTexture());
        float ratio = (float)texture->GetHeight() / (float)texture->GetWidth();
        float maxsize = width * 0.7f;
        sprite->SetSize((int)maxsize , (int)(maxsize*ratio));
        sprite->SetHotSpot(sprite->GetSize()/2);
        sprite->SetPosition(width / 2.f, height / 2.f);
    }
    // Set Urho Logo
    sprite = sprites_[3];
    if (sprite->GetSize().x_ != (int)(width * 0.1f) || sprite->GetSize().y_ != (int)(height* 0.1f))
    {
        texture = static_cast<Texture2D*>(sprite->GetTexture());
        float medsize = (width +  height) / 2.f;
        IntVector2 size(medsize*0.15f, medsize*0.15f);
        GameHelpers::ApplySizeRatio(texture->GetWidth(), texture->GetHeight(), size);
        sprite->SetSize(size);
        sprite->SetHotSpot(size.x_ / 2.f, size.y_ / 2.f);
        sprite->SetPosition(width - (float)size.x_/2.f, height - (float)size.y_/2.f);
    }

    for (unsigned i=0; i < sprites_.Size(); i++)
        sprites_[i]->SetAnimationEnabled(true);
}

void SplashState::Create()
{
    ResourceCache* cache = GetSubsystem<ResourceCache>();
    UI* ui = GameStatics::ui_;

    float width = ui->GetRoot()->GetSize().x_;
    float height = ui->GetRoot()->GetSize().y_;

    Sprite* sprite;
    IntVector2 size;

    SharedPtr<Texture2D> backtexture   = cache->GetTempResource<Texture2D>("Textures/splashback.webp");
    SharedPtr<Texture2D> splashtexture = cache->GetTempResource<Texture2D>("Textures/splasheffect.webp");
    SharedPtr<Texture2D> okkotexture   = cache->GetTempResource<Texture2D>("Textures/okkologo.webp");
    SharedPtr<Texture2D> urhotexture   = cache->GetTempResource<Texture2D>("Textures/urho3dlogo.webp");

    for (unsigned i=0; i < sprites_.Size(); ++i)
        ui->GetRoot()->RemoveChild(sprites_[i]);
    sprites_.Clear();

    // Create Background
    if (backtexture)
    {
        sprite = ui->GetRoot()->CreateChild<Sprite>();
        sprites_.Push(sprite);
        sprite->SetTexture(backtexture);
        sprite->SetBlendMode(BLEND_ALPHA);
    }
    // Create Background Effect
    if (splashtexture)
    {
        sprite = ui->GetRoot()->CreateChild<Sprite>();
        sprites_.Push(sprite);
        sprite->SetTexture(splashtexture);
        sprite->SetBlendMode(BLEND_ALPHA);
    }
    // Create Okko Logo sprite
    if (okkotexture)
    {
        sprite = ui->GetRoot()->CreateChild<Sprite>();
        sprites_.Push(sprite);
        sprite->SetTexture(okkotexture);
        sprite->SetBlendMode(BLEND_ALPHA);
    }
    // Create Urho3D Logo sprite
    if (urhotexture)
    {
        sprite = ui->GetRoot()->CreateChild<Sprite>();
        sprites_.Push(sprite);
        sprite->SetTexture(urhotexture);
        sprite->SetBlendMode(BLEND_ALPHA);
    }

    ResizeScreen();

    // Set Background animation
    if (backtexture)
    {
        SharedPtr<ObjectAnimation> effectAnimation(new ObjectAnimation(context_));
        SharedPtr<ValueAnimation> alphaAnimation(new ValueAnimation(context_));
        alphaAnimation->SetKeyFrame(0.f, 0.f);
        alphaAnimation->SetKeyFrame(1.f, 1.f);
        alphaAnimation->SetKeyFrame(RATIO_FADEOUT*DURATION, 1.f);
        alphaAnimation->SetKeyFrame(DURATION, 0.f);
        effectAnimation->AddAttributeAnimation("Opacity", alphaAnimation, WM_ONCE);
        sprites_[0]->SetObjectAnimation(effectAnimation);
    }
    // Set Background Effect animation
    if (splashtexture)
    {
        SharedPtr<ObjectAnimation> effectAnimation(new ObjectAnimation(context_));
        SharedPtr<ValueAnimation> rotateAnimation(new ValueAnimation(context_));
        rotateAnimation->SetKeyFrame(0.f, 0.f);
        rotateAnimation->SetKeyFrame(DURATION, 180.f);
        effectAnimation->AddAttributeAnimation("Rotation", rotateAnimation, WM_ONCE);
        SharedPtr<ValueAnimation> alphaAnimation(new ValueAnimation(context_));
        alphaAnimation->SetKeyFrame(0.f, 0.f);
        alphaAnimation->SetKeyFrame(RATIO_FADEOUT*DURATION, 1.f);
        alphaAnimation->SetKeyFrame(DURATION, 0.f);
        effectAnimation->AddAttributeAnimation("Opacity", alphaAnimation, WM_ONCE);
        sprites_[1]->SetObjectAnimation(effectAnimation);
    }
    // Set Okko Logo animation
    if (okkotexture)
    {
        SharedPtr<ObjectAnimation> objectAnimation(new ObjectAnimation(context_));
        SharedPtr<ValueAnimation> scaleAnimation(new ValueAnimation(context_));
        scaleAnimation->SetKeyFrame(0.f, Vector2(1.f, 1.f));
        scaleAnimation->SetKeyFrame(0.9f*DURATION, Vector2(1.1f, 1.1f));
        scaleAnimation->SetKeyFrame(DURATION, Vector2(1.f, 1.f));
        objectAnimation->AddAttributeAnimation("Scale", scaleAnimation, WM_ONCE);
        SharedPtr<ValueAnimation> rotateAnimation(new ValueAnimation(context_));
        rotateAnimation->SetKeyFrame(0.f, 0.f);
        rotateAnimation->SetKeyFrame(1.f*INTERVAL_LOGOROTATE, 1.f);
        rotateAnimation->SetKeyFrame(3.f*INTERVAL_LOGOROTATE, 0.f);
        objectAnimation->AddAttributeAnimation("Rotation", rotateAnimation, WM_LOOP);
        SharedPtr<ValueAnimation> alphaAnimation(new ValueAnimation(context_));
        alphaAnimation->SetKeyFrame(0.f, 0.0f);
        alphaAnimation->SetKeyFrame(RATIO_FADEIN*DURATION, 1.f);
        alphaAnimation->SetKeyFrame(RATIO_FADEOUT*DURATION, 1.f);
        alphaAnimation->SetKeyFrame(DURATION, 0.f);
        objectAnimation->AddAttributeAnimation("Opacity", alphaAnimation, WM_ONCE);
        sprites_[2]->SetObjectAnimation(objectAnimation);
    }
    // Set Urho3D Logo animation
    if (urhotexture)
    {
        SharedPtr<ObjectAnimation> objectAnimation(new ObjectAnimation(context_));
        SharedPtr<ValueAnimation> scaleAnimation(new ValueAnimation(context_));
        Vector2 scale1 = sprite->GetScale();
        Vector2 scale2 = scale1 * Vector2(1.2f, 1.f);
        scaleAnimation->SetKeyFrame(0.f, scale1);
        scaleAnimation->SetKeyFrame(INTERVAL_LOGOROTATE-0.1f, scale1);
        scaleAnimation->SetKeyFrame(INTERVAL_LOGOROTATE, scale2);
        objectAnimation->AddAttributeAnimation("Scale", scaleAnimation, WM_LOOP);
        SharedPtr<ValueAnimation> alphaAnimation(new ValueAnimation(context_));
        alphaAnimation->SetKeyFrame(0.f, 0.f);
        alphaAnimation->SetKeyFrame(0.25f, 1.f);
        alphaAnimation->SetKeyFrame(0.8f*DURATION, 1.f);
        alphaAnimation->SetKeyFrame(DURATION, 0.f);
        objectAnimation->AddAttributeAnimation("Opacity", alphaAnimation, WM_ONCE);
        sprites_[3]->SetObjectAnimation(objectAnimation);
    }
}

void SplashState::Begin()
{
    if (IsBegun())
        return;

    URHO3D_LOGINFO("SplashState() - ----------------------------------------");
    URHO3D_LOGINFO("SplashState() - Begin ...                             -");
    URHO3D_LOGINFO("SplashState() - ----------------------------------------");

    ResourceCache* cache = GetSubsystem<ResourceCache>();

    Create();

    SubscribeToEvent(this, SPLASH_STOP, URHO3D_HANDLER(SplashState, HandleStop));
    SubscribeToEvent(E_KEYDOWN, URHO3D_HANDLER(SplashState, HandleStop));
    SubscribeToEvent(E_TOUCHBEGIN, URHO3D_HANDLER(SplashState, HandleStop));
    SubscribeToEvent(GAME_SCREENRESIZED, URHO3D_HANDLER(SplashState, HandleScreenResized));

    DelayInformer::Get(this, 1.05f * DURATION, SPLASH_STOP);

    // Call base class implementation
    GameState::Begin();

    URHO3D_LOGINFO("SplashState() - ----------------------------------------");
    URHO3D_LOGINFO("SplashState() - Begin ...  OK !                       -");
    URHO3D_LOGINFO("SplashState() - ----------------------------------------");
}

void SplashState::End()
{
    if (!IsBegun())
        return;

    URHO3D_LOGINFO("SplashState() - ----------------------------------------");
    URHO3D_LOGINFO("SplashState() - End ...                               -");
    URHO3D_LOGINFO("SplashState() - ----------------------------------------");

    for (unsigned i=0; i < sprites_.Size(); ++i)
        GameStatics::ui_->GetRoot()->RemoveChild(sprites_[i]);
    sprites_.Clear();

    UnsubscribeFromAllEvents();

    // Reset controls
    GetSubsystem<Input>()->ResetStates();

    // Call base class implementation
    GameState::End();

    URHO3D_LOGINFO("SplashState() - ----------------------------------------");
    URHO3D_LOGINFO("SplashState() - End ... OK !                           -");
    URHO3D_LOGINFO("SplashState() - ----------------------------------------");
}

void SplashState::HandleStop(StringHash eventType, VariantMap& eventData)
{
    URHO3D_LOGINFO("SplashState() - HandleStop !");
    stateManager_->PopStack();
}

void SplashState::HandleScreenResized(StringHash eventType, VariantMap& eventData)
{
    URHO3D_LOGINFO("SplashState() - HandleScreenResized ...");

    ResizeScreen();

    URHO3D_LOGINFO("SplashState() - HandleScreenResized ... OK !");
}
