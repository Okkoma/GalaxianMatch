#include <Urho3D/Urho3D.h>

#include <Urho3D/Core/Context.h>

#include <Urho3D/Graphics/DebugRenderer.h>
#include <Urho3D/Graphics/Material.h>
#include <Urho3D/Graphics/Texture2D.h>
#include <Urho3D/Graphics/Technique.h>

#include <Urho3D/IO/Log.h>

#include <Urho3D/Resource/ResourceCache.h>

#include <Urho3D/Scene/Scene.h>

#include "Sprite2D.h"
#include "Renderer2D.h"

#include "StaticSprite2D.h"

namespace Urho3D
{
    extern const char* blendModeNames[];
}


StaticSprite2D::StaticSprite2D(Context* context) :
    Drawable2D(context),
    blendMode_(BLEND_ALPHA),
    flipX_(false),
    flipY_(false),
    swapXY_(false),
    color_(Color::WHITE),
    useHotSpot_(false),
    useDrawRect_(false),
    useTextureRect_(false),
    hotSpot_(0.5f, 0.5f),
    textureRect_(Rect::ZERO)
{
    sourceBatches_.Resize(1);
    sourceBatches_[0].owner_ = this;
}

StaticSprite2D::~StaticSprite2D() { }

void StaticSprite2D::RegisterObject(Context* context)
{
    context->RegisterFactory<StaticSprite2D>();

    URHO3D_ACCESSOR_ATTRIBUTE("Is Enabled", IsEnabled, SetEnabled, bool, true, AM_DEFAULT);
    URHO3D_COPY_BASE_ATTRIBUTES(Drawable2D);
    URHO3D_MIXED_ACCESSOR_ATTRIBUTE("Sprite", GetSpriteAttr, SetSpriteAttr, ResourceRef, ResourceRef(Sprite2D::GetTypeStatic(), String::EMPTY), AM_DEFAULT);
    URHO3D_ENUM_ACCESSOR_ATTRIBUTE("Blend Mode", GetBlendMode, SetBlendMode, BlendMode, blendModeNames, BLEND_ALPHA, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Flip X", GetFlipX, SetFlipX, bool, false, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Flip Y", GetFlipY, SetFlipY, bool, false, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Color", GetColor, SetColor, Color, Color::WHITE, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Alpha", GetAlpha, SetAlpha, float, 1.f, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Hot Spot", GetHotSpot, SetHotSpotAttr, Vector2, Vector2(0.5f, 0.5f), AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Draw Rectangle", GetDrawRect, SetDrawRect, Rect, Rect::ZERO, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Use Draw Rectangle", GetUseDrawRect, SetUseDrawRect, bool, false, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Texture Rectangle", GetTextureRect, SetTextureRect, Rect, Rect::ZERO, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Use Texture Rectangle", GetUseTextureRect, SetUseTextureRect, bool, false, AM_DEFAULT);
    URHO3D_MIXED_ACCESSOR_ATTRIBUTE("Custom material", GetCustomMaterialAttr, SetCustomMaterialAttr, ResourceRef,
                                    ResourceRef(Material::GetTypeStatic(), String::EMPTY), AM_DEFAULT);
}

void StaticSprite2D::SetSprite(Sprite2D* sprite)
{
    if (sprite == sprite_)
        return;

    sprite_ = sprite;
    UpdateMaterial();

    sourceBatchesDirty_ = true;
    drawRectDirty_ = true;

    MarkNetworkUpdate();
}

void StaticSprite2D::SetDrawRect(const Rect& rect)
{
    if (rect == Rect::ZERO)
    {
        drawRect_.Clear();
        drawRectDirty_ = true;
    }
    else
        drawRect_ = rect;

    if (useDrawRect_)
        sourceBatchesDirty_ = true;
}

void StaticSprite2D::SetTextureRect(const Rect& rect)
{
    textureRect_ = rect;

    if(useTextureRect_)
    {
        sourceBatchesDirty_ = true;
    }
}

void StaticSprite2D::SetBlendMode(BlendMode blendMode)
{
    if (blendMode == blendMode_)
        return;

    blendMode_ = blendMode;

    UpdateMaterial();
    MarkNetworkUpdate();
}

void StaticSprite2D::SetFlip(bool flipX, bool flipY, bool swapXY)
{
    if (flipX == flipX_ && flipY == flipY_ && swapXY == swapXY_)
        return;

    flipX_ = flipX;
    flipY_ = flipY;
    swapXY_ = swapXY;

    sourceBatchesDirty_ = true;
    drawRectDirty_ = true;

    MarkNetworkUpdate();
}

void StaticSprite2D::SetFlipX(bool flipX)
{
    SetFlip(flipX, flipY_, swapXY_);
}

void StaticSprite2D::SetFlipY(bool flipY)
{
    SetFlip(flipX_, flipY, swapXY_);
}

void StaticSprite2D::SetSwapXY(bool swapXY)
{
    SetFlip(flipX_, flipY_, swapXY);
}

void StaticSprite2D::SetColor(const Color& color)
{
    if (color == color_)
        return;

    color_ = color;
    sourceBatchesDirty_ = true;
    MarkNetworkUpdate();
}

void StaticSprite2D::SetAlpha(float alpha)
{
    if (alpha == color_.a_)
        return;

    color_.a_ = alpha;
    sourceBatchesDirty_ = true;
    MarkNetworkUpdate();
}

void StaticSprite2D::SetUseHotSpot(bool useHotSpot)
{
    if (useHotSpot == useHotSpot_)
        return;

    useHotSpot_ = useHotSpot;
    sourceBatchesDirty_ = true;
    drawRectDirty_ = true;

    MarkNetworkUpdate();
}

void StaticSprite2D::SetUseDrawRect(bool useDrawRect)
{
    if (useDrawRect == useDrawRect_)
        return;

    useDrawRect_ = useDrawRect;
    sourceBatchesDirty_ = true;

    MarkNetworkUpdate();
}

void StaticSprite2D::SetUseTextureRect(bool useTextureRect)
{
    if (useTextureRect == useTextureRect_)
        return;

    useTextureRect_ = useTextureRect;
    sourceBatchesDirty_ = true;
    MarkNetworkUpdate();
}

void StaticSprite2D::SetHotSpot(const Vector2& hotspot)
{
    if (hotspot == hotSpot_)
        return;

    hotSpot_ = hotspot;

    if (useHotSpot_)
    {
        sourceBatchesDirty_ = true;
        drawRectDirty_ = true;
        MarkNetworkUpdate();
    }
}

void StaticSprite2D::SetHotSpotAttr(const Vector2& hotspot)
{
    if (hotspot != hotSpot_)
    {
        hotSpot_ = hotspot;
        SetUseHotSpot(true);
    }
}

Sprite2D* StaticSprite2D::GetSprite() const
{
    return sprite_;
}

Material* StaticSprite2D::GetCustomMaterial() const
{
    return customMaterial_;
}

void StaticSprite2D::SetCustomMaterial(Material* customMaterial)
{
    if (customMaterial == customMaterial_)
        return;

    customMaterial_ = customMaterial;
    sourceBatchesDirty_ = true;

    UpdateMaterial();
    MarkNetworkUpdate();
}

void StaticSprite2D::SetCustomMaterialAttr(const ResourceRef& value)
{
    ResourceCache* cache = GetSubsystem<ResourceCache>();
    SetCustomMaterial(cache->GetResource<Material>(value.name_));
}

ResourceRef StaticSprite2D::GetCustomMaterialAttr() const
{
    return GetResourceRef(customMaterial_, Material::GetTypeStatic());
}

void StaticSprite2D::SetSpriteAttr(const ResourceRef& value)
{
    Sprite2D* sprite = Sprite2D::LoadFromResourceRef(this, value);
    if (sprite)
        SetSprite(sprite);
}

ResourceRef StaticSprite2D::GetSpriteAttr() const
{
    return Sprite2D::SaveToResourceRef(sprite_);
}

const BoundingBox& StaticSprite2D::GetWorldBoundingBox2D()
{
    if (worldBoundingBoxDirty_)
    {
        OnWorldBoundingBoxUpdate();
        worldBoundingBoxDirty_ = false;
    }
    if (!drawRect_.Defined())
    {
        Vector2 position = node_->GetWorldPosition2D();
        worldBoundingBox_.min_.x_ = position.x_ - 1.f;
        worldBoundingBox_.min_.y_ = position.y_ - 1.f;
        worldBoundingBox_.max_.x_ = position.x_ + 1.f;
        worldBoundingBox_.max_.y_ = position.y_ + 1.f;
        worldBoundingBox_.min_.z_ = 0.f;
        worldBoundingBox_.max_.z_ = 0.f;
    }
    return worldBoundingBox_;
}

void StaticSprite2D::OnWorldBoundingBoxUpdate()
{
    if (!UpdateDrawRectangle())
        return;

    Rect worldDrawRect = Transform2D(drawRect_, Matrix2x3(node_->GetWorldPosition2D(), node_->GetWorldRotation2D(), node_->GetWorldScale2D()));
    worldBoundingBox_.min_.x_ = worldDrawRect.min_.x_;
    worldBoundingBox_.min_.y_ = worldDrawRect.min_.y_;
    worldBoundingBox_.max_.x_ = worldDrawRect.max_.x_;
    worldBoundingBox_.max_.y_ = worldDrawRect.max_.y_;
    worldBoundingBox_.min_.z_ = node_->GetWorldPosition().z_ - 0.5f;
    worldBoundingBox_.max_.z_ = node_->GetWorldPosition().z_ + 0.5f;
    sourceBatchesDirty_ = true;
}

void StaticSprite2D::OnDrawOrderChanged()
{
    sourceBatches_[0].drawOrder_ = GetDrawOrder();
    sourceBatchesDirty_ = true;
}

bool StaticSprite2D::UpdateDrawRectangle()
{
    if (!drawRectDirty_ || useDrawRect_)
        return true;

	if (!sprite_ && !customMaterial_)
        return false;

    if (!useDrawRect_)
    {
        drawRect_.Clear();

        if (sprite_)
        {
            if (useHotSpot_)
            {
                if (!sprite_->GetDrawRectangle(drawRect_, hotSpot_, flipX_, flipY_))
                    return false;
            }
            else if (!sprite_->GetDrawRectangle(drawRect_, flipX_, flipY_))
                    return false;
        }
        else
        {
            Texture* texture = customMaterial_->GetTexture(TU_DIFFUSE);
            if (texture)
            {
                int w = texture->GetWidth();
                int h = texture->GetHeight();
                drawRect_.min_.x_ = -(float)w * PIXEL_SIZE * 0.5f;
                drawRect_.max_.x_ = (float)w * PIXEL_SIZE * 0.5f;
                drawRect_.min_.y_ = -(float)h * PIXEL_SIZE * 0.5f;
                drawRect_.max_.y_ = (float)h * PIXEL_SIZE * 0.5f;
                useDrawRect_ = true;
            }
            else
            {
                URHO3D_LOGERRORF("StaticSprite2D() - UpdateDrawRectangle : node=%s(%u) ... no sprite && no texture in custommaterial !!!",
                                node_->GetName().CString(), node_->GetID());
            }
        }
    }

    drawRectDirty_ = false;
    return true;
}

void StaticSprite2D::UpdateSourceBatches()
{
    if (!sourceBatchesDirty_)
        return;

    if (!StaticSprite2D::UpdateDrawRectangle())
        return;

    Vector<Vertex2D>& vertices = sourceBatches_[0].vertices_;
    vertices.Clear();

    if (!useTextureRect_)
    {
        if (sprite_)
        {
            if (!sprite_->GetTextureRectangle(textureRect_, flipX_, flipY_))
                return;
        }
        else
        {
            textureRect_ = Rect(Vector2::ZERO, Vector2::ONE);
            useDrawRect_ = true;
        }
    }

    Vector4 texmode;
    SetTextureMode(TXM_UNIT, sprite_ ? GetTextureUnit(sourceBatches_[0].material_, sprite_->GetTexture()) : TU_DIFFUSE, texmode);
    SetTextureMode(TXM_FX, textureFX_, texmode);

    /*
    V1---------V2
    |         / |
    |       /   |
    |     /     |
    |   /       |
    | /         |
    V0---------V3
    */
    Vertex2D vertex0;
    Vertex2D vertex1;
    Vertex2D vertex2;
    Vertex2D vertex3;

    // Convert to world space
    Matrix2x3 worldTransfo(node_->GetWorldPosition2D(), node_->GetWorldRotation2D(), node_->GetWorldScale2D());
    worldTransfo.Multiply(Vector2(drawRect_.min_.x_, drawRect_.min_.y_), vertex0.position_);
    worldTransfo.Multiply(Vector2(drawRect_.min_.x_, drawRect_.max_.y_), vertex1.position_);
    worldTransfo.Multiply(Vector2(drawRect_.max_.x_, drawRect_.max_.y_), vertex2.position_);
    worldTransfo.Multiply(Vector2(drawRect_.max_.x_, drawRect_.min_.y_), vertex3.position_);
    vertex0.position_.z_ = vertex1.position_.z_ = vertex2.position_.z_ = vertex3.position_.z_ = node_->GetWorldPosition().z_;

    vertex0.uv_ = textureRect_.min_;
    (swapXY_ ? vertex3.uv_ : vertex1.uv_) = Vector2(textureRect_.min_.x_, textureRect_.max_.y_);
    vertex2.uv_ = textureRect_.max_;
    (swapXY_ ? vertex1.uv_ : vertex3.uv_) = Vector2(textureRect_.max_.x_, textureRect_.min_.y_);

    vertex0.color_ = vertex1.color_ = vertex2.color_ = vertex3.color_ = color_.ToUInt();
	vertex0.texmode_ = vertex1.texmode_ = vertex2.texmode_ = vertex3.texmode_ = texmode;

    vertices.Push(vertex0);
    vertices.Push(vertex1);
    vertices.Push(vertex2);
    vertices.Push(vertex3);

    sourceBatchesDirty_ = false;
}

void StaticSprite2D::UpdateMaterial()
{
    if (customMaterial_)
        sourceBatches_[0].material_ = customMaterial_;
    else
    {
        if (sprite_ && renderer_)
            sourceBatches_[0].material_ = renderer_->GetMaterial(sprite_->GetTexture(), blendMode_);
        else
            sourceBatches_[0].material_ = nullptr;
    }
}

void StaticSprite2D::DrawDebugGeometry(DebugRenderer* debug, bool depthTest)
{
    if (debug && IsEnabledEffective())
    {
        debug->AddNode(node_, 1.f, false);
        debug->AddBoundingBox(worldBoundingBox_, Color::YELLOW, false);
    }
}

