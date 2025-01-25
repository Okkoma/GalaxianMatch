#pragma once

#include <Urho3D/Scene/Component.h>

namespace Urho3D
{
    class AnimatedSprite2D;
    class RigidBody2D;
}

using namespace Urho3D;

enum BossAnimation
{
    IDLE,
    MOVE,
    ATTACK1,
    ATTACK2,
    ATTACK3,
    ATTACK4,
    ATTACK5,
    HURT,
    BREAK1,
    BREAK2,
    BREAK3,
    BREAK4,
    BREAK5,
    DIE,
    STATIC,
    MAXANIMATIONS
};

class BossLogic : public Component
{
    URHO3D_OBJECT(BossLogic, Component);

    public:
        BossLogic(Context* context);
        virtual ~BossLogic();

        static void RegisterObject(Context* context);

        void Start();
        void Stop();

        void SetActive(bool state, bool alpha=true);
        void SetAlpha(float alphastart, float alphaend, float delay);

        void SetNumAttacks(int num);
        int GetNumAttacks() const { return numAttacks_; }
        void SetMaxAttackLoops(int num);
        int GetMaxAttackLoops() const { return maxLoopsByState_; }
        void SetBoss(int bossid);
        void SetPath(int path);
        bool SetState(int state);
        void SetLife(int life);
        int GetState() const { return state_; }
        int GetLife() const { return life_; }
        int GetCurrentLife() const { return life_ - hitted_; }
        void SetEffectOnHurt(const IntVector2& effect);
        const IntVector2& GetEffectOnHurt() const { return effectonhurt_; }
        void SetAttackBroken(bool state);
        bool GetAttackBroken() const { return attackBroken_; }
        const Vector<AnimatedSprite2D*>& GetAnimators() const { return animators_; }

        void ResetHitted();
        void Hit(int dps, bool updateobjectives=true, bool forced=false);

        virtual void OnSetEnabled();

    protected :
        virtual void OnNodeSet(Node* node);
        virtual void SubscribeToEvents();
        virtual void UnsubscribeFromEvents();

        void SetTrigAttacksEnable(bool enable);

        Vector<AnimatedSprite2D*> animators_;
        RigidBody2D* body_;
        Node* effectNode_;
        Node* sceneNode_;

    private :
        void AddEffects(Node* root, int index, int num, float x, float y, float delaybetweenspawns, float removedelay);

        /// Handler
        void OnUpdate(StringHash eventType, VariantMap& eventData);
        void OnEvent(StringHash eventType, VariantMap& eventData);
        void OnBeginContact(StringHash eventType, VariantMap& eventData);
        void OnSpawnEntity(StringHash eventType, VariantMap& eventData);

        /// Updater
        void Update(float timestep);

        int attackindex_;
        int numAttacks_;
        int bossid_;
        int state_;
        int path_;
        int life_;
        int hitted_;
        int numloops_, maxLoopsByState_;
        IntVector2 effectonhurt_;
        float actiontimer_;
        bool attackBroken_;
};

