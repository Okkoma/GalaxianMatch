﻿//
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
#include "../IO/Log.h"
#include "../IO/MemoryBuffer.h"
#include "../Resource/XMLFile.h"
#include "../Resource/JSONFile.h"
#include "../Scene/Component.h"
#include "../Scene/ObjectAnimation.h"
#include "../Scene/ReplicationState.h"
#include "../Scene/Scene.h"
#include "../Scene/SceneEvents.h"
#include "../Scene/SmoothedTransform.h"
#include "../Scene/UnknownComponent.h"

#include "../DebugNew.h"

#ifdef _MSC_VER
#pragma warning(disable:6293)
#endif

namespace Urho3D
{

Node::Node(Context* context) :
    Animatable(context),
    isPoolNode_(false),
    worldTransform_(Matrix3x4::IDENTITY),
    dirty_(false),
    dirty2D_(false),
    worldTransform2D_(Matrix2x3::IDENTITY),
    enabled_(true),
    enabledPrev_(true),
    networkUpdate_(false),
    parent_(0),
    scene_(0),
    id_(0),
    position_(Vector3::ZERO),
    rotation_(Quaternion::IDENTITY),
    scale_(Vector3::ONE),
    worldRotation_(Quaternion::IDENTITY),
    rotation2D_(0),
    worldRotation2D_(0),
    owner_(0),
    changeModeEnabled_(true)
{
}

Node::~Node()
{
    RemoveAllChildren();
    RemoveAllComponents();

    // Remove from the scene
    if (scene_)
        scene_->NodeRemoved(this);
}

void Node::RegisterObject(Context* context)
{
    context->RegisterFactory<Node>();

//    URHO3D_ACCESSOR_ATTRIBUTE("Is Enabled", IsEnabled, SetEnabled, bool, true, AM_FILE);
    URHO3D_ACCESSOR_ATTRIBUTE("Is Enabled", IsEnabled, SetEnabled, bool, true, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Name", GetName, SetName, String, String::EMPTY, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Position", GetPosition, SetPosition, Vector3, Vector3::ZERO, AM_FILE);
    URHO3D_ACCESSOR_ATTRIBUTE("Rotation", GetRotation, SetRotation, Quaternion, Quaternion::IDENTITY, AM_FILE);
    URHO3D_ACCESSOR_ATTRIBUTE("Scale", GetScale, SetScale, Vector3, Vector3::ONE, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Variables", VariantMap, vars_, Variant::emptyVariantMap, AM_FILE); // Network replication of vars uses custom data
    URHO3D_ACCESSOR_ATTRIBUTE("Tags", GetTags, SetTags, StringVector, Variant::emptyStringVector, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Network Position", GetNetPositionAttr, SetNetPositionAttr, Vector3, Vector3::ZERO,
        AM_NET | AM_LATESTDATA | AM_NOEDIT);
    URHO3D_ACCESSOR_ATTRIBUTE("Network Rotation", GetNetRotationAttr, SetNetRotationAttr, PODVector<unsigned char>, Variant::emptyBuffer,
        AM_NET | AM_LATESTDATA | AM_NOEDIT);
    URHO3D_ACCESSOR_ATTRIBUTE("Network Parent Node", GetNetParentAttr, SetNetParentAttr, PODVector<unsigned char>, Variant::emptyBuffer,
        AM_NET | AM_NOEDIT);
}

bool Node::Load(Deserializer& source, bool setInstanceDefault, bool applyAttr)
{
    SceneResolver resolver;

    // Read own ID. Will not be applied, only stored for resolving possible references
    unsigned nodeID = source.ReadUInt();
    resolver.AddNode(nodeID, this);

    // Read attributes, components and child nodes
    bool success = Load(source, resolver);
    if (success)
    {
        resolver.Resolve();
        if (applyAttr)
            ApplyAttributes();
    }

    return success;
}

bool Node::Save(Serializer& dest) const
{
    // Write node ID
    if (!dest.WriteUInt(id_))
        return false;

    // Write attributes
    if (!Animatable::Save(dest))
        return false;

    // Write components
    dest.WriteVLE(GetNumPersistentComponents());
    for (unsigned i = 0; i < components_.Size(); ++i)
    {
        Component* component = components_[i];
        if (component->IsTemporary())
            continue;

        // Create a separate buffer to be able to skip failing components during deserialization
        VectorBuffer compBuffer;
        if (!component->Save(compBuffer))
            return false;
        dest.WriteVLE(compBuffer.GetSize());
        dest.Write(compBuffer.GetData(), compBuffer.GetSize());
    }

    // Write child nodes
    dest.WriteVLE(GetNumPersistentChildren());
    for (unsigned i = 0; i < children_.Size(); ++i)
    {
        Node* node = children_[i];
        if (node->IsTemporary())
            continue;

        if (!node->Save(dest))
            return false;
    }

    return true;
}

bool Node::LoadXML(const XMLElement& source, bool setInstanceDefault, bool applyAttr)
{
    SceneResolver resolver;

    // Read own ID. Will not be applied, only stored for resolving possible references
    unsigned nodeID = source.GetUInt("id");
    resolver.AddNode(nodeID, this);

    // Read attributes, components and child nodes
    bool success = LoadXML(source, resolver);
    if (success)
    {
        resolver.Resolve();
        if (applyAttr)
            ApplyAttributes();
    }

    return success;
}

bool Node::LoadXML(const XMLElement& source, CreateMode mode, bool setInstanceDefault, bool applyAttr)
{
    SceneResolver resolver;

    // Read own ID. Will not be applied, only stored for resolving possible references
    unsigned nodeID = source.GetUInt("id");
    resolver.AddNode(nodeID, this);

    // Read attributes, components and child nodes
    bool success = LoadXML(source, resolver, true, false, mode);
    if (success)
    {
        resolver.Resolve();
        if (applyAttr)
            ApplyAttributes();
    }

    return success;
}

bool Node::LoadJSON(const JSONValue& source, bool setInstanceDefault)
{
    SceneResolver resolver;

    // Read own ID. Will not be applied, only stored for resolving possible references
    unsigned nodeID = source.Get("id").GetUInt();
    resolver.AddNode(nodeID, this);

    // Read attributes, components and child nodes
    bool success = LoadJSON(source, resolver);
    if (success)
    {
        resolver.Resolve();
        ApplyAttributes();
    }

    return success;
}

bool Node::SaveXML(XMLElement& dest) const
{
    // Write node ID
    if (!dest.SetUInt("id", id_))
        return false;

    // Write attributes
    if (!Animatable::SaveXML(dest))
        return false;

    // Write components
    for (unsigned i = 0; i < components_.Size(); ++i)
    {
        Component* component = components_[i];
        if (component->IsTemporary())
            continue;

        XMLElement compElem = dest.CreateChild("component");
        if (!component->SaveXML(compElem))
            return false;
    }

    // Write child nodes
    for (unsigned i = 0; i < children_.Size(); ++i)
    {
        Node* node = children_[i];
        if (node->IsTemporary())
            continue;

        XMLElement childElem = dest.CreateChild("node");
        if (!node->SaveXML(childElem))
            return false;
    }

    return true;
}

bool Node::SaveJSON(JSONValue& dest) const
{
    // Write node ID
    dest.Set("id", id_);

    // Write attributes
    if (!Animatable::SaveJSON(dest))
        return false;

    // Write components
    JSONArray componentsArray;
    componentsArray.Reserve(components_.Size());
    for (unsigned i = 0; i < components_.Size(); ++i)
    {
        Component* component = components_[i];
        if (component->IsTemporary())
            continue;

        JSONValue compVal;
        if (!component->SaveJSON(compVal))
            return false;
        componentsArray.Push(compVal);
    }
    dest.Set("components", componentsArray);

    // Write child nodes
    JSONArray childrenArray;
    childrenArray.Reserve(children_.Size());
    for (unsigned i = 0; i < children_.Size(); ++i)
    {
        Node* node = children_[i];
        if (node->IsTemporary())
            continue;

        JSONValue childVal;
        if (!node->SaveJSON(childVal))
            return false;
        childrenArray.Push(childVal);
    }
    dest.Set("children", childrenArray);

    return true;
}

void Node::ApplyAttributes()
{
    for (unsigned i = 0; i < components_.Size(); ++i)
        components_[i]->ApplyAttributes();

    for (unsigned i = 0; i < children_.Size(); ++i)
        children_[i]->ApplyAttributes();
}

void Node::MarkNetworkUpdate()
{
    if (!networkUpdate_ && scene_ && id_ < FIRST_LOCAL_ID)
    {
        scene_->MarkNetworkUpdate(this);
        networkUpdate_ = true;
    }
}

void Node::AddReplicationState(NodeReplicationState* state)
{
    if (!networkState_)
        AllocateNetworkState();

    networkState_->replicationStates_.Push(state);
}

bool Node::SaveXML(Serializer& dest, const String& indentation) const
{
    SharedPtr<XMLFile> xml(new XMLFile(context_));
    XMLElement rootElem = xml->CreateRoot("node");
    if (!SaveXML(rootElem))
        return false;

    return xml->Save(dest, indentation);
}

bool Node::SaveJSON(Serializer& dest, const String& indentation) const
{
    SharedPtr<JSONFile> json(new JSONFile(context_));
    JSONValue& rootElem = json->GetRoot();

    if (!SaveJSON(rootElem))
        return false;

    return json->Save(dest, indentation);
}

void Node::SetName(const String& name)
{
    if (name != name_)
    {
        name_ = name;
        nameHash_ = name_;

        MarkNetworkUpdate();

        // Send change event
        if (scene_)
        {
            using namespace NodeNameChanged;

            VariantMap& eventData = GetEventDataMap();
            eventData[P_SCENE] = scene_;
            eventData[P_NODE] = this;

            scene_->SendEvent(E_NODENAMECHANGED, eventData);
        }
    }
}

void Node::SetTags(const StringVector& tags)
{
    RemoveAllTags();
    AddTags(tags);
    // MarkNetworkUpdate() already called in RemoveAllTags() / AddTags()
}

void Node::AddTag(const String& tag)
{
    // Check if tag empty or already added
    if (tag.Empty() || HasTag(tag))
        return;

    // Add tag
    tags_.Push(tag);

    // Cache
    scene_->NodeTagAdded(this, tag);

    // Send event
    using namespace NodeTagAdded;
    VariantMap& eventData = GetEventDataMap();
    eventData[P_SCENE] = scene_;
    eventData[P_NODE] = this;
    eventData[P_TAG] = tag;
    scene_->SendEvent(E_NODETAGADDED, eventData);

    // Sync
    MarkNetworkUpdate();
}

void Node::AddTags(const String& tags, char separator)
{
    StringVector tagVector = tags.Split(separator);
    AddTags(tagVector);
}

void Node::AddTags(const StringVector& tags)
{
    // This is OK, as MarkNetworkUpdate() early-outs when called multiple times
    for (unsigned i = 0; i < tags.Size(); ++i)
        AddTag(tags[i]);
}

bool Node::RemoveTag(const String& tag)
{
    bool removed = tags_.Remove(tag);

    // Nothing to do
    if (!removed)
        return false;

    // Scene cache update
    if (scene_)
    {
        scene_->NodeTagRemoved(this, tag);
        // Send event
        using namespace NodeTagRemoved;
        VariantMap& eventData = GetEventDataMap();
        eventData[P_SCENE] = scene_;
        eventData[P_NODE] = this;
        eventData[P_TAG] = tag;
        scene_->SendEvent(E_NODETAGREMOVED, eventData);
    }

    // Sync
    MarkNetworkUpdate();
    return true;
}

void Node::RemoveAllTags()
{
    // Clear old scene cache
    if (scene_)
    {
        for (unsigned i = 0; i < tags_.Size(); ++i)
        {
            scene_->NodeTagRemoved(this, tags_[i]);

            // Send event
            using namespace NodeTagRemoved;
            VariantMap& eventData = GetEventDataMap();
            eventData[P_SCENE] = scene_;
            eventData[P_NODE] = this;
            eventData[P_TAG] = tags_[i];
            scene_->SendEvent(E_NODETAGREMOVED, eventData);
        }
    }

    tags_.Clear();

    // Sync
    MarkNetworkUpdate();
}

/// Urho3D positions

void Node::SetPosition(const Vector3& position)
{
    position_ = position;
    MarkDirty();

    MarkNetworkUpdate();
}

void Node::SetRotation(const Quaternion& rotation)
{
    rotation_ = rotation;
    rotation2D_ = rotation_.RollAngle();
    MarkDirty();

    MarkNetworkUpdate();
}

void Node::SetDirection(const Vector3& direction)
{
    SetRotation(Quaternion(Vector3::FORWARD, direction));
}

void Node::SetScale(float scale)
{
    SetScale(Vector3(scale, scale, scale));
}

void Node::SetScale(const Vector3& scale)
{
    scale_ = scale;
    // Prevent exact zero scale e.g. from momentary edits as this may cause division by zero
    // when decomposing the world transform matrix
    if (scale_.x_ == 0.0f)
        scale_.x_ = M_EPSILON;
    if (scale_.y_ == 0.0f)
        scale_.y_ = M_EPSILON;
    if (scale_.z_ == 0.0f)
        scale_.z_ = M_EPSILON;

    MarkDirty();
    MarkNetworkUpdate();
}

void Node::SetTransform(const Vector3& position, const Quaternion& rotation)
{
    position_ = position;
    rotation_ = rotation;
    rotation2D_ = rotation.RollAngle();
    MarkDirty();

    MarkNetworkUpdate();
}

void Node::SetTransform(const Vector3& position, const Quaternion& rotation, const Vector3& scale)
{
    position_ = position;
    rotation_ = rotation;
    rotation2D_ = rotation.RollAngle();
    scale_ = scale;
    MarkDirty();

    MarkNetworkUpdate();
}

void Node::SetTransform(const Vector3& position, const Quaternion& rotation, float scale)
{
    SetTransform(position, rotation, Vector3(scale, scale, scale));
}

void Node::SetTransform(const Matrix3x4& matrix)
{
    SetTransform(matrix.Translation(), matrix.Rotation(), matrix.Scale());
}

void Node::SetWorldPosition(const Vector3& position)
{
    SetPosition((parent_ == scene_ || !parent_) ? position : parent_->GetWorldTransform().Inverse() * position);
}

void Node::SetWorldRotation(const Quaternion& rotation)
{
    SetRotation((parent_ == scene_ || !parent_) ? rotation : parent_->GetWorldRotation().Inverse() * rotation);
}

void Node::SetWorldDirection(const Vector3& direction)
{
    Vector3 localDirection = (parent_ == scene_ || !parent_) ? direction : parent_->GetWorldRotation().Inverse() * direction;
    SetRotation(Quaternion(Vector3::FORWARD, localDirection));
}

void Node::SetWorldScale(const Vector3& scale)
{
    SetScale((parent_ == scene_ || !parent_) ? scale : scale / parent_->GetWorldScale());
}

void Node::SetWorldScale(float scale)
{
    SetWorldScale(Vector3(scale, scale, scale));
}

void Node::SetWorldTransform(const Vector3& position, const Quaternion& rotation)
{
    SetWorldPosition(position);
    SetWorldRotation(rotation);
}

void Node::SetWorldTransform(const Vector3& position, const Quaternion& rotation, float scale)
{
    SetWorldPosition(position);
    SetWorldRotation(rotation);
    SetWorldScale(scale);
}

void Node::SetWorldTransform(const Vector3& position, const Quaternion& rotation, const Vector3& scale)
{
    SetWorldPosition(position);
    SetWorldRotation(rotation);
    SetWorldScale(scale);
}

void Node::SetWorldTransform(const Matrix3x4& worldtransform)
{
    SetWorldPosition(worldtransform.Translation());
    SetWorldRotation(worldtransform.Rotation());
    SetWorldScale(worldtransform.Scale());
}

/// Urho2D positions

void Node::SetPosition2D(const Vector2& position)
{
    position_.x_ = position.x_;
    position_.y_ = position.y_;

    MarkDirty();
    MarkNetworkUpdate();
}

void Node::SetRotation2D(float rotation)
{
    rotation2D_ = rotation;
    rotation_ = Quaternion(rotation, Vector3::FORWARD);

    MarkDirty();
    MarkNetworkUpdate();
}

void Node::SetScale2D(const Vector2& scale)
{
    scale_.x_ = scale.x_;
    scale_.y_ = scale.y_;
    scale_.z_ = 1.f;

    // Prevent exact zero scale e.g. from momentary edits as this may cause division by zero
    // when decomposing the world transform matrix
    if (scale_.x_ == 0.0f)
        scale_.x_ = M_EPSILON;
    if (scale_.y_ == 0.0f)
        scale_.y_ = M_EPSILON;

    MarkDirty();
    MarkNetworkUpdate();
}

void Node::SetTransform2D(const Vector2& position, float rotation)
{
    position_.x_ = position.x_;
    position_.y_ = position.y_;
    rotation2D_ = rotation;
    rotation_ = Quaternion(rotation, Vector3::FORWARD);

    MarkDirty();
    MarkNetworkUpdate();
}

void Node::SetTransform2D(const Vector2& position, float rotation, const Vector2& scale)
{
    position_ = position;
    rotation2D_ = rotation;
    rotation_ = Quaternion(rotation, Vector3::FORWARD);
    SetScale2D(scale);

    MarkDirty();
    MarkNetworkUpdate();
}

void Node::SetTransform2D(const Matrix2x3& matrix)
{
    SetTransform2D(matrix.Translation(), matrix.Rotation(), matrix.Scale());
}

void Node::SetWorldPosition2D(const Vector2& position)
{
    SetPosition2D((parent_ == scene_ || !parent_) ? position : parent_->GetWorldTransform2D().Inverse() * position);
}

void Node::SetWorldRotation2D(float worldrotation)
{
    SetRotation2D((parent_ == scene_ || !parent_) ? worldrotation : worldrotation - parent_->GetWorldRotation2D());
}

void Node::SetWorldScale2D(const Vector2& scale)
{
    SetScale2D((parent_ == scene_ || !parent_) ? scale : scale / parent_->GetWorldScale2D());
}

void Node::SetWorldTransform2D(const Vector2& position, float rotation)
{
    SetWorldPosition2D(position);
    SetWorldRotation2D(rotation);
}

void Node::SetWorldTransform2D(const Vector2& position, float rotation, const Vector2& scale)
{
    SetWorldPosition2D(position);
    SetWorldRotation2D(rotation);
    SetWorldScale2D(scale);
}

void Node::SetWorldTransform2D(const Matrix2x3& worldtransform)
{
    SetWorldPosition2D(worldtransform.Translation());
    SetWorldRotation2D(worldtransform.Rotation());
    SetWorldScale2D(worldtransform.Scale());
}


void Node::Translate(const Vector3& delta, TransformSpace space)
{
    switch (space)
    {
    case TS_LOCAL:
        // Note: local space translation disregards local scale for scale-independent movement speed
        position_ += rotation_ * delta;
        break;

    case TS_PARENT:
        position_ += delta;
        break;

    case TS_WORLD:
        position_ += (parent_ == scene_ || !parent_) ? delta : parent_->GetWorldTransform().Inverse() * Vector4(delta, 0.0f);
        break;
    }

    MarkDirty();

    MarkNetworkUpdate();
}

void Node::Rotate(const Quaternion& delta, TransformSpace space)
{
    switch (space)
    {
    case TS_LOCAL:
        rotation_ = (rotation_ * delta).Normalized();
        rotation2D_ = rotation_.RollAngle();
        break;

    case TS_PARENT:
        rotation_ = (delta * rotation_).Normalized();
        rotation2D_ = rotation_.RollAngle();
        break;

    case TS_WORLD:
        if (parent_ == scene_ || !parent_)
        {
            rotation_ = (delta * rotation_).Normalized();
            rotation2D_ = rotation_.RollAngle();
        }
        else
        {
            Quaternion worldRotation = GetWorldRotation();
            rotation_ = rotation_ * worldRotation.Inverse() * delta * worldRotation;
            rotation2D_ = rotation_.RollAngle();
        }
        break;
    }

    MarkDirty();

    MarkNetworkUpdate();
}

void Node::RotateAround(const Vector3& point, const Quaternion& delta, TransformSpace space)
{
    Vector3 parentSpacePoint;
    Quaternion oldRotation = rotation_;

    switch (space)
    {
    case TS_LOCAL:
        parentSpacePoint = GetTransform() * point;
        rotation_ = (rotation_ * delta).Normalized();
        rotation2D_ = rotation_.RollAngle();
        break;

    case TS_PARENT:
        parentSpacePoint = point;
        rotation_ = (delta * rotation_).Normalized();
        rotation2D_ = rotation_.RollAngle();
        break;

    case TS_WORLD:
        if (parent_ == scene_ || !parent_)
        {
            parentSpacePoint = point;
            rotation_ = (delta * rotation_).Normalized();
            rotation2D_ = rotation_.RollAngle();
        }
        else
        {
            parentSpacePoint = parent_->GetWorldTransform().Inverse() * point;
            Quaternion worldRotation = GetWorldRotation();
            rotation_ = rotation_ * worldRotation.Inverse() * delta * worldRotation;
            rotation2D_ = rotation_.RollAngle();
        }
        break;
    }

    Vector3 oldRelativePos = oldRotation.Inverse() * (position_ - parentSpacePoint);
    position_ = rotation_ * oldRelativePos + parentSpacePoint;

    MarkDirty();

    MarkNetworkUpdate();
}

void Node::Yaw(float angle, TransformSpace space)
{
    Rotate(Quaternion(angle, Vector3::UP), space);
}

void Node::Pitch(float angle, TransformSpace space)
{
    Rotate(Quaternion(angle, Vector3::RIGHT), space);
}

void Node::Roll(float angle, TransformSpace space)
{
    Rotate(Quaternion(angle, Vector3::FORWARD), space);
}

bool Node::LookAt(const Vector3& target, const Vector3& up, TransformSpace space)
{
    Vector3 worldSpaceTarget;

    switch (space)
    {
    case TS_LOCAL:
        worldSpaceTarget = GetWorldTransform() * target;
        break;

    case TS_PARENT:
        worldSpaceTarget = (parent_ == scene_ || !parent_) ? target : parent_->GetWorldTransform() * target;
        break;

    case TS_WORLD:
        worldSpaceTarget = target;
        break;
    }

    Vector3 lookDir = worldSpaceTarget - GetWorldPosition();
    // Check if target is very close, in that case can not reliably calculate lookat direction
    if (lookDir.Equals(Vector3::ZERO))
        return false;
    Quaternion newRotation;
    // Do nothing if setting look rotation failed
    if (!newRotation.FromLookRotation(lookDir, up))
        return false;

    SetWorldRotation(newRotation);
    return true;
}

void Node::Scale(float scale)
{
    Scale(Vector3(scale, scale, scale));
}

void Node::Scale(const Vector3& scale)
{
    scale_ *= scale;
    MarkDirty();

    MarkNetworkUpdate();
}

/// FromBones :: Set enabled/disabled ChangeMode.
void Node::SetChangeModeEnable(bool enable)
{
    changeModeEnabled_ = enable;
}

void Node::SetEnabled(bool enable)
{
    SetEnabled(enable, false, true);
}

void Node::SetDeepEnabled(bool enable)
{
    SetEnabled(enable, true, false);
}

void Node::ResetDeepEnabled()
{
    SetEnabled(enabledPrev_, false, false);

    for (Vector<SharedPtr<Node> >::ConstIterator i = children_.Begin(); i != children_.End(); ++i)
        (*i)->ResetDeepEnabled();
}

void Node::SetEnabledRecursive(bool enable)
{
    SetEnabled(enable, true, true);
}

void Node::SetOwner(Connection* owner)
{
    owner_ = owner;
}

void Node::MarkDirty()
{
    Node *cur = this;
    for (;;)
    {
        // Precondition:
        // a) whenever a node is marked dirty, all its children are marked dirty as well.
        // b) whenever a node is cleared from being dirty, all its parents must have been
        //    cleared as well.
        // Therefore if we are recursing here to mark this node dirty, and it already was,
        // then all children of this node must also be already dirty, and we don't need to
        // reflag them again.
        if (cur->dirty_ && cur->dirty2D_)
            return;
        cur->dirty_ = true;
        cur->dirty2D_ = true;

        // Notify listener components first, then mark child nodes
        for (Vector<WeakPtr<Component> >::Iterator i = cur->listeners_.Begin(); i != cur->listeners_.End();)
        {
            Component *c = *i;
            if (c)
            {
                c->OnMarkedDirty(cur);
                ++i;
            }
            // If listener has expired, erase from list (swap with the last element to avoid O(n^2) behavior)
            else
            {
                *i = cur->listeners_.Back();
                cur->listeners_.Pop();
            }
        }

        // Tail call optimization: Don't recurse to mark the first child dirty, but
        // instead process it in the context of the current function. If there are more
        // than one child, then recurse to the excess children.
        Vector<SharedPtr<Node> >::Iterator i = cur->children_.Begin();
        if (i != cur->children_.End())
        {
            Node *next = *i;
            for (++i; i != cur->children_.End(); ++i)
                (*i)->MarkDirty();
            cur = next;
        }
        else
            return;
    }
}

Node* Node::CreateChild(const String& name, CreateMode mode, unsigned id, bool temporary)
{
    Node* newNode = CreateChild(id, mode, temporary);
    newNode->SetName(name);
    return newNode;
}

Node* Node::CreateTemporaryChild(const String& name, CreateMode mode, unsigned id)
{
    return CreateChild(name, mode, id, true);
}

void Node::AddChild(Node* node, unsigned index)
{
    // Check for illegal or redundant parent assignment
    if (!node || node == this || node->parent_ == this)
        return;
    // Check for possible cyclic parent assignment
    Node* parent = parent_;
    while (parent)
    {
        if (parent == node)
            return;
        parent = parent->parent_;
    }

    // Keep a shared ptr to the node while transfering
    SharedPtr<Node> nodeShared(node);
    Node* oldParent = node->parent_;
    if (oldParent)
    {
        // If old parent is in different scene, perform the full removal
        if (oldParent->GetScene() != scene_)
            oldParent->RemoveChild(node);
        else
        {
            if (scene_)
            {
                // Otherwise do not remove from the scene during reparenting, just send the necessary change event
                using namespace NodeRemoved;

                VariantMap& eventData = GetEventDataMap();
                eventData[P_SCENE] = scene_;
                eventData[P_PARENT] = oldParent;
                eventData[P_NODE] = node;

                scene_->SendEvent(E_NODEREMOVED, eventData);
            }

            oldParent->children_.Remove(nodeShared);
        }
    }

    // Add to the child vector, then add to the scene if not added yet
    children_.Insert(index, nodeShared);
    if (scene_ && node->GetScene() != scene_)
        scene_->NodeAdded(node);

    node->parent_ = this;
    node->MarkDirty();
    node->MarkNetworkUpdate();
    // If the child node has components, also mark network update on them to ensure they have a valid NetworkState
    for (Vector<SharedPtr<Component> >::Iterator i = node->components_.Begin(); i != node->components_.End(); ++i)
        (*i)->MarkNetworkUpdate();

    // Send change event
    if (scene_)
    {
        using namespace NodeAdded;

        VariantMap& eventData = GetEventDataMap();
        eventData[P_SCENE] = scene_;
        eventData[P_PARENT] = this;
        eventData[P_NODE] = node;

        scene_->SendEvent(E_NODEADDED, eventData);
    }
}

void Node::RemoveChild(Node* node)
{
    if (!node)
        return;

    for (Vector<SharedPtr<Node> >::Iterator i = children_.Begin(); i != children_.End(); ++i)
    {
        if (*i == node)
        {
            RemoveChild(i);
            return;
        }
    }
}

void Node::RemoveAllChildren()
{
    RemoveChildren(true, true, true);
}

void Node::RemoveChildren(bool removeReplicated, bool removeLocal, bool recursive)
{
    unsigned numRemoved = 0;

    for (unsigned i = children_.Size() - 1; i < children_.Size(); --i)
    {
        bool remove = false;
        Node* childNode = children_[i];

        if (recursive)
            childNode->RemoveChildren(removeReplicated, removeLocal, true);
        if (childNode->GetID() < FIRST_LOCAL_ID && removeReplicated)
            remove = true;
        else if (childNode->GetID() >= FIRST_LOCAL_ID && removeLocal)
            remove = true;

        if (remove)
        {
            RemoveChild(children_.Begin() + i);
            ++numRemoved;
        }
    }

    // Mark node dirty in all replication states
    if (numRemoved)
        MarkReplicationDirty();
}

Component* Node::CreateComponent(StringHash type, CreateMode mode, unsigned id)
{
    // Do not attempt to create replicated components to local nodes, as that may lead to component ID overwrite
    // as replicated components are synced over
    if (id_ >= FIRST_LOCAL_ID && mode == REPLICATED)
        mode = LOCAL;

    // Check that creation succeeds and that the object in fact is a component
    SharedPtr<Component> newComponent = DynamicCast<Component>(context_->CreateObject(type));
    if (!newComponent)
    {
//        URHO3D_LOGERROR("Could not create unknown component type " + type.ToString());
        URHO3D_LOGERRORF("Could not create unknown component type %u", type.Value());
        return 0;
    }

    AddComponent(newComponent, id, mode);
    return newComponent;
}

Component* Node::GetOrCreateComponent(StringHash type, CreateMode mode, unsigned id)
{
    Component* oldComponent = GetComponent(type);
    if (oldComponent)
        return oldComponent;
    else
        return CreateComponent(type, mode, id);
}

Component* Node::CloneComponent(Component* component, unsigned id)
{
    if (!component)
    {
        URHO3D_LOGERROR("Null source component given for CloneComponent");
        return 0;
    }

    return CloneComponent(component, component->GetID() < FIRST_LOCAL_ID && id < FIRST_LOCAL_ID ? REPLICATED : LOCAL, id);
}

Component* Node::CloneComponent(Component* component, CreateMode mode, unsigned id, bool applyAttr)
{
    if (!component)
    {
        URHO3D_LOGERROR("Null source component given for CloneComponent");
        return 0;
    }

    Component* cloneComponent = SafeCreateComponent(component->GetTypeName(), component->GetType(), mode, id);
    if (!cloneComponent)
    {
        URHO3D_LOGERROR("Could not clone component " + component->GetTypeName());
        return 0;
    }

    // FromBones ObjectPool Attr
    cloneComponent->changeModeEnabled_ = component->changeModeEnabled_;

    const Vector<AttributeInfo>* compAttributes = component->GetAttributes();
    const Vector<AttributeInfo>* cloneAttributes = cloneComponent->GetAttributes();

    if (compAttributes)
    {
        for (unsigned i = 0; i < compAttributes->Size() && i < cloneAttributes->Size(); ++i)
        {
            const AttributeInfo& attr = compAttributes->At(i);
            const AttributeInfo& cloneAttr = cloneAttributes->At(i);
            if (attr.mode_ & AM_FILE)
            {
                Variant value;
                component->OnGetAttribute(attr, value);
                // Note: when eg. a ScriptInstance component is cloned, its script object attributes are unique and therefore we
                // can not simply refer to the source component's AttributeInfo
                cloneComponent->OnSetAttribute(cloneAttr, value);
            }
        }
        if (applyAttr)
            cloneComponent->ApplyAttributes();
    }

    {
        using namespace ComponentCloned;

        VariantMap& eventData = GetEventDataMap();
        eventData[P_SCENE] = scene_;
        eventData[P_COMPONENT] = component;
        eventData[P_CLONECOMPONENT] = cloneComponent;

        scene_->SendEvent(E_COMPONENTCLONED, eventData);
    }

    return cloneComponent;
}

void Node::RemoveComponent(Component* component)
{
    for (Vector<SharedPtr<Component> >::Iterator i = components_.Begin(); i != components_.End(); ++i)
    {
        if (*i == component)
        {
            RemoveComponent(i);

            // Mark node dirty in all replication states
            MarkReplicationDirty();
            return;
        }
    }
}

void Node::RemoveComponent(StringHash type)
{
    for (Vector<SharedPtr<Component> >::Iterator i = components_.Begin(); i != components_.End(); ++i)
    {
        if ((*i)->GetType() == type)
        {
            RemoveComponent(i);

            // Mark node dirty in all replication states
            MarkReplicationDirty();
            return;
        }
    }
}

void Node::RemoveComponents(bool removeReplicated, bool removeLocal)
{
    unsigned numRemoved = 0;

    for (unsigned i = components_.Size() - 1; i < components_.Size(); --i)
    {
        bool remove = false;
        Component* component = components_[i];

        if (component->GetID() < FIRST_LOCAL_ID && removeReplicated)
            remove = true;
        else if (component->GetID() >= FIRST_LOCAL_ID && removeLocal)
            remove = true;

        if (remove)
        {
            RemoveComponent(components_.Begin() + i);
            ++numRemoved;
        }
    }

    // Mark node dirty in all replication states
    if (numRemoved)
        MarkReplicationDirty();
}

void Node::RemoveComponents(StringHash type)
{
    unsigned numRemoved = 0;

    for (unsigned i = components_.Size() - 1; i < components_.Size(); --i)
    {
        if (components_[i]->GetType() == type)
        {
            RemoveComponent(components_.Begin() + i);
            ++numRemoved;
        }
    }

    // Mark node dirty in all replication states
    if (numRemoved)
        MarkReplicationDirty();
}

void Node::RemoveAllComponents()
{
    RemoveComponents(true, true);
}

void Node::ReorderComponent(Component* component, unsigned index)
{
    if (!component || component->GetNode() != this)
        return;

    for (Vector<SharedPtr<Component> >::Iterator i = components_.Begin(); i != components_.End(); ++i)
    {
        if (*i == component)
        {
            // Need shared ptr to insert. Also, prevent destruction when removing first
            SharedPtr<Component> componentShared(component);
            components_.Erase(i);
            components_.Insert(index, componentShared);
            return;
        }
    }
}

//Node* Node::Clone(CreateMode mode, bool applyAttr)
//{
//    // The scene itself can not be cloned
//    if (this == scene_ || !parent_)
//    {
//        URHO3D_LOGERROR("Can not clone node without a parent");
//        return 0;
//    }
//
//    URHO3D_PROFILE(CloneNode);
//
//    SceneResolver resolver;
//    Node* clone = CloneRecursive(parent_, resolver, mode);
//    resolver.Resolve();
//
//    if (applyAttr)
//        clone->ApplyAttributes();
//    return clone;
//}

Node* Node::Clone(CreateMode mode, bool applyAttr, unsigned nodeid, unsigned componentid, Node* parent)
{
    if (!parent) parent = parent_;

    // The scene itself can not be cloned
    if (this == scene_ || !parent_)
    {
        URHO3D_LOGERROR("Can not clone node without a parent");
        return 0;
    }

    SceneResolver resolver;
    Node* clone = CloneRecursive(parent, resolver, mode, nodeid, componentid, applyAttr);
    resolver.Resolve();

    if (applyAttr)
        clone->ApplyAttributes();

    return clone;
}

void Node::Remove()
{
    if (parent_)
        parent_->RemoveChild(this);
}

void Node::SetParent(Node* parent)
{
    if (parent)
    {
        Matrix3x4 oldWorldTransform = GetWorldTransform();

        parent->AddChild(this);

        if (parent != scene_)
        {
            Matrix3x4 newTransform = parent->GetWorldTransform().Inverse() * oldWorldTransform;
            SetTransform(newTransform.Translation(), newTransform.Rotation(), newTransform.Scale());
        }
        else
        {
            // The root node is assumed to have identity transform, so can disregard it
            SetTransform(oldWorldTransform.Translation(), oldWorldTransform.Rotation(), oldWorldTransform.Scale());
        }
    }
}

void Node::SetVar(StringHash key, const Variant& value)
{
    vars_[key] = value;
    MarkNetworkUpdate();
}

void Node::AddListener(Component* component)
{
    if (!component)
        return;

    // Check for not adding twice
    for (Vector<WeakPtr<Component> >::Iterator i = listeners_.Begin(); i != listeners_.End(); ++i)
    {
        if (*i == component)
            return;
    }

    listeners_.Push(WeakPtr<Component>(component));
    // If the node is currently dirty, notify immediately
    if (dirty_ || dirty2D_)
        component->OnMarkedDirty(this);
}

void Node::RemoveListener(Component* component)
{
    for (Vector<WeakPtr<Component> >::Iterator i = listeners_.Begin(); i != listeners_.End(); ++i)
    {
        if (*i == component)
        {
            listeners_.Erase(i);
            return;
        }
    }
}

Vector3 Node::LocalToWorld(const Vector3& position) const
{
    return GetWorldTransform() * position;
}

Vector3 Node::LocalToWorld(const Vector4& vector) const
{
    return GetWorldTransform() * vector;
}

Vector2 Node::LocalToWorld2D(const Vector2& vector) const
{
    return GetWorldTransform2D() * vector;
}

Vector3 Node::WorldToLocal(const Vector3& position) const
{
    return GetWorldTransform().Inverse() * position;
}

Vector3 Node::WorldToLocal(const Vector4& vector) const
{
    return GetWorldTransform().Inverse() * vector;
}

Vector2 Node::WorldToLocal2D(const Vector2& vector) const
{
    return GetWorldTransform2D().Inverse() * vector;
}

unsigned Node::GetNumChildren(bool recursive) const
{
    if (!recursive)
        return children_.Size();
    else
    {
        unsigned allChildren = children_.Size();
        for (Vector<SharedPtr<Node> >::ConstIterator i = children_.Begin(); i != children_.End(); ++i)
            allChildren += (*i)->GetNumChildren(true);

        return allChildren;
    }
}

void Node::GetChildren(PODVector<Node*>& dest, bool recursive) const
{
    dest.Clear();

    if (!recursive)
    {
        for (Vector<SharedPtr<Node> >::ConstIterator i = children_.Begin(); i != children_.End(); ++i)
            dest.Push(*i);
    }
    else
        GetChildrenRecursive(dest);
}

PODVector<Node*> Node::GetChildren(bool recursive) const
{
    PODVector<Node*> dest;
    GetChildren(dest, recursive);
    return dest;
}

void Node::GetChildrenWithComponent(PODVector<Node*>& dest, StringHash type, bool recursive) const
{
    dest.Clear();

    if (!recursive)
    {
        for (Vector<SharedPtr<Node> >::ConstIterator i = children_.Begin(); i != children_.End(); ++i)
        {
            if ((*i)->HasComponent(type))
                dest.Push(*i);
        }
    }
    else
        GetChildrenWithComponentRecursive(dest, type);
}

PODVector<Node*> Node::GetChildrenWithComponent(StringHash type, bool recursive) const
{
    PODVector<Node*> dest;
    GetChildrenWithComponent(dest, type, recursive);
    return dest;
}

void Node::GetChildrenWithName(PODVector<Node*>& dest, const String& name, bool recursive /*= false*/) const
{
    dest.Clear();

    if (!recursive)
    {
        for (Vector<SharedPtr<Node> >::ConstIterator i = children_.Begin(); i != children_.End(); ++i)
        {
            if ((*i)->GetName() == name)
                dest.Push(*i);
        }
    }
    else
        GetChildrenWithNameRecursive(dest, name);
}

void Node::GetChildrenWithNameStartsWith(PODVector<Node*>& dest, const String& basename, bool recursive /*= false*/) const
{
    dest.Clear();

    if (!recursive)
    {
        for (Vector<SharedPtr<Node> >::ConstIterator i = children_.Begin(); i != children_.End(); ++i)
        {
            if ((*i)->GetName().StartsWith(basename))
                dest.Push(*i);
        }
    }
    else
        GetChildrenWithNameStartsWithRecursive(dest, basename);
}

void Node::GetChildrenWithTag(PODVector<Node*>& dest, const String& tag, bool recursive /*= true*/) const
{
    dest.Clear();

    if (!recursive)
    {
        for (Vector<SharedPtr<Node> >::ConstIterator i = children_.Begin(); i != children_.End(); ++i)
        {
            if ((*i)->HasTag(tag))
                dest.Push(*i);
        }
    }
    else
        GetChildrenWithTagRecursive(dest, tag);
}

PODVector<Node*> Node::GetChildrenWithTag(const String& tag, bool recursive) const
{
    PODVector<Node*> dest;
    GetChildrenWithTag(dest, tag, recursive);
    return dest;
}

Node* Node::GetChild(unsigned index) const
{
    return index < children_.Size() ? children_[index].Get() : 0;
}

Node* Node::GetChild(const String& name, bool recursive) const
{
    return GetChild(StringHash(name), recursive);
}

Node* Node::GetChild(const char* name, bool recursive) const
{
    return GetChild(StringHash(name), recursive);
}

Node* Node::GetChild(StringHash nameHash, bool recursive) const
{
    for (Vector<SharedPtr<Node> >::ConstIterator i = children_.Begin(); i != children_.End(); ++i)
    {
		if (*i == 0)
			continue;

        if ((*i)->GetNameHash() == nameHash)
            return *i;

        if (recursive)
        {
            Node* node = (*i)->GetChild(nameHash, true);
            if (node)
                return node;
        }
    }

    return 0;
}

unsigned Node::GetNumNetworkComponents() const
{
    unsigned num = 0;
    for (Vector<SharedPtr<Component> >::ConstIterator i = components_.Begin(); i != components_.End(); ++i)
    {
        if ((*i)->GetID() < FIRST_LOCAL_ID)
            ++num;
    }

    return num;
}

void Node::GetComponents(PODVector<Component*>& dest, StringHash type, bool recursive) const
{
    dest.Clear();

    if (!recursive)
    {
        for (Vector<SharedPtr<Component> >::ConstIterator i = components_.Begin(); i != components_.End(); ++i)
        {
            if ((*i)->GetType() == type)
                dest.Push(*i);
        }
    }
    else
        GetComponentsRecursive(dest, type);
}

bool Node::HasComponent(StringHash type) const
{
    for (Vector<SharedPtr<Component> >::ConstIterator i = components_.Begin(); i != components_.End(); ++i)
    {
        if ((*i)->GetType() == type)
            return true;
    }
    return false;
}

bool Node::HasTag(const String& tag) const
{
    return tags_.Contains(tag);
}

const Variant& Node::GetVar(StringHash key) const
{
    VariantMap::ConstIterator i = vars_.Find(key);
    return i != vars_.End() ? i->second_ : Variant::EMPTY;
}

Component* Node::GetComponent(StringHash type, bool recursive) const
{
    for (Vector<SharedPtr<Component> >::ConstIterator i = components_.Begin(); i != components_.End(); ++i)
    {
        if ((*i)->GetType() == type)
            return *i;
    }

    if (recursive)
    {
        for (Vector<SharedPtr<Node> >::ConstIterator i = children_.Begin(); i != children_.End(); ++i)
        {
            Component* component = (*i)->GetComponent(type, true);
            if (component)
                return component;
        }
    }

    return 0;
}

Component* Node::GetParentComponent(StringHash type, bool fullTraversal) const
{
    Node* current = GetParent();
    while (current)
    {
        Component* soughtComponent = current->GetComponent(type);
        if (soughtComponent)
            return soughtComponent;

        if (fullTraversal)
            current = current->GetParent();
        else
            break;
    }
    return 0;
}

void Node::SetID(unsigned id)
{
    id_ = id;
}

void Node::SetScene(Scene* scene)
{
    scene_ = scene;
}

void Node::ResetScene()
{
	if (!this)
    {
        URHO3D_LOGERROR("Node() - ResetScene : this==0 !");
        return;
    }

	SetID(0);
    SetScene(0);
    SetOwner(0);
}

void Node::SetNetPositionAttr(const Vector3& value)
{
    SmoothedTransform* transform = GetComponent<SmoothedTransform>();
    if (transform)
        transform->SetTargetPosition(value);
    else
        SetPosition(value);
}

void Node::SetNetRotation(const Quaternion& rotation)
{
    SmoothedTransform* transform = GetComponent<SmoothedTransform>();
    if (transform)
        transform->SetTargetRotation(rotation);
    else
        SetRotation(rotation);
}

void Node::SetNetRotationAttr(const PODVector<unsigned char>& value)
{
    MemoryBuffer buf(value);
    SmoothedTransform* transform = GetComponent<SmoothedTransform>();
    if (transform)
        transform->SetTargetRotation(buf.ReadPackedQuaternion());
    else
        SetRotation(buf.ReadPackedQuaternion());
}

void Node::SetNetParentAttr(const PODVector<unsigned char>& value)
{
    Scene* scene = GetScene();
    if (!scene)
        return;

    MemoryBuffer buf(value);
    // If nothing in the buffer, parent is the root node
    if (buf.IsEof())
    {
        scene->AddChild(this);
        return;
    }

    unsigned baseNodeID = buf.ReadNetID();
    Node* baseNode = scene->GetNode(baseNodeID);
    if (!baseNode)
    {
        URHO3D_LOGWARNING("Failed to find parent node " + String(baseNodeID));
        return;
    }

    // If buffer contains just an ID, the parent is replicated and we are done
    if (buf.IsEof())
        baseNode->AddChild(this);
    else
    {
        // Else the parent is local and we must find it recursively by name hash
        StringHash nameHash = buf.ReadStringHash();
        Node* parentNode = baseNode->GetChild(nameHash, true);
        if (!parentNode)
            URHO3D_LOGWARNING("Failed to find parent node with name hash " + nameHash.ToString());
        else
            parentNode->AddChild(this);
    }
}

const Vector3& Node::GetNetPositionAttr() const
{
    return position_;
}

const PODVector<unsigned char>& Node::GetNetRotationAttr() const
{
    attrBuffer_.Clear();
    attrBuffer_.WritePackedQuaternion(rotation_);
    return attrBuffer_.GetBuffer();
}

const PODVector<unsigned char>& Node::GetNetParentAttr() const
{
    attrBuffer_.Clear();
    Scene* scene = GetScene();
    if (scene && parent_ && parent_ != scene)
    {
        // If parent is replicated, can write the ID directly
        unsigned parentID = parent_->GetID();
        if (parentID < FIRST_LOCAL_ID)
            attrBuffer_.WriteNetID(parentID);
        else
        {
            // Parent is local: traverse hierarchy to find a non-local base node
            // This iteration always stops due to the scene (root) being non-local
            Node* current = parent_;
            while (current->GetID() >= FIRST_LOCAL_ID)
                current = current->GetParent();

            // Then write the base node ID and the parent's name hash
            attrBuffer_.WriteNetID(current->GetID());
            attrBuffer_.WriteStringHash(parent_->GetNameHash());
        }
    }

    return attrBuffer_.GetBuffer();
}

bool Node::Load(Deserializer& source, SceneResolver& resolver, bool readChildren, bool rewriteIDs, CreateMode mode)
{
    // Remove all children and components first in case this is not a fresh load
    RemoveAllChildren();
    RemoveAllComponents();

    // ID has been read at the parent level
    if (!Animatable::Load(source))
        return false;

    unsigned numComponents = source.ReadVLE();
    for (unsigned i = 0; i < numComponents; ++i)
    {
        VectorBuffer compBuffer(source, source.ReadVLE());
        StringHash compType = compBuffer.ReadStringHash();
        unsigned compID = compBuffer.ReadUInt();

        Component* newComponent = SafeCreateComponent(String::EMPTY, compType,
            (mode == REPLICATED && compID < FIRST_LOCAL_ID) ? REPLICATED : LOCAL, rewriteIDs ? 0 : compID);
        if (newComponent)
        {
            resolver.AddComponent(compID, newComponent);
            // Do not abort if component fails to load, as the component buffer is nested and we can skip to the next
            newComponent->Load(compBuffer);
        }
    }

    if (!readChildren)
        return true;

    unsigned numChildren = source.ReadVLE();
    for (unsigned i = 0; i < numChildren; ++i)
    {
        unsigned nodeID = source.ReadUInt();
        Node* newNode = CreateChild(rewriteIDs ? 0 : nodeID, (mode == REPLICATED && nodeID < FIRST_LOCAL_ID) ? REPLICATED :
            LOCAL);
        resolver.AddNode(nodeID, newNode);
        if (!newNode->Load(source, resolver, readChildren, rewriteIDs, mode))
            return false;
    }

    return true;
}

bool Node::LoadXML(const XMLElement& source, SceneResolver& resolver, bool readChildren, bool rewriteIDs, CreateMode mode)
{
    // Remove all children and components first in case this is not a fresh load
    RemoveAllChildren();
    RemoveAllComponents();

    if (!Animatable::LoadXML(source))
        return false;

    XMLElement compElem = source.GetChild("component");
    while (compElem)
    {
        String typeName = compElem.GetAttribute("type");
        unsigned compID = compElem.GetUInt("id");
        Component* newComponent = SafeCreateComponent(typeName, StringHash(typeName),
            (mode == REPLICATED && compID < FIRST_LOCAL_ID) ? REPLICATED : LOCAL, rewriteIDs ? 0 : compID);

        if (newComponent)
        {
            resolver.AddComponent(compID, newComponent);
            if (!newComponent->LoadXML(compElem))
            {
//                URHO3D_LOGERRORF("Node::LoadXML : node=%s(%u) component=%s compptr=%u ERROR", name_.CString(), id_, typeName.CString(), newComponent);
                return false;
            }
        }

        compElem = compElem.GetNext("component");
    }

    if (!readChildren)
        return true;

    XMLElement childElem = source.GetChild("node");
    while (childElem)
    {
        unsigned nodeID = childElem.GetUInt("id");
        Node* newNode = CreateChild(rewriteIDs ? 0 : nodeID, (mode == REPLICATED && nodeID < FIRST_LOCAL_ID) ? REPLICATED :
            LOCAL);
        resolver.AddNode(nodeID, newNode);
        if (!newNode->LoadXML(childElem, resolver, readChildren, rewriteIDs, mode))
            return false;

        childElem = childElem.GetNext("node");
    }

    return true;
}

bool Node::LoadJSON(const JSONValue& source, SceneResolver& resolver, bool readChildren, bool rewriteIDs, CreateMode mode)
{
    // Remove all children and components first in case this is not a fresh load
    RemoveAllChildren();
    RemoveAllComponents();

    if (!Animatable::LoadJSON(source))
        return false;

    const JSONArray& componentsArray = source.Get("components").GetArray();

    for (unsigned i = 0; i < componentsArray.Size(); i++)
    {
        const JSONValue& compVal = componentsArray.At(i);
        String typeName = compVal.Get("type").GetString();
        unsigned compID = compVal.Get("id").GetUInt();
        Component* newComponent = SafeCreateComponent(typeName, StringHash(typeName),
            (mode == REPLICATED && compID < FIRST_LOCAL_ID) ? REPLICATED : LOCAL, rewriteIDs ? 0 : compID);
        if (newComponent)
        {
            resolver.AddComponent(compID, newComponent);
            if (!newComponent->LoadJSON(compVal))
                return false;
        }
    }

    if (!readChildren)
        return true;

    const JSONArray& childrenArray = source.Get("children").GetArray();
    for (unsigned i = 0; i < childrenArray.Size(); i++)
    {
        const JSONValue& childVal = childrenArray.At(i);

        unsigned nodeID = childVal.Get("id").GetUInt();
        Node* newNode = CreateChild(rewriteIDs ? 0 : nodeID, (mode == REPLICATED && nodeID < FIRST_LOCAL_ID) ? REPLICATED :
            LOCAL);
        resolver.AddNode(nodeID, newNode);
        if (!newNode->LoadJSON(childVal, resolver, readChildren, rewriteIDs, mode))
            return false;
    }

    return true;
}

void Node::PrepareNetworkUpdate()
{
    // Update dependency nodes list first
    dependencyNodes_.Clear();

    // Add the parent node, but if it is local, traverse to the first non-local node
    if (parent_ && parent_ != scene_)
    {
        Node* current = parent_;
        while (current->id_ >= FIRST_LOCAL_ID)
            current = current->parent_;
        if (current && current != scene_)
            dependencyNodes_.Push(current);
    }

    // Let the components add their dependencies
    for (Vector<SharedPtr<Component> >::ConstIterator i = components_.Begin(); i != components_.End(); ++i)
    {
        Component* component = *i;
        if (component->GetID() < FIRST_LOCAL_ID)
            component->GetDependencyNodes(dependencyNodes_);
    }

    // Then check for node attribute changes
    if (!networkState_)
        AllocateNetworkState();

    const Vector<AttributeInfo>* attributes = networkState_->attributes_;
    unsigned numAttributes = attributes->Size();

    if (networkState_->currentValues_.Size() != numAttributes)
    {
        networkState_->currentValues_.Resize(numAttributes);
        networkState_->previousValues_.Resize(numAttributes);

        // Copy the default attribute values to the previous state as a starting point
        for (unsigned i = 0; i < numAttributes; ++i)
            networkState_->previousValues_[i] = attributes->At(i).defaultValue_;
    }

    // Check for attribute changes
    for (unsigned i = 0; i < numAttributes; ++i)
    {
        const AttributeInfo& attr = attributes->At(i);

        if (animationEnabled_ && IsAnimatedNetworkAttribute(attr))
            continue;

        OnGetAttribute(attr, networkState_->currentValues_[i]);

        if (networkState_->currentValues_[i] != networkState_->previousValues_[i])
        {
            networkState_->previousValues_[i] = networkState_->currentValues_[i];

            // Mark the attribute dirty in all replication states that are tracking this node
            for (PODVector<ReplicationState*>::Iterator j = networkState_->replicationStates_.Begin();
                 j != networkState_->replicationStates_.End(); ++j)
            {
                NodeReplicationState* nodeState = static_cast<NodeReplicationState*>(*j);
                nodeState->dirtyAttributes_.Set(i);

                // Add node to the dirty set if not added yet
                if (!nodeState->markedDirty_)
                {
                    nodeState->markedDirty_ = true;
                    nodeState->sceneState_->dirtyNodes_.Insert(id_);
                }
            }
        }
    }

//    if (GetName() == "Bombe")
//        URHO3D_LOGINFOF("Node() - PrepareNetworkUpdate : node=%s(%u) ... numAttributes=%u currentValues_.Size=%u",
//                        GetName().CString(), GetID(), numAttributes, networkState_->currentValues_.Size());

    // Finally check for user var changes
    for (VariantMap::ConstIterator i = vars_.Begin(); i != vars_.End(); ++i)
    {
        VariantMap::ConstIterator j = networkState_->previousVars_.Find(i->first_);
        if (j == networkState_->previousVars_.End() || j->second_ != i->second_)
        {
            networkState_->previousVars_[i->first_] = i->second_;

            // Mark the var dirty in all replication states that are tracking this node
            for (PODVector<ReplicationState*>::Iterator j = networkState_->replicationStates_.Begin();
                 j != networkState_->replicationStates_.End(); ++j)
            {
                NodeReplicationState* nodeState = static_cast<NodeReplicationState*>(*j);
                nodeState->dirtyVars_.Insert(i->first_);

                if (!nodeState->markedDirty_)
                {
                    nodeState->markedDirty_ = true;
                    nodeState->sceneState_->dirtyNodes_.Insert(id_);
                }
            }
        }
    }

//    URHO3D_LOGINFOF("Node() - PrepareNetworkUpdate : node=%s(%u) ... OK !", GetName().CString(), GetID());

    networkUpdate_ = false;
}

void Node::CleanupConnection(Connection* connection)
{
    if (owner_ == connection)
        owner_ = 0;

    if (networkState_)
    {
        for (unsigned i = networkState_->replicationStates_.Size() - 1; i < networkState_->replicationStates_.Size(); --i)
        {
            if (networkState_->replicationStates_[i]->connection_ == connection)
                networkState_->replicationStates_.Erase(i);
        }
    }
}

void Node::MarkReplicationDirty()
{
    if (networkState_)
    {
        for (PODVector<ReplicationState*>::Iterator j = networkState_->replicationStates_.Begin();
             j != networkState_->replicationStates_.End(); ++j)
        {
            NodeReplicationState* nodeState = static_cast<NodeReplicationState*>(*j);
            if (!nodeState->markedDirty_)
            {
                nodeState->markedDirty_ = true;
                nodeState->sceneState_->dirtyNodes_.Insert(id_);
            }
        }
    }
}

Node* Node::CreateChild(unsigned id, CreateMode mode, bool temporary)
{
    SharedPtr<Node> newNode(new Node(context_));
    newNode->SetTemporary(temporary);

    // If zero ID specified, or the ID is already taken, let the scene assign
    if (scene_)
    {
        if (!id || scene_->GetNode(id))
            id = scene_->GetFreeNodeID(mode);
        newNode->SetID(id);
    }
    else
        newNode->SetID(id);

    AddChild(newNode);
    return newNode;
}

void Node::AddComponent(Component* component, unsigned id, CreateMode mode)
{
    if (!component)
        return;

    components_.Push(SharedPtr<Component>(component));

    if (component->GetNode())
        URHO3D_LOGWARNING("Component " + component->GetTypeName() + " already belongs to a node!");

    component->SetNode(this);

    // If zero ID specified, or the ID is already taken, let the scene assign
    if (scene_)
    {
        if (!id || scene_->GetComponent(id))
            id = scene_->GetFreeComponentID(mode);
        component->SetID(id);
        scene_->ComponentAdded(component);
    }
    else
        component->SetID(id);

    component->OnMarkedDirty(this);

    // Check attributes of the new component on next network update, and mark node dirty in all replication states
    component->MarkNetworkUpdate();
    MarkNetworkUpdate();
    MarkReplicationDirty();

    // Send change event
    if (scene_)
    {
        using namespace ComponentAdded;

        VariantMap& eventData = GetEventDataMap();
        eventData[P_SCENE] = scene_;
        eventData[P_NODE] = this;
        eventData[P_COMPONENT] = component;

        scene_->SendEvent(E_COMPONENTADDED, eventData);
    }
}

unsigned Node::GetNumPersistentChildren() const
{
    unsigned ret = 0;

    for (Vector<SharedPtr<Node> >::ConstIterator i = children_.Begin(); i != children_.End(); ++i)
    {
        if (!(*i)->IsTemporary())
            ++ret;
    }

    return ret;
}

unsigned Node::GetNumPersistentComponents() const
{
    unsigned ret = 0;

    for (Vector<SharedPtr<Component> >::ConstIterator i = components_.Begin(); i != components_.End(); ++i)
    {
        if (!(*i)->IsTemporary())
            ++ret;
    }

    return ret;
}

void Node::SetTransformSilent(const Vector3& position, const Quaternion& rotation, const Vector3& scale)
{
    position_ = position;
    rotation_ = rotation;
    rotation2D_ = rotation.RollAngle();
    scale_ = scale;
}

void Node::OnAttributeAnimationAdded()
{
    if (attributeAnimationInfos_.Size() == 1)
        SubscribeToEvent(GetScene(), E_ATTRIBUTEANIMATIONUPDATE, URHO3D_HANDLER(Node, HandleAttributeAnimationUpdate));
}

void Node::OnAttributeAnimationRemoved()
{
    if (attributeAnimationInfos_.Empty())
        UnsubscribeFromEvent(GetScene(), E_ATTRIBUTEANIMATIONUPDATE);
}

Animatable* Node::FindAttributeAnimationTarget(const String& name, String& outName)
{
    Vector<String> names = name.Split('/');
    // Only attribute name
    if (names.Size() == 1)
    {
        outName = name;
        return this;
    }
    else
    {
        // Name must in following format: "#0/#1/@component#0/attribute"
        Node* node = this;
        unsigned i = 0;
        for (; i < names.Size() - 1; ++i)
        {
            if (names[i].Front() != '#')
                break;

            unsigned index = ToUInt(names[i].Substring(1, names[i].Length() - 1));
            node = node->GetChild(index);
            if (!node)
            {
                URHO3D_LOGERROR("Could not find node by name " + name);
                return 0;
            }
        }

        if (i == names.Size() - 1)
        {
            outName = names.Back();
            return node;
        }

        if (i != names.Size() - 2 || names[i].Front() != '@')
        {
            URHO3D_LOGERROR("Invalid name " + name);
            return 0;
        }

        String componentName = names[i].Substring(1, names[i].Length() - 1);
        Vector<String> componentNames = componentName.Split('#');
        if (componentNames.Size() == 1)
        {
            Component* component = node->GetComponent(StringHash(componentNames.Front()));
            if (!component)
            {
                URHO3D_LOGERROR("Could not find component by name " + name);
                return 0;
            }

            outName = names.Back();
            return component;
        }
        else
        {
            unsigned index = ToUInt(componentNames[1]);
            PODVector<Component*> components;
            node->GetComponents(components, StringHash(componentNames.Front()));
            if (index >= components.Size())
            {
                URHO3D_LOGERROR("Could not find component by name " + name);
                return 0;
            }

            outName = names.Back();
            return components[index];
        }
    }
}

void Node::SetEnabled(bool enable, bool recursive, bool storeSelf)
{
    // The enabled state of the whole scene can not be changed. SetUpdateEnabled() is used instead to start/stop updates.
    if (GetType() == Scene::GetTypeStatic())
    {
        URHO3D_LOGERROR("Can not change enabled state of the Scene");
        return;
    }

    if (storeSelf)
        enabledPrev_ = enable;

    if (enable != enabled_)
    {
        enabled_ = enable;
        MarkNetworkUpdate();

        // Notify listener components of the state change
        for (Vector<WeakPtr<Component> >::Iterator i = listeners_.Begin(); i != listeners_.End();)
        {
            if (*i)
            {
                (*i)->OnNodeSetEnabled(this);
                ++i;
            }
            // If listener has expired, erase from list
            else
                i = listeners_.Erase(i);
        }

        // Send change event
        if (scene_)
        {
            using namespace NodeEnabledChanged;

            VariantMap& eventData = GetEventDataMap();
            eventData[P_SCENE] = scene_;
            eventData[P_NODE] = this;

            scene_->SendEvent(E_NODEENABLEDCHANGED, eventData);
        }

        for (Vector<SharedPtr<Component> >::Iterator i = components_.Begin(); i != components_.End(); ++i)
        {
            (*i)->OnSetEnabled();

            // Send change event for the component
            if (scene_)
            {
                using namespace ComponentEnabledChanged;

                VariantMap& eventData = GetEventDataMap();
                eventData[P_SCENE] = scene_;
                eventData[P_NODE] = this;
                eventData[P_COMPONENT] = (*i);

                scene_->SendEvent(E_COMPONENTENABLEDCHANGED, eventData);
            }
        }
    }

    if (recursive)
    {
        for (Vector<SharedPtr<Node> >::Iterator i = children_.Begin(); i != children_.End(); ++i)
            (*i)->SetEnabled(enable, recursive, storeSelf);
    }
}

Component* Node::SafeCreateComponent(const String& typeName, StringHash type, CreateMode mode, unsigned id)
{
    // Do not attempt to create replicated components to local nodes, as that may lead to component ID overwrite
    // as replicated components are synced over
    if (id_ >= FIRST_LOCAL_ID && mode == REPLICATED)
        mode = LOCAL;

    // First check if factory for type exists
    if (!context_->GetTypeName(type).Empty())
        return CreateComponent(type, mode, id);
    else
    {
        URHO3D_LOGWARNING("Component type " + type.ToString() + " not known, creating UnknownComponent as placeholder");
        // Else create as UnknownComponent
        SharedPtr<UnknownComponent> newComponent(new UnknownComponent(context_));
        if (typeName.Empty() || typeName.StartsWith("Unknown", false))
            newComponent->SetType(type);
        else
            newComponent->SetTypeName(typeName);

        AddComponent(newComponent, id, mode);
        return newComponent;
    }
}

void Node::UpdateWorldTransform() const
{
    Matrix3x4 transform = GetTransform();
    Matrix2x3 transform2d = GetTransform2D();

    // Assume the root node (scene) has identity transform
    if (parent_ == scene_ || !parent_)
    {
        worldTransform_ = transform;
        worldRotation_ = rotation_;
        worldTransform2D_ = transform2d;
        worldRotation2D_ = rotation2D_;
    }
    else
    {
        worldTransform_ = parent_->GetWorldTransform() * transform;
        worldRotation_ = parent_->GetWorldRotation() * rotation_;
        worldTransform2D_ = parent_->GetWorldTransform2D() * transform2d;
        worldRotation2D_ = parent_->GetWorldRotation2D() + rotation2D_;
    }

    dirty_ = false;
    dirty2D_ = false;
}

void Node::UpdateWorldTransform2D() const
{
    Matrix2x3 transform2d = GetTransform2D();

    // Assume the root node (scene) has identity transform
    if (parent_ == scene_ || !parent_)
    {
        worldTransform2D_ = transform2d;
        worldRotation2D_ = rotation2D_;
    }
    else
    {
        worldTransform2D_ = parent_->GetWorldTransform2D() * transform2d;
        worldRotation2D_ = parent_->GetWorldRotation2D() + rotation2D_;
    }

    dirty2D_ = false;
}

void Node::RemoveChild(Vector<SharedPtr<Node> >::Iterator i)
{
    // Send change event. Do not send when already being destroyed
    Node* child = *i;

    if (Refs() > 0 && scene_)
    {
        using namespace NodeRemoved;

        VariantMap& eventData = GetEventDataMap();
        eventData[P_SCENE] = scene_;
        eventData[P_PARENT] = this;
        eventData[P_NODE] = child;

        scene_->SendEvent(E_NODEREMOVED, eventData);
    }

    child->parent_ = 0;
    child->MarkDirty();
    child->MarkNetworkUpdate();
    if (scene_)
        scene_->NodeRemoved(child);

    children_.Erase(i);
}

void Node::GetChildrenRecursive(PODVector<Node*>& dest) const
{
    for (Vector<SharedPtr<Node> >::ConstIterator i = children_.Begin(); i != children_.End(); ++i)
    {
        Node* node = *i;
        dest.Push(node);
        if (!node->children_.Empty())
            node->GetChildrenRecursive(dest);
    }
}

void Node::GetChildrenWithComponentRecursive(PODVector<Node*>& dest, StringHash type) const
{
    for (Vector<SharedPtr<Node> >::ConstIterator i = children_.Begin(); i != children_.End(); ++i)
    {
        Node* node = *i;
        if (node->HasComponent(type))
            dest.Push(node);
        if (!node->children_.Empty())
            node->GetChildrenWithComponentRecursive(dest, type);
    }
}

void Node::GetComponentsRecursive(PODVector<Component*>& dest, StringHash type) const
{
    for (Vector<SharedPtr<Component> >::ConstIterator i = components_.Begin(); i != components_.End(); ++i)
    {
        if ((*i)->GetType() == type)
            dest.Push(*i);
    }
    for (Vector<SharedPtr<Node> >::ConstIterator i = children_.Begin(); i != children_.End(); ++i)
        (*i)->GetComponentsRecursive(dest, type);
}

void Node::GetChildrenWithNameRecursive(PODVector<Node*>& dest, const String& name) const
{
    for (Vector<SharedPtr<Node> >::ConstIterator i = children_.Begin(); i != children_.End(); ++i)
    {
        Node* node = *i;
        if (node->GetName() == name)
            dest.Push(node);
        if (!node->children_.Empty())
            node->GetChildrenWithNameRecursive(dest, name);
    }
}

void Node::GetChildrenWithNameStartsWithRecursive(PODVector<Node*>& dest, const String& name) const
{
    for (Vector<SharedPtr<Node> >::ConstIterator i = children_.Begin(); i != children_.End(); ++i)
    {
        Node* node = *i;
        if (node->GetName().StartsWith(name))
            dest.Push(node);
        if (!node->children_.Empty())
            node->GetChildrenWithNameStartsWithRecursive(dest, name);
    }
}


void Node::GetChildrenWithTagRecursive(PODVector<Node*>& dest, const String& tag) const
{
    for (Vector<SharedPtr<Node> >::ConstIterator i = children_.Begin(); i != children_.End(); ++i)
    {
        Node* node = *i;
        if (node->HasTag(tag))
            dest.Push(node);
        if (!node->children_.Empty())
            node->GetChildrenWithTagRecursive(dest, tag);
    }
}

Node* Node::CloneRecursive(Node* parent, SceneResolver& resolver, CreateMode mode, unsigned nodeid, unsigned componentid, bool applyAttr)
{
    // Create clone node
//    Node* cloneNode = parent->CreateChild(0, (mode == REPLICATED && id_ < FIRST_LOCAL_ID) ? REPLICATED : LOCAL);
//    resolver.AddNode(id_, cloneNode);
//    Node* cloneNode = parent->CreateChild(id, id >= FIRST_LOCAL_ID ? LOCAL : REPLICATED);
//    if (!id) resolver.AddNode(id_, cloneNode);
    Node* cloneNode = parent->CreateChild(nodeid, (mode == LOCAL || nodeid >= FIRST_LOCAL_ID) ? LOCAL : REPLICATED);

    if (!nodeid)
        resolver.AddNode(id_, cloneNode);

    // FromBones ObjectPool Attr
    cloneNode->isPoolNode_ = isPoolNode_;
    cloneNode->changeModeEnabled_ = changeModeEnabled_;

    // Copy attributes
    const Vector<AttributeInfo>* attributes = GetAttributes();
    for (unsigned j = 0; j < attributes->Size(); ++j)
    {
        const AttributeInfo& attr = attributes->At(j);
        // Do not copy network-only attributes, as they may have unintended side effects
        if (attr.mode_ & AM_FILE)
        {
            Variant value;
            OnGetAttribute(attr, value);
            cloneNode->OnSetAttribute(attr, value);
        }
    }

    // Clone components
    if (isPoolNode_ && componentid)
    {
        for (Vector<SharedPtr<Component> >::ConstIterator i = components_.Begin(); i != components_.End(); ++i)
        {
            Component* component = *i;
            if (component->IsTemporary())
                continue;

            componentid++;

            Component* cloneComponent = cloneNode->CloneComponent(component,
                (mode == REPLICATED && component->GetID() < FIRST_LOCAL_ID) ? REPLICATED : LOCAL, componentid, applyAttr);
        }
    }
    else
    {
        for (Vector<SharedPtr<Component> >::ConstIterator i = components_.Begin(); i != components_.End(); ++i)
        {
            Component* component = *i;
            if (component->IsTemporary())
                continue;

            Component* cloneComponent = cloneNode->CloneComponent(component,
                (mode == REPLICATED && component->GetID() < FIRST_LOCAL_ID) ? REPLICATED : LOCAL, 0, applyAttr);

            resolver.AddComponent(component->GetID(), cloneComponent);
        }
    }

    // Clone child nodes recursively
    if (isPoolNode_ && nodeid)
    {
        for (Vector<SharedPtr<Node> >::ConstIterator i = children_.Begin(); i != children_.End(); ++i)
        {
            Node* node = *i;
            if (node->IsTemporary())
                continue;

            node->isPoolNode_ = isPoolNode_;
            node->changeModeEnabled_ = changeModeEnabled_;
            nodeid++;

            node->CloneRecursive(cloneNode, resolver, mode, nodeid, componentid, applyAttr);
        }
    }
    else
    {
        for (Vector<SharedPtr<Node> >::ConstIterator i = children_.Begin(); i != children_.End(); ++i)
        {
            Node* node = *i;
            if (node->IsTemporary())
                continue;

            node->CloneRecursive(cloneNode, resolver, mode, 0, 0, applyAttr);
        }
    }

    {
        using namespace NodeCloned;

        VariantMap& eventData = GetEventDataMap();
        eventData[P_SCENE] = scene_;
        eventData[P_NODE] = this;
        eventData[P_CLONENODE] = cloneNode;

        scene_->SendEvent(E_NODECLONED, eventData);
    }

    return cloneNode;
}

void Node::RemoveComponent(Vector<SharedPtr<Component> >::Iterator i)
{
    // Send node change event. Do not send when already being destroyed
    if (Refs() > 0 && scene_)
    {
        using namespace ComponentRemoved;

        VariantMap& eventData = GetEventDataMap();
        eventData[P_SCENE] = scene_;
        eventData[P_NODE] = this;
        eventData[P_COMPONENT] = (*i).Get();

        scene_->SendEvent(E_COMPONENTREMOVED, eventData);
    }

    RemoveListener(*i);
    if (scene_)
        scene_->ComponentRemoved(*i);
    (*i)->SetNode(0);
    components_.Erase(i);
}

void Node::HandleAttributeAnimationUpdate(StringHash eventType, VariantMap& eventData)
{
    using namespace AttributeAnimationUpdate;

    UpdateAttributeAnimations(eventData[P_TIMESTEP].GetFloat());
}

}
