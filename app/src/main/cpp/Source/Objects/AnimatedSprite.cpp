#include <Urho3D/Urho3D.h>

#include <Urho3D/Core/Context.h>
#include <Urho3D/Core/CoreEvents.h>

#include <Urho3D/IO/Log.h>

#include <Urho3D/Graphics/Texture2D.h>
#include <Urho3D/Urho2D/Sprite2D.h>

#include <Urho3D/UI/UI.h>

#include "GameStatics.h"

#include "AnimatedSprite.h"


const char* uiloopModeNames[] =
{
    "Default",
    "ForceLooped",
    "ForceClamped",
    0
};

AnimatedSprite::AnimatedSprite(Context* context) :
    Sprite(context),
    animatedSprite2D_(context)
{
    SetBlendMode(BLEND_ALPHA);
    SetHotSpot(Vector2(0.5f, 0.5f));
}

AnimatedSprite::~AnimatedSprite()
{
    URHO3D_LOGINFOF("~AnimatedSprite()");
}

void AnimatedSprite::RegisterObject(Context* context)
{
    context->RegisterFactory<AnimatedSprite>();

    URHO3D_COPY_BASE_ATTRIBUTES(Sprite);
    URHO3D_REMOVE_ATTRIBUTE("HotSpot");

    URHO3D_MIXED_ACCESSOR_ATTRIBUTE("Animation Set", GetAnimationSetAttr, SetAnimationSetAttr, ResourceRef, ResourceRef(AnimatedSprite2D::GetTypeStatic(), String::EMPTY), AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Entity", GetEntityName, SetEntity, String, String::EMPTY, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Animation", GetAnimation, SetAnimation, String, String::EMPTY, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("HotSpot", GetHotSpot, SetHotSpot, Vector2, Vector2::ZERO, AM_FILE);
    URHO3D_ENUM_ACCESSOR_ATTRIBUTE("Loop Mode", GetLoopMode, SetLoopMode, LoopMode2D, uiloopModeNames, LM_DEFAULT, AM_DEFAULT);
}

void AnimatedSprite::SetAnimationSetAttr(const ResourceRef& value)
{
    animatedSprite2D_.SetAnimationSetAttr(value);
}

void AnimatedSprite::SetEntity(const String& entity)
{
    animatedSprite2D_.SetEntity(entity);
}

void AnimatedSprite::SetAnimationSet(AnimationSet2D* animationSet)
{
    animatedSprite2D_.SetAnimationSet(animationSet);
}

void AnimatedSprite::SetAnimation(const String& animation)
{
    animatedSprite2D_.SetAnimationAttr(animation);
    animatedSprite2D_.UpdateAnimation(0.f);
    texture_ = static_cast<Texture*>(static_cast<StaticSprite2D*>(&animatedSprite2D_)->GetSprite()->GetTexture());

    if (!GetSize().x_)
    {
        SetSize(animatedSprite2D_.GetDrawRectangle().Size().x_ / PIXEL_SIZE, animatedSprite2D_.GetDrawRectangle().Size().y_ / PIXEL_SIZE);
        SetScale2D(1.f);
    }
    else
    {
        float sc;
        if (animatedSprite2D_.GetDrawRectangle().Size().x_ > animatedSprite2D_.GetDrawRectangle().Size().y_)
            sc = GetSize().x_ / animatedSprite2D_.GetDrawRectangle().Size().x_ * PIXEL_SIZE;
        else
            sc = GetSize().y_ / animatedSprite2D_.GetDrawRectangle().Size().y_ * PIXEL_SIZE;

        SetScale2D(sc);
    }

//    URHO3D_LOGINFOF("AnimatedSprite() - SetAnimationAttr : name=%s size=%d,%d ", name_.CString(), GetSize().x_, GetSize().y_);
}

void AnimatedSprite::SetScale2D(float scale)
{
    scale2D_ = Vector2(scale, -scale);
}

void AnimatedSprite::SetHotSpot(const Vector2& hotspot)
{
    animatedSprite2D_.SetHotSpot(hotspot);
}

void AnimatedSprite::SetLoopMode(LoopMode2D loopMode)
{
    animatedSprite2D_.SetLoopMode(loopMode);
}

void AnimatedSprite::SetFollowedElement(UIElement* element)
{
    followedElement_ = element;
}

void AnimatedSprite::SwitchIndependentUpdate()
{
    if (IsVisibleEffective() && IsEnabled())
    {
//        URHO3D_LOGINFOF("AnimatedSprite() - SwitchIndependentUpdate : name=%s ", name_.CString());
        SubscribeToEvent(E_UPDATE, URHO3D_HANDLER(AnimatedSprite, HandleUpdate));
        GameStatics::ui_->AddElementAlwaysOnTop(this);
    }
}

void AnimatedSprite::Update(float timeStep)
{
    if (IsVisibleEffective())
    {
        animatedSprite2D_.UpdateAnimation(timeStep);

        if (followedElement_)
            SetPosition(followedElement_->GetPosition().x_, followedElement_->GetPosition().y_);
    }
}

void AnimatedSprite::GetBatches(PODVector<UIBatch>& batches, PODVector<float>& vertexData, const IntRect& currentScissor)
{
    bool allOpaque = true;
    if (GetDerivedOpacity() < 1.0f || color_[C_TOPLEFT].a_ < 1.0f || color_[C_TOPRIGHT].a_ < 1.0f || color_[C_BOTTOMLEFT].a_ < 1.0f || color_[C_BOTTOMRIGHT].a_ < 1.0f)
        allOpaque = false;

    UIBatch batch(this, blendMode_ == BLEND_REPLACE && !allOpaque ? BLEND_ALPHA : blendMode_, currentScissor, texture_, &vertexData);

    animatedSprite2D_.GetVertices(GetSize(), GetTransform(), vertexData);
    batch.vertexEnd_ = vertexData.Size();
    UIBatch::AddOrMerge(batch, batches);

//    if (GetName() == "zoom")
//        URHO3D_LOGINFOF("AnimatedSprite() - GetBatches : name=%s vertexdatasize=%u pos=%s enable=%s visible=%s size=%d,%d scale=%F,%F scale2D=%F,%F",
//                        name_.CString(), batch.vertexEnd_-batch.vertexStart_, GetPosition().ToString().CString(),
//                        IsEnabled() ? "true":"false", IsVisible() ? "true":"false", GetSize().x_, GetSize().y_, scale_.x_, scale_.y_, scale2D_.x_, scale2D_.y_);

    // Reset hovering for next frame
    hovering_ = false;
}

const Matrix3x4& AnimatedSprite::GetTransform() const
{
    if (positionDirty_)
    {
        Vector2 pos = floatPosition_;

        Matrix3x4 parentTransform;

        if (parent_)
        {
            Sprite* parentSprite = dynamic_cast<Sprite*>(parent_);
            if (parentSprite)
                parentTransform = parentSprite->GetTransform();
            else
            {
                const IntVector2& parentScreenPos = parent_->GetScreenPosition() + parent_->GetChildOffset();
                parentTransform = Matrix3x4::IDENTITY;
                parentTransform.SetTranslation(Vector3((float)parentScreenPos.x_, (float)parentScreenPos.y_, 0.0f));
            }

            switch (GetHorizontalAlignment())
            {
            case HA_LEFT:
                break;

            case HA_CENTER:
                pos.x_ += (float)(parent_->GetSize().x_ / 2);
                break;

            case HA_RIGHT:
                pos.x_ += (float)parent_->GetSize().x_;
                break;
            }
            switch (GetVerticalAlignment())
            {
            case VA_TOP:
                break;

            case VA_CENTER:
                pos.y_ += (float)(parent_->GetSize().y_ / 2);
                break;

            case VA_BOTTOM:
                pos.y_ += (float)(parent_->GetSize().y_);
                break;
            }
        }
        else
            parentTransform = Matrix3x4::IDENTITY;

        Matrix3x4 mainTransform(Vector3(pos, 0.0f), Quaternion(rotation_, Vector3::FORWARD), Vector3(scale_ * scale2D_, 1.0f));

        transform_ = parentTransform * mainTransform;
        positionDirty_ = false;

        // Calculate an approximate screen position for GetElementAt(), or pixel-perfect child elements
        Vector3 topLeftCorner = transform_ * Vector3::ZERO;
        screenPosition_ = IntVector2((int)topLeftCorner.x_, (int)topLeftCorner.y_);
    }

    return transform_;
}

void AnimatedSprite::HandleUpdate(StringHash eventType, VariantMap& eventData)
{
    Update(eventData[Update::P_TIMESTEP].GetFloat());
}
