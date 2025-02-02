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

#pragma once

#include "../Graphics/Drawable.h"
#include "../Graphics/GraphicsDefs.h"

namespace Urho3D
{

class Drawable2D;
class Renderer2D;
class Texture2D;
class VertexBuffer;

/// 2D vertex.
struct Vertex2D
{
    /// Position.
    Vector3 position_;
    /// Color.
    unsigned color_;
    /// UV.
    Vector2 uv_;
};

/// 2D source batch.
struct SourceBatch2D
{
    /// Construct.
    SourceBatch2D();

    /// Owner.
    WeakPtr<Drawable2D> owner_;
    /// Distance to camera.
    mutable float distance_;
    /// Draw order.
    int drawOrder_;
    /// Material.
    SharedPtr<Material> material_;
    /// Triangle or Quad Vertices ? (base 3 or 4)
    bool quadvertices_;
    /// Vertices.
    Vector<Vertex2D> vertices_;
};

/// Pixel size (equal 0.01f).
extern URHO3D_API const float PIXEL_SIZE;

/// Base class for 2D visible components.
class URHO3D_API Drawable2D : public Drawable
{
    URHO3D_OBJECT(Drawable2D, Drawable);

public:
    /// Construct.
    Drawable2D(Context* context);
    /// Destruct.
    ~Drawable2D();
    /// Register object factory. Drawable must be registered first.
    static void RegisterObject(Context* context);

    /// Handle enabled/disabled state change.
    virtual void OnSetEnabled();

    /// Set layer.
    void SetLayer(int layer);
    /// Set order in layer.
    void SetOrderInLayer(int orderInLayer);

    /// Return layer.
    int GetLayer() const { return layer_; }

    /// Return order in layer.
    int GetOrderInLayer() const { return orderInLayer_; }

    const Rect& GetDrawRectangle();

    Renderer2D* GetRenderer() const { return renderer_; }

    virtual const BoundingBox& GetWorldBoundingBox2D();

    void MarkDirty();

//    /// Return all source batches (called by Renderer2D).
//    const Vector<SourceBatch2D>& GetSourceBatches();
    /// Return all source batches To Renderer (called by Renderer2D).
    const Vector<SourceBatch2D* >& GetSourceBatchesToRender();
//    virtual BoundingBox GetFixedWorldBoundingBox();

    void ForceUpdateBatches();

    void ClearSourceBatches();

protected:
    /// Handle scene being assigned.
    virtual void OnSceneSet(Scene* scene);
    /// Handle node transform being dirtied.
    virtual void OnMarkedDirty(Node* node);
    /// Handle draw order changed.
    virtual void OnDrawOrderChanged() = 0;

    /// Update source batches.
    virtual void UpdateSourceBatches() = 0;
    virtual void UpdateSourceBatchesToRender();
    virtual bool UpdateDrawRectangle();

    /// Return draw order by layer and order in layer.
    int GetDrawOrder() const { return (layer_ << 20) + (orderInLayer_ << 10); }

    /// Layer.
    int layer_;
    /// Order in layer.
    int orderInLayer_;

    /// DrawRect.
	Rect drawRect_;
    bool drawRectDirty_;

    /// Visibility.
	bool visibility_;

    /// Source batches.
    Vector<SourceBatch2D> sourceBatches_;
    Vector<SourceBatch2D*> sourceBatchesToRender_;

    /// Source batches dirty flag.
    bool sourceBatchesDirty_;

    /// Renderer2D.
    WeakPtr<Renderer2D> renderer_;
};

}
