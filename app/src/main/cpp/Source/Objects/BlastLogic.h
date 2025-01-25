#pragma once

#include <Urho3D/Scene/Component.h>

namespace Urho3D
{
    class AnimatedSprite2D;
    class RigidBody2D;
}

using namespace Urho3D;

enum BlastSteeringMode
{
   NOSTEERING = 0,
   BULLET,
   MISSILE,
   TENTACULE
};

class BlastLogic : public Component
{
    URHO3D_OBJECT(BlastLogic, Component);

    public:
        BlastLogic(Context* context);
        virtual ~BlastLogic();

        static void RegisterObject(Context* context);

        void SetSteeringMode(BlastSteeringMode mode);
        BlastSteeringMode GetSteeringMode() const { return steeringMode_; }
        void SetAutoTargeting(bool state);
        bool GetAutoTargeting() const { return autotargeting_; }
        void SetAutoStart(bool state);
        bool GetAutoStart() const { return autostart_; }
        void SetInitialDirection(const Vector2& dir);
        const Vector2& GetInitialDirection() const { return initialDirection_; }
        void SetLifeTime(float time);
        float GetLifeTime() const { return lifetime_; }
        void SetSpeed(float speed);
        float GetSpeed() const { return speed_; }
        void SetDieOnContact(bool state);
        bool GetDieOnContact() const { return dieoncontact_; }
        void SetEffectOnDie(const IntVector2& effect);
        const IntVector2& GetEffectOnDie() const { return effectondie_; }
        void Start();
        void Stop();

        virtual void OnSetEnabled();

        void OnBeginContact(StringHash eventType, VariantMap& eventData);

    protected :
        virtual void OnNodeSet(Node* node);

    private :

        void OnUpdate(StringHash eventType, VariantMap& eventData);

        /// Updater
        void SeekTarget(Node* target);
        void FocusOnTarget(Node* target, bool applyvelocity);
        void Update(float timestep);

        AnimatedSprite2D* animator_;
        RigidBody2D* body_;
        Node* sceneNode_;

        BlastSteeringMode steeringMode_;
        bool autotargeting_, autostart_;
        Vector2 initialDirection_;
        float lifetime_, speed_, maxforce_;
        bool dieoncontact_;
        IntVector2 effectondie_;

        WeakPtr<Node> target_;
};

