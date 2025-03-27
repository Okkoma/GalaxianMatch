#pragma once

#include <Urho3D/Graphics/Drawable.h>

namespace Urho3D
{
    struct FrameInfo;
    class Material;
    class Technique;
    class IndexBuffer;
    class VertexBuffer;
    class Texture2D;
}

class Drawable2D;
struct SourceBatch2D;

using namespace Urho3D;

/// 2D view batch info.
struct ViewBatchInfo2D
{
    /// Construct.
    ViewBatchInfo2D();

    /// Vertex buffer update frame number.
    unsigned vertexBufferUpdateFrameNumber_;
    /// Index count.
    unsigned indexCount_[2];
    /// Vertex count.
    unsigned vertexCount_[2];
    /// Vertex buffer.
    SharedPtr<VertexBuffer> vertexBuffer_[2];
    /// Batch updated frame number.
    unsigned batchUpdatedFrameNumber_;
    /// Source batches.
    PODVector<const SourceBatch2D*> sourceBatches_;
    /// Batch count.
    unsigned batchCount_;
    /// Distances.
    PODVector<float> distances_;
    /// Materials.
    Vector<SharedPtr<Material> > materials_;
    /// Geometries.
    Vector<SharedPtr<Geometry> > geometries_;

    FrameInfo frame_;
    const Frustum* frustum_;
    BoundingBox frustum2D_;
};

/// 2D renderer component.
class Renderer2D : public Drawable
{
    URHO3D_OBJECT(Renderer2D, Drawable);

    friend void CheckDrawableVisibilityWork(const WorkItem* item, unsigned threadIndex);

public:
    /// Construct.
    explicit Renderer2D(Context* context);
    /// Destruct.
    ~Renderer2D() override;
    /// Register object factory.
    /// @nobind
    static void RegisterObject(Context* context);

    void SetInitialVertexBufferSize(unsigned size) { initialVertexBufferSize_ = size; }

    /// Process octree raycast. May be called from a worker thread.
    void ProcessRayQuery(const RayOctreeQuery& query, PODVector<RayQueryResult>& results) override;
    /// Calculate distance and prepare batches for rendering. May be called from worker thread(s), possibly re-entrantly.
    void UpdateBatches(const FrameInfo& frame) override;
    /// Prepare geometry for rendering. Called from a worker thread if possible (no GPU update).
    void UpdateGeometry(const FrameInfo& frame) override;
    /// Return whether a geometry update is necessary, and if it can happen in a worker thread.
    UpdateGeometryType GetUpdateGeometryType() override;

    /// Add Drawable2D.
    void AddDrawable(Drawable2D* drawable);
    /// Remove Drawable2D.
    void RemoveDrawable(Drawable2D* drawable);

    /// Return material by texture and blend mode.
    Material* GetMaterial(Texture2D* texture, BlendMode blendMode);

    const FrameInfo& GetCurrentFrameInfo() const { return currentViewBatchInfo_->frame_; }

    /// Check visibility.
    void SetCheckVisibility(bool enable) { checkVisibility_ = enable; }
    bool CheckVisibility(ViewBatchInfo2D* viewinfo,  Drawable2D* drawable) const;

    void Dump() const;

private:
    /// Recalculate the world-space bounding box.
    void OnWorldBoundingBoxUpdate() override;
    /// Load default material for a texture and blend mode from material directory
    SharedPtr<Material> LoadDefaultMaterial(Texture2D* texture, BlendMode blendMode);

    /// Create material by texture and blend mode.
    SharedPtr<Material> CreateMaterial(Texture2D* texture, BlendMode blendMode);

    /// Handle view update begin event. Determine Drawable2D's and their batches here.
    void HandleBeginViewUpdate(StringHash eventType, VariantMap& eventData);
    /// Get all drawables in node.
    void GetDrawables(PODVector<Drawable2D*>& drawables, Node* node);
    /// Update view batch info.
    void UpdateViewBatchInfo(ViewBatchInfo2D& viewBatchInfo);
    /// Add view batch.
    void AddViewBatch(ViewBatchInfo2D& viewBatchInfo, int primitivetype, Material* material, unsigned indexStart, 
						unsigned indexCount, unsigned vertexStart, unsigned vertexCount, float distance);

    unsigned initialVertexBufferSize_;

    /// Index buffer.
    SharedPtr<IndexBuffer> indexBuffer_[2];
    /// Material.
    SharedPtr<Material> material_;
    /// Drawables.
    PODVector<Drawable2D*> drawables_;
    /// View batch info.
    HashMap<Camera*, ViewBatchInfo2D> viewBatchInfos_;
    ViewBatchInfo2D* currentViewBatchInfo_;
    /// Cached materials.
    HashMap<Texture2D*, HashMap<int, SharedPtr<Material> > > cachedMaterials_;
    /// Cached techniques per blend mode.
    HashMap<int, SharedPtr<Technique> > cachedTechniques_;

    bool checkVisibility_{true};
};

