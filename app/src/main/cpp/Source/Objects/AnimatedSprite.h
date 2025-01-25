#pragma once

#include <Urho3D/UI/UIBatch.h>
#include <Urho3D/UI/Sprite.h>
#include <Urho3D/Urho2D/AnimatedSprite2D.h>


using namespace Urho3D;

class AnimatedSprite : public Sprite
{
    URHO3D_OBJECT(AnimatedSprite, Sprite);

public:
    AnimatedSprite(Context* context);

    virtual ~AnimatedSprite();

    static void RegisterObject(Context* context);

    void SetAnimationSet(AnimationSet2D* animationSet);
    void SetAnimationSetAttr(const ResourceRef& value);
    void SetEntity(const String& entity);
    void SetAnimation(const String& animation);
    void SetHotSpot(const Vector2& hotspot);
    void SetLoopMode(LoopMode2D loopMode);

    void SetFollowedElement(UIElement* element);
    void SwitchIndependentUpdate();
    void SetScale2D(float scale);

    virtual void Update(float timeStep);

    virtual void GetBatches(PODVector<UIBatch>& batches, PODVector<float>& vertexData, const IntRect& currentScissor);

    ResourceRef GetAnimationSetAttr() const { return animatedSprite2D_.GetAnimationSetAttr() ; }
    const String& GetEntityName() const { return animatedSprite2D_.GetEntityName(); }
    const String& GetAnimation() const { return animatedSprite2D_.GetAnimation(); }
    const Vector2& GetHotSpot() const { return animatedSprite2D_.GetHotSpot(); }
    LoopMode2D GetLoopMode() const { return animatedSprite2D_.GetLoopMode(); }

    AnimatedSprite2D& GetAnimatedSprite2D() { return animatedSprite2D_; }

    virtual const Matrix3x4& GetTransform() const;

private:
    void HandleUpdate(StringHash eventType, VariantMap& eventData);

    AnimatedSprite2D animatedSprite2D_;
    Vector2 scale2D_;
    WeakPtr<UIElement> followedElement_;
};

