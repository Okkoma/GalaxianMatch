#include <Urho3D/Urho3D.h>

#include <Urho3D/Core/StringUtils.h>
#include <Urho3D/Core/Context.h>
#include <Urho3D/IO/Deserializer.h>
#include <Urho3D/IO/Log.h>
#include <Urho3D/IO/Serializer.h>

#include <Urho3D/Resource/XMLFile.h>
#include <Urho3D/Resource/JSONFile.h>

#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Scene/SceneEvents.h>



#include "SplinePath2D.h"

#include "SceneAnimation2D.h"


/// SceneAction2D

SceneObject2D::SceneObject2D(Context* context) :
    Serializable(context),
    speedfactor_(1.f),
    noderef_(0),
    dirty_(true)
{
    states_.Resize(1);
}

void SceneObject2D::RegisterObject(Context* context)
{
    context->RegisterFactory<SceneObject2D>();
    URHO3D_ACCESSOR_ATTRIBUTE("Object Node Ref", GetNodeRef, SetNodeRef, unsigned, 0, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Speed Factor", GetSpeedFactor, SetSpeedFactor, float, 1.f, AM_DEFAULT);
}

void SceneObject2D::SetSpeedFactor(float speedfactor)
{
    speedfactor_ = speedfactor;
    dirty_ = true;
}

void SceneObject2D::SetNodeRef(unsigned ref)
{
    noderef_ = ref;
    dirty_ = true;
}

void SceneObject2D::ApplyAttributes()
{
    if (dirty_ && node_)
    {
        if (UpdateRefs(node_->GetScene()))
            dirty_ = false;
    }
}

bool SceneObject2D::UpdateRefs(Scene* scene)
{
    bool updated = true;

    if (noderef_ && (!node_ || (node_ && node_->GetID() != noderef_)))
    {
        updated = false;

        node_ = scene->GetNode(noderef_);

        if (node_)
        {
            updated = true;

            sprite_ = node_->GetDerivedComponent<StaticSprite2D>();
            if (sprite_ && sprite_->IsInstanceOf<AnimatedSprite2D>())
				animatedsprite_ = static_cast<AnimatedSprite2D*>(sprite_.Get());

            GetCurrentStates(states_[0]);

//            URHO3D_LOGINFOF("SceneObject2D() - UpdateRefs : object=%s(%u)", node_ ? node_->GetName().CString() : "", node_ ? node_->GetID() : 0);
        }
    }

    return updated;
}

void SceneObject2D::GetCurrentStates(ObjectActionState2D& state)
{
    if (!node_)
        return;

    state.translation_ = node_->GetWorldPosition();
    state.scale_ = node_->GetWorldScale();
    state.rotation_ = node_->GetWorldRotation();
    state.alpha_ = sprite_ ? sprite_->GetAlpha() : 1.f;
    state.animationid_ = animatedsprite_ ? animatedsprite_->GetSpriterAnimationId() : -1;
}

void SceneObject2D::CopyState(const ObjectActionState2D& src, ObjectActionState2D& dst)
{
    dst.translation_ = src.translation_;
    dst.scale_ = src.scale_;
    dst.rotation_ = src.rotation_;
    dst.alpha_ = src.alpha_;
    dst.animationid_ = src.animationid_;
}

/// TODO : allow setting start position for a key
/// explain the need of absolute_
void SceneObject2D::UpdateActionStates(int key, const Vector<SharedPtr<SceneAction2D> >& actions, float maxduration, bool setPosition)
{
    if (!node_)
        return;

    if (actions.Size()+1 != states_.Size())
        states_.Resize(actions.Size()+1);

    SceneAction2D* action = actions[key];
    ObjectActionState2D& state = states_[key+1];

    if (key == 0 && setPosition)
    {
        GetCurrentStates(state);

        if (action->starttime_ == 0.f)
            CopyState(state, states_[0]);

        if (action->absolute_)
            absoluteoffset_ = SceneAction2D::GetTravel(0, action, this, 0.f, false) - state.translation_;
    }
    else
    {
        if (key == 0)
        {
            CopyState(states_[0], state);
        }
        else
        {
            state.translation_ = action->positionset_ ? action->position_ : SceneAction2D::GetPosition(key-1, actions[key-1], this, MAXDURATION, false);
            state.alpha_ = SceneAction2D::GetAlpha(key, actions, this, action->starttime_);
        }
    }

    if (animatedsprite_ && !action->animation_.Empty())
    {
        if (!action->animatedobjects_.Size() || action->animatedobjects_.Contains(node_->GetID()))
            state.animationid_ = animatedsprite_->GetSpriterAnimationId(action->animation_);
    }

    if (action->path_)
        state.duration_ = SceneAction2D::GetDuration(key, action, this, maxduration);

    state.traveled_ = 0.f;
}

float GetMaxDuration(unsigned key, const Vector<SharedPtr<SceneAction2D> >& actions)
{
    unsigned i = key+1;
    while (i < actions.Size() && actions[i]->pathref_ == 0) { i++; }
    return i < actions.Size() ? actions[i]->starttime_ - actions[key]->starttime_ : MAXDURATION;
}

void SceneObject2D::UpdateActionStates(const Vector<SharedPtr<SceneAction2D> >& actions, bool setPosition)
{
    if (actions.Size()+1 != states_.Size())
        states_.Resize(actions.Size()+1);

    for (unsigned i=0; i < actions.Size(); i++)
    {
        float maxduration = GetMaxDuration(i, actions);
        UpdateActionStates(i, actions, maxduration, setPosition);
    }
}

void SceneObject2D::Reset()
{
    const ObjectActionState2D& state = states_[0];

    // Reset Position
    if (node_)
        node_->SetWorldTransform(state.translation_, state.rotation_, state.scale_);

    // Reset Alpha
    if (sprite_)
        sprite_->SetAlpha(state.alpha_);

    // Reset Animation
    if (animatedsprite_)
        animatedsprite_->SetSpriterAnimation(state.animationid_);

    // Reset Travel
    for (Vector<ObjectActionState2D>::Iterator it=states_.Begin();it!=states_.End();++it)
        it->traveled_ = 0.f;
}



/// SceneAction2D

SceneAction2D::SceneAction2D(Context* context) :
    Serializable(context),
    timeline_(0),
    rotate_(false),
    absolute_(false),
    pathref_(0),
    speedref_(0),
    event_(StringHash::ZERO),
    enabled_(true),
	finished_(false),
    dirty_(true)
{ }

void SceneAction2D::RegisterObject(Context* context)
{
    context->RegisterFactory<SceneAction2D>();
    URHO3D_ACCESSOR_ATTRIBUTE("Path Spline Ref", GetPathRef, SetPathRef, unsigned, 0, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Path Is Absolute", bool, absolute_, false, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Auto Rotate On Path", bool, rotate_, false, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Path Speed Ref", GetSpeedRef, SetSpeedRef, unsigned, 0, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Start Position", GetStartPosition, SetStartPosition, Vector3, Vector3::ZERO, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Alpha Speed", float, alphaspeed_, 0.f, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Alpha To Reach", float, alphagoal_, 0.f, AM_DEFAULT);
    URHO3D_MIXED_ACCESSOR_ATTRIBUTE("Objects To Animate", GetAnimatedObjects, SetAnimatedObjects, String, String::EMPTY, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Animation", String, animation_, String::EMPTY, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Event", String, eventname_, String::EMPTY, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Enable", bool, enabled_, true, AM_DEFAULT);
}

void SceneAction2D::Init(SceneTimeline2D* timeline, float time)
{
    timeline_ = timeline;
    started_ = finished_ = false;
    updatedtime_ = -1.f;
    starttime_ = time;
    nextkeytime_ = starttime_ + MAXDURATION;
    rotate_ = false;
    absolute_ = false;
    positionset_ = false;
    pathref_ = 0;
    path_.Reset();
    speedref_ = 0;
    speedcurve_.Reset();
    animation_.Clear();
    alphaspeed_ = 0.f;
    alphagoal_ = 0.f;
    eventname_.Clear();
    event_ = StringHash::ZERO;
    enabled_ = true;
    dirty_ = true;
}

void SceneAction2D::Reset(unsigned id)
{
    animationok_ = id > 0 && animation_.Empty();
    motionok_ = false;
    alphaok_ = false;
    eventok_ = false;
	enableok_ = false;
    started_ = finished_ = false;
    updatedtime_ = -1.f;

//    URHO3D_LOGINFOF("SceneAction2D() - Reset : timeline=%s actionkey=%u animationok=%s",  timeline_->name_.CString(), id, animationok_ ? "true":"false");
}

void SceneAction2D::SetAnimatedObjects(const String& value)
{
    animatedobjects_.Clear();

    if (value.Empty())
        return;

    Vector<String> values = value.Split(' ');
    for (unsigned i=0; i < values.Size(); i++)
        animatedobjects_.Push(ToUInt(values[i]));
}

String SceneAction2D::GetAnimatedObjects() const
{
    String value;
    for (unsigned i=0; i < animatedobjects_.Size(); i++)
    {
        value += String(animatedobjects_[i]);
        if (i+1 < animatedobjects_.Size())
                value += " ";
    }
    return value;
}

void SceneAction2D::SetPathRef(unsigned ref)
{
    if (pathref_ != ref)
    {
        pathref_ = ref;
        path_.Reset();
//        URHO3D_LOGERRORF("SceneAction2D() - SetPathRef : pathref_=%u", pathref_);
        dirty_ = true;
    }
}

void SceneAction2D::SetSpeedRef(unsigned ref)
{
    if (speedref_ != ref)
    {
        speedref_ = ref;
        speedcurve_.Reset();
//        URHO3D_LOGERRORF("SceneAction2D() - SetSpeedRef : speedref_=%u", speedref_);
        dirty_ = true;
    }
}

void SceneAction2D::SetStartPosition(const Vector3& position)
{
    if (position != position_)
    {
        position_ = position;
        positionset_ = true;
        dirty_ = true;
    }
}

void SceneAction2D::ApplyAttributes()
{
    if (!timeline_)
        return;

    bool updated = UpdateRefs(timeline_->animation_->GetScene());

    if (dirty_ && (path_ || positionset_))
    {
        int actionindex = timeline_->GetActionIndex(this);
        if (actionindex != -1)
            timeline_->UpdateActionStates(actionindex);

//        URHO3D_LOGINFOF("SceneAction2D() - ApplyAttributes : timeline=%s actionindex=%d pathref=%u", timeline_->name_.CString(), actionindex, pathref_);
    }

    if (updated)
        dirty_ = false;
}

bool SceneAction2D::UpdateRefs(Scene* scene)
{
    bool updated = true;

    if (pathref_ && (!path_ || path_->GetID() != pathref_))
    {
        updated = false;

        path_.Reset();

        Component* component = scene->GetComponent(pathref_);

        if (component)
            path_ = static_cast<SplinePath2D*>(component);

        if (path_)
            updated = true;
    }

    if (speedref_ && (!speedcurve_ || (speedcurve_ && speedcurve_->GetID() != speedref_)))
    {
        updated = false;

        speedcurve_.Reset();

        Component* component = scene->GetComponent(speedref_);

        if (component)
            speedcurve_ = static_cast<SplinePath2D*>(component);

        if (speedcurve_)
            updated = true;
    }

    return updated;
}

float SceneAction2D::GetDuration(unsigned key, SceneAction2D* action, SceneObject2D* object, float maxduration)
{
    float& traveled = object->states_[key+1].traveled_;

    traveled = 0.f;

    Vector3 position;
    float time = 0.f;

    for (;; time+=0.01f)
    {
        position = SceneAction2D::GetTravel(key, action, object, time);
        if (traveled >= 1.f || time >= maxduration)
            break;
    }

    // reset traveled
    traveled = 0.f;

    return Min(time, maxduration);
}

Vector3 SceneAction2D::GetPosition(unsigned key, SceneAction2D* action, SceneObject2D* object, float time, bool travelupdate)
{
    if (action->absolute_)
        return GetTravel(key, action, object, time, travelupdate) - object->absoluteoffset_;

    return object->states_[key+1].translation_ + GetTravel(key, action, object, time, travelupdate);
}

float SceneAction2D::GetTangentAngleXY(unsigned key, SceneAction2D* action, SceneObject2D* object, float time)
{
    if (action->path_)
    {
        ObjectActionState2D& state = object->states_[key+1];
        Vector3 tangent = action->path_->GetPoint(Min(state.traveled_+0.1f, 1.f)) - action->path_->GetPoint(Min(state.traveled_, 0.9f));
        return Atan(tangent.y_ / tangent.x_);
    }

    return 0.f;
}

Vector3 SceneAction2D::GetTravel(unsigned key, SceneAction2D* action, SceneObject2D* object, float time, bool travelupdate)
{
    // Don't Work if path_ (SplinePath2D) use nodes which are used in others timelines
    // TODO : update dependencies nodes before (Beware of Cycle)
    //        or Convert path_ with Nodes in a Dead SplinePath2D that use Fixed Vector2

    ObjectActionState2D& state = object->states_[key+1];

    if (action->path_)
    {
        time = Min(state.duration_, time);
        float speedcurvevalue = action->speedcurve_ ? action->speedcurve_->GetSpline().GetPoint(state.traveled_).GetVector3().y_ : 1.0f;
        float traveled = Min(time * (object->speedfactor_ * speedcurvevalue) / action->path_->GetLength(), 1.0f);

        if (travelupdate)
            state.traveled_ = traveled;

        return action->absolute_ ? action->path_->GetPoint(traveled) : action->path_->GetPoint(traveled) - action->path_->GetPoint(0.f);
    }

    if (travelupdate)
        state.traveled_ = 1.f;

    return Vector3::ZERO;
}

float SceneAction2D::GetAlpha(int key, const Vector<SharedPtr<SceneAction2D> >& actions, SceneObject2D* object, float time)
{
    key = Max(key, 0);

    SceneAction2D* action = actions[key].Get();
    while (action->alphaspeed_ == 0.f && key > 0)
    {
        key--;
        action = actions[key].Get();
    }

    float alpha = key > 0 ? SceneAction2D::GetAlpha(key-1, actions, object, time) : object->states_[0].alpha_;

    if (action->alphaspeed_ != 0.f && time > 0.f)
    {
        float alphainc = action->alphagoal_ - alpha;
        alphainc /= Abs(alphainc);
        alpha += alphainc * action->alphaspeed_ * (time - action->starttime_);
        alpha = alphainc > 0.f ? Clamp(alpha, 0.f, action->alphagoal_) : Clamp(alpha, action->alphagoal_, 1.f);
    }

    return alpha;
}

void SceneAction2D::Execute(unsigned key, const Vector<SharedPtr<SceneAction2D> >& actions, float time)
{
    if (started_ && time == updatedtime_)
        return;

    if (time < starttime_ || finished_)
    {
        started_ = false;
        return;
    }

    if (!started_)
        started_ = true;

    assert(timeline_);

    Vector<SharedPtr<SceneObject2D> >& objects = timeline_->objects_;
    unsigned numobjects = objects.Size();

    if (!enableok_)
    {
        for (unsigned i=0; i < numobjects; i++)
        {
            SceneObject2D* object = objects[i].Get();

            if (!object->node_)
                continue;

            // skip if parent disabled
            if (enabled_ && (object->node_->GetParent() && !object->node_->GetParent()->IsEnabled()))
                continue;

//            URHO3D_LOGINFOF("SceneAction2D() - Execute : timeline=%s object=%s(%u) actionkey=%u Setting Enabled to %s",
//                            timeline_->name_.CString(), object->node_->GetName().CString(), object->node_->GetID(), key, enabled_ ? "true" : "false");

            object->node_->SetEnabledRecursive(enabled_);
        }

        enableok_ = true;

        if (!enabled_)
        {
            finished_ = true;
            return;
        }
    }

    time -= starttime_;

    bool pathok = false;

    if (!motionok_)
    {
        unsigned numobjectsToMove = numobjects;

        if (positionset_ || (path_ && path_->GetLength() > 0.f))
        {
            for (unsigned i=0; i < numobjects; i++)
            {
                SceneObject2D* object = objects[i].Get();

                ObjectActionState2D& state = object->states_[key+1];

                if (time > state.duration_)
                    state.traveled_ = updatedtime_ - starttime_ > state.duration_ ? 1.1f : 1.f;

                if (object->node_ && state.traveled_ <= 1.f)
                {
                    object->node_->SetWorldPosition(SceneAction2D::GetPosition(key, this, object, time));
                    if (path_ && rotate_)
                        object->node_->SetWorldRotation2D(SceneAction2D::GetTangentAngleXY(key, this, object, time));
                }

                if (state.traveled_ >= 1.f || object->node_.Null())
                    numobjectsToMove--;
            }
        }

        if (path_ && path_->GetLength() > 0.f)
            pathok = numobjectsToMove == 0;
        else
            motionok_ = true;
    }

    updatedtime_ = time+starttime_;

    if (!alphaok_)
    {
        unsigned numobjectsalphaset = 0;
        for (unsigned i=0; i < numobjects; i++)
        {
            SceneObject2D* object = objects[i].Get();

            if (!object->sprite_ || (alphaspeed_ && object->sprite_->GetAlpha() == alphagoal_))
            {
                numobjectsalphaset++;
                continue;
            }

            float newalpha = SceneAction2D::GetAlpha(key, actions, object, updatedtime_);

            if (newalpha != object->sprite_->GetAlpha())
            {
                object->sprite_->SetAlpha(newalpha);
//                URHO3D_LOGINFOF("SceneAction2D() - Execute : timeline=%s actionkey=%u Setting Alpha to %f",  timeline_->name_.CString(), key, newalpha);
            }

            if (!alphaspeed_ || newalpha == alphagoal_)
                numobjectsalphaset++;
        }

        if (numobjectsalphaset == numobjects)
            alphaok_ = true;
    }

    if (updatedtime_ <= nextkeytime_)
    {
        if (!animationok_)
        {
            for (unsigned i=0; i < numobjects; i++)
            {
                SceneObject2D* object = objects[i].Get();
                if (!object->animatedsprite_)
                    continue;

                const int& animationid = object->states_[key+1].animationid_;

                if (animationid != -1)
                {
                    object->animatedsprite_->SetSpriterAnimation(animationid);
                    object->animatedsprite_->SetTime(time);

//                    URHO3D_LOGINFOF("SceneAction2D() - Execute : timeline=%s object=%s(%u) actionkey=%u Setting Animation to %s",
//                                    timeline_->name_.CString(), object->node_->GetName().CString(), object->node_->GetID(), key, object->animatedsprite_->GetAnimation().CString());
                }
            }

//            URHO3D_LOGINFOF("SceneAction2D() - Execute : timeline=%s actionkey=%u Setting Animation OK !", timeline_->name_.CString(), key);
            animationok_ = true;
        }

        if (event_.Value() && !eventok_)
        {
            SendEvent(event_);
            eventok_ = true;
        }
    }

    if ((motionok_ || pathok) && alphaok_)
        finished_ = true;
}



/// SceneTimeline2D

SceneTimeline2D::SceneTimeline2D(Context* context) :
	Serializable(context),
	finished_(false)
{ }

SceneTimeline2D::~SceneTimeline2D()
{ }

void SceneTimeline2D::RegisterObject(Context* context)
{
    context->RegisterFactory<SceneTimeline2D>();

    URHO3D_ATTRIBUTE("Name", String, name_, String::EMPTY, AM_DEFAULT);
}

void SceneTimeline2D::ApplyAttributes()
{
    if (!animation_)
        return;

//    animation_->SendEvent(E_SCENEANIMATIONTIMELINESUPDATED);

//    URHO3D_LOGINFOF("SceneTimeline2D() - ApplyAttributes : timeline=%s", name_.CString());
}

bool SceneTimeline2D::LoadXML(const XMLElement& source)
{
    unsigned numactions = 0, numobjects = 0;
    XMLElement elem = source.GetChild("action");
    while (elem)
    {
        SharedPtr<SceneAction2D> action(new SceneAction2D(animation_->GetContext()));
        action->Init(this, Max(elem.GetFloat("time"), 0.f));
        action->rotate_ = elem.GetBool("rotate");
        action->absolute_ = elem.GetBool("absolute");
        action->positionset_ = elem.GetBool("positionset");
        if (action->positionset_)
            action->position_ = elem.GetVector3("position");
        action->pathref_ = elem.GetUInt("pathref");
        action->speedref_ = elem.GetUInt("speedref");
        action->alphaspeed_ = elem.GetFloat("alphaspeed");
        action->alphagoal_ = elem.GetFloat("alphagoal");
        action->animation_ = elem.GetAttribute("animation");
        action->SetAnimatedObjects(elem.GetAttribute("animatedobjects"));
        action->eventname_ = elem.GetAttribute("event");
        action->event_ = StringHash(action->eventname_);
        if (elem.HasAttribute("enable"))
            action->enabled_ = elem.GetBool("enable");
        action->Reset(actions_.Size());
        actions_.Push(action);
        numactions++;

        elem = elem.GetNext("action");
    }

//    URHO3D_LOGINFOF("SceneTimeline2D() - LoadXML : timeline=%s Actions loaded numactions=%u", name_.CString(), numactions);

    elem = source.GetChild("object");
    while (elem)
    {
        unsigned noderef = elem.GetUInt("ref");
        if (noderef)
        {
            SharedPtr<SceneObject2D> object(new SceneObject2D(animation_->GetContext()));
            object->noderef_ = noderef;
            object->speedfactor_ = elem.GetFloat("speed");
            if (!object->speedfactor_)
                object->speedfactor_ = 1.0f;
            object->states_.Resize(numactions+1);
            objects_.Push(object);
            numobjects++;
        }

        elem = elem.GetNext("object");
    }

//    URHO3D_LOGINFOF("SceneTimeline2D() - LoadXML : timeline=%s Objects loaded numobjects=%u", name_.CString(), numobjects);

    return true;
}

bool SceneTimeline2D::SaveXML(XMLElement& dest) const
{
    for (Vector<SharedPtr<SceneObject2D> >::ConstIterator it = objects_.Begin(); it != objects_.End(); ++it)
    {
        SceneObject2D* object = it->Get();
        XMLElement elem = dest.CreateChild("object");
        elem.SetUInt("ref", object->noderef_);
        if (object->speedfactor_ != 1.f)
            elem.SetFloat("speed", object->speedfactor_);
    }

    for (Vector<SharedPtr<SceneAction2D> >::ConstIterator it = actions_.Begin(); it != actions_.End(); ++it)
    {
        SceneAction2D* action = it->Get();
        XMLElement elem = dest.CreateChild("action");
        elem.SetFloat("time", action->starttime_);
        if (action->absolute_)
            elem.SetBool("absolute", action->absolute_);
        if (action->rotate_)
            elem.SetBool("rotate", action->rotate_);
        if (action->positionset_)
        {
            elem.SetBool("positionset", action->positionset_);
            elem.SetVector3("position", action->position_);
        }
        if (action->pathref_)
            elem.SetUInt("pathref", action->pathref_);
        if (action->speedref_)
            elem.SetUInt("speedref", action->speedref_);

        if (action->alphaspeed_)
            elem.SetFloat("alphaspeed", action->alphaspeed_);
        if (action->alphagoal_)
            elem.SetFloat("alphagoal", action->alphagoal_);
        if (!action->animation_.Empty())
        {
            elem.SetAttribute("animation", action->animation_);
            if (action->animatedobjects_.Size())
                elem.SetAttribute("animatedobjects", action->GetAnimatedObjects());
        }
        if (!action->eventname_.Empty())
            elem.SetAttribute("event", action->eventname_);
        if (!action->enabled_ || action->starttime_ > 0.f)
            elem.SetBool("enable", action->enabled_);
    }

    return true;
}

void SceneTimeline2D::Update(float time)
{
    if (finished_)
        return;

    if (updatedtime_ == time)
        return;

    updatedtime_ = Max(time, 0.f);

//    URHO3D_LOGINFOF("SceneTimeline2D() - Update : timeline=%s time=%f ...", name_.CString(), updatedtime_);

    UpdateRefs();

    int finished = 0;

    for (unsigned i=0; i < actions_.Size(); i++)
    {
        SceneAction2D* action = actions_[i].Get();

        if (action)
        {
//            URHO3D_LOGINFOF("SceneTimeline2D() - Update : timeline=%s execute actionkey=%u type=%d...", name_.CString(), i, action->type_);
            action->Execute(i, actions_, updatedtime_);

            if (action->finished_)
                finished++;
        }
    }

    if (finished == actions_.Size())
        finished_ = true;

//    URHO3D_LOGINFOF("SceneTimeline2D() - Update : timeline=%s time=%f ... OK !", name_.CString(), updatedtime_);
}

void SceneTimeline2D::Reset(float time)
{
	finished_ = false;

	// be sure to update
    updatedtime_ = time-0.01f;

//    URHO3D_LOGINFOF("SceneTimeline2D() - Reset : timeline=%s time=%f ...", name_.CString(), time);

    for (Vector<SharedPtr<SceneAction2D> >::Iterator it=actions_.Begin();
         it!=actions_.End(); it++)
    {
        (*it)->Reset(it-actions_.Begin());
    }

    for (Vector<SharedPtr<SceneObject2D> >::Iterator it=objects_.Begin();
         it!=objects_.End(); it++)
    {
        (*it)->Reset();
    }

    if (actions_.Size())
    {
        int key = Max(GetActionIndex(time), 0);
        for (int i=0; i <= key; i++)
        {
//            URHO3D_LOGINFOF("SceneTimeline2D() - Reset : timeline=%s time=%f key=%d !", name_.CString(), time, i);
            UpdateActionStates(i);
        }

        Update(time);
    }

//    URHO3D_LOGINFOF("SceneTimeline2D() - Reset : timeline=%s time=%f ... OK !", name_.CString(), time);
}

bool SceneTimeline2D::UpdateRefs(bool resetPosition)
{
    if (!animation_)
        return false;

    Scene* scene = animation_->GetScene();

    if (!scene)
        return false;

//    URHO3D_LOGINFOF("SceneTimeline2D() - UpdateRefs : timeline=%s resetPosition=%s ... OK !", name_.CString(), resetPosition ? "true":"false");

    for (Vector<SharedPtr<SceneAction2D> >::Iterator it=actions_.Begin();
         it!=actions_.End(); it++)
    {
        SceneAction2D* action = it->Get();
        if (action->dirty_)
        {
            if (action->UpdateRefs(scene))
                action->dirty_ = false;
        }
    }

    for (Vector<SharedPtr<SceneObject2D> >::Iterator it=objects_.Begin();
         it!=objects_.End(); it++)
    {
        SceneObject2D* object = it->Get();
        if (object->dirty_)
        {
            if (object->UpdateRefs(scene))
                object->dirty_ = false;

            object->UpdateActionStates(actions_, resetPosition);
            if (object->node_)
                object->node_->AddListener(animation_);
        }
    }

    return true;
}

void SceneTimeline2D::UpdateActionStates(unsigned key)
{
    SceneAction2D* action = actions_[key];

    action->nextkeytime_ = key+1 < actions_.Size() ? actions_[key+1]->starttime_ : MAXDURATION;

    float maxduration = GetMaxDuration(key, actions_);
    for (Vector<SharedPtr<SceneObject2D> >::Iterator it=objects_.Begin(); it!=objects_.End(); it++)
    {
        SceneObject2D* object = it->Get();
        object->UpdateActionStates(key, actions_, maxduration, false);

//        URHO3D_LOGINFOF("SceneTimeline2D() - UpdateActionStates : timeline=%s object=%s absolute=%s start=%f duration=%f startposition=%s offset=%s alpha=%f !",
//                        name_.CString(), object->node_->GetName().CString(), action->absolute_ ? "true":"false", action->starttime_,
//                        object->states_[key+1].duration_, action->absolute_ ? object->transform_.Translation().ToString().CString() : object->states_[key+1].transform_.Translation().ToString().CString(),
//                        object->absoluteoffset_.ToString().CString(), object->alpha_);
    }
}

void SceneTimeline2D::UpdateActionStatesNear(int index)
{
    if (index > 0)
        UpdateActionStates(index-1);

    if (index < actions_.Size())
    {
        UpdateActionStates(index);

        if (index+1 < actions_.Size())
            UpdateActionStates(index+1);
    }
}

/// TODO : update only for a specific action
void SceneTimeline2D::UpdateObjectPosition(Node* node, float time)
{
    if (!node)
        return;

    // Find SceneObject for the node
    for (Vector<SharedPtr<SceneObject2D> >::Iterator it=objects_.Begin();
         it!=objects_.End(); it++)
    {
        SceneObject2D* object = it->Get();
        if (object->node_ == node)
        {
            //int actionkey = GetActionIndex(time);
            object->UpdateActionStates(actions_, true);
        }
    }
}

int SceneTimeline2D::GetActionIndex(SceneAction2D* action) const
{
    for (Vector<SharedPtr<SceneAction2D> >::ConstIterator it=actions_.Begin();
         it!=actions_.End(); it++)
    {
        if (it->Get() == action)
            return it-actions_.Begin();
    }

    return -1;
}

int SceneTimeline2D::GetActionIndex(float time) const
{
    int index = -1;

    for (Vector<SharedPtr<SceneAction2D> >::ConstIterator it=actions_.Begin();
         it!=actions_.End(); it++)
    {
        float dt = time - (*it)->starttime_;

        if (dt < 0.f)
            break;

        index = it-actions_.Begin();
    }

    return index;
}


/// SceneAnimation2D

SceneAnimation2D::SceneAnimation2D(Context* context) :
    LogicComponent(context),
    elapsedTime_(0.0f),
    refsUpdated_(false),
    started_(false),
    finished_(false),
    updateRunning_(false)
{

    // Only the scene update event is needed: unsubscribe from the rest for optimization
    SetUpdateEventMask(0x0);
}

SceneAnimation2D::~SceneAnimation2D()
{ }


void SceneAnimation2D::RegisterObject(Context* context)
{
    context->RegisterFactory<SceneAnimation2D>();
}

bool SceneAnimation2D::LoadXML(const XMLElement& source, bool setInstanceDefault, bool applyAttr)
{
    if (!Serializable::LoadXML(source, setInstanceDefault, applyAttr))
        return false;

    XMLElement elem = source.GetChild("timeline");
    while (elem)
    {
        SharedPtr<SceneTimeline2D> timeline(new SceneTimeline2D(context_));
        timeline->animation_ = this;
        timeline->name_ = elem.GetAttribute("name");

        if (!timeline->LoadXML(elem))
            return false;

        timelines_.Push(timeline);

        elem = elem.GetNext("timeline");
    }

    refsUpdated_ = false;

    return true;
}

bool SceneAnimation2D::SaveXML(XMLElement& dest) const
{
    if (!LogicComponent::SaveXML(dest))
        return false;

    for (Vector<SharedPtr<SceneTimeline2D> >::ConstIterator it = timelines_.Begin();
         it != timelines_.End(); ++it)
    {
        SceneTimeline2D* timeline = it->Get();
        XMLElement elem = dest.CreateChild("timeline");
        elem.SetAttribute("name", timeline->name_);

        if (!timeline->SaveXML(elem))
            return false;
    }

    return true;
}

/// Called before the first update. At this point all other components of the node should exist. Will also be called if update events are not wanted; in that case the event is immediately unsubscribed afterward.
void SceneAnimation2D::DelayedStart()
{
    if (refsUpdated_)
        return;

    if (!GetScene())
        return;

    if (!timelines_.Size())
        return;

//    URHO3D_LOGINFOF("SceneAnimation2D() - DelayedStart : ");

    unsigned count=0;
    for (Vector<SharedPtr<SceneTimeline2D> >::Iterator it=timelines_.Begin();
         it!=timelines_.End(); it++)
    {
        if ((*it)->UpdateRefs(true))
            count++;
    }

    if (count >= timelines_.Size())
    {
        refsUpdated_ = true;
//        started_ = IsEnabledEffective();
//        if (started_)
//        {
//            startTime_ = elapsedTime_;
//            timer_.Reset();
//        }

        if (GetUpdateEventMask() == USE_UPDATE)
        {
            URHO3D_LOGINFOF("SceneAnimation2D() - DelayedStart : autostart running at time=%f ...", elapsedTime_);
            SetRunning(true);
        }

/*
        using namespace SceneAnimationTimeLinesUpdated;
        VariantMap& eventData = GetEventDataMap();
        eventData[P_ELEMENT] = this;
        SendEvent(E_SCENEANIMATIONTIMELINESUPDATED, eventData);
*/
    }
}

void SceneAnimation2D::SetRunning(bool state)
{
    if (updateRunning_ != state)
    {
        updateRunning_ = state;

        if (updateRunning_)
        {
            // reset
            if (finished_)
                elapsedTime_ = 0.f;

            URHO3D_LOGINFOF("SceneAnimation2D() - SetRunning : time=%f ...", elapsedTime_);
            SetTime(elapsedTime_);
        }
    }
}

/// Called when the component is detached from a scene node, usually on destruction. Note that you will no longer have access to the node and scene at that point.
void SceneAnimation2D::Stop()
{
    if (!started_)
        return;

//    URHO3D_LOGINFOF("SceneAnimation2D() - Stop : ");

    started_ = false;
    finished_ = false;

    OnSceneSet(0);
}

/// Put the states of the objects of the animation at the specified time
void SceneAnimation2D::SetTime(float time)
{
//    URHO3D_LOGINFOF("SceneAnimation2D() - SetTime : time = %f ...", time);

    timeSetting_ = true;
	finished_ = false;

    if (!refsUpdated_)
        DelayedStart();

    time = Max(time, 0.f);

    started_ = IsEnabledEffective();
    if (started_)
    {
        startTime_ = time;
        timer_.Reset();
    }
    elapsedTime_ = time;
    GetScene()->SetElapsedTime(time);

    int finished = 0;
    for (Vector<SharedPtr<SceneTimeline2D> >::ConstIterator it=timelines_.Begin(); it != timelines_.End(); it++)
    {
        (*it)->Reset(time);
        if ((*it)->IsFinished())
            finished++;
    }

    if (finished == timelines_.Size())
    {
//        URHO3D_LOGINFOF("SceneAnimation2D() - SetTime : time = %f ... numtimelines=%u Finished ...", time, timelines_.Size());
        finished_ = true;
    }

    using namespace SceneAnimationTimeChanged;
    VariantMap& eventData = GetEventDataMap();
    eventData[P_ELEMENT] = this;
    eventData[P_TIME] = elapsedTime_;
    SendEvent(E_SCENEANIMATIONTIMECHANGED, eventData);

    timeSetting_ = false;

//    URHO3D_LOGINFOF("SceneAnimation2D() - SetTime : time = %f ... OK !", time);
}

/// Called on scene update, variable timestep.
void SceneAnimation2D::Update(float timestep)
{
    if (!updateRunning_ || finished_)
        return;

    elapsedTime_ = startTime_ + (float)timer_.GetMSec(false) * 0.001f;

    int finished = 0;
    for (Vector<SharedPtr<SceneTimeline2D> >::Iterator it=timelines_.Begin(); it != timelines_.End(); it++)
    {
        (*it)->Update(elapsedTime_);
        if ((*it)->IsFinished())
            finished++;
    }

    if (finished == timelines_.Size())
    {
//        URHO3D_LOGINFOF("SceneAnimation2D() - Update : numtimelines=%u Finished ... OK !", timelines_.Size());
        finished_ = true;
        updateRunning_ = false;

        using namespace SceneAnimationFinished;
        VariantMap& eventData = GetEventDataMap();
        eventData[P_ELEMENT] = this;
        eventData[P_TIME] = elapsedTime_;
        SendEvent(E_SCENEANIMATIONFINISHED, eventData);
    }
    else
    {
        using namespace SceneAnimationTimeChanged;
        VariantMap& eventData = GetEventDataMap();
        eventData[P_ELEMENT] = this;
        eventData[P_TIME] = elapsedTime_;
        SendEvent(E_SCENEANIMATIONTIMECHANGED, eventData);
    }
}

void SceneAnimation2D::AddTimeLine(unsigned index, const String& name)
{
    SceneTimeline2D* timeline = new SceneTimeline2D(context_);
    timeline->animation_ = this;
    timeline->name_ = name;
    timelines_.Insert(index, SharedPtr<SceneTimeline2D>(timeline));
}

void SceneAnimation2D::RemoveTimeLine(const String& name)
{
    int index = GetTimeLineIndex(name);
    if (index != -1)
    {
        {
            SceneTimeline2D* timeline = timelines_[index].Get();
            for (unsigned i=0; i < timeline->objects_.Size(); i++)
                RemoveNodeObject(index, i);
        }
        timelines_.Erase(index);
    }
}

void SceneAnimation2D::AddNodeObject(Node* node, unsigned line, unsigned index)
{
    SceneTimeline2D* timeline = timelines_[line].Get();

    URHO3D_LOGINFOF("SceneAnimation2D() - AddNodeObject : timeline=%s node=%s(%u) !", timeline->name_.CString(), node->GetName().CString(), node->GetID());

    SharedPtr<SceneObject2D> object(new SceneObject2D(context_));
    timeline->objects_.Insert(index, object);
    object->SetNodeRef(node->GetID());
    bool updated = object->UpdateRefs(GetScene());
    if (updated)
    {
        object->UpdateActionStates(timeline->actions_, true);
        object->dirty_ = false;
        object->node_->AddListener(this);
    }
}

void SceneAnimation2D::RemoveNodeObject(unsigned line, unsigned index)
{
    SceneTimeline2D* timeline = timelines_[line].Get();

    // reset states (position...)
    SceneObject2D* object = timeline->objects_[index];
    if (object->node_)
    {
        object->Reset();
        object->node_->RemoveListener(this);
    }

    timeline->objects_.Erase(index);
}

void SceneAnimation2D::AddAction(unsigned line, float time)
{
    SceneTimeline2D* timeline = timelines_[line].Get();

    time = Max(time, 0.f);

    int key = timeline->GetActionIndex(time)+1;

    URHO3D_LOGINFOF("SceneAnimation2D() - AddAction : timeline=%s time=%f at key=%d...", timeline->name_.CString(), time, key);

    SharedPtr<SceneAction2D> action(new SceneAction2D(context_));
    action->Init(timeline, time);
    // insert action at key
    timeline->actions_.Insert(key, action);
    // insert state for action at key
    for (unsigned i=0; i < timeline->objects_.Size(); i++)
        timeline->objects_[i]->states_.Insert(key+1, ObjectActionState2D());
    timeline->UpdateActionStatesNear(key);
}

void SceneAnimation2D::RemoveAction(unsigned line, unsigned index)
{
    SceneTimeline2D* timeline = timelines_[line].Get();
    // erase action at index
    timeline->actions_.Erase(index);
    // erase state for action at index
    for (unsigned i=0; i < timeline->objects_.Size(); i++)
        timeline->objects_[i]->states_.Erase(index+1);
    timeline->UpdateActionStatesNear(index);
}

int SceneAnimation2D::SetTimeKey(unsigned line, unsigned key, float time)
{
    SceneTimeline2D* timeline = timelines_[line].Get();
    if (!timeline)
        return -1;

    time = Max(time, 0.f);

    URHO3D_LOGINFOF("SceneAnimation2D() - SetTimeKey timeline=%u timekey=%u time=%f", line, key, time);

    timeline->actions_[key]->starttime_ = time;

    // Reorders Keys if need
    if (key > 0 && time < timeline->actions_[key-1]->starttime_)
    {
        PermuteTimeKey(line, key-1, key);
        key--;
    }
    else if (key+1 < timeline->actions_.Size() && time > timeline->actions_[key+1]->starttime_)
    {
        PermuteTimeKey(line, key, key+1);
        key++;
    }

    timeline->UpdateActionStatesNear(key);
//    timeline->Reset(time);

    return key;
}

void SceneAnimation2D::PermuteTimeKey(unsigned line, unsigned key1, unsigned key2)
{
    if (key1 == key2)
        return;

    SceneTimeline2D* timeline = timelines_[line].Get();
    if (timeline)
    {
        URHO3D_LOGINFOF("SceneAnimation2D() - PermuteTimeKey timeline=%u key1=%u key2=%u", line, key1, key2);

        // key1 must be lesser than key2
        if (key1 > key2)
            Swap(key1, key2);

        // permute times
        Swap(timeline->actions_[key1]->starttime_, timeline->actions_[key2]->starttime_);
        // permute actions
        Swap(timeline->actions_[key1], timeline->actions_[key2]);
    }
}

int SceneAnimation2D::GetTimeLineIndex(const String& name) const
{
    for (unsigned i=0; i < timelines_.Size(); ++i)
    {
        if (timelines_[i]->name_ == name)
            return i;
    }
    return -1;
}

Vector<String> SceneAnimation2D::GetTimeLineNames() const
{
    Vector<String> names;
    for (Vector<SharedPtr<SceneTimeline2D> >::ConstIterator it=timelines_.Begin(); it != timelines_.End(); it++)
        names.Push((*it)->name_);

    return names;
}

Vector<String> SceneAnimation2D::GetTimeLineObjectNames(unsigned line) const
{
    Vector<String> names;
    SceneTimeline2D* timeline = timelines_[line];
    for (Vector<SharedPtr<SceneObject2D> >::ConstIterator it=timeline->objects_.Begin(); it != timeline->objects_.End(); it++)
    {
        SceneObject2D* object = it->Get();
        String name;
        if (object->node_)
            name = object->node_->GetName();
        else
        {
            name = String("(");
            name += String(object->noderef_);
            name += String(")");
        }
        names.Push(name);
    }

    return names;
}

Serializable* SceneAnimation2D::GetTimeLine(unsigned line) const
{
    if (line >= timelines_.Size())
        return 0;

    SceneTimeline2D* timeline = timelines_[line].Get();

    return static_cast<Serializable*>(timeline);
}

Serializable* SceneAnimation2D::GetTimeLineObject(unsigned line, unsigned index) const
{
    if (line >= timelines_.Size())
        return 0;

    if (index >= timelines_[line]->objects_.Size())
        return 0;

    SceneObject2D* object = timelines_[line]->objects_[index].Get();

    return static_cast<Serializable*>(object);
}

unsigned SceneAnimation2D::GetTimeLineObjectNodeID(unsigned line, unsigned index) const
{
    if (line >= timelines_.Size())
        return 0;

    if (index >= timelines_[line]->objects_.Size())
        return 0;

    SceneObject2D* object = timelines_[line]->objects_[index].Get();

    return object->noderef_;
}

Serializable* SceneAnimation2D::GetTimeLineActionSpeedCurve(unsigned line, unsigned index) const
{
    if (line >= timelines_.Size())
        return 0;

    if (index >= timelines_[line]->actions_.Size())
        return 0;

    SplinePath2D* path = timelines_[line]->actions_[index]->speedcurve_.Get();

    if (path)
        return static_cast<Serializable*>(path);
    else
        return 0;
}

Serializable* SceneAnimation2D::GetTimeLineAction(unsigned line, unsigned index) const
{
    if (line >= timelines_.Size())
        return 0;

    if (index >= timelines_[line]->actions_.Size())
        return 0;

    SceneAction2D* action = timelines_[line]->actions_[index].Get();

    return static_cast<Serializable*>(action);
}

Serializable* SceneAnimation2D::GetTimeLineActionPath(unsigned line, unsigned index) const
{
    if (line >= timelines_.Size())
        return 0;

    if (index >= timelines_[line]->actions_.Size())
        return 0;

    SplinePath2D* path = timelines_[line]->actions_[index]->path_.Get();

    if (path)
        return static_cast<Serializable*>(path);
    else
        return 0;
}

void SceneAnimation2D::OnSetEnabled()
{
//    URHO3D_LOGINFOF("SceneAnimation2D() - OnSetEnabled : enabled=%s", IsEnabledEffective() ? "true":"false");

    LogicComponent::OnSetEnabled();

    started_ = false;
}


void SceneAnimation2D::OnMarkedDirty(Node* node)
{
    if (!started_ || finished_ || updateRunning_ || timeSetting_ || !IsEnabledEffective())
        return;

    URHO3D_LOGINFOF("SceneAnimation2D() - OnMarkedDirty !");

    for (Vector<SharedPtr<SceneTimeline2D> >::ConstIterator it=timelines_.Begin(); it != timelines_.End(); it++)
    {
        (*it)->UpdateObjectPosition(node, elapsedTime_);
    }
}


