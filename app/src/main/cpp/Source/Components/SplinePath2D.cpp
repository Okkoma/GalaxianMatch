#include <Urho3D/Urho3D.h>

#include <Urho3D/Core/Context.h>

#include <Urho3D/IO/Log.h>
#include <Urho3D/Graphics/DebugRenderer.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Scene/SceneEvents.h>

#include "SplinePath2D.h"


const char* interpolationModeNames[] =
{
    "Bezier",
    "Catmull-Rom",
    "Linear",
    "Catmull-Rom Full",
    0
};

SplinePath2D::SplinePath2D(Context* context) :
    Component(context),
    spline_(BEZIER_CURVE),
    length_(0.f),
    dirty_(false),
    pointNodeMode_(false)
{
    UpdateNodeIds();
}

void SplinePath2D::RegisterObject(Context* context)
{
    context->RegisterFactory<SplinePath2D>();

    URHO3D_ATTRIBUTE("PointNode Mode Enabled", bool, pointNodeMode_, false, AM_DEFAULT);
    URHO3D_ENUM_ACCESSOR_ATTRIBUTE("Interpolation Mode", GetInterpolationMode, SetInterpolationMode, InterpolationMode,
        interpolationModeNames, BEZIER_CURVE, AM_FILE);
    URHO3D_ACCESSOR_ATTRIBUTE("Points", GetPointsAttr, SetPointsAttr, VariantVector, Variant::emptyVariantVector, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Point Nodes", GetPointNodeIdsAttr, SetPointNodeIdsAttr,
        VariantVector, Variant::emptyVariantVector, AM_FILE | AM_NODEIDVECTOR);

}

void SplinePath2D::ApplyAttributes()
{
    if (!dirty_)
        return;

    // Remove all old instance nodes before searching for new. Can not call RemoveAllInstances() as that would modify
    // the ID list on its own
    for (unsigned i = 0; i < pointNodes_.Size(); ++i)
    {
        Node* node = pointNodes_[i];
        if (node)
            node->RemoveListener(this);
    }

    pointNodes_.Clear();

    Scene* scene = GetScene();

    if (scene)
    {
        spline_.Clear();

        if (pointNodeMode_)
        {
            // The first index stores the number of IDs redundantly. This is for editing
            for (unsigned i = 1; i < pointNodeIdsAttr_.Size(); ++i)
            {
                Node* node = scene->GetNode(pointNodeIdsAttr_[i].GetUInt());
                if (node)
                {
                    WeakPtr<Node> nodePoint(node);
                    node->AddListener(this);
                    pointNodes_.Push(nodePoint);
                    spline_.AddKnot(node->GetWorldPosition());
                }
            }
        }
        else
        {
            for (unsigned i = 1; i < pointsAttr_.Size(); ++i)
                spline_.AddKnot(pointsAttr_[i].GetVector3());
        }
    }

    CalculateLength();
    dirty_ = false;
}

void SplinePath2D::DrawDebugGeometry(DebugRenderer* debug, bool /*depthTest*/)
{
    if (debug && node_ && IsEnabledEffective())
    {
        if (spline_.GetKnots().Size() > 1)
        {
            Vector3 a = spline_.GetPoint(0.f).GetVector3();
            for (unsigned i = 1; i <= 100; ++i)
            {
                Vector3 b = spline_.GetPoint(i / 100.f).GetVector3();
                debug->AddLine(a, b, Color::GREEN);
                a = b;
            }
        }

        for (Vector<WeakPtr<Node> >::ConstIterator i = pointNodes_.Begin(); i != pointNodes_.End(); ++i)
            debug->AddNode(*i);
    }
}

void SplinePath2D::AddPoint(const Vector2& point, unsigned index)
{
    spline_.AddKnot(point, index);
}

void SplinePath2D::RemovePoint(unsigned index)
{
    spline_.RemoveKnot(index);
}

void SplinePath2D::ClearPoints()
{
    spline_.Clear();
}

void SplinePath2D::AddPointNode(Node* point, unsigned index)
{
    if (!point)
        return;

    WeakPtr<Node> nodePoint(point);

    point->AddListener(this);
    pointNodes_.Insert(index, nodePoint);
    spline_.AddKnot(point->GetWorldPosition(), index);

    UpdateNodeIds();
    CalculateLength();
}

void SplinePath2D::RemovePointNode(Node* point)
{
    if (!point)
        return;

    WeakPtr<Node> nodePoint(point);

    point->RemoveListener(this);

    for (unsigned i = 0; i < pointNodes_.Size(); ++i)
    {
        if (pointNodes_[i] == nodePoint)
        {
            pointNodes_.Erase(i);
            spline_.RemoveKnot(i);
            break;
        }
    }

    UpdateNodeIds();
    CalculateLength();
}

void SplinePath2D::ClearPointNodes()
{
    for (unsigned i = 0; i < pointNodes_.Size(); ++i)
    {
        Node* node = pointNodes_[i];
        if (node)
            node->RemoveListener(this);
    }

    pointNodes_.Clear();
    spline_.Clear();

    UpdateNodeIds();
    CalculateLength();
}

void SplinePath2D::SetInterpolationMode(InterpolationMode interpolationMode)
{
    spline_.SetInterpolationMode(interpolationMode);
    CalculateLength();
}

Vector3 SplinePath2D::GetPoint(float factor) const
{
    return spline_.GetPoint(factor).GetVector3();
}

void SplinePath2D::SetPointsAttr(const VariantVector& value)
{
    pointsAttr_.Clear();

    unsigned numInstances = value.Size() ? value[0].GetUInt() : 0;

    pointsAttr_.Push(numInstances);
    for (unsigned i = 1; i <= numInstances; ++i)
    {
        pointsAttr_.Push(value.Size() > i ? value[i].GetVector3() : Vector3::ZERO);
    }

    dirty_ = true;
}

void SplinePath2D::SetPointNodeIdsAttr(const VariantVector& value)
{
    // Just remember the node IDs. They need to go through the SceneResolver, and we actually find the nodes during
    // ApplyAttributes()

    pointNodeMode_ = value.Size() > 1;

    if (pointNodeMode_)
    {
        pointNodeIdsAttr_.Clear();

        unsigned index = 0;
        unsigned numInstances = value[index++].GetUInt();
        // Prevent crash on entering negative value in the editor
        if (numInstances > M_MAX_INT)
            numInstances = 0;

        pointNodeIdsAttr_.Push(numInstances);
        while (numInstances--)
        {
            // If vector contains less IDs than should, fill the rest with zeros
            if (index < value.Size())
                pointNodeIdsAttr_.Push(value[index++].GetUInt());
            else
                pointNodeIdsAttr_.Push(0);
        }

        dirty_ = true;
    }
    else
    {
        pointNodeIdsAttr_.Clear();
        pointNodeIdsAttr_.Push(0);

        dirty_ = true;
    }
}

void SplinePath2D::OnMarkedDirty(Node* point)
{
    if (!point)
        return;

    WeakPtr<Node> nodePoint(point);

    for (unsigned i = 0; i < pointNodes_.Size(); ++i)
    {
        if (pointNodes_[i] == nodePoint)
        {
            spline_.SetKnot(point->GetWorldPosition(), i);
            break;
        }
    }

    CalculateLength();
}

void SplinePath2D::OnNodeSetEnabled(Node* point)
{
    if (!point)
        return;

    if (pointNodeMode_)
    {
        WeakPtr<Node> nodePoint(point);

        for (unsigned i = 0; i < pointNodes_.Size(); ++i)
        {
            if (pointNodes_[i] == nodePoint)
            {
                if (point->IsEnabled())
                    spline_.AddKnot(point->GetWorldPosition(), i);
                else
                    spline_.RemoveKnot(i);

                break;
            }
        }
    }

    CalculateLength();
}

void SplinePath2D::UpdateNodeIds()
{
    unsigned numInstances = pointNodes_.Size();

    pointNodeIdsAttr_.Clear();
    pointNodeIdsAttr_.Push(numInstances);

    for (unsigned i = 0; i < numInstances; ++i)
    {
        Node* node = pointNodes_[i];
        pointNodeIdsAttr_.Push(node ? node->GetID() : 0);
    }
}

void SplinePath2D::CalculateLength()
{
    if (spline_.GetKnots().Size() <= 0)
        return;

    length_ = 0.f;

    Vector3 a = spline_.GetKnot(0).GetVector3();
    for (unsigned i = 0; i <= 1000; ++i)
    {
        Vector3 b = spline_.GetPoint(i / 1000.f).GetVector3();
        length_ += Abs((a - b).Length());
        a = b;
    }
}

