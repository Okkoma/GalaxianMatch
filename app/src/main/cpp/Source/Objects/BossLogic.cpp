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

#include "GameAttributes.h"
#include "GameStatics.h"
#include "GameEvents.h"
#include "GameHelpers.h"
#include "TimerRemover.h"

#include "MAN_Matches.h"

#include "BossLogic.h"


const float IDLE_DELAY = 3.f;


struct WayPath
{
    WayPath(unsigned numpoints, const Vector3* points, const float* times) : numpoints_(numpoints), points_(points), times_(times) { }

    unsigned numpoints_;
    const Vector3* points_;
    const float* times_;
};


const char* animationNames_[] =
{
    "idle",
    "move",
    "attack1",
    "attack2",
    "attack3",
    "attack4",
    "attack5",
    "hurt",
    "break1",
    "break2",
    "break3",
    "break4",
    "break5",
    "die",
    "static",
    0
};

const Vector3 positions_[] =
{
    // Boss1 : Big Loop (0-5)
    Vector3(0.f, 2.f, 0.f), Vector3(1.f, 2.5f, 0.f), Vector3(2.f, 2.7f, 0.f), Vector3(-0.2f, 3.f, 0.f), Vector3(-1.f, 2.7f, 0.f), Vector3(-2.f, 2.3f, 0.f),
    // Boss1 : Left Loop (6-9)
    Vector3(0.f, 2.f, 0.f), Vector3(-0.2f, 3.f, 0.f), Vector3(-1.f, 2.7f, 0.f), Vector3(-2.f, 2.3f, 0.f),
    // Boss1 : Right Loop (10-13)
    Vector3(0.f, 2.f, 0.f), Vector3(-0.2f, 3.f, 0.f), Vector3(2.f, 2.7f, 0.f), Vector3(1.f, 2.5f, 0.f),
    // Boss2 : ToRight Horizontal (14-16)
    Vector3(-1.f, 1.f, 0.f), Vector3(0.f, 1.f, 0.f), Vector3(1.f, 1.f, 0.f),
    // Boss2 : ToBottomLeft Loop (17-19)
    Vector3(1.f, 1.f, 0.f), Vector3(0.f, 0.f, 0.f), Vector3(-1.f, -1.f, 0.f),
    // Boss2 : ToLeft Horizontal (20-22)
    Vector3(-1.f, -1.f, 0.f), Vector3(0.f, -1.f, 0.f), Vector3(1.f, -1.f, 0.f),
};

const float translatetimes_[] =
{
    2.f, 1.f, 1.f, 1.f, 1.f, 2.f,
    2.f, 2.f, 2.f, 2.f, 2.f,
    2.f, 2.f, 2.f, 2.f, 2.f,
    2.f, 1.f, 2.f,
    2.f, 1.f, 2.f,
    2.f, 1.f, 2.f,
};

WayPath paths_[][3] =
{
    {
        WayPath(6, &positions_[0], &translatetimes_[0]),
        WayPath(4, &positions_[6], &translatetimes_[6]),
        WayPath(4, &positions_[10], &translatetimes_[10])
    },
    {
        WayPath(3, &positions_[14], &translatetimes_[14]),
        WayPath(3, &positions_[17], &translatetimes_[17]),
        WayPath(3, &positions_[20], &translatetimes_[20])
    },
    {
        WayPath(3, &positions_[14], &translatetimes_[14]),
        WayPath(3, &positions_[17], &translatetimes_[17]),
        WayPath(3, &positions_[20], &translatetimes_[20])
    },
    {
        WayPath(6, &positions_[0], &translatetimes_[0]),
        WayPath(4, &positions_[6], &translatetimes_[6]),
        WayPath(4, &positions_[10], &translatetimes_[10])
    },
    {
        WayPath(6, &positions_[0], &translatetimes_[0]),
        WayPath(4, &positions_[6], &translatetimes_[6]),
        WayPath(4, &positions_[10], &translatetimes_[10])
    },
    {
        WayPath(6, &positions_[0], &translatetimes_[0]),
        WayPath(4, &positions_[6], &translatetimes_[6]),
        WayPath(4, &positions_[10], &translatetimes_[10])
    },
    {
        WayPath(6, &positions_[0], &translatetimes_[0]),
        WayPath(4, &positions_[6], &translatetimes_[6]),
        WayPath(4, &positions_[10], &translatetimes_[10])
    },
    {
        WayPath(3, &positions_[14], &translatetimes_[14]),
        WayPath(3, &positions_[17], &translatetimes_[17]),
        WayPath(3, &positions_[20], &translatetimes_[20])
    },
    {
        WayPath(3, &positions_[14], &translatetimes_[14]),
        WayPath(3, &positions_[17], &translatetimes_[17]),
        WayPath(3, &positions_[20], &translatetimes_[20])
    },
    {
        WayPath(3, &positions_[14], &translatetimes_[14]),
        WayPath(3, &positions_[17], &translatetimes_[17]),
        WayPath(3, &positions_[20], &translatetimes_[20])
    },
    {
        WayPath(3, &positions_[14], &translatetimes_[14]),
        WayPath(3, &positions_[17], &translatetimes_[17]),
        WayPath(3, &positions_[20], &translatetimes_[20])
    },
    {
        WayPath(3, &positions_[14], &translatetimes_[14]),
        WayPath(3, &positions_[17], &translatetimes_[17]),
        WayPath(3, &positions_[20], &translatetimes_[20])
    },
    {
        WayPath(3, &positions_[14], &translatetimes_[14]),
        WayPath(3, &positions_[17], &translatetimes_[17]),
        WayPath(3, &positions_[20], &translatetimes_[20])
    },
};



enum BossState
{
    MOVING,
    WAITING,
};


BossLogic::BossLogic(Context* context) :
    Component(context),
    attackindex_(0),
    numAttacks_(1),
    bossid_(0),
    state_(IDLE),
    path_(-1),
    life_(20),
    attackBroken_(false),
    maxLoopsByState_(3)
{ }

BossLogic::~BossLogic()
{
//    URHO3D_LOGINFOF("~BossLogic()");

}

void BossLogic::RegisterObject(Context* context)
{
    context->RegisterFactory<BossLogic>();

    URHO3D_ACCESSOR_ATTRIBUTE("Life", GetLife, SetLife, int, 20, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Number Of Attacks", GetNumAttacks, SetNumAttacks, int, 1, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Effects On Hurt", GetEffectOnHurt, SetEffectOnHurt, IntVector2, IntVector2(0,5), AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("No Attacks On Break", GetAttackBroken, SetAttackBroken, bool, false, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Max Loops On Attack", GetMaxAttackLoops, SetMaxAttackLoops, int, 3, AM_DEFAULT);
}

void BossLogic::SetNumAttacks(int num)
{
    numAttacks_ = num;
}

void BossLogic::SetAttackBroken(bool state)
{
    attackBroken_ = state;
}

void BossLogic::SetMaxAttackLoops(int num)
{
    maxLoopsByState_ = num;
}

void BossLogic::Start()
{
    if (GetScene() && node_->GetVar(GOA::ENTITYCLONE) != Variant::EMPTY)
    {
        URHO3D_LOGINFOF("BossLogic() - Start : %s(%u) !", node_->GetName().CString(), node_->GetID());

        effectNode_ = node_->GetChild("EffectNode");
        if (!effectNode_)
            effectNode_ = node_->CreateChild("EffectNode", LOCAL);

        state_ = IDLE;
        path_ = -1;
        actiontimer_ = 0.f;
        hitted_ = 0;
        numloops_ = 0;
        node_->SetVar(GOA::FACTION, GO_AI_Enemy);
        body_ = 0;

        if (node_->GetObjectAnimation())
            node_->RemoveObjectAnimation();

        animators_.Clear();

        if (node_->GetComponent<AnimatedSprite2D>())
            animators_.Push(node_->GetComponent<AnimatedSprite2D>());

        const Vector<SharedPtr<Node> >& children = node_->GetChildren();
        for (Vector<SharedPtr<Node> >::ConstIterator it = children.Begin(); it != children.End(); ++it)
        {
            Node* childnode = it->Get();
            if (childnode)
            {
                AnimatedSprite2D* animator = childnode->GetComponent<AnimatedSprite2D>();
                if (animator)
                    animators_.Push(animator);
            }
        }

        URHO3D_LOGINFOF("BossLogic() - Start : %s(%u) num animators=%u !", node_->GetName().CString(), node_->GetID(), animators_.Size());

        // Boss Appearing
        MatchesManager::SetPhysicsEnable(false);
        node_->SetEnabledRecursive(true);
        SetActive(true);

        if (animators_.Size())
        {
            body_ = animators_.Front()->GetNode()->GetComponent<RigidBody2D>();

            for (unsigned i=0; i < animators_.Size(); i++)
            {
                AnimatedSprite2D* animatedSprite = animators_[i];
                animatedSprite->ApplyCharacterMap(String("unbreak"));
            }
        }

        // All nodes are enable, so need now to disable the nodes "TrigAttack" to prevent indesire attacks
        SetTrigAttacksEnable(false);
        MatchesManager::SetPhysicsEnable(true);
    }
}

void BossLogic::Stop()
{
    if (GetScene() && node_->GetVar(GOA::ENTITYCLONE) != Variant::EMPTY)
    {
        URHO3D_LOGINFOF("BossLogic() - Stop : %s(%u) !", node_->GetName().CString(), node_->GetID());

        SetActive(false);
    }
}

void BossLogic::SubscribeToEvents()
{
    SubscribeToEvent(GetScene(), E_SCENEPOSTUPDATE, URHO3D_HANDLER(BossLogic, OnUpdate));

    if (animators_.Size())
    {
        SubscribeToEvent(animators_[0]->GetNode(), E_PHYSICSBEGINCONTACT2D, URHO3D_HANDLER(BossLogic, OnBeginContact));
        SubscribeToEvent(animators_[0]->GetNode(), SPRITER_ENTITY, URHO3D_HANDLER(BossLogic, OnSpawnEntity));
    }
}

void BossLogic::UnsubscribeFromEvents()
{
    UnsubscribeFromAllEvents();
}

void BossLogic::SetAlpha(float alphastart, float alphaend, float delay)
{
    // Boss Appearing
    for (unsigned i=0; i < animators_.Size(); i++)
    {
        AnimatedSprite2D* sprite = animators_[i];
        sprite->SetAlpha(alphastart);
        SharedPtr<ObjectAnimation> objectAnimation(new ObjectAnimation(GetContext()));
        SharedPtr<ValueAnimation> alphaAnimation(new ValueAnimation(GetContext()));
        alphaAnimation->SetKeyFrame(0.f, alphastart);
        alphaAnimation->SetKeyFrame(delay, alphaend);
        objectAnimation->AddAttributeAnimation("Alpha", alphaAnimation, WM_ONCE);
        sprite->SetObjectAnimation(objectAnimation);
    }
}

void BossLogic::SetTrigAttacksEnable(bool enable)
{
    PODVector<Node*> trigAttacks;
    node_->GetChildrenWithNameStartsWith(trigAttacks, TRIGATTACK, true);
    for (unsigned i=0; i < trigAttacks.Size(); i++)
        trigAttacks[i]->SetEnabled(enable);
}

void BossLogic::SetActive(bool state, bool alpha)
{
    URHO3D_LOGINFOF("BossLogic() - SetActive : %s(%u) %s !", node_->GetName().CString(), node_->GetID(), state ? "true":"false");

    // set enable move animation
    node_->SetAnimationEnabled(state);
    for (unsigned i=0; i < animators_.Size(); i++)
        animators_[i]->GetNode()->SetAnimationEnabled(state);

    if (alpha)
    {
        if (state)
            SetAlpha(0.f, 1.f, 2.f);
        else if (!animators_.Front()->HasAnimation(animationNames_[DIE]))
            SetAlpha(animators_.Front()->GetAlpha(), 0.f, 3.f);
    }

    if (state)
        SubscribeToEvents();
    else
        UnsubscribeFromEvents();
}

void BossLogic::SetBoss(int bossid)
{
    bossid_ = bossid;
}

void BossLogic::SetPath(int path)
{
    if (path != path_ && IsEnabledEffective())
    {
        path_ = path;

//        URHO3D_LOGINFOF("BossLogic() - SetPath : %s(%u) => bossid=%d path=%d !", node_->GetName().CString(), node_->GetID(), bossid_, path_);

        bool addnewobject = !node_->GetObjectAnimation();
        SharedPtr<ObjectAnimation> objectAnimation(addnewobject ? new ObjectAnimation(context_) : node_->GetObjectAnimation());

        SharedPtr<ValueAnimation> moveAnimation(new ValueAnimation(context_));
        moveAnimation->SetKeyFrame(0.f, node_->GetPosition());

        const WayPath& wpath = paths_[bossid_][path_];
        actiontimer_ = 0.f;
        for (unsigned i=0; i< wpath.numpoints_;i++)
        {
            actiontimer_ += wpath.times_[i];
            moveAnimation->SetKeyFrame(actiontimer_, wpath.points_[i]);
        }

        objectAnimation->AddAttributeAnimation("Position", moveAnimation, WM_ONCE);

        if (addnewobject)
            node_->SetObjectAnimation(objectAnimation);

//        GameHelpers::SaveNodeXML(GetContext(), node_, String().AppendWithFormat("boss%d-nodepath%d.xml", bossid_, path_).CString());
    }
}

bool BossLogic::SetState(int state)
{
    if (state != state_)
    {
        bool changestate = false;
        numloops_ = 0;

//        SetTrigAttacksEnable(state >= ATTACK1 && state <= ATTACK5 ? true : false);

        const String animName(animationNames_[state]);

        for (unsigned i=0; i < animators_.Size(); i++)
        {
            int newstate = state;

            AnimatedSprite2D* animator = animators_[i];
            Node* node = animator->GetNode();

            if (animator->HasAnimation(animName))
            {
                // for the first break, show all parts (boss13)
//                if (state == BREAK1)
//                    animator->ApplyCharacterMap(String("all"));
//                    animator->ApplyCharacterMap(String("unbreak"));
                // if oldstate is a break state, apply character Map Break.
                if (state >= BREAK1 && state <= BREAK5)
                {
                    if (!animator->IsCharacterMapApplied(animName))
                        animator->ApplyCharacterMap(animName);
                    else
                        newstate = HURT;
                }

                animator->SetAnimation(animationNames_[newstate]);

                changestate = true;

//                URHO3D_LOGINFOF("BossLogic() - SetState : %s(%u) state=%s(%d)",
//                                node->GetName().CString(), node->GetID(), animationNames_[newstate], newstate);
            }
            else
            {
                // if newstate is a break state, and no animation for it, set HURT and apply character Map Break.
                if (state >= BREAK1 && state <= BREAK5)
                {
                    changestate = true;
                    animator->ApplyCharacterMap(animName);
                    newstate = HURT;
                    animator->SetAnimation(animationNames_[newstate]);
                }
                else
                {
                    /// TODO : change this !
                    // Desactive entities made for attacks only for Boss3 "Tentacule" or boss9 "Laser"
                    if (animator->GetEntity() == "boss03_langue" || animator->GetEntity() == "laser")
                        animators_[i]->GetNode()->SetEnabledRecursive(false);

                    URHO3D_LOGWARNINGF("BossLogic() - SetState : %s(%u) no state=%s(%d) !",
                                node->GetName().CString(), node->GetID(), animationNames_[state], state);
                }
            }
        }

        if (state == DIE)
            SetActive(false);

        if (changestate)
        {
            state_ = state;

            node_->SendEvent(GAME_BOSSSTATECHANGE);
//            URHO3D_LOGINFOF("BossLogic() - SetState : %s(%u) state=%s(%d) send GAME_BOSSSTATECHANGE",
//                            node_->GetName().CString(), node_->GetID(), animationNames_[state], state);
        }
    }

    return state_ == state;
}

void BossLogic::SetLife(int life)
{
    life_ = life;
    hitted_ = 0;
}

void BossLogic::SetEffectOnHurt(const IntVector2& effect)
{
    effectonhurt_ = effect;
}


void BossLogic::OnSetEnabled()
{
    URHO3D_LOGINFOF("BossLogic() - OnSetEnabled : Node=%s(%u) enabled=%s ...", node_->GetName().CString(), node_->GetID(), IsEnabledEffective() ? "true" : "false");

    if (IsEnabledEffective())
        Start();
    else
        Stop();

    URHO3D_LOGINFOF("BossLogic() - OnSetEnabled : Node=%s(%u) enabled=%s ... OK !", node_->GetName().CString(), node_->GetID(), IsEnabledEffective() ? "true" : "false");
}

void BossLogic::OnNodeSet(Node* node)
{
    if (node)
    {
        URHO3D_LOGINFOF("BossLogic() - OnNodeSet : Node=%s(%u) enabled=%s ...",
                        node_->GetName().CString(), node_->GetID(), IsEnabledEffective() ? "true" : "false");

        sceneNode_ = GetScene()->GetChild("Scene");

        OnSetEnabled();
    }
}

void BossLogic::OnEvent(StringHash eventType, VariantMap& eventData)
{
//    URHO3D_LOGINFOF("BossLogic() - OnEvent : Node=%s(%u) - currentState %s - event %s", node_->GetName().CString(), node_->GetID(), currentState->name.CString(), GOE::GetEventName(eventType).CString());


}

void BossLogic::OnBeginContact(StringHash eventType, VariantMap& eventData)
{
    using namespace PhysicsBeginContact2D;

    RigidBody2D* other = 0;

    {
        RigidBody2D* b1 = (RigidBody2D*) eventData[P_BODYA].GetPtr();
        RigidBody2D* b2 = (RigidBody2D*) eventData[P_BODYB].GetPtr();
        CollisionShape2D* csOther = 0;

        if (b1 == body_)
        {
            // no trigger for body shape
            if (((CollisionShape2D*) eventData[P_SHAPEA].GetPtr())->IsTrigger())
                return;

            other = (RigidBody2D*) eventData[P_BODYB].GetPtr();;
            csOther = (CollisionShape2D*) eventData[P_SHAPEB].GetPtr();
        }
        else
        {
            // no trigger for body shape
            if (((CollisionShape2D*) eventData[P_SHAPEB].GetPtr())->IsTrigger())
                return;

            other = (RigidBody2D*) eventData[P_BODYA].GetPtr();;
            csOther = (CollisionShape2D*) eventData[P_SHAPEA].GetPtr();
        }

        // other shape must be a Trigger
        if (!csOther->IsTrigger())
            return;
    }

    if (other && other->GetNode() && other->GetNode()->GetVar(GOA::FACTION) != Variant::EMPTY)
        if (other->GetNode()->GetVar(GOA::FACTION).GetInt() == GO_Player)
            Hit(5);
}

void BossLogic::ResetHitted()
{
    hitted_ = 0;

    /// Update Objectives
    MatchesManager::SetObjectiveRemainingQty(0, GetCurrentLife());
    SendEvent(GAME_OBJECTIVECHANGE);
}

void BossLogic::Hit(int dps, bool updateobjectives, bool forced)
{
    Vector2 position = animators_[0]->GetNode()->GetWorldPosition2D();

    /// TODO : Correct the pb with C_POINT
//    float xcontact = eventData.Find(P_CPOINT) != eventData.End() ? eventData[P_CPOINT].GetVector2().x_ : node_->GetWorldPosition().x_ + Random(-0.5f,0.5f);
    float xcontact = position.x_ + Random(-0.5f,0.5f);

    /// Add Hurt Effects
    GameHelpers::AddEffects(effectNode_, effectonhurt_.x_, effectonhurt_.y_, xcontact, position.y_ - 1.f, 0.15f, 1.1f,
                            animators_[animators_.Size()-1]->GetLayer()+1);

    if (!forced && state_ >= HURT)
        return;

    URHO3D_LOGINFOF("BossLogic() - Hit : %s(%u) Hitted on pointx=%f effectNodeScale=%s",
                      body_->GetNode()->GetName().CString(), body_->GetNode()->GetID(), xcontact, effectNode_->GetScale2D().ToString().CString());

    /// Update Life States
    if (GetCurrentLife() > 0)
        hitted_ += dps;

    int animinc = hitted_/((float)GetLife()/6);
    // 5 => 5/20/6 = 1 = BREAK1
    // 10 => 10/3.33 = 3 = BREAK3
    // 15 => 15/3.33 = 4  = BREAK4
    // 20 => 20/3.33 = 6  = BREAK5
    if (GetCurrentLife() > 0)
        SetState(Min(HURT+animinc, BREAK5));
    else
        SetState(DIE);

    URHO3D_LOGWARNINGF("BossLogic() - Hit : Node=%s(%u) hitted_=%d animinc=%d !", node_->GetName().CString(), node_->GetID(), hitted_, animinc);

    /// Update Objectives
    if (updateobjectives)
    {
        MatchesManager::SetObjectiveRemainingQty(0, GetCurrentLife());
        SendEvent(GAME_OBJECTIVECHANGE);
    }
}

void BossLogic::OnSpawnEntity(StringHash eventType, VariantMap& eventData)
{
//    URHO3D_LOGINFOF("BossLogic() - OnSpawnEntity : Node=%s(%u) ...", node_->GetName().CString(), node_->GetID());
    const EventTriggerInfo& triggerInfo = animators_[0]->GetEventTriggerInfo();

    Node* node = 0;

    if (!triggerInfo.type_)
    {
        unsigned animatorid = ToUInt(triggerInfo.datas_);

        if (animatorid < animators_.Size())
        {
            animators_[animatorid]->SetAnimation(animationNames_[ATTACK1]);
            node = animators_[animatorid]->GetNode();

            // necessary for boss3 blast tentacule activation
            node->SetVar(GOA::ENTITYCLONE, true);
            node->SetEnabled(false);

//            URHO3D_LOGINFOF("BossLogic() - OnSpawnEntity : Node=%s(%u) Spawn animatorid=%u/%u node=%s(%u) !",
//                               node_->GetName().CString(), node_->GetID(), animatorid+1, animators_.Size(),
//                               node->GetName().CString(), node->GetID());
        }
        else
        {
//            DumpVariantMap(eventData);
            URHO3D_LOGWARNINGF("BossLogic() - OnSpawnEntity : Node=%s(%u) no GOT !", node_->GetName().CString(), node_->GetID());
            return;
        }
    }
    else
    {
        node = GameHelpers::SpawnGOtoNode(context_, triggerInfo.type_, sceneNode_);
        if (!node)
        {
            URHO3D_LOGWARNINGF("BossLogic() - OnSpawnEntity : Node=%s(%u) noSpawn !", node_->GetName().CString(), node_->GetID());
            return;
        }
        Vector<String> datas = triggerInfo.datas_.Split(';');
        node->SetWorldRotation2D(datas.Size() > 0 ? ToFloat(datas[0]) : 0.f);

        Drawable2D* drawable = node->GetDerivedComponent<Drawable2D>();
        drawable->SetLayer(EFFECTLAYER);
        drawable->SetOrderInLayer(triggerInfo.zindex_);
    }

    node->SetVar(GOA::FACTION, GO_AI_Enemy);
    node->SetWorldPosition2D(triggerInfo.position_);
    node->SetEnabled(true);

//    URHO3D_LOGINFOF("BossLogic() - OnSpawnEntity : Node=%s(%u) ... spawn=%s(%u) zindex=%d ... OK !",
//                    node_->GetName().CString(), node_->GetID(), node->GetName().CString(), node->GetID(), triggerInfo.zindex_);
}

void BossLogic::OnUpdate(StringHash eventType, VariantMap& eventData)
{
    Update(eventData[SceneUpdate::P_TIMESTEP].GetFloat());
}

void BossLogic::Update(float timestep)
{
    if (animators_[0]->HasFinishedAnimation() && state_ >= ATTACK1 && state_ <= BREAK5)
    {
        if (GetCurrentLife() > 0)
        {
            if (animators_[0]->GetSpriterInstance()->GetLooping())
            {
                numloops_++;
                if (numloops_ >= maxLoopsByState_)
                {
                    SetState(MOVE);
                    numloops_ = 0;
                }
//                URHO3D_LOGINFOF("BossLogic() - Update : Node=%s(%u) ... state=%d loops=%d ", node_->GetName().CString(), node_->GetID(), state_, numloops_);
            }
            else
                SetState(MOVE);
        }
        else
            SetState(DIE);

//        SetTrigAttacksEnable(false);

        return;
    }

    if (state_ == DIE)
    {
        URHO3D_LOGINFOF("BossLogic() - Update : Node=%s(%u) ... Stopped ! ", node_->GetName().CString(), node_->GetID());
        Stop();
        return;
    }

    actiontimer_ -= timestep;

    if (state_ == IDLE && actiontimer_ < 0.f)
    {
        if ((path_+1) % 2)
        {
            if (numAttacks_ > 1)
            {
                for (;;)
                {
                    attackindex_ = (attackindex_+1) % numAttacks_;
                    if (!attackBroken_ || !animators_[0]->IsCharacterMapApplied(String(animationNames_[BREAK1 + attackindex_])))
                    {
                        SetState(ATTACK1 + attackindex_);
                        break;
                    }

                    if (attackindex_ == 0)
                        break;

                    URHO3D_LOGINFOF("BossLogic() - Update : Node=%s(%u) ... state=%d path=%d actionTimer=%f attacindex_=%d", node_->GetName().CString(), node_->GetID(), state_, path_, actiontimer_, attackindex_);
                }
            }
            else
            {
                SetState(ATTACK1 + attackindex_);
            }
        }
        else
            SetState(MOVE);

        SetPath((path_+1) % 3);
    }

    if (state_ > IDLE && actiontimer_ < 0.f)
    {
        actiontimer_ = IDLE_DELAY;
        SetState(IDLE);
    }

//    URHO3D_LOGINFOF("BossLogic() - Update : Node=%s(%u) ... state=%d path=%d actionTimer=%f ", node_->GetName().CString(), node_->GetID(), state_, path_, actiontimer_);
}


