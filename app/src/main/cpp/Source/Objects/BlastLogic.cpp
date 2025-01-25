#include <Urho3D/Urho3D.h>

#include <Urho3D/Core/Context.h>
#include <Urho3D/Core/StringUtils.h>

#include <Urho3D/IO/Log.h>

#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Scene/SceneEvents.h>
#include <Urho3D/Scene/ObjectAnimation.h>
#include <Urho3D/Scene/ValueAnimation.h>

#include <Urho3D/Urho2D/CollisionShape2D.h>
#include <Urho3D/Urho2D/RigidBody2D.h>
#include <Urho3D/Urho2D/PhysicsEvents2D.h>
#include <Urho3D/Urho2D/SpriterInstance2D.h>
#include <Urho3D/Urho2D/AnimatedSprite2D.h>
#include <Urho3D/Urho2D/ConstraintDistance2D.h>

#include "GameAttributes.h"
#include "GameEvents.h"
#include "GameHelpers.h"
#include "GameStatics.h"
#include "TimerRemover.h"

#include "MAN_Matches.h"
#include "BossLogic.h"

#include "BlastLogic.h"


const float IDLE_DELAY = 3.f;
Vector2 position2D_;

const char* BlastSteeringModeNames_[] =
{
    "NoSteering",
    "Bullet",
    "Missile",
    "Tentacule",
    0
};


BlastLogic::BlastLogic(Context* context) :
    Component(context),
    steeringMode_(NOSTEERING),
    autotargeting_(true),
    autostart_(false),
    initialDirection_(Vector2(0.f, -1.f)),
    lifetime_(1.f),
    speed_(5.f),
    maxforce_(0.5f),
    dieoncontact_(false)
{ }

BlastLogic::~BlastLogic()
{
//    URHO3D_LOGINFOF("~BlastLogic()");

}

void BlastLogic::RegisterObject(Context* context)
{
    context->RegisterFactory<BlastLogic>();

    URHO3D_ENUM_ACCESSOR_ATTRIBUTE("Steering Mode", GetSteeringMode, SetSteeringMode, BlastSteeringMode, BlastSteeringModeNames_, NOSTEERING, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Auto Targeting", GetAutoTargeting, SetAutoTargeting, bool, false, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Auto Start", GetAutoStart, SetAutoStart, bool, false, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Initial Direction", GetInitialDirection, SetInitialDirection, Vector2, Vector2(0.f, -1.f), AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Life Time", GetLifeTime, SetLifeTime, float, 1.f, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Speed", GetSpeed, SetSpeed, float, 5.f, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Die On Contact", GetDieOnContact, SetDieOnContact, bool, false, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Effects On Die", GetEffectOnDie, SetEffectOnDie, IntVector2, IntVector2(0,1), AM_DEFAULT);
}

void BlastLogic::SetSteeringMode(BlastSteeringMode mode)
{
    steeringMode_ = mode;
}

void BlastLogic::SetAutoTargeting(bool state)
{
    autotargeting_ = state;
}

void BlastLogic::SetAutoStart(bool state)
{
    autostart_ = state;
}
void BlastLogic::SetInitialDirection(const Vector2& dir)
{
    initialDirection_ = dir;
}

void BlastLogic::SetLifeTime(float time)
{
    lifetime_ = time;
}

void BlastLogic::SetSpeed(float speed)
{
    speed_ = speed;
}

void BlastLogic::SetDieOnContact(bool state)
{
    dieoncontact_ = state;
}

void BlastLogic::SetEffectOnDie(const IntVector2& effect)
{
    effectondie_ = effect;
}

void BlastLogic::Start()
{
    if (GetScene() && (node_->GetVar(GOA::ENTITYCLONE) != Variant::EMPTY || autostart_))
    {
        target_.Reset();

//        URHO3D_LOGINFOF("BlastLogic() - Start : %s(%u) : lifetime=%f speed=%f body=%u bodynode=%u",
//                        node_->GetName().CString(), node_->GetID(), lifetime_, speed_, body_, body_->GetNode()->GetID());

        node_->SetAnimationEnabled(true);

        if (body_)
            SubscribeToEvent(node_, E_PHYSICSBEGINCONTACT2D, URHO3D_HANDLER(BlastLogic, OnBeginContact));

        if (steeringMode_ == NOSTEERING)
            body_->SetLinearVelocity(speed_ * initialDirection_);
        else
            SubscribeToEvent(GetScene(), E_SCENEPOSTUPDATE, URHO3D_HANDLER(BlastLogic, OnUpdate));

        if (lifetime_)
            TimerRemover::Get()->Start(node_, lifetime_, FREEMEMORY);

        if (steeringMode_ == TENTACULE)
        {
            BossLogic* bosslogic = node_->GetParent()->GetDerivedComponent<BossLogic>();
            RigidBody2D* parentbody = bosslogic->GetAnimators()[0]->GetNode()->GetComponent<RigidBody2D>();
            ConstraintDistance2D* constraint = node_->GetOrCreateComponent<ConstraintDistance2D>();
            constraint->SetOtherBody(parentbody);
            constraint->SetCollideConnected(true);
            position2D_ = node_->GetPosition2D();
//            URHO3D_LOGINFOF("BlastLogic() - Start : %s(%u) Add Constraint with body=%u !", node_->GetName().CString(), node_->GetID(), parentbody ? parentbody->GetID() : 0);
        }
    }
}

void BlastLogic::Stop()
{
//    URHO3D_LOGINFOF("BlastLogic() - Stop : %s(%u) !", node_->GetName().CString(), node_->GetID());

    // stop move animation
    node_->SetAnimationEnabled(false);
//    node_->SetEnabledRecursive(false);

    // stop update
    UnsubscribeFromAllEvents();
}

void BlastLogic::OnSetEnabled()
{
//    URHO3D_LOGINFOF("BlastLogic() - OnSetEnabled : Node=%s(%u) enabled=%s ...", node_->GetName().CString(), node_->GetID(), IsEnabledEffective() ? "true" : "false");

    if (IsEnabledEffective())
        Start();
    else
        Stop();

//    URHO3D_LOGINFOF("BlastLogic() - OnSetEnabled : Node=%s(%u) enabled=%s ... OK !", node_->GetName().CString(), node_->GetID(), IsEnabledEffective() ? "true" : "false");
}

void BlastLogic::OnNodeSet(Node* node)
{
    if (node)
    {
        animator_ = node->GetComponent<AnimatedSprite2D>();
        body_ = node->GetComponent<RigidBody2D>();
        if (!body_ && node->GetParent())
            body_ = node->GetParent()->GetComponent<RigidBody2D>();
        sceneNode_ = GetScene()->GetChild("Scene");

        OnSetEnabled();

//        URHO3D_LOGINFOF("BlastLogic() - OnNodeSet : Node=%s(%u) bodyNode=%s(%u) enabled=%s ...", node_->GetName().CString(), node_->GetID(),
//                        body_ ? body_->GetNode()->GetName().CString() : "", body_ ? body_->GetNode()->GetID() : 0, IsEnabledEffective() ? "true" : "false");
    }
}

void BlastLogic::OnBeginContact(StringHash eventType, VariantMap& eventData)
{
    using namespace PhysicsBeginContact2D;

    Node* othernode = ((RigidBody2D*) (eventData[P_BODYA].GetPtr() == body_ ? eventData[P_BODYB].GetPtr() : eventData[P_BODYA].GetPtr()))->GetNode();

//    URHO3D_LOGINFOF("BlastLogic() - OnBeginContact : %s(%u) Begin Contact with node %s(%u)",
//                      body_->GetNode()->GetName().CString(), body_->GetNode()->GetID(), othernode->GetName().CString(), othernode->GetID());

    if (!othernode || othernode->GetVar(GOA::FACTION).GetInt() != GO_None)
        return;

    /// Add effects on die
    Vector2 position = othernode->GetWorldPosition2D();
    GameHelpers::AddEffects(sceneNode_, effectondie_.x_, effectondie_.y_, position.x_, position.y_, 0.15f, 1.1f, EFFECTLAYER);
    MatchesManager::DestroyMatch(position);
    GameHelpers::SpawnSound(GameStatics::rootScene_, SND_DESTROY1, 0.8f);

    if (dieoncontact_)
        node_->Remove();
}

void BlastLogic::OnUpdate(StringHash eventType, VariantMap& eventData)
{
    Update(eventData[SceneUpdate::P_TIMESTEP].GetFloat());
}

void BlastLogic::SeekTarget(Node* target)
{
    const float angularthreshold = 5.f;
    const float angularspeed = 2.f;

    float nodeangle = node_->GetWorldRotation2D();
    if (nodeangle < 0.f)
        nodeangle = 360.f + nodeangle;

    Vector2 targetdir = target_->GetWorldPosition2D() - node_->GetWorldPosition2D();
    float targetangle = M_RADTODEG * atan(Abs(targetdir.y_) / Abs(targetdir.x_));
    if (targetdir.x_ < 0.f)
        targetangle = 180.f - targetangle;
    if (targetdir.y_ < 0.f)
        targetangle = 360.f - targetangle;

    if (Abs(targetangle - nodeangle) > angularthreshold)
    {
        float angle = targetangle - nodeangle > 0.f ? node_->GetWorldRotation2D()+angularspeed : node_->GetWorldRotation2D()-angularspeed;
        node_->SetWorldRotation2D(angle);
        body_->SetLinearVelocity(speed_ * Vector2(Cos(angle), speed_*Sin(angle)));
    }
}

void BlastLogic::FocusOnTarget(Node* target, bool applyvelocity)
{
    Vector2 targetdir = target_->GetWorldPosition2D() - node_->GetWorldPosition2D();
    float targetangle = M_RADTODEG * atan(Abs(targetdir.y_) / Abs(targetdir.x_));
    if (targetdir.x_ < 0.f)
        targetangle = 180.f - targetangle;
    if (targetdir.y_ < 0.f)
        targetangle = 360.f - targetangle;
    node_->SetWorldRotation2D(targetangle);

    if (applyvelocity)
        body_->SetLinearVelocity(Vector2(speed_*Cos(targetangle), speed_*Sin(targetangle)));
}

void BlastLogic::Update(float timestep)
{
    if (autotargeting_)
    {
        // find a target
        if (!target_)
        {
            target_ = MatchesManager::GetRandomObject();
//            URHO3D_LOGINFOF("BlastLogic() - Update : %s(%u) : find target = %s(%u)",
//                            node_->GetName().CString(), node_->GetID(), target_ ? target_->GetName().CString() : "None", target_ ? target_->GetID() : 0);

            if (!target_)
            {
                // no target : reset steering mode to default
                // set velocity to bottom
                steeringMode_ = NOSTEERING;
                UnsubscribeFromEvent(GetScene(), E_SCENEPOSTUPDATE);
                body_->SetLinearVelocity(speed_ * initialDirection_);
            }
            else
            {
                // set direction
                if (steeringMode_ == NOSTEERING)
                    FocusOnTarget(target_, true);
            }
        }
    }

    if (target_)
    {
        if (steeringMode_ == BULLET)
            body_->SetLinearVelocity(speed_ * Vector2(Cos(node_->GetWorldRotation2D()), Sin(node_->GetWorldRotation2D())));
        else if (steeringMode_ == MISSILE)
            SeekTarget(target_);
        else if (steeringMode_ == TENTACULE)
        {
            node_->SetPosition2D(position2D_);
            FocusOnTarget(target_, false);
        }
    }
}


