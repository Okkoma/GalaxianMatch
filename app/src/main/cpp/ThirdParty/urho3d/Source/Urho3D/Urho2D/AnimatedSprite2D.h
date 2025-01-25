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

#include "../Urho2D/StaticSprite2D.h"

#ifdef URHO3D_SPINE
struct spAnimationState;
struct spAnimationStateData;
struct spSkeleton;
#endif

/// Loop mode.
enum LoopMode2D
{
    /// Default, use animation's value.
    LM_DEFAULT = 0,
    /// Force looped.
    LM_FORCE_LOOPED,
    /// Force clamped.
    LM_FORCE_CLAMPED
};



namespace Urho3D
{

namespace Spriter
{
    class SpriterInstance;
    struct Animation;
    struct CharacterMap;
    struct SpatialTimelineKey;
    struct SpriteTimelineKey;
}

class AnimationSet2D;

    struct SpriteInfo
    {
        SpriteInfo() : sprite_(0) { }

        Sprite2D* sprite_;
        float scalex_;
        float scaley_;
        float deltaHotspotx_;
        float deltaHotspoty_;
    };

    struct EventTriggerInfo
    {
        StringHash type_;
        StringHash type2_;
        unsigned char entityid_;
        Vector2 position_;
        float rotation_;
        int zindex_;
        Node* node_;
        String datas_;
    };

/// Animated sprite component, it uses to play animation created by Spine (http://www.esotericsoftware.com) and Spriter (http://www.brashmonkey.com/).
class URHO3D_API AnimatedSprite2D : public StaticSprite2D
{
    URHO3D_OBJECT(AnimatedSprite2D, StaticSprite2D);

public:
    /// Construct.
    AnimatedSprite2D(Context* context);
    /// Destruct.
    virtual ~AnimatedSprite2D();
    /// Register object factory.
    static void RegisterObject(Context* context);

    /// Handle enabled/disabled state change.
    virtual void OnSetEnabled();

/// ENTITY/ANIMATION SETTERS

    /// Set animation set.
    void SetAnimationSet(AnimationSet2D* animationSet);
    /// Set entity name (skin name for spine, entity name for spriter).
    void SetEntity(const String& name);
    /// Set spriter entity by index
    void SetSpriterEntity(int index);
    /// Set animation by name and loop mode.
    void SetAnimation(const String& name = String::EMPTY, LoopMode2D loopMode = LM_DEFAULT);
    /// Set Spriter animation by index.
    void SetSpriterAnimation(int index=-1, LoopMode2D loopMode = LM_DEFAULT);
    /// Set loop mode.
    void SetLoopMode(LoopMode2D loopMode);
    /// Set speed.
    void SetSpeed(float speed);
    /// Set animation enable for rendering
    void SetRenderEnable(bool enable, int zindex=0);
    void SetCustomSpriteSheetAttr(const String& value);
    /// Set animation set attribute.
    void SetAnimationSetAttr(const ResourceRef& value);
    /// Set animation by name.
    void SetAnimationAttr(const String& name);

    /// Reset variables
    virtual void CleanDependences();
    /// Reset Animation
    void ResetAnimation();

    void SetTime(float time);

/// ENTITY/ANIMATION GETTERS

    /// Return animation Set.
    AnimationSet2D* GetAnimationSet() const;
    /// Return Spriter Animation by index or the current animation.
    Spriter::Animation* GetSpriterAnimation(int index=-1) const;
    /// Return Current Spriter Animation ID.
	int GetSpriterAnimationId() const;
    /// Return Spriter Animation ID with name.
	int GetSpriterAnimationId(const String& animationName) const;
    /// Return Spriter Animation by Name
    Spriter::Animation* GetSpriterAnimation(const String& animationName) const;
    /// Return entity name.
	const String& GetEntity() const { return entityName_; }
    const String& GetEntityName() const { return entityName_; }
    /// Return num entities in the AnimationSet
    unsigned GetNumSpriterEntities() const;
    /// Return entity name by index.
    const String& GetSpriterEntity(int index) const;
    unsigned GetSpriterEntityIndex() const;

    /// Return animation name.
    bool HasAnimation(const String& name) const;
    const String& GetAnimation() const { return animationName_; }
    /// Return animation name by default.
    const String& GetDefaultAnimation() const;
    /// Return loop mode.
    LoopMode2D GetLoopMode() const { return loopMode_; }
    /// Return speed.
    float GetSpeed() const { return speed_; }

    /// Return time passed on the current animation.
    float GetCurrentAnimationLength() const;
    float GetCurrentAnimationTime() const;
    bool HasFinishedAnimation() const;

    /// Return SpriterInstance
    Spriter::SpriterInstance* GetSpriterInstance() const;

    void GetLocalSpritePositions(unsigned spriteindex, Vector2& position, float& angle, Vector2& scale);
    Sprite2D* GetSprite(unsigned spriteindex) const; //, bool fromInstance=false) const;

    /// Get Event Trigger Infos
    const EventTriggerInfo& GetEventTriggerInfo() const { return triggerInfo_; }

    /// Return animation set attribute.
    ResourceRef GetAnimationSetAttr() const;

/// CHARACTER MAPPING SETTERS

    void SetAppliedCharacterMapsAttr(const VariantVector& characterMapApplied);
    void SetCharacterMapAttr(const String& names);

    bool ApplyCharacterMap(const StringHash& hashname);
    bool ApplyCharacterMap(const String& name);

    void SwapSprite(const StringHash& characterMap, Sprite2D* replacement, unsigned index=0, bool keepProportion=false);
    void SwapSprites(const StringHash& characterMap, const PODVector<Sprite2D*>& replacements, bool keepProportion=false);
    void SwapSprite(const String& characterMap, Sprite2D* replacement, unsigned index=0, bool keepProportion=false);
    void SwapSprites(const String& characterMap, const PODVector<Sprite2D*>& replacements, bool keepProportion=false);

    void UnSwapSprite(Sprite2D* original);
    void UnSwapAllSprites();

    void ResetCharacterMapping();

/// CHARACTER MAPPING GETTERS

//    String GetCharacterMapAttr() const;
    const VariantVector& GetAppliedCharacterMapsAttr() const;
    const String& GetEmptyString() const { return String::EMPTY; }

    bool HasCharacterMapping() const;
    bool HasCharacterMap(const StringHash& hashname) const;
    bool HasCharacterMap(const String& name) const;

    bool IsCharacterMapApplied(const StringHash& hashname) const;
    bool IsCharacterMapApplied(const String& name) const;

    Spriter::CharacterMap* GetCharacterMap(const StringHash& characterMap) const;
    Sprite2D* GetCharacterMapSprite(const StringHash& characterMap, unsigned index=0) const;
    Spriter::CharacterMap* GetCharacterMap(const String& characterMap) const;
    Sprite2D* GetCharacterMapSprite(const String& characterMap, unsigned index=0) const;

    void GetMappedSprites(Spriter::CharacterMap* characterMap, PODVector<Sprite2D*>& sprites) const;
    Sprite2D* GetMappedSprite(unsigned key) const;
    Sprite2D* GetMappedSprite(int folderid, int fileid) const;
    Sprite2D* GetSwappedSprite(Sprite2D* original) const;
    const PODVector<SpriteInfo*>& GetSpriteInfos();

/// FOR UI Batch / UI AnimatedSprite
    template< typename T > void GetVertices(const IntVector2& size, const T& transform, PODVector<float>& verticeData);

    /// Update animation.
    void UpdateAnimation(float timeStep);

/// HELPERS

    void DumpSpritesInfos() const;

protected:
    /// Handle draw order changed.
	virtual void OnDrawOrderChanged();
    /// Handle scene being assigned.
    virtual void OnSceneSet(Scene* scene);
    /// Handle update vertices.
    virtual void UpdateSourceBatches();
//    /// Update material.
//    virtual void UpdateMaterial();
    virtual bool UpdateDrawRectangle();

    /// Handle scene post update.
    void HandleScenePostUpdate(StringHash eventType, VariantMap& eventData);

#ifdef URHO3D_SPINE
    /// Handle set spine animation.
    void SetSpineAnimation();
    /// Update spine animation.
    void UpdateSpineAnimation(float timeStep);
    /// Update vertices for spine animation;
    void UpdateSourceBatchesSpine();
#endif

    /// Set Spriter triggers.
    void SetTriggers();
    void HideTriggers();
    void ClearTriggers(bool removeNode);

    /// Update Spriter triggers.
    inline void LocalToWorld(Spriter::SpatialTimelineKey* key, Vector2& position, float& rotation);
    void UpdateTriggers();

    /// Update spriter animation.
    void UpdateSpriterAnimation(float timeStep);

    /// Update Batches
    template< typename T > void UpdateSourceBatchesSpriter_OneMaterial(const T& nodeWorldTransform, Vector<SourceBatch2D>& sourceBatches, bool resetBatches=true);
    template< typename T > void UpdateSourceBatchesSpriter_MultiMaterials(const T& nodeWorldTransform, Vector<SourceBatch2D>& sourceBatches, int breakZIndex=-1, bool resetBatches=true);

    void UpdateSourceBatchesSpriter_RenderNodes();
    void UpdateSourceBatchesSpriter_RenderNodesOrthographic();

    inline void AddSprite(Sprite2D* sprite, const Matrix3x4& transform, float x, float y, float px, float py, float sx, float sy, float angle, float alpha, float* vertices);
    inline void AddSprite(Sprite2D* sprite, const Matrix2x3& transform, float x, float y, float px, float py, float sx, float sy, float angle, float alpha, float* vertices);
    inline void AddSprite_UI(Sprite2D* sprite, const IntVector2& size, const Matrix3x4& transform, float x, float y, float px, float py, float sx, float sy, float angle, float alpha, float* vertices);

    /// Dispose.
    void Dispose(bool removeNode=false);

    /// Character Maps
    bool ApplyCharacterMap(Spriter::CharacterMap* characterMap);

    void SwapSprite(Sprite2D* original, Sprite2D* replacement, bool keepProportion=false);
    void SwapSprites(const PODVector<Sprite2D*>& originals, const PODVector<Sprite2D*>& replacements, bool keepProportion=false);

    SpriteInfo* GetSpriteInfo(Sprite2D* sprite);

    /// Speed.
    float speed_;
    /// Entity name.
    String entityName_;
    /// Animation set.
    SharedPtr<AnimationSet2D> animationSet_;
    /// Animation name.
    String animationName_;

    /// Loop mode.
    LoopMode2D loopMode_;
    /// characterMap using
    bool useCharacterMap_;
    bool characterMapDirty_;
    bool renderEnabled_;
	int renderZIndex_;
	unsigned firstKeyIndex_, stopKeyIndex_;

#ifdef URHO3D_SPINE
    /// Skeleton.
    spSkeleton* skeleton_;
    /// Animation state data.
    spAnimationStateData* animationStateData_;
    /// Animation state.
    spAnimationState* animationState_;
#endif

    /// Spriter instance.
    UniquePtr<Spriter::SpriterInstance> spriterInstance_;

    PODVector<StringHash> activedEventTriggers_;
    PODVector<Node* > updatedPhysicNodes_;
    PODVector<Node* > triggerNodes_;
    PODVector<Node* > renderNodes_;

    /// Spriter Batch Update
    PODVector<Spriter::SpriteTimelineKey* > spritesKeys_;
//    PODVector<Sprite2D*> sprites_;
    PODVector<SpriteInfo*> spritesInfos_;

    /// Applied Character Maps.
    PODVector<Spriter::CharacterMap* > characterMaps_;
    VariantVector characterMapApplied_;

    /// Current Sprite Mapping.
    HashMap<unsigned, SharedPtr<Sprite2D> > spriteMapping_;
    /// Swap Sprite Mapping
    HashMap<Sprite2D*, SharedPtr<Sprite2D> > swappedSprites_;
    /// Swap Sprite Mapping Info
    HashMap<Sprite2D*, SpriteInfo > spriteInfoMapping_;

    /// Trigger Infos
    EventTriggerInfo triggerInfo_;
};

}
