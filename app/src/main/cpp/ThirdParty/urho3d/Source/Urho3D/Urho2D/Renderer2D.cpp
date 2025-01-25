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
#include "../Core/Profiler.h"
#include "../Core/WorkQueue.h"
#include "../Graphics/Camera.h"
#include "../Graphics/Geometry.h"
#include "../Graphics/GraphicsEvents.h"
#include "../Graphics/IndexBuffer.h"
#include "../Graphics/Material.h"
#include "../Graphics/OctreeQuery.h"
#include "../Graphics/Technique.h"
#include "../Graphics/Texture2D.h"
#include "../Graphics/VertexBuffer.h"
#include "../Graphics/View.h"
#include "../Resource/ResourceCache.h"
#include "../IO/FileSystem.h"
#include "../IO/Log.h"
#include "../Scene/Node.h"
#include "../Scene/Scene.h"
#include "../Urho2D/Drawable2D.h"
#include "../Urho2D/Renderer2D.h"

#include "../DebugNew.h"

namespace Urho3D
{

extern const char* blendModeNames[];

static const unsigned MASK_VERTEX2D = MASK_POSITION | MASK_COLOR | MASK_TEXCOORD1;

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
    material_(new Material(context)),
//    indexBuffer_(new IndexBuffer(context_)),
    frustum_(0),
    viewMask_(DEFAULT_VIEWMASK),
    orthographicMode_(true),
    resetcamera_(0)
{
    for (int i=0; i<2; i++)
        indexBuffer_[i] = new IndexBuffer(context_);

    material_->SetName("Urho2D");

    Technique* tech = new Technique(context_);
    Pass* pass = tech->CreatePass("alpha");
    pass->SetVertexShader("Urho2D");
    pass->SetPixelShader("Urho2D");
    pass->SetDepthWrite(false);
    cachedTechniques_[BLEND_REPLACE] = tech;

    material_->SetTechnique(0, tech);
    material_->SetCullMode(CULL_NONE);

    frame_.frameNumber_ = 0;
    SubscribeToEvent(E_BEGINVIEWUPDATE, URHO3D_HANDLER(Renderer2D, HandleBeginViewUpdate));
    SubscribeToEvent(E_CAMERAFRUSTUMUPDATED, URHO3D_HANDLER(Renderer2D, HandleCameraFrustumChanged));
}

Renderer2D::~Renderer2D()
{
}

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
    // update index buffers
    unsigned indexCount[2] = { 0, 0 };
    for (HashMap<Camera*, ViewBatchInfo2D>::ConstIterator i = viewBatchInfos_.Begin(); i != viewBatchInfos_.End(); ++i)
    {
        if (i->second_.batchUpdatedFrameNumber_ == frame_.frameNumber_)
        {
            indexCount[TRIANGLE2D] = (unsigned)Max((int)indexCount[TRIANGLE2D], (int)i->second_.indexCount_[TRIANGLE2D]);
            indexCount[QUAD2D]     = (unsigned)Max((int)indexCount[QUAD2D], (int)i->second_.indexCount_[QUAD2D]);
        }
    }
    // update index buffer triangles
    if (indexBuffer_[TRIANGLE2D]->IsDataLost() || indexBuffer_[TRIANGLE2D]->GetIndexCount() < indexCount[TRIANGLE2D])
    {
//        if (indexBuffer_[TRIANGLE2D]->IsDataLost())
//            URHO3D_LOGERROR("Renderer2D : TRIANGLE2D DataLost !");
//        else
//            URHO3D_LOGWARNINGF("Renderer2D : New IndexCount for TRIANGLE2D new=%u old=%u !", indexCount[TRIANGLE2D], indexBuffer_[TRIANGLE2D]->GetIndexCount());

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
//            URHO3D_LOGERROR("Renderer2D : Failed to lock index buffer for TRIANGLE2D");
            indexBuffer_[TRIANGLE2D]->ClearDataLost();
//            return;
        }
    }
    // update index buffer quads
    if (indexBuffer_[QUAD2D]->IsDataLost() || indexBuffer_[QUAD2D]->GetIndexCount() < indexCount[QUAD2D])
    {
//        if (indexBuffer_[QUAD2D]->IsDataLost())
//            URHO3D_LOGERROR("Renderer2D : QUAD2D DataLost !");
//        else
//            URHO3D_LOGWARNINGF("Renderer2D : New IndexCount for QUAD2D new=%u old=%u !", indexCount[QUAD2D], indexBuffer_[QUAD2D]->GetIndexCount());

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
//            URHO3D_LOGERROR("Renderer2D : Failed to lock index buffer for QUAD2D");
            indexBuffer_[QUAD2D]->ClearDataLost();
//            return;
        }
    }

    Camera* camera = frame.camera_;
    ViewBatchInfo2D& viewBatchInfo = viewBatchInfos_[camera];

    if (viewBatchInfo.vertexBufferUpdateFrameNumber_ != frame_.frameNumber_)
    {
        // update vertex buffers
        for (int primitiveType=0; primitiveType<2; primitiveType++)
        {
            VertexBuffer* vertexBuffer = viewBatchInfo.vertexBuffer_[primitiveType];
            if (vertexBuffer->GetVertexCount() < viewBatchInfo.vertexCount_[primitiveType])
                vertexBuffer->SetSize(viewBatchInfo.vertexCount_[primitiveType], MASK_VERTEX2D, true);
            if (viewBatchInfo.vertexCount_[primitiveType])
            {
                Vertex2D* dest = reinterpret_cast<Vertex2D*>(vertexBuffer->Lock(0, viewBatchInfo.vertexCount_[primitiveType], true));
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
                    URHO3D_LOGERRORF("Renderer2D : Failed to lock vertex buffer %d", primitiveType);
            }
        }
        viewBatchInfo.vertexBufferUpdateFrameNumber_ = frame_.frameNumber_;
    }
}

/*
void Renderer2D::UpdateGeometry(const FrameInfo& frame)
{
    ViewBatchInfo2D& viewBatchInfo = viewBatchInfos_[frame.camera_];
    unsigned indexCount = viewBatchInfo.indexCount_;
    unsigned vertexCount = viewBatchInfo.vertexCount_;
    geometryDirty_ = geometryDirty_ || indexBuffer_->IsDataLost() || indexBuffer_->GetIndexCount() != indexCount
                        || viewBatchInfo.numquadvertices_ != viewBatchInfo.lastnumquadvertices_;

    // Fill index buffer
    if (geometryDirty_)
    {
        bool largeIndices = indexCount > 0xffff;
        indexBuffer_->SetSize(indexCount, largeIndices);

        viewBatchInfo.lastnumquadvertices_ = viewBatchInfo.numquadvertices_;

        void* buffer = indexBuffer_->Lock(0, indexCount, true);
        if (buffer)
        {
            const PODVector<const SourceBatch2D*>& sourceBatches = viewBatchInfo.sourceBatches_;

            if (largeIndices)
            {
                unsigned* dest = reinterpret_cast<unsigned*>(buffer);
                unsigned vertexCounter = 0;
                for (unsigned b = 0; b < sourceBatches.Size(); ++b)
                {
                    if (sourceBatches[b]->quadvertices_)
                    {
                        unsigned numquads = sourceBatches[b]->vertices_.Size() / 4;
                        for (unsigned i = 0; i < numquads; ++i)
                        {
                            unsigned base = vertexCounter + i * 4;
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
                        unsigned numtriangles = sourceBatches[b]->vertices_.Size() / 3;
                        for (unsigned i = 0; i < numtriangles; ++i)
                        {
                            unsigned base = vertexCounter + i * 3;
                            dest[0] = base;
                            dest[1] = base + 1;
                            dest[2] = base + 2;
                            dest += 3;
                        }
                    }
                    vertexCounter += sourceBatches[b]->vertices_.Size();
                }
            }
            else
            {
                unsigned short* dest = reinterpret_cast<unsigned short*>(buffer);
                unsigned vertexCounter = 0;
                for (unsigned b = 0; b < sourceBatches.Size(); ++b)
                {
                    if (sourceBatches[b]->quadvertices_)
                    {
                        unsigned numquads = sourceBatches[b]->vertices_.Size() / 4;
                        for (unsigned i = 0; i < numquads; ++i)
                        {
                            unsigned base = vertexCounter + i * 4;
                            dest[0] = (unsigned short)(base);
                            dest[1] = (unsigned short)(base + 1);
                            dest[2] = (unsigned short)(base + 2);
                            dest[3] = (unsigned short)(base);
                            dest[4] = (unsigned short)(base + 2);
                            dest[5] = (unsigned short)(base + 3);
                            dest += 6;
                        }
                    }
                    else
                    {
                        unsigned numtriangles = sourceBatches[b]->vertices_.Size() / 3;
                        for (unsigned i = 0; i < numtriangles; ++i)
                        {
                            unsigned base = vertexCounter + i * 3;
                            dest[0] = (unsigned short)(base);
                            dest[1] = (unsigned short)(base + 1);
                            dest[2] = (unsigned short)(base + 2);
                            dest += 3;
                        }
                    }
                    vertexCounter += sourceBatches[b]->vertices_.Size();
                }
            }

            indexBuffer_->Unlock();
        }
        else
        {
            URHO3D_LOGERROR("Failed to lock index buffer");
            return;
        }
    }

    if (geometryDirty_ || viewBatchInfo.vertexBufferUpdateFrameNumber_ != frame_.frameNumber_)
    {
        VertexBuffer* vertexBuffer = viewBatchInfo.vertexBuffer_;
        if (vertexBuffer->GetVertexCount() < vertexCount)
            vertexBuffer->SetSize(vertexCount, MASK_VERTEX2D, true);

        if (vertexCount)
        {
            Vertex2D* dest = reinterpret_cast<Vertex2D*>(vertexBuffer->Lock(0, vertexCount, true));
            if (dest)
            {
                const PODVector<const SourceBatch2D*>& sourceBatches = viewBatchInfo.sourceBatches_;
                for (unsigned b = 0; b < sourceBatches.Size(); ++b)
                {
                    const Vector<Vertex2D>& vertices = sourceBatches[b]->vertices_;
                    for (unsigned i = 0; i < vertices.Size(); ++i)
                        dest[i] = vertices[i];
                    dest += vertices.Size();
                }

                vertexBuffer->Unlock();
            }
            else
                URHO3D_LOGERROR("Failed to lock vertex buffer");
        }

        viewBatchInfo.vertexBufferUpdateFrameNumber_ = frame_.frameNumber_;
        geometryDirty_ = false;
    }
}
*/

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

//    URHO3D_LOGINFOF("Renderer2D() - GetMaterial : texture=%s blendMode=%s", texture->GetName().CString(), blendModeNames[blendMode]);

    HashMap<Texture2D*, HashMap<int, SharedPtr<Material> > >::Iterator t = cachedMaterials_.Find(texture);
    if (t == cachedMaterials_.End())
    {
//        SharedPtr<Material> newMaterial = LoadDefaultMaterial(texture, blendMode);
        SharedPtr<Material> newMaterial(GetSubsystem<ResourceCache>()->GetResource<Material>("Materials/" + GetFileName(texture->GetName()) + String(".xml")));

        if (!newMaterial)
        {
            URHO3D_LOGWARNINGF("Renderer2D() - GetMaterial : no Material => Create Default Urho2D Material !");

            newMaterial = CreateMaterial(texture, blendMode);
        }

        cachedMaterials_[texture][blendMode] = newMaterial;

//        newMaterial->Dump();
        return newMaterial;
    }

    HashMap<int, SharedPtr<Material> >& materials = t->second_;
    HashMap<int, SharedPtr<Material> >::Iterator b = materials.Find(blendMode);
    if (b != materials.End())
        return b->second_;

    SharedPtr<Material> newMaterial = CreateMaterial(texture, blendMode);
    materials[blendMode] = newMaterial;

//    newMaterial->Dump();
    return newMaterial;
}

//bool Renderer2D::CheckVisibility(Drawable2D* drawable) const
//{
//    if ((viewMask_ & drawable->GetViewMask()) == 0)
//        return false;
//
//    const BoundingBox& box = drawable->GetWorldBoundingBox();
//    if (frustum_)
//        return frustum_->IsInsideFast(box) != OUTSIDE;
//
//    return frustumBoundingBox_.IsInsideFast(box) != OUTSIDE;
//}

bool Renderer2D::CheckVisibility(Drawable2D* drawable) const
{
    if (!orthographicMode_)
        return true;

    if ((viewMask_ & drawable->GetViewMask()) == 0)
        return false;

    if (frustumBoundingBox_.Defined())
        return frustumBoundingBox_.IsInsideFast2D(drawable->GetWorldBoundingBox()) != OUTSIDE;

    if (frustum_)
        return frustum_->IsInsideFast(drawable->GetWorldBoundingBox()) != OUTSIDE;

    return false;
}

bool Renderer2D::IsDrawableVisible(Drawable2D* drawable) const
{
    if (!orthographicMode_)
        return true;

    if ((viewMask_ & drawable->GetViewMask()) == 0 || !drawable->GetNode()->IsEnabled())
        return false;

    if (frustumBoundingBox_.Defined())
        return frustumBoundingBox_.IsInsideFast2D(drawable->GetWorldBoundingBox()) != OUTSIDE;

    if (frustum_)
        return frustum_->IsInsideFast(drawable->GetWorldBoundingBox()) != OUTSIDE;

    return false;
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

void CheckDrawableVisibility(const WorkItem* item, unsigned threadIndex)
{
    Renderer2D* renderer = reinterpret_cast<Renderer2D*>(item->aux_);
    Drawable2D** start = reinterpret_cast<Drawable2D**>(item->start_);
    Drawable2D** end = reinterpret_cast<Drawable2D**>(item->end_);

    while (start != end)
    {
        Drawable2D* drawable = *start++;
        if (renderer->CheckVisibility(drawable))
            drawable->MarkInView(renderer->frame_);
    }
}

void Renderer2D::UpdateFrustumBoundingBox(Camera* camera)
{
    frustum_ = &camera->GetFrustum();
    frustumBoundingBox_.Define(frustum_->vertices_[2], frustum_->vertices_[4]);
    frustum_ = 0;

//    URHO3D_LOGWARNINGF("Renderer2D() - UpdateFrustrumBoundingBox : camera=%u frustumBoundingBox_=%s!", camera, frustumBoundingBox_.ToString().CString());
}

const BoundingBox& Renderer2D::GetFrustumBoundingBox() const
{
    return frustumBoundingBox_;
}

void Renderer2D::HandleCameraFrustumChanged(StringHash eventType, VariantMap& eventData)
{
    Node* sendernode = static_cast<Node*>(context_->GetEventSender());
    if (sendernode)
    {
        Camera* camera = sendernode->GetComponent<Camera>();
        if (camera)
            resetcamera_ = camera;
    }

//    URHO3D_LOGWARNINGF("Renderer2D() - HandleCameraFrustumChanged ! resetcamera=%u", resetcamera_);
}

void Renderer2D::HandleBeginViewUpdate(StringHash eventType, VariantMap& eventData)
{
    using namespace BeginViewRender;

    // Check that we are updating the correct scene
    if (GetScene() != eventData[P_SCENE].GetPtr())
        return;

    View* view = static_cast<View*>(eventData[P_VIEW].GetPtr());
    if (view)
        frame_ = view->GetFrameInfo();

    URHO3D_PROFILE(UpdateRenderer2D);

    Camera* camera = static_cast<Camera*>(eventData[P_CAMERA].GetPtr());

    if (camera == resetcamera_)
    {
        UpdateFrustumBoundingBox(camera);
        resetcamera_ = 0;
    }

    viewMask_ = camera->GetViewMask();

    // Check visibility
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
            item->workFunction_ = CheckDrawableVisibility;
            item->aux_ = this;

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

    ViewBatchInfo2D& viewBatchInfo = viewBatchInfos_[camera];

    // Create vertex buffer
    for (int primitiveType=0; primitiveType<2; primitiveType++)
        viewBatchInfo.vertexBuffer_[primitiveType] = new VertexBuffer(context_);

    UpdateViewBatchInfo(viewBatchInfo, camera);

    // Go through the drawables to form geometries & batches and calculate the total vertex / index count,
    // but upload the actual vertex data later. The idea is that the View class copies our batch vector to
    // its internal data structures, so we can reuse the batches for each view, provided that unique Geometry
    // objects are used for each view to specify the draw ranges
    batches_.Resize(viewBatchInfo.batchCount_);
    for (unsigned i = 0; i < viewBatchInfo.batchCount_; ++i)
    {
        batches_[i].material_ = viewBatchInfo.materials_[i];
        batches_[i].geometry_ = viewBatchInfo.geometries_[i];
    }
}

void Renderer2D::GetDrawables(PODVector<Drawable2D*>& dest, Node* node)
{
    if (!node || !node->IsEnabled())
        return;

    const Vector<SharedPtr<Component> >& components = node->GetComponents();
    for (Vector<SharedPtr<Component> >::ConstIterator i = components.Begin(); i != components.End(); ++i)
    {
        Drawable2D* drawable = dynamic_cast<Drawable2D*>(i->Get());
        if (drawable && drawable->IsEnabled())
            dest.Push(drawable);
    }

    const Vector<SharedPtr<Node> >& children = node->GetChildren();
    for (Vector<SharedPtr<Node> >::ConstIterator i = children.Begin(); i != children.End(); ++i)
        GetDrawables(dest, i->Get());
}

static inline bool CompareSourceBatch2Ds(const SourceBatch2D* lhs, const SourceBatch2D* rhs)
{
    if (lhs->drawOrder_ != rhs->drawOrder_)
        return lhs->drawOrder_ < rhs->drawOrder_;

    if (lhs->material_ != rhs->material_)
        return lhs->material_->GetNameHash() < rhs->material_->GetNameHash();

    if (lhs->quadvertices_ != rhs->quadvertices_)
        return lhs->quadvertices_;

    return lhs < rhs;
}

void Renderer2D::UpdateViewBatchInfo(ViewBatchInfo2D& viewBatchInfo, Camera* camera)
{
    // Already update in same frame ?
    if (viewBatchInfo.batchUpdatedFrameNumber_ == frame_.frameNumber_)
        return;

    PODVector<const SourceBatch2D*>& sourceBatches = viewBatchInfo.sourceBatches_;
    sourceBatches.Clear();
    for (unsigned d = 0; d < drawables_.Size(); ++d)
    {
        if (!drawables_[d]->IsInView(camera))
            continue;

        const Vector<SourceBatch2D*>& batches = drawables_[d]->GetSourceBatchesToRender();

        for (unsigned b = 0; b < batches.Size(); ++b)
        {
            const SourceBatch2D* batch = batches[b];
            if (batch && batch->material_ && !batch->vertices_.Empty())
//            if (batch->material_ && !batch->vertices_.Empty())
                sourceBatches.Push(batch);
        }
    }

    Sort(sourceBatches.Begin(), sourceBatches.End(), CompareSourceBatch2Ds);

    viewBatchInfo.batchCount_ = 0;
    Material* currMaterial = 0;

    unsigned iStart[2] = { 0, 0 };
    unsigned iCount[2] = { 0, 0 };
    unsigned vStart[2] = { 0, 0 };
    unsigned vCount[2] = { 0, 0 };
    int currType = sourceBatches.Size() ? sourceBatches[0]->quadvertices_ : QUAD2D;

    for (unsigned b = 0; b < sourceBatches.Size(); ++b)
    {
        Material* material = sourceBatches[b]->material_;
        int primitiveType = sourceBatches[b]->quadvertices_;
        const Vector<Vertex2D>& vertices = sourceBatches[b]->vertices_;

        // When new material encountered, finish the current batch and start new
        if (currMaterial != material || currType != primitiveType)
        {
            if (currMaterial)
            {
                AddViewBatch(viewBatchInfo, currType, currMaterial, iStart[currType], iCount[currType], vStart[currType], vCount[currType]);
                iStart[primitiveType] += iCount[primitiveType];
                iCount[primitiveType] = 0;
                vStart[primitiveType] += vCount[primitiveType];
                vCount[primitiveType] = 0;
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
        AddViewBatch(viewBatchInfo, currType, currMaterial, iStart[currType], iCount[currType], vStart[currType], vCount[currType]);

    for (int primitiveType=0; primitiveType<2; primitiveType++)
    {
        viewBatchInfo.indexCount_[primitiveType]  = iStart[primitiveType] + iCount[primitiveType];
        viewBatchInfo.vertexCount_[primitiveType] = vStart[primitiveType] + vCount[primitiveType];
    }

    viewBatchInfo.batchUpdatedFrameNumber_ = frame_.frameNumber_;
}

void Renderer2D::AddViewBatch(ViewBatchInfo2D& viewBatchInfo, int primitivetype, Material* material, unsigned indexStart, unsigned indexCount,
    unsigned vertexStart, unsigned vertexCount)
{
    if (!material || indexCount == 0 || vertexCount == 0)
        return;

    if (viewBatchInfo.materials_.Size() <= viewBatchInfo.batchCount_)
        viewBatchInfo.materials_.Resize(viewBatchInfo.batchCount_ + 1);
    viewBatchInfo.materials_[viewBatchInfo.batchCount_] = material;

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
    URHO3D_LOGINFOF("Renderer2D() - Dump : ");
    for (unsigned i=0; i < drawables_.Size(); i++)
        URHO3D_LOGINFOF("   -> drawable[%d] ptr=%u id=%u type=%s node=%s(%u)", i,
                        drawables_[i], drawables_[i]->GetID(), drawables_[i]->GetTypeName().CString(),
                        drawables_[i]->GetNode()->GetName().CString(), drawables_[i]->GetNode()->GetID());
}

}
