#pragma once

#include <Urho3D/Core/Spline.h>
#include <Urho3D/Scene/Component.h>

namespace Urho3D
{
    class Node;
}

using namespace Urho3D;


/// Spline for creating smooth movement based on Speed along a set of Control Points modified by the Interpolation Mode.
class SplinePath2D : public Component
{
    URHO3D_OBJECT(SplinePath2D, Component)

public:
    /// Construct an Empty SplinePath.
    SplinePath2D(Context* context);
    /// Destructor.
    virtual ~SplinePath2D() { }
    /// Register object factory.
    static void RegisterObject(Context* context);

    /// Apply Attributes to the SplinePath.
    void ApplyAttributes();
    /// Draw the Debug Geometry.
    void DrawDebugGeometry(DebugRenderer* debug, bool depthTest);

    /// Add a Vector2 to the SplinePath as a Control Point.
    void AddPoint(const Vector2& point, unsigned index = M_MAX_UNSIGNED);
    /// Remove a Point from the SplinePath.
    void RemovePoint(unsigned index);
    /// Clear the Points from the SplinePath.
    void ClearPoints();

    /// Add a Node to the SplinePath as a Control Point.
    void AddPointNode(Node* point, unsigned index = M_MAX_UNSIGNED);
    /// Remove a Node Control Point from the SplinePath.
    void RemovePointNode(Node* point);
    /// Clear the Control Points from the SplinePath.
    void ClearPointNodes();

    /// Set the Interpolation Mode.
    void SetInterpolationMode(InterpolationMode interpolationMode);

    /// Get the Interpolation Mode.
    InterpolationMode GetInterpolationMode() const { return spline_.GetInterpolationMode(); }

    /// Get the length of SplinePath;
    float GetLength() const { return length_; }

    /// Get a point on the SplinePath from 0.f to 1.f where 0 is the start and 1 is the end.
    Vector3 GetPoint(float factor) const;

    const Spline& GetSpline() const { return spline_; }

    /// Set Control Points attribute.
    void SetPointsAttr(const VariantVector& value);

    /// Return Control Points attribute.
    const VariantVector& GetPointsAttr() const { return pointsAttr_; }

    /// Set Control Point Node IDs attribute.
    void SetPointNodeIdsAttr(const VariantVector& value);

    /// Return Control Point Node IDs attribute.
    const VariantVector& GetPointNodeIdsAttr() const { return pointNodeIdsAttr_; }

protected:
    /// Listener to manage Control Point movement.
    void OnMarkedDirty(Node* point);
    /// Listener to manage Control Point enabling.
    void OnNodeSetEnabled(Node* point);

private:
    /// Update the Node IDs of the Control Points.
    void UpdateNodeIds();
    /// Calculate the length of the SplinePath. Used for movement calculations.
    void CalculateLength();

    bool pointNodeMode_;

    /// The Control Points of the Spline.
    Spline spline_;
    /// The length of the SplinePath.
    float length_;
    /// Whether the Control Point IDs are dirty.
    bool dirty_;
    /// Control Points for the SplinePath.
    Vector<WeakPtr<Node> > pointNodes_;
    /// Control Point ID's for the SplinePath.
    mutable VariantVector pointNodeIdsAttr_;
    /// Control Point's for the SplinePath.
    mutable VariantVector pointsAttr_;
};

