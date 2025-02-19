#pragma once

#include <Urho3D/Graphics/Drawable.h>
#include <Urho3D/Graphics/GraphicsDefs.h>
#include <Urho3D/Graphics/Material.h>

#include "Matrix2x3.h"

namespace Urho3D
{
    class Texture;
    class Texture2D;
    class VertexBuffer;
}

/// Drawable2D FX
enum TextureModeFlag
{
    TXM_UNIT = 0,
    TXM_FX,
    TXM_FX_LIT,
    TXM_FX_CROPALPHA,
    TXM_FX_BLUR,
    TXM_FX_FXAA,
    TXM_FX_TILEINDEX
};

using namespace Urho3D;

class Drawable2D;
class Renderer2D;

/// 2D vertex.
struct Vertex2D
{
    /// Position.
    Vector3 position_;
    /// Color.
    unsigned color_;
    /// UV.
    Vector2 uv_;
    /// Texture Id & Effect
	Vector4 texmode_;
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
extern const float PIXEL_SIZE;

/// Base class for 2D visible components.
class Drawable2D : public Drawable
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
    void SetLayerModifier(int layermodifier);
    /// Set order in layer.
    void SetOrderInLayer(int orderInLayer);

    void SetTextureFX(int effect) { textureFX_ = effect; }
    int GetTextureFX() const { return textureFX_; }

    static void SetTextureMode(TextureModeFlag flag, unsigned value, unsigned& texmode);
    static unsigned GetTextureMode(TextureModeFlag flag, unsigned texmode);
    static void SetTextureMode(TextureModeFlag flag, unsigned value, Vector4& texmode);
    static unsigned GetTextureMode(TextureModeFlag flag, const Vector4& texmode);

    /// Return layer.
    int GetLayer() const { return layer_; }
    int GetLayerModifier() const { return layerModifier_; }

    /// Return order in layer.
    int GetOrderInLayer() const { return orderInLayer_; }

    const Rect& GetDrawRectangle();

    Renderer2D* GetRenderer() const { return renderer_; }

    virtual const BoundingBox& GetWorldBoundingBox2D();

    void MarkDirty();

    /// Return all source batches To Renderer (called by Renderer2D).
    virtual const Vector<SourceBatch2D* >& GetSourceBatchesToRender(Camera* camera);

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
    void UpdateSourceBatchesToRender();
    virtual bool UpdateDrawRectangle();

    /// Return draw order by layer and order in layer.
    int GetDrawOrder() const { return ((layer_ + layerModifier_) << 20) + (orderInLayer_ << 10); }

    /// Layer.
    int layer_;
    int layerModifier_;
    /// Order in layer.
    int orderInLayer_;

    int textureFX_;

    /// DrawRect.
	Rect drawRect_;
    bool drawRectDirty_;

    /// Prepared Internal Source batches.
    Vector<SourceBatch2D> sourceBatches_;
    /// Source batches to be rendered.
    Vector<SourceBatch2D* > sourceBatchesToRender_;

    /// Source batches dirty flag.
    bool sourceBatchesDirty_;

    /// Renderer2D.
    WeakPtr<Renderer2D> renderer_;
};

int GetTextureUnit(Material* material, Texture* texture);

Color MultColors(const Color& c1, const Color& c2);
Color MultColors(const Color& c1, const Color& c2, const Color& c3);

Rect Transform2D(const Rect& rect, const Matrix2x3& transform);

