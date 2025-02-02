//
// Copyright (c) 2008-2016 the Urho3D project.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#include "../Precompiled.h"

#include "../Core/Context.h"
#include "../Graphics/Texture2D.h"
#include "../Resource/ResourceCache.h"
#include "../Urho2D/Sprite2D.h"
#include "../Urho2D/SpriteSheet2D.h"

#include "../UI/Sprite.h"

#include "../DebugNew.h"

namespace Urho3D
{

extern const char* blendModeNames[];
extern const char* horizontalAlignments[];
extern const char* verticalAlignments[];
extern const char* UI_CATEGORY;

Sprite::Sprite(Context* context) :
    UIElement(context),
    floatPosition_(Vector2::ZERO),
    hotSpot_(IntVector2::ZERO),
    scale_(Vector2::ONE),
    rotation_(0.0f),
    imageRect_(IntRect::ZERO),
    border_(IntRect::ZERO),
    blendMode_(BLEND_REPLACE)
{
}

Sprite::~Sprite()
{
}

void Sprite::RegisterObject(Context* context)
{
    context->RegisterFactory<Sprite>(UI_CATEGORY);

    URHO3D_ACCESSOR_ATTRIBUTE("Name", GetName, SetName, String, String::EMPTY, AM_FILE);
    URHO3D_ACCESSOR_ATTRIBUTE("Position", GetPosition, SetPosition, Vector2, Vector2::ZERO, AM_FILE);
    URHO3D_ACCESSOR_ATTRIBUTE("Size", GetSize, SetSize, IntVector2, IntVector2::ZERO, AM_FILE);
    URHO3D_ACCESSOR_ATTRIBUTE("Hotspot", GetHotSpot, SetHotSpot, IntVector2, IntVector2::ZERO, AM_FILE);
    URHO3D_ACCESSOR_ATTRIBUTE("Scale", GetScale, SetScale, Vector2, Vector2::ONE, AM_FILE);
    URHO3D_ACCESSOR_ATTRIBUTE("Rotation", GetRotation, SetRotation, float, 0.0f, AM_FILE);
    URHO3D_MIXED_ACCESSOR_ATTRIBUTE("Texture", GetTextureAttr, SetTextureAttr, ResourceRef, ResourceRef(Texture2D::GetTypeStatic()),
        AM_FILE);
    URHO3D_ACCESSOR_ATTRIBUTE("Image Rect", GetImageRect, SetImageRect, IntRect, IntRect::ZERO, AM_FILE);
    URHO3D_ACCESSOR_ATTRIBUTE("Border", GetBorder, SetBorder, IntRect, IntRect::ZERO, AM_FILE);
    URHO3D_ENUM_ACCESSOR_ATTRIBUTE("Blend Mode", GetBlendMode, SetBlendMode, BlendMode, blendModeNames, 0, AM_FILE);
    URHO3D_ENUM_ACCESSOR_ATTRIBUTE("Horiz Alignment", GetHorizontalAlignment, SetHorizontalAlignment, HorizontalAlignment,
        horizontalAlignments, HA_LEFT, AM_FILE);
    URHO3D_ENUM_ACCESSOR_ATTRIBUTE("Vert Alignment", GetVerticalAlignment, SetVerticalAlignment, VerticalAlignment, verticalAlignments,
        VA_TOP, AM_FILE);
    URHO3D_ACCESSOR_ATTRIBUTE("Priority", GetPriority, SetPriority, int, 0, AM_FILE);
    URHO3D_ACCESSOR_ATTRIBUTE("Opacity", GetOpacity, SetOpacity, float, 1.0f, AM_FILE);
    URHO3D_ACCESSOR_ATTRIBUTE("Color", GetColorAttr, SetColor, Color, Color::WHITE, AM_FILE);
    URHO3D_ATTRIBUTE("Top Left Color", Color, color_[0], Color::WHITE, AM_FILE);
    URHO3D_ATTRIBUTE("Top Right Color", Color, color_[1], Color::WHITE, AM_FILE);
    URHO3D_ATTRIBUTE("Bottom Left Color", Color, color_[2], Color::WHITE, AM_FILE);
    URHO3D_ATTRIBUTE("Bottom Right Color", Color, color_[3], Color::WHITE, AM_FILE);
    URHO3D_ACCESSOR_ATTRIBUTE("Is Visible", IsVisible, SetVisible, bool, true, AM_FILE);
    URHO3D_ACCESSOR_ATTRIBUTE("Use Derived Opacity", GetUseDerivedOpacity, SetUseDerivedOpacity, bool, true, AM_FILE);
    URHO3D_ATTRIBUTE("Variables", VariantMap, vars_, Variant::emptyVariantMap, AM_FILE);
}

bool Sprite::IsWithinScissor(const IntRect& currentScissor)
{
    /// \todo Implement properly, for now just checks visibility flag
    return visible_;
}

const IntVector2& Sprite::GetScreenPosition() const
{
    // This updates screen position for a sprite
    GetTransform();
    return screenPosition_;
}

IntVector2 Sprite::ScreenToElement(const IntVector2& screenPosition)
{
    Vector3 floatPos((float)screenPosition.x_, (float)screenPosition.y_, 0.0f);
    Vector3 transformedPos = GetTransform().Inverse() * floatPos;
    return IntVector2((int)transformedPos.x_, (int)transformedPos.y_);
}

IntVector2 Sprite::ElementToScreen(const IntVector2& position)
{
    Vector3 floatPos((float)position.x_, (float)position.y_, 0.0f);
    Vector3 transformedPos = GetTransform() * floatPos;
    return IntVector2((int)transformedPos.x_, (int)transformedPos.y_);
}
/*
void Sprite::GetBatches(PODVector<UIBatch>& batches, PODVector<float>& vertexData, const IntRect& currentScissor)
{
    bool allOpaque = true;
    if (GetDerivedOpacity() < 1.0f || color_[C_TOPLEFT].a_ < 1.0f || color_[C_TOPRIGHT].a_ < 1.0f ||
        color_[C_BOTTOMLEFT].a_ < 1.0f || color_[C_BOTTOMRIGHT].a_ < 1.0f)
        allOpaque = false;

    const IntVector2& size = GetSize();
    UIBatch batch(this, blendMode_ == BLEND_REPLACE && !allOpaque ? BLEND_ALPHA : blendMode_, currentScissor, texture_, &vertexData);

    batch.AddQuad(GetTransform(), 0, 0, size.x_, size.y_, imageRect_.left_, imageRect_.top_, imageRect_.right_ - imageRect_.left_,
        imageRect_.bottom_ - imageRect_.top_);

    UIBatch::AddOrMerge(batch, batches);

    // Reset hovering for next frame
    hovering_ = false;
}
*/
void Sprite::GetBatches(PODVector<UIBatch>& batches, PODVector<float>& vertexData, const IntRect& currentScissor)
{
    bool allOpaque = true;
    if (GetDerivedOpacity() < 1.0f || color_[C_TOPLEFT].a_ < 1.0f || color_[C_TOPRIGHT].a_ < 1.0f || color_[C_BOTTOMLEFT].a_ < 1.0f || color_[C_BOTTOMRIGHT].a_ < 1.0f)
        allOpaque = false;

    UIBatch batch(this, blendMode_ == BLEND_REPLACE && !allOpaque ? BLEND_ALPHA : blendMode_, currentScissor, texture_, &vertexData);

    const IntRect& uvBorder = border_;
    const IntVector2& size = GetSize();

    IntVector2 innerSize(Max(size.x_ - border_.left_ - border_.right_, 0), Max(size.y_ - border_.top_ - border_.bottom_, 0));
    IntVector2 innerUvSize(Max(imageRect_.right_ - imageRect_.left_ - uvBorder.left_ - uvBorder.right_, 0), Max(imageRect_.bottom_ - imageRect_.top_ - uvBorder.top_ - uvBorder.bottom_, 0));

    // Top
    if (border_.top_)
    {
        if (border_.left_)
            batch.AddQuad(GetTransform(), 0, 0, border_.left_, border_.top_,
                          imageRect_.left_, imageRect_.left_, uvBorder.left_, uvBorder.top_);
        if (innerSize.x_)
            batch.AddQuad(GetTransform(), border_.left_, 0, innerSize.x_, border_.top_,
                          imageRect_.left_ + uvBorder.left_, imageRect_.left_, innerUvSize.x_, uvBorder.top_);
        if (border_.right_)
            batch.AddQuad(GetTransform(), border_.left_ + innerSize.x_, 0, border_.right_, border_.top_,
                          imageRect_.left_ + uvBorder.left_ + innerUvSize.x_, imageRect_.left_, uvBorder.right_, uvBorder.top_);
    }
    // Middle
    if (innerSize.y_)
    {
        if (border_.left_)
            batch.AddQuad(GetTransform(), 0, border_.top_, border_.left_, innerSize.y_, imageRect_.left_,
                          imageRect_.left_ + uvBorder.top_, uvBorder.left_, innerUvSize.y_);
        if (innerSize.x_)
            batch.AddQuad(GetTransform(), border_.left_, border_.top_, innerSize.x_, innerSize.y_,
                          imageRect_.left_ + uvBorder.left_, imageRect_.left_ + uvBorder.top_, innerUvSize.x_, innerUvSize.y_);
        if (border_.right_)
            batch.AddQuad(GetTransform(), border_.left_ + innerSize.x_, border_.top_, border_.right_, innerSize.y_,
                          imageRect_.left_ + uvBorder.left_ + innerUvSize.x_, imageRect_.left_ + uvBorder.top_, uvBorder.right_, innerUvSize.y_);
    }
    // Bottom
    if (border_.bottom_)
    {
        if (border_.left_)
            batch.AddQuad(GetTransform(), 0, border_.top_ + innerSize.y_, border_.left_, border_.bottom_,
                          imageRect_.left_, imageRect_.left_ + uvBorder.top_ + innerUvSize.y_, uvBorder.left_, uvBorder.bottom_);
        if (innerSize.x_)
            batch.AddQuad(GetTransform(), border_.left_, border_.top_ + innerSize.y_, innerSize.x_, border_.bottom_,
                          imageRect_.left_ + uvBorder.left_, imageRect_.left_ + uvBorder.top_ + innerUvSize.y_, innerUvSize.x_, uvBorder.bottom_);
        if (border_.right_)
            batch.AddQuad(GetTransform(), border_.left_ + innerSize.x_, border_.top_ + innerSize.y_, border_.right_, border_.bottom_,
                          imageRect_.left_ + uvBorder.left_ + innerUvSize.x_, imageRect_.left_ + uvBorder.top_ + innerUvSize.y_, uvBorder.right_, uvBorder.bottom_);
    }

    UIBatch::AddOrMerge(batch, batches);

    // Reset hovering for next frame
    hovering_ = false;
}

void Sprite::OnPositionSet(const IntVector2& newPosition)
{
    // If the integer position was set (layout update?), copy to the float position
    floatPosition_ = Vector2((float)newPosition.x_, (float)newPosition.y_);
}

void Sprite::SetPosition(const Vector2& position)
{
    if (position != floatPosition_)
    {
        floatPosition_ = position;
        // Copy to the integer position
        position_ = IntVector2((int)position.x_, (int)position.y_);
        MarkDirty();
    }
}

void Sprite::SetPosition(float x, float y)
{
    SetPosition(Vector2(x, y));
}

void Sprite::SetHotSpot(const IntVector2& hotSpot)
{
    if (hotSpot != hotSpot_)
    {
        hotSpot_ = hotSpot;
        MarkDirty();
    }
}

void Sprite::SetHotSpot(int x, int y)
{
    SetHotSpot(IntVector2(x, y));
}

void Sprite::SetScale(const Vector2& scale)
{
    if (scale != scale_)
    {
        scale_ = scale;
        MarkDirty();
    }
}

void Sprite::SetScale(float x, float y)
{
    SetScale(Vector2(x, y));
}

void Sprite::SetScale(float scale)
{
    SetScale(Vector2(scale, scale));
}

void Sprite::SetRotation(float angle)
{
    if (angle != rotation_)
    {
        rotation_ = angle;
        MarkDirty();
    }
}

void Sprite::SetTexture(Texture* texture)
{
    texture_ = texture;
    if (imageRect_ == IntRect::ZERO)
        SetFullImageRect();
}

void Sprite::SetImageRect(const IntRect& rect)
{
    if (rect != IntRect::ZERO)
        imageRect_ = rect;
}

void Sprite::SetFullImageRect()
{
    if (texture_)
        SetImageRect(IntRect(0, 0, texture_->GetWidth(), texture_->GetHeight()));
}

void Sprite::SetImage(const String& refname)
{
    SetImage(Sprite2D::LoadFromResourceRef(context_, ResourceRef(SpriteSheet2D::GetTypeStatic(), refname)));
}

void Sprite::SetImage(Sprite2D* sprite)
{
    if (sprite)
    {
        SetTexture(sprite->GetTexture());
        SetImageRect(sprite->GetRectangle());
    }
}

void Sprite::SetBorder(const IntRect& rect)
{
    border_.left_ = Max(rect.left_, 0);
    border_.top_ = Max(rect.top_, 0);
    border_.right_ = Max(rect.right_, 0);
    border_.bottom_ = Max(rect.bottom_, 0);
}

void Sprite::SetBlendMode(BlendMode mode)
{
    blendMode_ = mode;
}

const Matrix3x4& Sprite::GetTransform() const
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

        Matrix3x4 hotspotAdjust(Matrix3x4::IDENTITY);
        hotspotAdjust.SetTranslation(Vector3((float)-hotSpot_.x_, (float)-hotSpot_.y_, 0.0f));

        Matrix3x4 mainTransform(Vector3(pos, 0.0f), Quaternion(rotation_, Vector3::FORWARD), Vector3(scale_, 1.0f));

        transform_ = parentTransform * mainTransform * hotspotAdjust;
        positionDirty_ = false;

        // Calculate an approximate screen position for GetElementAt(), or pixel-perfect child elements
        Vector3 topLeftCorner = transform_ * Vector3::ZERO;
        screenPosition_ = IntVector2((int)topLeftCorner.x_, (int)topLeftCorner.y_);
    }

    return transform_;
}

void Sprite::SetTextureAttr(const ResourceRef& value)
{
    ResourceCache* cache = GetSubsystem<ResourceCache>();
    SetTexture(cache->GetResource<Texture2D>(value.name_));
}

ResourceRef Sprite::GetTextureAttr() const
{
    return GetResourceRef(texture_, Texture2D::GetTypeStatic());
}

}
