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
#include "../IO/Deserializer.h"
#include "../Resource/ResourceCache.h"
#include "../Urho2D/Drawable2D.h"
#include "../Urho2D/Sprite2D.h"
#include "../Urho2D/SpriteSheet2D.h"

#include "../DebugNew.h"

namespace Urho3D
{

Sprite2D::Sprite2D(Context* context) :
    Resource(context),
    hotSpot_(0.5f, 0.5f),
    offset_(0, 0),
    sourceSize_(0, 0),
    edgeOffset_(0.0f)
{
}

Sprite2D::~Sprite2D() = default;

void Sprite2D::RegisterObject(Context* context)
{
    context->RegisterFactory<Sprite2D>();
}

bool Sprite2D::BeginLoad(Deserializer& source)
{
    if (GetName().Empty())
        SetName(source.GetName());

    // Reload
    if (texture_)
        loadTexture_ = texture_;
    else
    {
        loadTexture_ = new Texture2D(context_);
        loadTexture_->SetName(GetName());
    }
    // In case we're async loading, only call BeginLoad() for the texture (load image but do not upload to GPU)
    if (!loadTexture_->BeginLoad(source))
    {
        // Reload failed
        if (loadTexture_ == texture_)
            texture_.Reset();

        loadTexture_.Reset();
        return false;
    }

    return true;
}

bool Sprite2D::EndLoad()
{
    // Finish loading of the texture in the main thread
    bool success = false;
    if (loadTexture_ && loadTexture_->EndLoad())
    {
        success = true;
        SetTexture(loadTexture_);

        if (texture_)
        {
            SetRectangle(IntRect(0, 0, texture_->GetWidth(), texture_->GetHeight()));
            SetSourceSize(texture_->GetWidth(), texture_->GetHeight());
        }
    }
    else
    {
        // Reload failed
        if (loadTexture_ == texture_)
            texture_.Reset();
    }

    loadTexture_.Reset();
    return success;
}

void Sprite2D::SetTexture(Texture2D* texture)
{
    texture_ = texture;
    // Ensure the texture doesn't have wrap addressing as that will cause bleeding bugs on the edges.
    // Could also choose border mode, but in that case a universally good border color (without alpha bugs)
    // would be hard to choose. Ideal is for the user to configure the texture parameters in its parameter
    // XML file.
    if (texture_->GetAddressMode(COORD_U) == ADDRESS_WRAP)
    {
        texture_->SetAddressMode(COORD_U, ADDRESS_CLAMP);
        texture_->SetAddressMode(COORD_V, ADDRESS_CLAMP);
    }
}

void Sprite2D::SetRectangle(const IntRect& rectangle)
{
	sourceSize_ = rectangle.Size();
    rectangle_ = rectangle;
}

void Sprite2D::SetOffset(const IntVector2& offset)
{
    offset_ = offset;
}

void Sprite2D::SetHotSpot(const Vector2& hotSpot)
{
    hotSpot_ = hotSpot;
}

void Sprite2D::SetSourceSize(int width, int height)
{
    sourceSize_.x_ = width;
    sourceSize_.y_ = height;
}

void Sprite2D::SetTextureEdgeOffset(float offset)
{
    edgeOffset_ = offset;
}

void Sprite2D::SetSpriteSheet(SpriteSheet2D* spriteSheet)
{
    spriteSheet_ = spriteSheet;
}

bool Sprite2D::GetDrawRectangle(Rect& rect, bool flipX, bool flipY) const
{
    return GetDrawRectangle(rect, hotSpot_, flipX, flipY);
}

bool Sprite2D::GetDrawRectangle(Rect& rect, const Vector2& pivot, bool flipX, bool flipY) const
{
    if (sourceSize_.x_ == 0 || sourceSize_.y_ == 0)
        return false;

    if (!flipX)
    {
        rect.min_.x_ = -(float)sourceSize_.x_ * PIXEL_SIZE * pivot.x_;
        rect.max_.x_ = (float)sourceSize_.x_ * PIXEL_SIZE * (1.0f - pivot.x_);
    }
    else
    {
        rect.min_.x_ = -(float)sourceSize_.x_ * PIXEL_SIZE * (1.0f - pivot.x_);
        rect.max_.x_ = (float)sourceSize_.x_ * PIXEL_SIZE * pivot.x_;
    }
    if (!flipY)
    {
        rect.min_.y_ = -(float)sourceSize_.y_ * PIXEL_SIZE * pivot.y_;
        rect.max_.y_ = (float)sourceSize_.y_ * PIXEL_SIZE * (1.0f - pivot.y_);
    }
    else
    {
        rect.min_.y_ = -(float)sourceSize_.y_ * PIXEL_SIZE * (1.0f - pivot.y_);
        rect.max_.y_ = (float)sourceSize_.y_ * PIXEL_SIZE * pivot.y_;
    }

    return true;
}

bool Sprite2D::GetTextureRectangle(Rect& rect, bool flipX, bool flipY) const
{
    if (!texture_)
        return false;

    float invWidth = 1.0f / (float)texture_->GetWidth();
    float invHeight = 1.0f / (float)texture_->GetHeight();

    unsigned texturelevel = Min(texture_->GetLevels(), renderertexturelevels_) - 1;
    rect.min_.x_ = ((float)(rectangle_.left_ >> texturelevel) + edgeOffset_) * invWidth;
    rect.max_.x_ = ((float)(rectangle_.right_ >> texturelevel) - edgeOffset_) * invWidth;
    rect.min_.y_ = ((float)(rectangle_.bottom_ >> texturelevel) - edgeOffset_) * invHeight;
    rect.max_.y_ = ((float)(rectangle_.top_ >> texturelevel) + edgeOffset_) * invHeight;


    if (flipX)
        Swap(rect.min_.x_, rect.max_.x_);

    if (flipY)
        Swap(rect.min_.y_, rect.max_.y_);

    return true;
}

float round(float f, float prec)
{
    return (float)(floor(f*(1.0f/prec) + 0.5f) * prec);
}

void Sprite2D::SetFixedRectangles(const Vector2& scale, float spanOffset, bool flipX, bool flipY)
{
    Vector2 hotspot(flipX ? (1.0f - hotSpot_.x_) : hotSpot_.x_, flipY ? (1.0f - hotSpot_.y_) : hotSpot_.y_);

    GetDrawRectangle(fixedDrawRect_, hotspot, flipX, flipY);
    fixedDrawRect_.min_.x_ = (fixedDrawRect_.min_.x_ - spanOffset) * scale.x_;
    fixedDrawRect_.max_.x_ = (fixedDrawRect_.max_.x_ + spanOffset) * scale.x_;
    fixedDrawRect_.min_.y_ = (fixedDrawRect_.min_.y_ - spanOffset) * scale.y_;
    fixedDrawRect_.max_.y_ = (fixedDrawRect_.max_.y_ + spanOffset) * scale.y_;

    float invWidth = 1.0f / (float)texture_->GetWidth();
    float invHeight = 1.0f / (float)texture_->GetHeight();

    unsigned texturelevel = Min(texture_->GetLevels(), renderertexturelevels_) - 1;
    fixedTextRect_.min_.x_ = ((float)(rectangle_.left_ >> texturelevel) + edgeOffset_) * invWidth;
    fixedTextRect_.max_.x_ = ((float)(rectangle_.right_ >> texturelevel) - edgeOffset_) * invWidth;
    fixedTextRect_.min_.y_ = ((float)(rectangle_.bottom_ >> texturelevel) - edgeOffset_) * invHeight;
    fixedTextRect_.max_.y_ = ((float)(rectangle_.top_ >> texturelevel) + edgeOffset_) * invHeight;

    if (flipX)
        Swap(fixedTextRect_.min_.x_, fixedTextRect_.max_.x_);

    if (flipY)
        Swap(fixedTextRect_.min_.y_, fixedTextRect_.max_.y_);
}

const Rect& Sprite2D::GetFixedDrawRectangle() const
{
    return fixedDrawRect_;
}

const Rect& Sprite2D::GetFixedTextRectangle() const
{
    return fixedTextRect_;
}

String Sprite2D::Dump() const
{
    String s;
    s += GetName() + String(" => ");
    s += String("rect=") + rectangle_.ToString() + String(" | ");
    s += String("size=") + sourceSize_.ToString() + String(" | ");
    s += String("off=") + offset_.ToString() + String(" | ");
    s += String("hot=") + hotSpot_.ToString() + String(" | ");
    s += String("fxdrawrect=") + fixedDrawRect_.ToString() + String(" | ");
    s += String("fxtextrect=") + fixedTextRect_.ToString();

    return s;
}

ResourceRef Sprite2D::SaveToResourceRef(Sprite2D* sprite)
{
    if (!sprite)
        return Variant::emptyResourceRef;

    if (!sprite->GetSpriteSheet() || sprite->GetSpriteSheet()->GetName().Empty())
        return GetResourceRef(sprite, Sprite2D::GetTypeStatic());

    // Combine sprite sheet name and sprite name as resource name.
    return ResourceRef(SpriteSheet2D::GetTypeStatic(), sprite->GetSpriteSheet()->GetName() + "@" + sprite->GetName());
}

Sprite2D* Sprite2D::LoadFromResourceRef(Object* object, const ResourceRef& value)
{
    if (!object)
        return nullptr;

    ResourceCache* cache = object->GetSubsystem<ResourceCache>();

    if (value.type_ == Sprite2D::GetTypeStatic())
        return cache->GetResource<Sprite2D>(value.name_);

    if (value.type_ == SpriteSheet2D::GetTypeStatic())
    {
        // value.name_ include sprite sheet name and sprite name.
        Vector<String> names = value.name_.Split('@');
        if (names.Size() != 2)
            return nullptr;

        const String& spriteSheetName = names[0];
        const String& spriteName = names[1];

        SpriteSheet2D* spriteSheet = cache->GetResource<SpriteSheet2D>(spriteSheetName);
        if (!spriteSheet)
            return nullptr;

        return spriteSheet->GetSprite(spriteName);
    }

    return nullptr;
}

void Sprite2D::LoadFromResourceRefList(Object* object, const ResourceRefList& valuelist, PODVector<Sprite2D*>& sprites)
{
    if (!object)
        return;

    ResourceCache* cache = object->GetSubsystem<ResourceCache>();

    sprites.Clear();
    unsigned numsprites = valuelist.names_.Size();

    sprites.Resize(numsprites);

    if (valuelist.type_ == Sprite2D::GetTypeStatic())
    {
        for (unsigned i=0; i < numsprites; ++i)
            sprites[i] = cache->GetResource<Sprite2D>(valuelist.names_[i]);
    }
    else if (valuelist.type_ == SpriteSheet2D::GetTypeStatic())
    {
        Vector<String> names;
        SpriteSheet2D* spriteSheet = nullptr;
        for (unsigned i=0; i < numsprites; ++i)
        {
            names = valuelist.names_[i].Split('@');
            if (names.Size() > 1) // valuelist = "spritesheetname@spritename"
            {
                if (!spriteSheet || spriteSheet->GetName() != names.Front())
                    spriteSheet = cache->GetResource<SpriteSheet2D>(names.Front());

                sprites[i] = spriteSheet ? spriteSheet->GetSprite(names[1]) : nullptr;
            }
            else if (spriteSheet) // valuelist = "spritename" use previous spritesheet to get sprite            
                sprites[i] = spriteSheet->GetSprite(names.Front());           
        }
    }
}

unsigned Sprite2D::renderertexturelevels_ = 1;

void Sprite2D::SetTextureLevels(int textureQuality)
{
    renderertexturelevels_ = MAX_TEXTURE_QUALITY_LEVELS - textureQuality;
}

}
