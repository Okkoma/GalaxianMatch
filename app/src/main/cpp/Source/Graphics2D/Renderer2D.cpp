#include <Urho3D/Urho3D.h>

#include <Urho3D/Core/Context.h>
#include <Urho3D/Core/Profiler.h>
#include <Urho3D/Core/WorkQueue.h>

#include <Urho3D/IO/Log.h>
#include <Urho3D/IO/FileSystem.h>

#include <Urho3D/Graphics/Camera.h>
#include <Urho3D/Graphics/Geometry.h>
#include <Urho3D/Graphics/GraphicsEvents.h>
#include <Urho3D/Graphics/GraphicsImpl.h>
#include <Urho3D/Graphics/IndexBuffer.h>
#include <Urho3D/Graphics/Material.h>
#include <Urho3D/Graphics/OctreeQuery.h>
#include <Urho3D/Graphics/Technique.h>
#include <Urho3D/Graphics/Texture2D.h>
#include <Urho3D/Graphics/VertexBuffer.h>
#include <Urho3D/Graphics/View.h>

#include <Urho3D/Resource/ResourceCache.h>

#include <Urho3D/Scene/Node.h>
#include <Urho3D/Scene/Scene.h>

#include "Drawable2D.h"

#include "Renderer2D.h"


namespace Urho3D
{
    extern const char* blendModeNames[];
}

static const unsigned MASK_VERTEX2D = MASK_POSITION | MASK_COLOR | MASK_TEXCOORD1 | MASK_TANGENT;

static const int TRIANGLE2D = 0;
static const int QUAD2D = 1;

ViewBatchInfo2D::ViewBatchInfo2D() :
    vertexBufferUpdateFrameNumber_(0),
    batchUpdatedFrameNumber_(0),
    batchCount_(0)
{
    for (int i=0; i<2; i++)
    {
        indexCount_[i] = 0;
        vertexCount_[i] = 0;
    }
}

Renderer2D::Renderer2D(Context* context) :
    Drawable(context, DRAWABLE_GEOMETRY),
    initialVertexBufferSize_(8000U),
    material_(new Material(context))
{
    for (int i=0; i<2; i++)
        indexBuffer_[i] = new IndexBuffer(context_);

    material_->SetName("Urho2D");

    Technique* tech = new Technique(context_);
    Pass* pass = tech->CreatePass("alpha");
    pass->SetVertexShader("Urho2D");
    pass->SetPixelShader("Urho2D");
    pass->SetDepthTestMode(CMP_ALWAYS);
    pass->SetDepthWrite(false);
    cachedTechniques_[BLEND_REPLACE] = tech;

    material_->SetTechnique(0, tech);
    material_->SetCullMode(CULL_NONE);

    SubscribeToEvent(E_BEGINVIEWUPDATE, URHO3D_HANDLER(Renderer2D, HandleBeginViewUpdate));
}

Renderer2D::~Renderer2D() = default;

void Renderer2D::RegisterObject(Context* context)
{
    context->RegisterFactory<Renderer2D>();
}

static inline bool CompareRayQueryResults(RayQueryResult& lr, RayQueryResult& rr)
{
    Drawable2D* lhs = static_cast<Drawable2D*>(lr.drawable_);
    Drawable2D* rhs = static_cast<Drawable2D*>(rr.drawable_);
    if (lhs->GetLayer() != rhs->GetLayer())
        return lhs->GetLayer() > rhs->GetLayer();

    if (lhs->GetOrderInLayer() != rhs->GetOrderInLayer())
        return lhs->GetOrderInLayer() > rhs->GetOrderInLayer();

    return lhs->GetID() > rhs->GetID();
}

void Renderer2D::ProcessRayQuery(const RayOctreeQuery& query, PODVector<RayQueryResult>& results)
{
    unsigned resultSize = results.Size();
    for (unsigned i = 0; i < drawables_.Size(); ++i)
    {
        if (drawables_[i]->GetViewMask() & query.viewMask_)
            drawables_[i]->ProcessRayQuery(query, results);
    }

    if (results.Size() != resultSize)
        Sort(results.Begin() + resultSize, results.End(), CompareRayQueryResults);
}

void Renderer2D::UpdateBatches(const FrameInfo& frame)
{
    unsigned count = batches_.Size();

    // Update non-thread critical parts of the source batches
    for (unsigned i = 0; i < count; ++i)
    {
        batches_[i].distance_ = 10.0f + (count - i) * 0.001f;
        batches_[i].worldTransform_ = &Matrix3x4::IDENTITY;
    }
}

void Renderer2D::UpdateGeometry(const FrameInfo& frame)
{
    ViewBatchInfo2D& viewBatchInfo = viewBatchInfos_[frame.camera_];

    // update index buffers
    unsigned indexCount[2] = { 0, 0 };
    for (HashMap<Camera*, ViewBatchInfo2D>::ConstIterator i = viewBatchInfos_.Begin(); i != viewBatchInfos_.End(); ++i)
    {
        if (i->second_.batchUpdatedFrameNumber_ == frame.frameNumber_)
        {
            indexCount[TRIANGLE2D] = (unsigned)Max((int)indexCount[TRIANGLE2D], (int)i->second_.indexCount_[TRIANGLE2D]);
            indexCount[QUAD2D]     = (unsigned)Max((int)indexCount[QUAD2D], (int)i->second_.indexCount_[QUAD2D]);
        }
    }
    // update index buffer triangles
    if (indexBuffer_[TRIANGLE2D]->IsDataLost() || indexBuffer_[TRIANGLE2D]->GetIndexCount() < indexCount[TRIANGLE2D])
    {
        bool largeIndices = indexCount[TRIANGLE2D] > 0xffff;
        indexBuffer_[TRIANGLE2D]->SetSize(indexCount[TRIANGLE2D], largeIndices);

        void* buffer = indexBuffer_[TRIANGLE2D]->Lock(0, indexCount[TRIANGLE2D], true);
        if (buffer)
        {
            unsigned trianglescount = indexCount[TRIANGLE2D] / 3;
            if (largeIndices)
            {
                unsigned* dest = reinterpret_cast<unsigned*>(buffer);
                for (unsigned i = 0; i < trianglescount; ++i)
                {
                    unsigned base = i * 3;
                    dest[0] = base;
                    dest[1] = base + 1;
                    dest[2] = base + 2;
                    dest += 3;
                }
            }
            else
            {
                unsigned short* dest = reinterpret_cast<unsigned short*>(buffer);
                for (unsigned i = 0; i < trianglescount; ++i)
                {
                    unsigned base = i * 3;
                    dest[0] = (unsigned short)(base);
                    dest[1] = (unsigned short)(base + 1);
                    dest[2] = (unsigned short)(base + 2);
                    dest += 3;
                }
            }
            indexBuffer_[TRIANGLE2D]->Unlock();
        }
        else
        {
            indexBuffer_[TRIANGLE2D]->ClearDataLost();
        }
    }
    // update index buffer quads
    if (indexBuffer_[QUAD2D]->IsDataLost() || indexBuffer_[QUAD2D]->GetIndexCount() < indexCount[QUAD2D])
    {
        bool largeIndices = (indexCount[QUAD2D] * 4 / 6) > 0xffff;
        indexBuffer_[QUAD2D]->SetSize(indexCount[QUAD2D], largeIndices);
        void* buffer = indexBuffer_[QUAD2D]->Lock(0, indexCount[QUAD2D], true);
        if (buffer)
        {
            unsigned quadCount = indexCount[QUAD2D] / 6;
            if (largeIndices)
            {
                unsigned* dest = reinterpret_cast<unsigned*>(buffer);
                for (unsigned i = 0; i < quadCount; ++i)
                {
                    unsigned base = i * 4;
                    dest[0] = base;
                    dest[1] = base + 1;
                    dest[2] = base + 2;
                    dest[3] = base;
                    dest[4] = base + 2;
                    dest[5] = base + 3;
                    dest += 6;
                }
            }
            else
            {
                unsigned short* dest = reinterpret_cast<unsigned short*>(buffer);
                for (unsigned i = 0; i < quadCount; ++i)
                {
                    unsigned base = i * 4;
                    dest[0] = (unsigned short)(base);
                    dest[1] = (unsigned short)(base + 1);
                    dest[2] = (unsigned short)(base + 2);
                    dest[3] = (unsigned short)(base);
                    dest[4] = (unsigned short)(base + 2);
                    dest[5] = (unsigned short)(base + 3);
                    dest += 6;
                }
            }
            indexBuffer_[QUAD2D]->Unlock();
        }
        else
        {
            indexBuffer_[QUAD2D]->ClearDataLost();
        }
    }

    if (viewBatchInfo.vertexBufferUpdateFrameNumber_ != frame.frameNumber_)
    {
        // update vertex buffers
		for (int primitiveType = 0; primitiveType < 2; primitiveType++)
		{
			unsigned vertexcount = viewBatchInfo.vertexCount_[primitiveType];
			VertexBuffer* vertexBuffer = viewBatchInfo.vertexBuffer_[primitiveType];

			if (vertexBuffer->GetVertexCount() < vertexcount)
                vertexBuffer->SetSize(vertexcount, MASK_VERTEX2D, true);

            if (vertexcount)
            {
                Vertex2D* dest = reinterpret_cast<Vertex2D*>(vertexBuffer->Lock(0, vertexcount, false));
                if (dest)
                {
                    const PODVector<const SourceBatch2D*>& sourceBatches = viewBatchInfo.sourceBatches_;
                    for (unsigned b = 0; b < sourceBatches.Size(); ++b)
                    {
                        if (sourceBatches[b]->quadvertices_ != primitiveType)
                            continue;

                        const Vector<Vertex2D>& vertices = sourceBatches[b]->vertices_;
                        for (unsigned i = 0; i < vertices.Size(); ++i)
                            dest[i] = vertices[i];
                        dest += vertices.Size();
                    }

                    vertexBuffer->Unlock();
                }
                else
                    URHO3D_LOGERRORF("Renderer2D : Failed to lock vertex buffer prim=%d", primitiveType);
            }
        }

        viewBatchInfo.vertexBufferUpdateFrameNumber_ = frame.frameNumber_;
    }
}

UpdateGeometryType Renderer2D::GetUpdateGeometryType()
{
    return UPDATE_MAIN_THREAD;
}

void Renderer2D::AddDrawable(Drawable2D* drawable)
{
    if (!drawable)
        return;

//    drawables_.Push(drawable);
    /// TEST : reduce the insertions in renderer2D but if same ptr but not same drawable it's problematic
    if (!drawables_.Contains(drawable))
        drawables_.Push(drawable);
}

void Renderer2D::RemoveDrawable(Drawable2D* drawable)
{
    if (!drawable)
        return;

    drawables_.Remove(drawable);
}

Material* Renderer2D::GetMaterial(Texture2D* texture, BlendMode blendMode)
{
    if (!texture)
        return material_;

    HashMap<Texture2D*, HashMap<int, SharedPtr<Material> > >::Iterator t = cachedMaterials_.Find(texture);
    if (t == cachedMaterials_.End())
    {
        SharedPtr<Material> newMaterial(GetSubsystem<ResourceCache>()->GetResource<Material>("Materials/" + GetFileName(texture->GetName()) + String(".xml")));

        if (!newMaterial)
        {
            URHO3D_LOGWARNINGF("Renderer2D() - GetMaterial : no Material => Create Default Urho2D Material !");
            newMaterial = CreateMaterial(texture, blendMode);
        }

        cachedMaterials_[texture][blendMode] = newMaterial;
        return newMaterial;
    }

    HashMap<int, SharedPtr<Material> >& materials = t->second_;
    HashMap<int, SharedPtr<Material> >::Iterator b = materials.Find(blendMode);
    if (b != materials.End())
        return b->second_;

    SharedPtr<Material> newMaterial = CreateMaterial(texture, blendMode);
    materials[blendMode] = newMaterial;

    return newMaterial;
}

bool Renderer2D::CheckVisibility(ViewBatchInfo2D* viewinfo, Drawable2D* drawable) const
{
    if ((viewinfo->frame_.camera_->GetViewMask() & drawable->GetViewMask()) == 0)
        return false;

//    return viewinfo->frustum2D_.Defined() ?
//        viewinfo->frustum2D_.IsInsideFast2D(drawable->GetWorldBoundingBox2D()) != OUTSIDE :
//        viewinfo->frustum_->IsInsideFast(drawable->GetWorldBoundingBox2D()) != OUTSIDE;
    return viewinfo->frustum_->IsInsideFast(drawable->GetWorldBoundingBox2D()) != OUTSIDE;
}

void Renderer2D::OnWorldBoundingBoxUpdate()
{
    // Set a large dummy bounding box to ensure the renderer is rendered
    boundingBox_.Define(-M_LARGE_VALUE, M_LARGE_VALUE);
    worldBoundingBox_ = boundingBox_;
}

SharedPtr<Material> Renderer2D::LoadDefaultMaterial(Texture2D* texture, BlendMode blendMode)
{
//    SharedPtr<Material> loadedMaterial(GetSubsystem<ResourceCache>()->GetResource<Material>("Materials/" + GetFileName(texture->GetName()) + String(".xml")));
//    return loadedMaterial;

    return GetSubsystem<ResourceCache>()->GetTempResource<Material>("Materials/" + GetFileName(texture->GetName()) + String(".xml"));
}

SharedPtr<Material> Renderer2D::CreateMaterial(Texture2D* texture, BlendMode blendMode)
{
    SharedPtr<Material> newMaterial = material_->Clone();

    HashMap<int, SharedPtr<Technique> >::Iterator techIt = cachedTechniques_.Find((int)blendMode);
    if (techIt == cachedTechniques_.End())
    {
        SharedPtr<Technique> tech(new Technique(context_));
        Pass* pass = tech->CreatePass("alpha");
        pass->SetVertexShader("Urho2D");
        pass->SetPixelShader("Urho2D");
        pass->SetDepthWrite(false);
        pass->SetBlendMode(blendMode);
        techIt = cachedTechniques_.Insert(MakePair((int)blendMode, tech));
    }

    newMaterial->SetTechnique(0, techIt->second_.Get());
    newMaterial->SetName(texture->GetName() + "_" + blendModeNames[blendMode]);
    newMaterial->SetTexture(TU_DIFFUSE, texture);

    return newMaterial;
}

void CheckDrawableVisibilityWork(const WorkItem* item, unsigned threadIndex)
{
    ViewBatchInfo2D* viewinfo = reinterpret_cast<ViewBatchInfo2D*>(item->aux_);
    Drawable2D** start = reinterpret_cast<Drawable2D**>(item->start_);
    Drawable2D** end = reinterpret_cast<Drawable2D**>(item->end_);

    while (start != end)
    {
        Drawable2D* drawable = *start++;
        if (drawable->GetRenderer()->CheckVisibility(viewinfo, drawable))
            drawable->MarkInView(viewinfo->frame_);
    }
}

void Renderer2D::HandleBeginViewUpdate(StringHash eventType, VariantMap& eventData)
{
    using namespace BeginViewUpdate;

    // Check that we are updating the correct scene
    if (GetScene() != eventData[P_SCENE].GetPtr())
        return;

    Camera* camera = static_cast<Camera*>(eventData[P_CAMERA].GetPtr());
    if (!camera)
        return;

    URHO3D_PROFILE(UpdateRenderer2D);

    ViewBatchInfo2D& viewBatchInfo = viewBatchInfos_[camera];
    currentViewBatchInfo_        = &viewBatchInfo;
    viewBatchInfo.frame_         = static_cast<View*>(eventData[P_VIEW].GetPtr())->GetFrameInfo();
    viewBatchInfo.frame_.camera_ = camera;
    viewBatchInfo.frustum_       = &camera->GetFrustum();
    if (camera->IsOrthographic() && camera->GetNode()->GetWorldDirection() == Vector3::FORWARD)
        viewBatchInfo.frustum2D_.Define(viewBatchInfo.frustum_->vertices_[2], viewBatchInfo.frustum_->vertices_[4]);
    else
        viewBatchInfo.frustum2D_.Clear();

    // Check visibility
    if (checkVisibility_)
    {
        URHO3D_PROFILE(CheckDrawableVisibility);

        WorkQueue* queue = GetSubsystem<WorkQueue>();
//        int numWorkItems = queue->GetNumThreads() + 1; // Worker threads + main thread

        int numWorkItems = queue->GetNumThreads(); // Worker threads

        if (!numWorkItems) // if no Worker thread, Use Main Thread
            numWorkItems = 1;

        int drawablesPerItem = drawables_.Size() / numWorkItems;

        PODVector<Drawable2D*>::Iterator start = drawables_.Begin();
        for (int i = 0; i < numWorkItems; ++i)
        {
            SharedPtr<WorkItem> item = queue->GetFreeItem();
            item->priority_ = M_MAX_UNSIGNED;
            item->workFunction_ = CheckDrawableVisibilityWork;
            item->aux_ = currentViewBatchInfo_;

            PODVector<Drawable2D*>::Iterator end = drawables_.End();
            if (i < numWorkItems - 1 && end - start > drawablesPerItem)
                end = start + drawablesPerItem;

            item->start_ = &(*start);
            item->end_ = &(*end);
            queue->AddWorkItem(item);

            start = end;
        }

        queue->Complete(M_MAX_UNSIGNED);
    }

    // Create vertex buffer if not allocated
    for (int primitiveType=0; primitiveType<2; primitiveType++)
    {
        if (!viewBatchInfo.vertexBuffer_[primitiveType])
        {
            viewBatchInfo.vertexBuffer_[primitiveType] = new VertexBuffer(context_);
            viewBatchInfo.vertexBuffer_[primitiveType]->SetSize(initialVertexBufferSize_, MASK_VERTEX2D, true);
        }
    }

    UpdateViewBatchInfo(viewBatchInfo);

    // Go through the drawables to form geometries & batches and calculate the total vertex / index count,
    // but upload the actual vertex data later. The idea is that the View class copies our batch vector to
    // its internal data structures, so we can reuse the batches for each view, provided that unique Geometry
    // objects are used for each view to specify the draw ranges
    batches_.Resize(viewBatchInfo.batchCount_);
    for (unsigned i = 0; i < viewBatchInfo.batchCount_; ++i)
    {
        batches_[i].distance_ = viewBatchInfo.distances_[i];
        batches_[i].material_ = viewBatchInfo.materials_[i];
        batches_[i].geometry_ = viewBatchInfo.geometries_[i];
    }
}

void Renderer2D::GetDrawables(PODVector<Drawable2D*>& drawables, Node* node)
{
    if (!node || !node->IsEnabled())
        return;

    const Vector<SharedPtr<Component> >& components = node->GetComponents();
    for (Vector<SharedPtr<Component> >::ConstIterator i = components.Begin(); i != components.End(); ++i)
    {
        Drawable2D* drawable = dynamic_cast<Drawable2D*>(i->Get());
        if (drawable && drawable->IsEnabled())
            drawables.Push(drawable);
    }

    const Vector<SharedPtr<Node> >& children = node->GetChildren();
    for (Vector<SharedPtr<Node> >::ConstIterator i = children.Begin(); i != children.End(); ++i)
        GetDrawables(drawables, i->Get());
}

static inline bool CompareSourceBatch2Ds(const SourceBatch2D* lhs, const SourceBatch2D* rhs)
{
    if (lhs->drawOrder_ != rhs->drawOrder_)
        return lhs->drawOrder_ < rhs->drawOrder_;

    if (lhs->distance_ != rhs->distance_)
        return lhs->distance_ > rhs->distance_;

    if (lhs->material_ != rhs->material_)
        return lhs->material_->GetNameHash() < rhs->material_->GetNameHash();

    if (lhs->quadvertices_ != rhs->quadvertices_)
        return lhs->quadvertices_;

    return lhs < rhs;
}

void Renderer2D::UpdateViewBatchInfo(ViewBatchInfo2D& viewBatchInfo)
{
    // Already update in same frame ?
    if (viewBatchInfo.batchUpdatedFrameNumber_ == viewBatchInfo.frame_.frameNumber_)
        return;

    Camera* camera = viewBatchInfo.frame_.camera_;

    PODVector<const SourceBatch2D*>& sourceBatches = viewBatchInfo.sourceBatches_;
    sourceBatches.Clear();
    for (unsigned d = 0; d < drawables_.Size(); ++d)
    {
        Drawable2D* drawable = drawables_[d];

        const Vector<SourceBatch2D*>& batches = drawable->GetSourceBatchesToRender(camera);
        if (batches.Empty())
        {
            URHO3D_LOGWARNINGF("Renderer2D : no source batch to render for drawable=%s", drawable->GetNode() ? drawable->GetNode()->GetName().CString() : "");
            continue;
        }

        for (unsigned b = 0; b < batches.Size(); ++b)
        {
            const SourceBatch2D* batch = batches[b];
            if (batch && batch->material_ && batch->owner_ && !batch->vertices_.Empty())
            {
                sourceBatches.Push(batch);
                sourceBatches.Back()->distance_ = camera->GetDistance(batch->owner_->GetNode()->GetWorldPosition());
            }
        }
    }

    Sort(sourceBatches.Begin(), sourceBatches.End(), CompareSourceBatch2Ds);

    viewBatchInfo.batchCount_ = 0;
    Material* currMaterial = nullptr;

    unsigned iStart[2] = { 0, 0 };
    unsigned iCount[2] = { 0, 0 };
    unsigned vStart[2] = { 0, 0 };
    unsigned vCount[2] = { 0, 0 };
    int currType = sourceBatches.Size() ? sourceBatches[0]->quadvertices_ : QUAD2D;
	float distance = M_INFINITY;

    for (unsigned b = 0; b < sourceBatches.Size(); ++b)
    {
		distance = Min(distance, sourceBatches[b]->distance_);
        Material* material = sourceBatches[b]->material_;
        int primitiveType = sourceBatches[b]->quadvertices_;
        const Vector<Vertex2D>& vertices = sourceBatches[b]->vertices_;

        // When new material encountered, finish the current batch and start new
        if (currMaterial != material || currType != primitiveType)
        {
            if (currMaterial)
            {
                AddViewBatch(viewBatchInfo, currType, currMaterial, iStart[currType], iCount[currType],
								vStart[currType], vCount[currType], distance);
                iStart[primitiveType] += iCount[primitiveType];
                iCount[primitiveType] = 0;
                vStart[primitiveType] += vCount[primitiveType];
                vCount[primitiveType] = 0;
				distance = M_INFINITY;
            }

            currMaterial = material;
            currType = primitiveType;
        }

        if (currType == QUAD2D)
            iCount[currType] += vertices.Size() * 6 / 4;
        else
            iCount[currType] += vertices.Size();

        vCount[currType] += vertices.Size();
    }

    // Add the final batch if necessary
    if (currMaterial && vCount[currType])
        AddViewBatch(viewBatchInfo, currType, currMaterial, iStart[currType], iCount[currType],
						vStart[currType], vCount[currType], distance);

    for (int primitiveType=0; primitiveType<2; primitiveType++)
    {
        viewBatchInfo.indexCount_[primitiveType]  = iStart[primitiveType] + iCount[primitiveType];
        viewBatchInfo.vertexCount_[primitiveType] = vStart[primitiveType] + vCount[primitiveType];
    }

    viewBatchInfo.batchUpdatedFrameNumber_ = viewBatchInfo.frame_.frameNumber_;
}

void Renderer2D::AddViewBatch(ViewBatchInfo2D& viewBatchInfo, int primitivetype, Material* material, unsigned indexStart, unsigned indexCount,
    unsigned vertexStart, unsigned vertexCount, float distance)
{
    if (!material || indexCount == 0 || vertexCount == 0)
        return;

    if (viewBatchInfo.materials_.Size() <= viewBatchInfo.batchCount_)
    {
        viewBatchInfo.materials_.Resize(viewBatchInfo.batchCount_ + 1);
        viewBatchInfo.distances_.Resize(viewBatchInfo.batchCount_ + 1);
    }
    viewBatchInfo.materials_[viewBatchInfo.batchCount_] = material;
	viewBatchInfo.distances_[viewBatchInfo.batchCount_] = distance;

    // Allocate new geometry if necessary
    if (viewBatchInfo.geometries_.Size() <= viewBatchInfo.batchCount_)
    {
        SharedPtr<Geometry> geometry(new Geometry(context_));
        viewBatchInfo.geometries_.Push(geometry);
    }

    Geometry* geometry = viewBatchInfo.geometries_[viewBatchInfo.batchCount_];
    geometry->SetIndexBuffer(indexBuffer_[primitivetype]);
    geometry->SetVertexBuffer(0, viewBatchInfo.vertexBuffer_[primitivetype]);
    geometry->SetDrawRange(TRIANGLE_LIST, indexStart, indexCount, vertexStart, vertexCount, false);

    viewBatchInfo.batchCount_++;
}

void Renderer2D::Dump() const
{
    URHO3D_LOGINFOF("Renderer2D() - Dump : numviewBatchInfos_=%u", viewBatchInfos_.Size(), drawables_.Size());
    unsigned index = 0;
    const ViewBatchInfo2D& firstviewbatch = viewBatchInfos_.Begin()->second_;
    Camera* firstcamera = viewBatchInfos_.Begin()->first_;
    for (HashMap<Camera*, ViewBatchInfo2D>::ConstIterator it = viewBatchInfos_.Begin(); it != viewBatchInfos_.End(); ++it, index++)
    {
        const ViewBatchInfo2D& viewbatchinfo = it->second_;
        URHO3D_LOGINFOF("Renderer2D() - Dump : viewBatchInfo[%d] camera=%u frustum2D=%u viewbatchinfoptr=%u numbatches=%u",
                        index, it->first_, viewbatchinfo.frustum_, &viewbatchinfo, viewbatchinfo.batchCount_);

		for (unsigned i=0; i < viewbatchinfo.sourceBatches_.Size(); ++i)
			URHO3D_LOGINFOF("  sourceBatches_[%d] dist=%f", i, viewbatchinfo.sourceBatches_[i]->distance_);
    }
	URHO3D_LOGINFOF("Renderer2D() - Dump : firstiewbatchinfoptr=%u", &firstviewbatch);
    for (unsigned i=0; i < drawables_.Size(); i++)
    {
        bool visibility = CheckVisibility((ViewBatchInfo2D*)&firstviewbatch, drawables_[i]);
        if (visibility)
            URHO3D_LOGINFOF("   drawable[%d] type=%s node=%s(%u) dist=%f visible=%s drect=%s numbatches=%u", i,
                            drawables_[i]->GetTypeName().CString(),
                            drawables_[i]->GetNode()->GetName().CString(), drawables_[i]->GetNode()->GetID(),
                            drawables_[i]->sourceBatches_[0].distance_,
                            visibility ? "true" : "false", drawables_[i]->GetDrawRectangle().ToString().CString(),
                            drawables_[i]->GetSourceBatchesToRender(firstcamera).Size());
    }
}

