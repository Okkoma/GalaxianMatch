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
//#include "../IO/Log.h"
#include "../Core/Context.h"

#include "../Graphics/Camera.h"
#include "../Graphics/Material.h"
#include "../Graphics/Texture2D.h"
#include "../Scene/Scene.h"
#include "../Urho2D/Drawable2D.h"
#include "../Urho2D/Renderer2D.h"

#include "../DebugNew.h"

namespace Urho3D
{

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
    orderInLayer_(0),
    sourceBatchesDirty_(false),
    drawRectDirty_(true),
    visibility_(true)
{
    worldBoundingBox_.min_.z_ = worldBoundingBox_.max_.z_ = 0.f;
}

Drawable2D::~Drawable2D()
{
    if (renderer_)
        renderer_->RemoveDrawable(this);
}

void Drawable2D::RegisterObject(Context* context)
{
    URHO3D_ACCESSOR_ATTRIBUTE("Layer", GetLayer, SetLayer, int, 0, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Order in Layer", GetOrderInLayer, SetOrderInLayer, int, 0, AM_DEFAULT);
    URHO3D_ATTRIBUTE("View Mask", int, viewMask_, DEFAULT_VIEWMASK, AM_DEFAULT);
}

void Drawable2D::OnSetEnabled()
{
    bool enabled = IsEnabledEffective();

    if (enabled)
    {
        visibility_ = worldBoundingBoxDirty_ = true;
        if (renderer_)
            renderer_->AddDrawable(this);
    }
    else
    {
        if (renderer_)
            renderer_->RemoveDrawable(this);

        sourceBatchesDirty_ = worldBoundingBoxDirty_ = visibility_ = false;
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

void Drawable2D::SetOrderInLayer(int orderInLayer)
{
    if (orderInLayer == orderInLayer_)
        return;

    orderInLayer_ = orderInLayer;

    OnDrawOrderChanged();
    MarkNetworkUpdate();
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
        Rect worldDrawRect = drawRect_.Transformed(node_->GetWorldTransform2D());
        worldBoundingBox_.min_.x_ = worldDrawRect.min_.x_;
        worldBoundingBox_.min_.y_ = worldDrawRect.min_.y_;
        worldBoundingBox_.max_.x_ = worldDrawRect.max_.x_;
        worldBoundingBox_.max_.y_ = worldDrawRect.max_.y_;
        worldBoundingBoxDirty_ = false;
    }
}

void Drawable2D::ClearSourceBatches()
{
//    URHO3D_LOGINFOF("Drawable2D() - ClearSourceBatches : node=%s(%u) enabled=%s visibility_=%s ",
//             node_->GetName().CString(), node_->GetID(), IsEnabledEffective() ? "true" : "false", visibility_ ? "true" : "false");

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

//const Vector<SourceBatch2D>& Drawable2D::GetSourceBatches()
//{
//    if (sourceBatchesDirty_)
//        UpdateSourceBatches();
//
//    return sourceBatches_;
//}

const Vector<SourceBatch2D*>& Drawable2D::GetSourceBatchesToRender()
{
//    URHO3D_LOGINFOF("Drawable2D() - GetSourceBatchesToRender : node=%s ... dirty=%s",
//                    node_->GetName().CString(), sourceBatchesDirty_ ? "true" : "false");

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

}
