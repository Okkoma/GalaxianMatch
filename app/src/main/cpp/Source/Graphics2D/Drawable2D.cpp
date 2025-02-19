#include <Urho3D/Urho3D.h>

#include <Urho3D/Core/Context.h>
#include <Urho3D/IO/Log.h>

#include <Urho3D/Graphics/Camera.h>
#include <Urho3D/Graphics/Material.h>
#include <Urho3D/Graphics/Texture2D.h>

#include <Urho3D/Scene/Scene.h>

#include "Renderer2D.h"

#include "Drawable2D.h"


const float PIXEL_SIZE = 0.01f;

SourceBatch2D::SourceBatch2D() :
    distance_(0.0f),
    drawOrder_(0),
    quadvertices_(true)
{
}

Drawable2D::Drawable2D(Context* context) :
    Drawable(context, DRAWABLE_GEOMETRY2D),
    layer_(0),
    layerModifier_(0),
    orderInLayer_(0),
    textureFX_(0),
    sourceBatchesDirty_(false),
    drawRect_(Rect::ZERO),
    drawRectDirty_(true)
{
    worldBoundingBox_.min_.z_ = 0.f;
    worldBoundingBox_.max_.z_ = 1.f;
}

Drawable2D::~Drawable2D()
{
    if (renderer_)
        renderer_->RemoveDrawable(this);
}

void Drawable2D::RegisterObject(Context* context)
{
    URHO3D_ACCESSOR_ATTRIBUTE("Layer", GetLayer, SetLayer, int, 0, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Layer Modifier", GetLayerModifier, SetLayerModifier, int, 0, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Order in Layer", GetOrderInLayer, SetOrderInLayer, int, 0, AM_DEFAULT);
    URHO3D_ATTRIBUTE("View Mask", int, viewMask_, DEFAULT_VIEWMASK, AM_DEFAULT);
    URHO3D_ATTRIBUTE("TextureFx", int, textureFX_, 0, AM_DEFAULT);
}

void Drawable2D::OnSetEnabled()
{
    bool enabled = IsEnabledEffective();

    if (enabled)
    {
        sourceBatchesDirty_ = worldBoundingBoxDirty_ = true;
        if (renderer_)
            renderer_->AddDrawable(this);
    }
    else
    {
        if (renderer_)
            renderer_->RemoveDrawable(this);

        sourceBatchesDirty_ = worldBoundingBoxDirty_ = false;
        ClearSourceBatches();
    }
}

void Drawable2D::SetLayer(int layer)
{
    if (layer == layer_)
        return;

    layer_ = layer;

    OnDrawOrderChanged();
    MarkNetworkUpdate();
}

void Drawable2D::SetLayerModifier(int layermodifier)
{
    if (layermodifier == layerModifier_)
        return;

    layerModifier_ = layermodifier;

    OnDrawOrderChanged();
    MarkNetworkUpdate();
}

void Drawable2D::SetOrderInLayer(int orderInLayer)
{
    if (orderInLayer == orderInLayer_)
        return;

    orderInLayer_ = orderInLayer;

    OnDrawOrderChanged();
    MarkNetworkUpdate();
}

void Drawable2D::SetTextureMode(TextureModeFlag flag, unsigned value, Vector4& texmode)
{
    if (flag == TXM_UNIT)
    {
        texmode.x_ = value & 0xF;
    }
    else if (flag == TXM_FX)
    {
        texmode.y_ = value & 0x1; // bit 0
        texmode.z_ = (value & 0xE) >> 1; // bit 1-2-3
        texmode.w_ = value >> 4; // bit 4-5
    }
    else if (flag == TXM_FX_LIT)
    {
        texmode.y_ = value & 0x1; // bit 0
    }
}

unsigned Drawable2D::GetTextureMode(TextureModeFlag flag, const Vector4& texmode)
{
    if (flag == TXM_UNIT)
        return texmode.x_;
    else if (flag == TXM_FX)
        return texmode.y_;
    else if (flag == TXM_FX_LIT)
        return unsigned(texmode.y_) & 0x1;
    return 0U;
}

const Rect& Drawable2D::GetDrawRectangle()
{
    bool ok = UpdateDrawRectangle();
    return drawRect_;
}

const BoundingBox& Drawable2D::GetWorldBoundingBox2D()
{
    return Drawable::GetWorldBoundingBox();
}

bool Drawable2D::UpdateDrawRectangle()
{
    drawRectDirty_ = false;
    return true;
}

void Drawable2D::ForceUpdateBatches()
{
    sourceBatchesDirty_ = drawRectDirty_ = true;

    UpdateSourceBatchesToRender();

    if (drawRect_.Defined() && worldBoundingBoxDirty_)
    {
        Rect worldDrawRect = Transform2D(drawRect_, Matrix2x3(node_->GetWorldPosition2D(), node_->GetWorldRotation2D(), node_->GetWorldScale2D()));
        worldBoundingBox_.min_.x_ = worldDrawRect.min_.x_;
        worldBoundingBox_.min_.y_ = worldDrawRect.min_.y_;
        worldBoundingBox_.max_.x_ = worldDrawRect.max_.x_;
        worldBoundingBox_.max_.y_ = worldDrawRect.max_.y_;
        worldBoundingBoxDirty_ = false;
    }
}

void Drawable2D::ClearSourceBatches()
{
    sourceBatchesToRender_.Clear();

    for (unsigned i=0; i < sourceBatches_.Size(); i++)
        sourceBatches_[i].vertices_.Clear();
}

void Drawable2D::UpdateSourceBatchesToRender()
{
    UpdateSourceBatches();

    sourceBatchesToRender_.Clear();
    for (unsigned i=0; i < sourceBatches_.Size(); i++)
        sourceBatchesToRender_.Push(&(sourceBatches_[i]));
}

const Vector<SourceBatch2D*>& Drawable2D::GetSourceBatchesToRender(Camera* camera)
{
    if (sourceBatchesDirty_)
        UpdateSourceBatchesToRender();

    return sourceBatchesToRender_;
}

void Drawable2D::OnSceneSet(Scene* scene)
{
    // Do not call Drawable::OnSceneSet(node), as 2D drawable components should not be added to the octree
    // but are instead rendered through Renderer2D
    if (scene)
    {
        renderer_ = scene->GetOrCreateComponent<Renderer2D>(LOCAL);

        if (IsEnabledEffective())
            renderer_->AddDrawable(this);
    }
    else
    {
        if (renderer_)
            renderer_->RemoveDrawable(this);
    }
}

void Drawable2D::MarkDirty()
{
    OnMarkedDirty(node_);
}

void Drawable2D::OnMarkedDirty(Node* node)
{
    sourceBatchesDirty_ = worldBoundingBoxDirty_ = true;
}

int GetTextureUnit(Material* material, Texture* texture)
{
    const HashMap<TextureUnit, SharedPtr<Texture> >& textures = material->GetTextures();
    for (HashMap<TextureUnit, SharedPtr<Texture> >::ConstIterator it = textures.Begin(); it != textures.End(); ++it)
        if (it->second_.Get() == texture)
            return (int)it->first_;
    return -1;
}

Color MultColors(const Color& c1, const Color& c2)
{
    return Color(c1.r_ * c2.r_, c1.g_ * c2.g_, c1.b_ * c2.b_, c1.a_ * c2.a_);
}

Color MultColors(const Color& c1, const Color& c2, const Color& c3)
{
    return Color(c1.r_ * c2.r_ * c3.r_, c1.g_ * c2.g_ * c3.g_, c1.b_ * c2.b_ * c3.b_, c1.a_ * c2.a_ * c3.a_);
}

Rect Transform2D(const Rect& rect, const Matrix2x3& transform)
{
    Vector2 oldEdge = rect.Size() * 0.5f;
    Vector2 newEdge = Vector2(Abs(transform.m00_) * oldEdge.x_ + Abs(transform.m01_) * oldEdge.y_,
                              Abs(transform.m10_) * oldEdge.x_ + Abs(transform.m11_) * oldEdge.y_);
    Vector2 newCenter = transform * rect.Center();
    return Rect(newCenter - newEdge, newCenter + newEdge);
}

