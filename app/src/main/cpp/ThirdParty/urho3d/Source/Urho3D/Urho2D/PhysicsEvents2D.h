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

#include "../Core/Object.h"

namespace Urho3D
{

/// Physics world is about to be stepped.
URHO3D_EVENT(E_PHYSICSPRESTEP2D, PhysicsPreStep2D)
{
    URHO3D_PARAM(P_WORLD, World);                  // PhysicsWorld2D pointer
    URHO3D_PARAM(P_TIMESTEP, TimeStep);            // float
}

/// Physics world has been stepped.
URHO3D_EVENT(E_PHYSICSPOSTSTEP2D, PhysicsPostStep2D)
{
    URHO3D_PARAM(P_WORLD, World);                  // PhysicsWorld2D pointer
    URHO3D_PARAM(P_TIMESTEP, TimeStep);            // float
}

/// Physics begin contact.
URHO3D_EVENT(E_PHYSICSBEGINCONTACT2D, PhysicsBeginContact2D)
{
    URHO3D_PARAM(P_WORLD, World);                  // PhysicsWorld2D pointer
    URHO3D_PARAM(P_BODYA, BodyA);                  // RigidBody2D pointer
    URHO3D_PARAM(P_BODYB, BodyB);                  // RigidBody2D pointer
//    URHO3D_PARAM(P_NODEA, NodeA);                  // Node pointer
//    URHO3D_PARAM(P_NODEB, NodeB);                  // Node pointer
    URHO3D_PARAM(P_SHAPEA, ShapeA);                // CollisionShape2D pointer
    URHO3D_PARAM(P_SHAPEB, ShapeB);                // CollisionShape2D pointer
    URHO3D_PARAM(P_ISHAPEA, iShapeA);           // Index Vertice on ShapeA (unsigned)
    URHO3D_PARAM(P_ISHAPEB, iShapeB);           // Index Vertice on ShapeB (unsigned)
//    URHO3D_PARAM(P_CONTACTPTR, contactPtr);           // b2Contact Ptr
    URHO3D_PARAM(P_CPOINT, contactPoint);           // Point de Contact local (Vector2)
    URHO3D_PARAM(P_NORMAL, NormalAtoB);           // Normal : Pointing from Fixture A to Fixture B (Vector2)
}

/// Physics end contact.
URHO3D_EVENT(E_PHYSICSENDCONTACT2D, PhysicsEndContact2D)
{
    URHO3D_PARAM(P_WORLD, World);                  // PhysicsWorld2D pointer
    URHO3D_PARAM(P_BODYA, BodyA);                  // RigidBody2D pointer
    URHO3D_PARAM(P_BODYB, BodyB);                  // RigidBody2D pointer
//    URHO3D_PARAM(P_NODEA, NodeA);                  // Node pointer
//    URHO3D_PARAM(P_NODEB, NodeB);                  // Node pointer
    URHO3D_PARAM(P_SHAPEA, ShapeA);                // CollisionShape2D pointer
    URHO3D_PARAM(P_SHAPEB, ShapeB);                // CollisionShape2D pointer
    URHO3D_PARAM(P_ISHAPEA, iShapeA);           // Index Vertice on ShapeA (unsigned)
    URHO3D_PARAM(P_ISHAPEB, iShapeB);           // Index Vertice on ShapeB (unsigned)
//    URHO3D_PARAM(P_CONTACTPTR, contactPtr);           // b2Contact Ptr
//    URHO3D_PARAM(P_CPOINT, contactPoint);           // Point de Contact local (Vector2)
//    URHO3D_PARAM(P_NORMAL, NormalAtoB);           // Normal : Pointing from Fixture A to Fixture B (Vector2)
}

}
