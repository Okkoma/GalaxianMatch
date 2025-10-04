#include <Urho3D/Urho3D.h>
#include <Urho3D/Core/Context.h>
#include <catch2/catch_test_macros.hpp>

#include "SplinePath2D.h"


TEST_CASE("SplinePath2D Test", "[components]") {
    Urho3D::Context context;
    SplinePath2D spline(&context);

    SECTION("Initialization Test") {
        REQUIRE(spline.GetSpline().GetKnots().Empty());
    }

    SECTION("Point Addition Test") {
        Vector2 point1(0.0f, 0.0f);
        Vector2 point2(1.0f, 1.0f);
        
        spline.AddPoint(point1);
        spline.AddPoint(point2);
        
        REQUIRE(spline.GetSpline().GetKnots().Size() == 2);
    }

    SECTION("Curve Point Calculation Test") {
        Vector2 p0(0.0f, 0.0f);
        Vector2 p1(1.0f, 0.0f);
        Vector2 p2(1.0f, 1.0f);
        
        spline.AddPoint(p0);
        spline.AddPoint(p1);
        spline.AddPoint(p2);
        
        Vector3 interpolated = spline.GetPoint(0.5f);
        REQUIRE(interpolated.x_ >= 0.0f);
        REQUIRE(interpolated.x_ <= 1.0f);
    }
} 