#include <Urho3D/Urho3D.h>

#include <Urho3D/Core/Context.h>
#include <Urho3D/Core/Profiler.h>

#include <Urho3D/IO/Log.h>

#include <Urho3D/Graphics/Camera.h>
#include <Urho3D/Graphics/Graphics.h>
#include <Urho3D/Graphics/GraphicsDefs.h>
#include <Urho3D/Graphics/Material.h>
#include <Urho3D/Graphics/Octree.h>
#include <Urho3D/Graphics/RenderPath.h>
#include <Urho3D/Graphics/Texture2D.h>
#include <Urho3D/Graphics/Technique.h>

#include <Urho3D/Physics2D/CollisionCircle2D.h>
#include <Urho3D/Physics2D/CollisionBox2D.h>

#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Resource/XMLFile.h>

#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Scene/SceneEvents.h>

#ifdef URHO3D_SPINE
#include <spine/spine.h>
#endif

#include "Sprite2D.h"
#include "Renderer2D.h"
#include "SpriterInstance2D.h"
#include "AnimationSet2D.h"

#include "AnimatedSprite2D.h"


static const Urho3D::StringHash SPRITER_SOUND           = Urho3D::StringHash("SPRITER_Sound");
static const Urho3D::StringHash SPRITER_ANIMATION       = Urho3D::StringHash("SPRITER_Animation");
static const Urho3D::StringHash SPRITER_ANIMATIONINSIDE = Urho3D::StringHash("SPRITER_AnimationInside");
static const Urho3D::StringHash SPRITER_ENTITY          = Urho3D::StringHash("SPRITER_Entity");


extern const char* URHO2D_CATEGORY;
extern const char* blendModeNames[];

const char* loopModeNames[] =
{
    "Default",
    "ForceLooped",
    "ForceClamped",
    0
};


SpriteMapInfo::SpriteMapInfo()
{ }

void SpriteMapInfo::Clear()
{
    sprite_.Reset();
    map_ = nullptr;
    instruction_ = nullptr;
}

void SpriteMapInfo::Set(unsigned key, Sprite2D* sprite, Spriter::CharacterMap* map, Spriter::MapInstruction* instruction)
{
    key_ = key;
    sprite_ = sprite;
    map_ = map;
    instruction_ = instruction;
}

AnimatedSprite2D::AnimatedSprite2D(Context* context) :
    StaticSprite2D(context),
#ifdef URHO3D_SPINE
    skeleton_(nullptr),
    animationStateData_(nullptr),
    animationState_(nullptr),
#endif
    speed_(1.0f),
    loopMode_(LM_DEFAULT),
    useCharacterMap_(false),
    characterMapDirty_(true),
    customSourceBatches_(nullptr),
    animationIndex_(0)
{
    sourceBatches_.Reserve(10);
    sourceBatches_.Resize(1);

    triggerNodes_.Reserve(5);
    spriteInfoMapping_.Clear();
    worldBoundingBoxDirty_ = true;
}

AnimatedSprite2D::~AnimatedSprite2D()
{
    Dispose();
}

void AnimatedSprite2D::RegisterObject(Context* context)
{
    context->RegisterFactory<AnimatedSprite2D>();

    URHO3D_COPY_BASE_ATTRIBUTES(StaticSprite2D);
    URHO3D_ACCESSOR_ATTRIBUTE("Speed", GetSpeed, SetSpeed, float, 1.0f, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Custom Spritesheet", GetEmptyString, SetCustomSpriteSheetAttr, String, String::EMPTY, AM_DEFAULT);
    URHO3D_MIXED_ACCESSOR_ATTRIBUTE("Animation Set", GetAnimationSetAttr, SetAnimationSetAttr, ResourceRef, ResourceRef(AnimatedSprite2D::GetTypeStatic(), String::EMPTY), AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Entity", GetEntityName, SetEntity, String, String::EMPTY, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Animation", GetAnimation, SetAnimationAttr, String, String::EMPTY, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Applied Character Maps", GetAppliedCharacterMapsAttr, SetAppliedCharacterMapsAttr, VariantVector, Variant::emptyVariantVector, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Apply Character Map", GetEmptyString, SetCharacterMapAttr, String, String::EMPTY, AM_DEFAULT);
    URHO3D_ENUM_ACCESSOR_ATTRIBUTE("Loop Mode", GetLoopMode, SetLoopMode, LoopMode2D, loopModeNames, LM_DEFAULT, AM_DEFAULT);
}


/// ENTITY/ANIMATION SETTERS

void AnimatedSprite2D::SetAnimationSet(AnimationSet2D* animationSet)
{
    if (animationSet == animationSet_)
        return;

    Dispose(true);

    animationSet_ = animationSet;
    if (!animationSet_)
        return;

#ifdef URHO3D_SPINE
    if (animationSet_->GetSkeletonData())
    {
        spSkeletonData* skeletonData = animationSet->GetSkeletonData();

        // Create skeleton
        skeleton_ = spSkeleton_create(skeletonData);
        skeleton_->scaleX = flipX_ ? -1.f : 1.f;
        skeleton_->scaleY = flipY_ ? -1.f : 1.f;

        if (skeleton_->data->skinsCount > 0)
        {
            // If entity is empty use first skin in spine
            if (entityName_.Empty())
                entityName_ = skeleton_->data->skins[0]->name;
            spSkeleton_setSkinByName(skeleton_, entityName_.CString());
        }

        spSkeleton_updateWorldTransform(skeleton_);
    }
#endif
    if (animationSet_->GetSpriterData())
    {
        spriterInstance_ = new Spriter::SpriterInstance(this, animationSet_->GetSpriterData());
        const PODVector<Spriter::Entity* > entities = animationSet_->GetSpriterData()->entities_;

        if (!entities.Empty())
        {
            bool entityNameToSet = true;

            if (!entityName_.Empty())
            {
                for (unsigned i = 0; i < entities.Size(); i++)
                {
                    if (entities[i]->name_ == entityName_)
                    {
                        entityNameToSet = false;
                        break;
                    }
                }
            }

            if (entityNameToSet)
                entityName_ = animationSet_->GetSpriterData()->entities_[0]->name_;

            spriterInstance_->SetEntity(entityName_);
        }
    }

    if (!StaticSprite2D::GetSprite())
    {
        if (animationSet_->GetSprite())
            StaticSprite2D::SetSprite(animationSet_->GetSprite());
        else
            StaticSprite2D::SetSprite(GetSprite(0));
    }

    // Clear animation name
    animationName_.Clear();
}

void AnimatedSprite2D::SetEntity(const String& entity)
{
    if (entity == entityName_)
        return;

    drawRectDirty_ = true;

    entityName_ = entity;

#ifdef URHO3D_SPINE
    if (skeleton_)
        spSkeleton_setSkinByName(skeleton_, entityName_.CString());
#endif
    if (spriterInstance_)
        spriterInstance_->SetEntity(entityName_);
}

void AnimatedSprite2D::SetSpriterEntity(int index)
{
    if (!animationSet_ || !spriterInstance_)
        return;

    index %= GetNumSpriterEntities();

    if (!animationSet_->GetSpriterData()->entities_[index])
        return;

    const String& entityname = animationSet_->GetSpriterData()->entities_[index]->name_;

    if (entityname == entityName_)
        return;

    worldBoundingBoxDirty_ = drawRectDirty_ = true;

    entityName_ = entityname;

    spriterInstance_->SetEntity(index);
    SetAnimation(GetAnimation());
}

void AnimatedSprite2D::SetAnimation(const String& name)
{
    if (!animationSet_)
        return;

    if (!name.Empty())
    {
        if (animationSet_->HasAnimation(name))
            animationName_ = name;
    }

    if (animationName_.Empty() || !animationSet_->HasAnimation(animationName_))
        animationName_ = GetDefaultAnimation();

    if (animationName_.Empty())
    {
        URHO3D_LOGWARNINGF("AnimatedSprite2D() - SetAnimation : No Animation Name !");
        return;
    }

#ifdef URHO3D_SPINE
    if (skeleton_)
        SetSpineAnimation();
#endif
    if (spriterInstance_)
        SetSpriterAnimation();
}

void AnimatedSprite2D::SetLoopMode(LoopMode2D loopMode)
{
    if (loopMode_ != loopMode)
        loopMode_ = loopMode;

    if (spriterInstance_ && spriterInstance_->GetLooping() != (Spriter::LoopMode)loopMode)
        SetSpriterAnimation();

}

void AnimatedSprite2D::SetSpeed(float speed)
{
    speed_ = speed;
    MarkNetworkUpdate();
}

void AnimatedSprite2D::SetCurrentAnimationTime(float time)
{
    if (spriterInstance_ && spriterInstance_->GetAnimation())
        spriterInstance_->SetCurrentTime(time);
}

void AnimatedSprite2D::SetCustomSpriteSheetAttr(const String& value)
{
    AnimationSet2D::customSpritesheetFile_ = value;
}

void AnimatedSprite2D::SetAnimationSetAttr(const ResourceRef& value)
{
    ResourceCache* cache = GetSubsystem<ResourceCache>();
    SetAnimationSet(cache->GetResource<AnimationSet2D>(value.name_));
    AnimationSet2D::customSpritesheetFile_.Clear();
}

void AnimatedSprite2D::SetAnimationAttr(const String& name)
{
    animationName_ = name;
    SetAnimation(name);
}

void AnimatedSprite2D::CleanDependences()
{
    ClearTriggers(true);
}

void AnimatedSprite2D::ResetAnimation()
{
    if (spriterInstance_)
    {
        spriterInstance_->ResetCurrentTime();
        spriterInstance_->Update(0.f);
    }
}


/// ENTITY/ANIMATION GETTERS

unsigned AnimatedSprite2D::GetNumSpriterEntities() const
{
    return animationSet_ && spriterInstance_ ? animationSet_->GetSpriterData()->entities_.Size() : 0;
}

const String& AnimatedSprite2D::GetSpriterEntity(int index) const
{
    return spriterInstance_ && spriterInstance_->GetEntity(index) ? spriterInstance_->GetEntity(index)->name_ : String::EMPTY;
}

unsigned AnimatedSprite2D::GetSpriterEntityIndex() const
{
    return spriterInstance_ && spriterInstance_->GetEntity() ? spriterInstance_->GetEntity()->id_ : 0;
}

const String& AnimatedSprite2D::GetDefaultAnimation() const
{
	if (!spriterInstance_)
		return String::EMPTY;

	if (spriterInstance_->GetAnimation())
		return spriterInstance_->GetAnimation()->name_;

    if (animationSet_->GetNumAnimations())
        return animationSet_->GetAnimation(0);

	return String::EMPTY;
}

bool AnimatedSprite2D::HasAnimation(const String& name) const
{
    return spriterInstance_ && spriterInstance_->GetAnimation(name);
}

AnimationSet2D* AnimatedSprite2D::GetAnimationSet() const
{
    return animationSet_;
}

float AnimatedSprite2D::GetCurrentAnimationLength() const
{
    if (spriterInstance_ && spriterInstance_->GetAnimation())
        return spriterInstance_->GetAnimation()->length_ * speed_;

    return 0.f;
}

float AnimatedSprite2D::GetCurrentAnimationTime() const
{
    return spriterInstance_ ? spriterInstance_->GetCurrentTime() : 0.f;
}

bool AnimatedSprite2D::HasFinishedAnimation() const
{
    return spriterInstance_ ? spriterInstance_->HasFinishedAnimation() : true;
}

Spriter::SpriterInstance* AnimatedSprite2D::GetSpriterInstance() const
{
    return spriterInstance_.Get();
}

Spriter::Animation* AnimatedSprite2D::GetSpriterAnimation(int index) const
{
    if (!spriterInstance_)
        return nullptr;
    return index == -1 ? spriterInstance_->GetAnimation() : spriterInstance_->GetAnimation(index);
}

Spriter::Animation* AnimatedSprite2D::GetSpriterAnimation(const String& animationName) const
{
    return animationName != String::EMPTY && spriterInstance_ ? spriterInstance_->GetAnimation(animationName) : nullptr;
}

ResourceRef AnimatedSprite2D::GetAnimationSetAttr() const
{
    return GetResourceRef(animationSet_, AnimationSet2D::GetTypeStatic());
}


/// CHARACTER MAPPING SETTERS

void AnimatedSprite2D::SetAppliedCharacterMapsAttr(const VariantVector& characterMapApplied)
{
    ResetCharacterMapping(false);

    if (characterMapApplied.Empty())
        return;

    for (VariantVector::ConstIterator it=characterMapApplied.Begin(); it != characterMapApplied.End(); ++it)
    {
        Spriter::CharacterMap* characterMap = GetCharacterMap(it->GetStringHash());
        if (characterMap)
        ApplyCharacterMap(it->GetStringHash());
    }

    MarkNetworkUpdate();
}

void AnimatedSprite2D::SetCharacterMapAttr(const String& characterMapNames)
{
    characterMapApplied_.Clear();

    if (characterMapNames.Empty())
        return;

    bool state = false;
    Vector<String> names = characterMapNames.Split('|', false);

    for (Vector<String>::ConstIterator it=names.Begin(); it != names.End(); ++it)
        state |= ApplyCharacterMap(StringHash(*it));

    MarkNetworkUpdate();
}

bool AnimatedSprite2D::ApplyCharacterMap(const StringHash& characterMap)
{
    return ApplyCharacterMap(GetCharacterMap(characterMap));
}

bool AnimatedSprite2D::ApplyCharacterMap(const String& characterMap)
{
    return ApplyCharacterMap(StringHash(characterMap));
}

bool AnimatedSprite2D::ApplyCharacterMap(Spriter::CharacterMap* characterMap)
{
    if (!characterMap)
        return false;

    unsigned key;
    const PODVector<Spriter::MapInstruction*>& mapInstructions = characterMap->maps_;
    for (PODVector<Spriter::MapInstruction*>::ConstIterator it = mapInstructions.Begin(); it != mapInstructions.End(); ++it)
    {
        Spriter::MapInstruction* instruct = *it;
        key = (instruct->folder_ << 16) + instruct->file_;

        if (instruct->targetFolder_ == -1)
            spriteMapping_[key].Clear();
        else
            spriteMapping_[key].Set(key, animationSet_->GetSpriterFileSprite(instruct->targetFolder_, instruct->targetFile_), characterMap, instruct);
    }

    if (!IsCharacterMapApplied(characterMap->hashname_))
        characterMapApplied_.Push(characterMap->hashname_);

    sourceBatchesDirty_ = true;
    useCharacterMap_ = true;
    characterMaps_.Push(characterMap);

    return true;
}

void AnimatedSprite2D::SwapSprite(const StringHash& characterMap, Sprite2D* replacement, unsigned index, bool keepProportion)
{
    Sprite2D* original = GetCharacterMapSprite(characterMap, index);
    SwapSprite(original, replacement, keepProportion);
    sourceBatchesDirty_ = true;
}

void AnimatedSprite2D::SwapSprites(const StringHash& characterMap, const PODVector<Sprite2D*>& replacements, bool keepProportion)
{
    Spriter::CharacterMap* characterMapOrigin = GetCharacterMap(characterMap);
    if (!characterMapOrigin)
    {
        URHO3D_LOGWARNINGF("AnimatedSprite2D() - SwapSprites : no characterMap origin !");
        return;
    }

    PODVector<Sprite2D*> originalSprites;
    GetMappedSprites(characterMapOrigin, originalSprites);

    if (!originalSprites.Size() || !replacements.Size())
    {
        URHO3D_LOGWARNINGF("AnimatedSprite2D() - SwapSprites : no spriteslist !");
        return;
    }

    SwapSprites(originalSprites, replacements, keepProportion);

    ApplyCharacterMap(characterMap);
    sourceBatchesDirty_ = true;
}

void AnimatedSprite2D::SwapSprite(const String& characterMap, Sprite2D* replacement, unsigned index, bool keepProportion)
{
    SwapSprite(StringHash(characterMap), replacement, index, keepProportion);
}

void AnimatedSprite2D::SwapSprites(const String& characterMap, const PODVector<Sprite2D*>& replacements, bool keepProportion)
{
    SwapSprites(StringHash(characterMap), replacements, keepProportion);
}

void AnimatedSprite2D::UnSwapAllSprites()
{
    swappedSprites_.Clear();
    spriteInfoMapping_.Clear();
}

void AnimatedSprite2D::SwapSprite(Sprite2D* original, Sprite2D* replacement, bool keepRatio)
{
    if (!original)
    {
        URHO3D_LOGWARNINGF("AnimatedSprite2D() - SwapSprite : node=%s(%u) original=NONE replacement=%s => verify original in CharacterMap in SCML !",
                            node_->GetName().CString(), node_->GetID(), replacement ? replacement->GetName().CString() : "NONE");
        return;
    }

    swappedSprites_[original] = SharedPtr<Sprite2D>(replacement);

    if (original == replacement)
        return;

    if (replacement)
    {
        const IntRect& orect = original->GetRectangle();
        const IntRect& rrect = replacement->GetRectangle();

        SpriteInfo& info = spriteInfoMapping_[replacement][original];
        info.sprite_ = replacement;
        info.dPivot_.x_ = replacement->GetHotSpot().x_ - original->GetHotSpot().x_;
        info.dPivot_.y_ = replacement->GetHotSpot().y_ - original->GetHotSpot().y_;

        if (!keepRatio)
        {
            info.scale_.x_ = (float)(orect.right_ - orect.left_) / (float)(rrect.right_ - rrect.left_);
            info.scale_.y_ = (float)(orect.bottom_ - orect.top_) / (float)(rrect.bottom_ - rrect.top_);
        }
        else
            info.scale_.x_ = info.scale_.y_ = 1.f;
    }
}

void AnimatedSprite2D::SwapSprites(const PODVector<Sprite2D*>& originals, const PODVector<Sprite2D*>& replacements, bool keepRatio)
{
    int size = originals.Size();
    if (!size)
        return;

    for (int i = 0; i < size; ++i)
        SwapSprite(originals[i], i >= replacements.Size() ? 0 : replacements[i], keepRatio);
}

void AnimatedSprite2D::UnSwapSprite(Sprite2D* original)
{
    if (!original)
        return;

    swappedSprites_.Erase(original);
}

void AnimatedSprite2D::ResetCharacterMapping(bool resetSwappedSprites)
{
    characterMaps_.Clear();
    characterMapApplied_.Clear();

    spriteMapping_.Clear();
    spritesInfos_.Clear();

    if (resetSwappedSprites)
        UnSwapAllSprites();

    characterMapDirty_ = false;
    useCharacterMap_ = false;
    sourceBatchesDirty_ = true;
}


/// CHARACTER MAPPING GETTERS

const VariantVector& AnimatedSprite2D::GetAppliedCharacterMapsAttr() const
{
    return characterMapApplied_;
}

bool AnimatedSprite2D::HasCharacterMapping() const
{
    return spriterInstance_ && spriterInstance_->GetEntity() ? spriterInstance_->GetEntity()->characterMaps_.Size() != 0 : false;
}

bool AnimatedSprite2D::HasCharacterMap(const StringHash& hashname) const
{
    return GetCharacterMap(hashname) != nullptr;
}

bool AnimatedSprite2D::HasCharacterMap(const String& name) const
{
    return GetCharacterMap(name) != nullptr;
}

Spriter::CharacterMap* AnimatedSprite2D::GetCharacterMap(const StringHash& characterMap) const
{
    if (!spriterInstance_)
        return nullptr;

    Spriter::Entity* entity = spriterInstance_->GetEntity();
    if (!entity)
        return nullptr;

    const PODVector<Spriter::CharacterMap*>& characterMaps = entity->characterMaps_;
    for (PODVector<Spriter::CharacterMap*>::ConstIterator it=characterMaps.Begin(); it != characterMaps.End(); ++it)
    {
        if ((*it)->hashname_ == characterMap)
            return *it;
    }
    return nullptr;
}

Spriter::CharacterMap* AnimatedSprite2D::GetCharacterMap(const String& characterMap) const
{
    return GetCharacterMap(StringHash(characterMap));
}

bool AnimatedSprite2D::IsCharacterMapApplied(const StringHash& characterMap) const
{
    return characterMapApplied_.Contains(characterMap);
}

bool AnimatedSprite2D::IsCharacterMapApplied(const String& characterMap) const
{
    return IsCharacterMapApplied(StringHash(characterMap));
}

Sprite2D* AnimatedSprite2D::GetSprite(unsigned zorder) const
{
    if (zorder >= GetNumSpriteKeys())
        return nullptr;   

    if (zorder < spritesInfos_.Size())
        return spritesInfos_[zorder]->sprite_;

    const Spriter::SpriteTimelineKey& spriteKey = *spriterInstance_->GetSpriteKeys()[zorder];
    return GetMappedSprite((spriteKey.folderId_ << 16) + spriteKey.fileId_);
}

unsigned AnimatedSprite2D::GetNumSpriteKeys() const
{
    return spritesInfos_.Size() ? spritesKeys_.Size() : spriterInstance_->GetNumSpriteKeys();
}

const PODVector<Spriter::SpriteTimelineKey* >& AnimatedSprite2D::GetSpriteKeys() const
{
    return spritesInfos_.Size() ? spritesKeys_ : spriterInstance_->GetSpriteKeys();
}

const SpriteMapInfo* AnimatedSprite2D::GetSpriteMapInfo(unsigned key) const
{
    HashMap<unsigned, SpriteMapInfo >::ConstIterator it = spriteMapping_.Find(key);
    return it != spriteMapping_.End() ? &it->second_ : nullptr;
}

SpriteInfo* AnimatedSprite2D::GetSpriteInfo(unsigned key, const SpriteMapInfo* mapinfo, Sprite2D* sprite, Sprite2D* origin)
{
    SpriteInfo& info = spriteInfoMapping_[sprite][origin];
    if (info.sprite_ != sprite)
        info.Set(sprite);
    if (info.mapinfo_ != mapinfo)
        info.mapinfo_ = mapinfo;

    return &info;
}

const PODVector<SpriteInfo*>& AnimatedSprite2D::GetSpriteInfos()
{
    spritesKeys_.Clear();
    spritesInfos_.Clear();

    if (!spriterInstance_->GetSpriteKeys().Size())
        UpdateSpriterAnimation(0.f);

    unsigned numSpriteKeys = spriterInstance_->GetNumSpriteKeys();
    if (numSpriteKeys)
    {
        const PODVector<Spriter::SpriteTimelineKey* >& spriteKeys = spriterInstance_->GetSpriteKeys();
        Sprite2D *sprite, *origin;
        Spriter::SpriteTimelineKey* spriteKey;
        unsigned key;

        // Get Sprite Keys only
        for (unsigned i = 0; i < numSpriteKeys; ++i)
        {
            spriteKey = spriteKeys[i];
            key = (spriteKey->folderId_ << 16) + spriteKey->fileId_;
            const SpriteMapInfo* mapinfo = GetSpriteMapInfo(key);
            origin = mapinfo ? mapinfo->sprite_ : animationSet_->GetSpriterFileSprite(key);
            if (!origin)
                continue;

            sprite = GetSwappedSprite(origin);
            if (!sprite)
                continue;

            spritesKeys_.Push(spriteKey);
            spritesInfos_.Push(GetSpriteInfo(key, mapinfo, sprite, origin));
        }
    }

    return spritesInfos_;
}

/// Sprites Getters

Sprite2D* AnimatedSprite2D::GetCharacterMapSprite(const StringHash& characterMap, unsigned index) const
{
    return animationSet_->GetCharacterMapSprite(GetCharacterMap(characterMap), index);
}

Sprite2D* AnimatedSprite2D::GetCharacterMapSprite(const String& characterMap, unsigned index) const
{
    return GetCharacterMapSprite(StringHash(characterMap), index);
}

void AnimatedSprite2D::GetMappedSprites(Spriter::CharacterMap* characterMap, PODVector<Sprite2D*>& sprites) const
{
    if (!characterMap)
        return;

    sprites.Clear();

    Spriter::MapInstruction* map;
    const PODVector<Spriter::MapInstruction*>& mapInstructions = characterMap->maps_;
    for (PODVector<Spriter::MapInstruction*>::ConstIterator it = mapInstructions.Begin(); it != mapInstructions.End(); ++it)
    {
        map = *it;
        sprites.Push(map->targetFolder_ == -1 ? 0 : animationSet_->GetSpriterFileSprite(map->targetFolder_, map->targetFile_));
    }
}

Sprite2D* AnimatedSprite2D::GetMappedSprite(unsigned key) const
{
    HashMap<unsigned, SpriteMapInfo >::ConstIterator it = spriteMapping_.Find(key);
    return it != spriteMapping_.End() ? it->second_.sprite_.Get() : animationSet_->GetSpriterFileSprite(key);
}

Sprite2D* AnimatedSprite2D::GetMappedSprite(int folderid, int fileid) const
{
    return GetMappedSprite((folderid << 16) + fileid);
}

Sprite2D* AnimatedSprite2D::GetSwappedSprite(Sprite2D* original) const
{
    if (!original)
        return nullptr;   

    HashMap<Sprite2D*, SharedPtr<Sprite2D> >::ConstIterator it = swappedSprites_.Find(original);
    return it != swappedSprites_.End() ? it->second_.Get() : original;
}


/// HELPERS

void AnimatedSprite2D::DumpSpritesInfos() const
{
    URHO3D_LOGINFOF("AnimatedSprite2D() - DumpSpritesInfos : node=%s(%u), numSprites=%u",
                    node_->GetName().CString(), node_->GetID(), spritesInfos_.Size());

    String name;

    for (unsigned i=0;i<spritesInfos_.Size();i++)
    {
        name = spritesInfos_[i]->sprite_ ? spritesInfos_[i]->sprite_->GetName() : String::EMPTY;
        URHO3D_LOGINFOF("sprite %u/%u = %s", i+1, spritesInfos_.Size(), name.CString());
    }
}


/// HANDLERS

void AnimatedSprite2D::OnSetEnabled()
{
    Drawable2D::OnSetEnabled();

    bool enabled = IsEnabledEffective();

    Scene* scene = GetScene();
    if (scene)
    {
        if (spriterInstance_)
            spriterInstance_->ResetCurrentTime();

        if (enabled)
        {
            UpdateAnimation(0.f);
            SubscribeToEvent(scene, E_SCENEPOSTUPDATE, URHO3D_HANDLER(AnimatedSprite2D, HandleScenePostUpdate));
        }
        else
        {
            UnsubscribeFromEvent(scene, E_SCENEPOSTUPDATE);
            HideTriggers();
        }
    }
}

void AnimatedSprite2D::OnDrawOrderChanged()
{
}

void AnimatedSprite2D::OnSceneSet(Scene* scene)
{
    StaticSprite2D::OnSceneSet(scene);

    if (scene)
    {
        if (scene == node_)
            URHO3D_LOGWARNING(GetTypeName() + " should not be created to the root scene node");

        if (IsEnabledEffective())
            SubscribeToEvent(scene, E_SCENEPOSTUPDATE, URHO3D_HANDLER(AnimatedSprite2D, HandleScenePostUpdate));
    }
    else
    {
        UnsubscribeFromEvent(E_SCENEPOSTUPDATE);
    }
}

void AnimatedSprite2D::HandleScenePostUpdate(StringHash eventType, VariantMap& eventData)
{
    if (speed_)
        UpdateAnimation(eventData[ScenePostUpdate::P_TIMESTEP].GetFloat());
    else
        URHO3D_LOGWARNINGF("AnimatedSprite2D() - HandleScenePostUpdate : node=%s(%u) no speed !", node_->GetName().CString(), node_->GetID());
}


/// UPDATERS

void AnimatedSprite2D::UpdateAnimation(float timeStep)
{
    if (!timeStep)
    {
        drawRectDirty_ = true;
        UpdateDrawRectangle();
    }

    URHO3D_PROFILE(AnimatedSprite2D_Update);

#ifdef URHO3D_SPINE
    if (skeleton_ && animationState_)
        UpdateSpineAnimation(timeStep);
#endif
    if (spriterInstance_ && spriterInstance_->GetAnimation())
        UpdateSpriterAnimation(timeStep);

}

#ifdef URHO3D_SPINE
void AnimatedSprite2D::SetSpineAnimation()
{
    if (!animationStateData_)
    {
        animationStateData_ = spAnimationStateData_create(animationSet_->GetSkeletonData());
        if (!animationStateData_)
        {
            URHO3D_LOGERROR("Create animation state data failed");
            return;
        }
    }

    if (!animationState_)
    {
        animationState_ = spAnimationState_create(animationStateData_);
        if (!animationState_)
        {
            URHO3D_LOGERROR("Create animation state failed");
            return;
        }
    }

    // Reset slots to setup pose, fix issue #932
    spSkeleton_setSlotsToSetupPose(skeleton_);
    spAnimationState_setAnimationByName(animationState_, 0, animationName_.CString(), loopMode_ != LM_FORCE_CLAMPED ? true : false);

    UpdateAnimation(0.0f);
    MarkNetworkUpdate();
}

void AnimatedSprite2D::UpdateSpineAnimation(float timeStep)
{
    URHO3D_PROFILE(AnimatedSprite2D_UpdateSpine);

    timeStep *= speed_;

    skeleton_->scaleX = flipX_ ? -1.f : 1.f;
    skeleton_->scaleY = flipY_ ? -1.f : 1.f;

    spAnimationState_update(animationState_, timeStep);
    spAnimationState_apply(animationState_, skeleton_);
    spSkeleton_updateWorldTransform(skeleton_);

    sourceBatchesDirty_ = true;
    worldBoundingBoxDirty_ = true;
}

void AnimatedSprite2D::UpdateSourceBatchesSpine()
{
    Matrix2x3 transform(node_->GetWorldPosition2D(), node_->GetWorldRotation2D(), node_->GetWorldScale2D());

    SourceBatch2D& sourceBatch = sourceBatches_[0];
    sourceBatch.vertices_.Clear();

    const int SLOT_VERTEX_COUNT_MAX = 1024;
    float slotVertices[SLOT_VERTEX_COUNT_MAX];

    for (int i = 0; i < skeleton_->slotsCount; ++i)
    {
        spSlot* slot = skeleton_->drawOrder[i];
        spAttachment* attachment = slot->attachment;
        if (!attachment)
            continue;

        unsigned color = Color(color_.r_ * slot->color.r,
            color_.g_ * slot->color.g,
            color_.b_ * slot->color.b,
            color_.a_ * slot->color.a).ToUInt();

        if (attachment->type == SP_ATTACHMENT_REGION)
        {
            spRegionAttachment* region = (spRegionAttachment*)attachment;
            spRegionAttachment_computeWorldVertices(region, slot, slotVertices, 0 , 2);

            Vertex2DFX vertices[4];
            vertices[0].position_ = transform * Vector2(slotVertices[0], slotVertices[1]);
            vertices[1].position_ = transform * Vector2(slotVertices[2], slotVertices[3]);
            vertices[2].position_ = transform * Vector2(slotVertices[4], slotVertices[5]);
            vertices[3].position_ = transform * Vector2(slotVertices[6], slotVertices[7]);

            vertices[0].color_ = color;
            vertices[1].color_ = color;
            vertices[2].color_ = color;
            vertices[3].color_ = color;

            vertices[0].uv_ = Vector2(region->uvs[0], region->uvs[1]);
            vertices[1].uv_ = Vector2(region->uvs[2], region->uvs[3]);
            vertices[2].uv_ = Vector2(region->uvs[4], region->uvs[5]);
            vertices[3].uv_ = Vector2(region->uvs[6], region->uvs[7]);

            sourceBatch.vertices_.Push(vertices[0]);
            sourceBatch.vertices_.Push(vertices[1]);
            sourceBatch.vertices_.Push(vertices[2]);
            sourceBatch.vertices_.Push(vertices[3]);
        }
        else if (attachment->type == SP_ATTACHMENT_MESH)
        {
            spMeshAttachment* mesh = (spMeshAttachment*)attachment;
            if (mesh->super.worldVerticesLength > SLOT_VERTEX_COUNT_MAX)
                continue;

            spVertexAttachment_computeWorldVertices(&mesh->super, slot, 0, mesh->super.worldVerticesLength, slotVertices, 0, 2);

            Vertex2D vertex;
            vertex.color_ = color;
            for (int j = 0; j < mesh->trianglesCount; ++j)
            {
                int index = mesh->triangles[j] << 1;
                vertex.position_ = transform * Vector2(slotVertices[index], slotVertices[index + 1]);
                vertex.uv_ = Vector2(mesh->uvs[index], mesh->uvs[index + 1]);

                sourceBatch.vertices_.Push(vertex);
                // Add padding vertex
                if (j % 3 == 2)
                    sourceBatch.vertices_.Push(vertex);
            }
        }
/*
        else if (attachment->type == SP_ATTACHMENT_SKINNED_MESH)
        {
            spSkinnedMeshAttachment* skinnedMesh = (spSkinnedMeshAttachment*)attachment;
            if (skinnedMesh->uvsCount > SLOT_VERTEX_COUNT_MAX)
                continue;

            spSkinnedMeshAttachment_computeWorldVertices(skinnedMesh, slot, slotVertices);

            Vertex2D vertex;
            vertex.color_ = color;
            for (int j = 0; j < skinnedMesh->trianglesCount; ++j)
            {
                int index = skinnedMesh->triangles[j] << 1;
                vertex.position_ = worldTransform * Vector3(slotVertices[index], slotVertices[index + 1]);
                vertex.uv_ = Vector2(skinnedMesh->uvs[index], skinnedMesh->uvs[index + 1]);

                sourceBatches_[0].vertices_.Push(vertex);
                // Add padding vertex
                if (j % 3 == 2)
                    sourceBatches_[0].vertices_.Push(vertex);
             }
        }
*/
//		else if (attachment->type == SP_ATTACHMENT_CLIPPING)
//		{
//			spClippingAttachment *clip = (spClippingAttachment *) slot->attachment;
//			spSkeletonClipping_clipStart(clipper, slot, clip);
//			continue;
//		}
		else
            continue;
    }
}
#endif

void AnimatedSprite2D::SetSpriterAnimation(int index)
{
    if (!spriterInstance_)
        return;

    if (index == -1)
    {
        if (!spriterInstance_->SetAnimation(animationName_, (Spriter::LoopMode)loopMode_))
            return;
    }
    else
    {
        if (!spriterInstance_->SetAnimation(index, (Spriter::LoopMode)loopMode_))
            return;

        animationIndex_ = index;
        animationName_ = spriterInstance_->GetAnimation()->name_;
    }

    if (IsEnabledEffective())
        HideTriggers();

    worldBoundingBoxDirty_ = drawRectDirty_ = true;

    MarkNetworkUpdate();
}

void AnimatedSprite2D::HideTriggers()
{
    activedEventTriggers_.Clear();

    if (!triggerNodes_.Size())
        return;

    // Inactive Trigger Nodes
    for (Vector<WeakPtr<Node> >::ConstIterator it = triggerNodes_.Begin();it!=triggerNodes_.End();++it)
    {
        if (*it)
            (*it)->SetEnabled(false);
    }
}

void AnimatedSprite2D::ClearTriggers(bool removeNode)
{
    if (removeNode)
    {
        for (Vector<WeakPtr<Node> >::ConstIterator it = triggerNodes_.Begin();it!=triggerNodes_.End();++it)
        {
            if (*it)
                (*it)->Remove();
        }
        triggerNodes_.Clear();
    }

    activedEventTriggers_.Clear();
}

inline void AnimatedSprite2D::LocalToWorld(Spriter::SpatialTimelineKey* key, Vector2& position, float& rotation)
{
    const Spriter::SpatialInfo& spatialinfo = key->info_;

    float centerx = spatialinfo.x_ * PIXEL_SIZE;
    float centery = spatialinfo.y_ * PIXEL_SIZE;
    rotation = spatialinfo.angle_;

    if (flipX_)
    {
        centerx = -centerx;
        rotation = 180.f - rotation;
    }
    else
        rotation = spatialinfo.angle_;

    if (flipY_)
    {
        centery = -centery;
        rotation = 360.f - rotation;
    }

    Matrix2x3 worldtransform(GetNode()->GetWorldPosition2D(), GetNode()->GetWorldRotation2D(), GetNode()->GetWorldScale2D());
    position = (worldtransform * Matrix2x3(Vector2(centerx, centery), 0.f, Vector2(spatialinfo.scaleX_, spatialinfo.scaleY_))).Translation();
}

void AnimatedSprite2D::UpdateTriggers()
{
    if (!IsEnabledEffective() || !spriterInstance_)
        return;

    // Update Event Triggers
    const HashMap<Spriter::Timeline*, Spriter::PointTimelineKey* >& eventTriggers = spriterInstance_->GetEventTriggers();
    if (eventTriggers.Size())
    {
        StringHash triggerEventName;
        StringHash triggerEvent;
        Vector<String> args;
        Spriter::Timeline* timeline;
        Spriter::PointTimelineKey* key;
        Node* triggerNode;

        for (HashMap<Spriter::Timeline*, Spriter::PointTimelineKey* >::ConstIterator it=eventTriggers.Begin(); it!=eventTriggers.End() ; ++it)
        {
            timeline = it->first_;
            if (!timeline)
                continue;

            args = timeline->name_.Split('_');
            triggerEventName = StringHash(timeline->name_);
            triggerEvent = StringHash("SPRITER_" + (args.Size() ? args[0] : timeline->name_));

            if (!activedEventTriggers_.Contains(triggerEventName))
            {
                activedEventTriggers_.Push(triggerEventName);

                if (triggerEvent == SPRITER_SOUND)
                {
                    VariantMap& paramEvent = context_->GetEventDataMap();
                    paramEvent[SPRITER_Event::TYPE] = StringHash(args[1]);
                    node_->SendEvent(triggerEvent, paramEvent);
                }
                else if (triggerEvent == SPRITER_ANIMATIONINSIDE)
                {
                    key = it->second_;

                    triggerNode = node_->GetChild(timeline->name_);

                    if (!triggerNode)
                    {
                        triggerNode = node_->CreateChild(timeline->name_, LOCAL);
                        triggerNode->SetTemporary(true);
                        triggerNodes_.Push(WeakPtr<Node>(triggerNode));
                    }

                    Vector<String> params = args[1].Split('|');

                    triggerInfo_.type_ = params.Size() > 1 ? StringHash(params[1]) : StringHash::ZERO;
                    triggerInfo_.type2_ = StringHash(params.Size() > 0 ? params[0] : args[1]);
                    triggerInfo_.node_ = triggerNode;
                    node_->SendEvent(triggerEvent);
                }
                // triggerEvent == SPRITER_ANIMATION or SPRITER_ENTITY or simple SPRITER EVENT (like SPRITER_Explode)
                else
                {
                    /// FOR SPACEMATCH
                    if (args.Size() > 1)
                    {
                        key = it->second_;
                        triggerInfo_.zindex_ = key->zIndex_;
                        LocalToWorld(key, triggerInfo_.position_, triggerInfo_.rotation_);
                        Vector<String> params = args[1].Split(',');

                        if (params.Size() == 1)
                        {
                            triggerInfo_.type_ = StringHash::ZERO;
                            triggerInfo_.datas_ = args[1];
                        }
                        else
                        {
                            triggerInfo_.type_ = params.Size() > 0 ? (params[0].Empty() ? StringHash::ZERO : StringHash(params[0])) : StringHash(args[1]);
                            triggerInfo_.datas_ = params.Size() > 1 ? params[1] : String::EMPTY;
                        }
                    }

					node_->SendEvent(triggerEvent);
                }
            }
        }
    }
    else
    {
        activedEventTriggers_.Clear();
    }

    // Update Tagged Nodes
    HashMap<String, Spriter::NodeUpdater >& nodeUpdaters = spriterInstance_->GetNodeUpdaters();
    if (nodeUpdaters.Size())
    {
        float centerx, centery, angle;

        for (HashMap<String, Spriter::NodeUpdater >::Iterator it = nodeUpdaters.Begin(); it != nodeUpdaters.End() ; ++it)
        {
            Node* node;
            AnimatedSprite2D* animation = nullptr;

            Spriter::NodeUpdater& updater = it->second_;
            // Mount Node
            if (it->first_.StartsWith("MT"))
            {
                if (!updater.ucomponent_)
                {
                    node = node_->GetChild(it->first_);
                    if (!node)
                    {
                        node = node_->CreateChild(it->first_, LOCAL);
                        node->SetTemporary(true);
                    }
                    updater.ucomponent_ = node;
                }
                else
                    node = static_cast<Node*>(updater.ucomponent_);
            }
            // Animation
            else
            {
                if (!updater.ucomponent_)
                    continue;

                 animation = static_cast<AnimatedSprite2D*>(updater.ucomponent_);
                 node = animation->GetNode();
            }

            const Spriter::SpatialInfo& info = updater.timekey_->info_;
            centerx = info.x_;
            centery = info.y_;

            if (flipX_)
                centerx = -centerx;

            if (flipY_)
                centery = -centery;

            node->SetPosition2D(centerx * PIXEL_SIZE, centery * PIXEL_SIZE);

            angle = info.angle_;

            // y orientation on x
            if (flipX_ != flipY_)
                angle = -angle;

            node->SetRotation2D(angle);

            if (animation)
                animation->SetFlip(flipX_, flipY_);
        }
    }

    updatedPhysicNodes_.Clear();

    // Update Physic Triggers
    const HashMap<Spriter::Timeline*, Spriter::BoxTimelineKey* >& physicTriggers = spriterInstance_->GetPhysicTriggers();
    if (physicTriggers.Size())
    {
        Vector2 center, size, pivot;
        float angle;
        Node* physicNode;
        CollisionCircle2D* collisionCircle;
        CollisionBox2D* collisionBox;
        Spriter::Timeline* timeline;
        Spriter::BoxTimelineKey* key;

        for (HashMap<Spriter::Timeline*, Spriter::BoxTimelineKey* >::ConstIterator it=physicTriggers.Begin();it!=physicTriggers.End();++it)
        {
            timeline = it->first_;
            key = it->second_;
            const Spriter::SpatialInfo& info = key->info_;

            char collidertype = timeline->name_.Front();
            bool isAbox = collidertype == 'B';

            physicNode = node_->GetChild(timeline->name_);

            /*
                Timeline name begin by
                'T' it's a Trigger
                'C' it's a Circle
                'B' it's a Box
            */

            if (!physicNode)
            {
                physicNode = node_->CreateChild(timeline->name_, LOCAL);
                physicNode->SetTemporary(true);

                triggerNodes_.Push(WeakPtr<Node>(physicNode));

                if (isAbox)
                {
                    collisionBox = physicNode->CreateComponent<CollisionBox2D>(LOCAL);
                    collisionBox->SetTrigger(false);
                }
                else
                {
                    collisionCircle = physicNode->CreateComponent<CollisionCircle2D>(LOCAL);
                    collisionCircle->SetTrigger(collidertype == 'T');
                }
            }
            else
            {
                physicNode->SetEnabled(true);
                if (isAbox)
                    collisionBox = physicNode->GetComponent<CollisionBox2D>(LOCAL);
                else
                    collisionCircle = physicNode->GetComponent<CollisionCircle2D>(LOCAL);
            }

            if (isAbox)
            {
                angle = info.angle_;
                if (flipX_)
                    angle = 180.f-angle;
                center.x_ = info.x_ * PIXEL_SIZE + (0.5f - key->pivotX_) * key->width_ * info.scaleX_ * PIXEL_SIZE;
                center.y_ = info.y_ * PIXEL_SIZE + (0.5f - key->pivotY_) * key->height_ * info.scaleY_ * PIXEL_SIZE;
                size.x_ = key->width_ * info.scaleX_ * PIXEL_SIZE;
                size.y_ = key->height_ * info.scaleY_ * PIXEL_SIZE;
                pivot.x_ = info.x_ * PIXEL_SIZE,
                pivot.y_ = info.y_ * PIXEL_SIZE;
                collisionBox->SetBox(center, size, pivot, angle);
            }
            else
            {
                // For Circle : don't handle pivots with Spriter::BoxTimelineKey => you must use default spriter pivot(0.f,0.f)
                // the spriter box
                // 1-----
                // |    |
                // |----2
                // Important : Point1 = in Spriter it's the first point clicked when we create a box

                center.x_ = info.x_ * PIXEL_SIZE + key->width_ * PIXEL_SIZE * 0.5f;
                center.y_ = info.y_ * PIXEL_SIZE - key->height_ * PIXEL_SIZE * 0.5f;

                if (flipX_)
                    center.x_ = -center.x_;
                if (flipY_)
                    center.y_ = -center.y_;

                collisionCircle->SetCenter(center);
                collisionCircle->SetRadius(Max(key->width_, key->height_) * Max(info.scaleX_, info.scaleY_) * 0.5f * PIXEL_SIZE);
            }

            updatedPhysicNodes_.Push(physicNode);
        }
    }

    for (Vector<WeakPtr<Node> >::ConstIterator it = triggerNodes_.Begin(); it != triggerNodes_.End(); ++it)
    {
        if (*it && !updatedPhysicNodes_.Contains(*it))
            (*it)->SetEnabled(false);
    }
}

void AnimatedSprite2D::UpdateSpriterAnimation(float timeStep)
{
    if (spriterInstance_ && spriterInstance_->Update(timeStep * speed_))
    {
        UpdateTriggers();

        sourceBatchesDirty_ = true;
    }
}

bool AnimatedSprite2D::UpdateDrawRectangle()
{
#ifdef URHO3D_SPINE
    if (skeleton_)
        return true;
#endif
    if (!spriterInstance_)
        return false;

    if (!drawRectDirty_)
        return true;

    const PODVector<Spriter::SpriteTimelineKey* >& spriteKeys = spriterInstance_->GetSpriteKeys();
    if (!spriteKeys.Size())
        ResetAnimation();

    drawRect_.Clear();

    Rect drawRect;
    Vector2 position;
    Vector2 scale;
    Vector2 pivot;
    float angle;
    Sprite2D* sprite;
    Spriter::SpriteTimelineKey* spriteKey;

    const unsigned numSpriteKeys = Min(spriterInstance_->GetNumSpriteKeys(), spriteKeys.Size());
    for (unsigned i = 0; i < numSpriteKeys; ++i)
    {
        spriteKey = spriteKeys[i];
        sprite = animationSet_->GetSpriterFileSprite((spriteKey->folderId_ << 16) + spriteKey->fileId_);

        if (!sprite)
            continue;

        const Spriter::SpatialInfo& spatialinfo = spriteKey->info_;

        if (!flipX_)
        {
            position.x_ = spatialinfo.x_;
            pivot.x_ = spriteKey->pivotX_;
        }
        else
        {
            position.x_ = -spatialinfo.x_;
            pivot.x_ = 1.0f - spriteKey->pivotX_;
        }

        if (!flipY_)
        {
            position.y_ = spatialinfo.y_;
            pivot.y_ = spriteKey->pivotY_;
        }
        else
        {
            position.y_ = -spatialinfo.y_;
            pivot.y_ = 1.0f - spriteKey->pivotY_;
        }

        angle = spatialinfo.angle_;
        if (flipX_ != flipY_)
            angle = -angle;

        scale.x_ = spatialinfo.scaleX_;
        scale.y_ = spatialinfo.scaleY_;

        sprite->GetDrawRectangle(drawRect, pivot);
        drawRect_.Merge(Transform2D(drawRect, Matrix2x3(position * PIXEL_SIZE, angle, scale)));
    }

    drawRectDirty_ = false;
    worldBoundingBoxDirty_ = true;
    return true;
}

enum
{
    RESETFIRSTKEY = -1,
    KEEPFIRSTKEY = -2,
};

void AnimatedSprite2D::SetCustomSourceBatches(Vector<SourceBatch2D>* sourceBatches)
{
    customSourceBatches_ = sourceBatches;
    firstKeyIndex_ = 0;
}

void AnimatedSprite2D::UpdateSourceBatches()
{
    if (!sourceBatchesDirty_)
        return;

    URHO3D_PROFILE(AnimatedSprite2D_Batch);

#ifdef URHO3D_SPINE
    if (skeleton_ && animationState_)
        UpdateSourceBatchesSpine();
#endif

    if (spriterInstance_ && spriterInstance_->GetAnimation())
    {
        if (!UpdateDrawRectangle())
        {
            URHO3D_LOGWARNINGF("AnimatedSprite2D::UpdateSourceBatches : %s(%u) !UpdateDrawRectangle()", node_ ? node_->GetName().CString() : "", node_ ? node_->GetID() : 0);
            return;
        }

        Vector<SourceBatch2D>* sourcebatches = customSourceBatches_ ? customSourceBatches_ : &sourceBatches_;
        if (customSourceBatches_)
            UpdateSourceBatchesSpriter_Custom(sourcebatches, RESETFIRSTKEY, false);
        else if (useCharacterMap_ || animationSet_->IsMultiTextures())
            UpdateSourceBatchesSpriter_Custom(sourcebatches);
        else
            UpdateSourceBatchesSpriter(sourcebatches);
    }
	sourceBatchesDirty_ = false;
}

void AnimatedSprite2D::UpdateSourceBatchesSpriter(Vector<SourceBatch2D>* sourceBatchesPtr, bool resetBatches)
{
    if (!spriterInstance_->GetSpriteKeys().Size())
        UpdateSpriterAnimation(0.f);

    unsigned numSpriteKeys = spriterInstance_->GetNumSpriteKeys();
	const PODVector<Spriter::SpriteTimelineKey* >& spriteKeys = spriterInstance_->GetSpriteKeys();
    if (!numSpriteKeys)
        return;

    Sprite2D* sprite;
    Spriter::SpriteTimelineKey* spriteKey;
    unsigned fileKey;

    Vector<SourceBatch2D>& sourceBatches = *sourceBatchesPtr;
    if (!sourceBatches[0].material_)
    {
        spriteKey = spriteKeys[0];
        sprite = animationSet_->GetSpriterFileSprite((spriteKey->folderId_ << 16) + spriteKey->fileId_);
        sourceBatches[0].material_ = customMaterial_ ? customMaterial_ : renderer_->GetMaterial(sprite->GetTexture(), blendMode_);
    }

    Material* material = sourceBatches.Front().material_;

    int iBatch = resetBatches ? 0 : sourceBatches.Size();
    sourceBatches.Resize(iBatch+1);
    sourceBatches[iBatch].vertices_.Clear();
    sourceBatches[iBatch].drawOrder_ = iBatch > 0 ? sourceBatches[iBatch-1].drawOrder_ + 1 : GetDrawOrder();
    if (iBatch > 0)
        sourceBatches[iBatch].material_ = material;

    // Start Loop
    Matrix2x3 nodeWorldTransform(GetNode()->GetWorldPosition2D(), GetNode()->GetWorldRotation2D(), GetNode()->GetWorldScale2D());
    Matrix2x3 spriteWorldTransform;

    Rect drawRect;
    Rect textureRect;
    Color color = MultColors(color_, spriterInstance_->GetEntity()->color_);

    Vertex2D vertex0;
    Vertex2D vertex1;
    Vertex2D vertex2;
    Vertex2D vertex3;

    vertex0.position_.z_ = vertex1.position_.z_ = vertex2.position_.z_ = vertex3.position_.z_ = node_->GetWorldPosition().z_;

    Vector2 position;
    Vector2 scale;
    Vector2 pivot;

    Vector4 texmode;
    SetTextureMode(TXM_FX, textureFX_, texmode);

    float angle;

    Vector<Vertex2D>& vertices = sourceBatches[iBatch].vertices_;

    Texture *texture = nullptr, *ttexture = nullptr;

    for (unsigned i = 0; i < numSpriteKeys; ++i)
    {
        spriteKey = spriteKeys[i];
        fileKey = (spriteKey->folderId_ << 16) + spriteKey->fileId_;

        sprite = animationSet_->GetSpriterFileSprite(fileKey);
        if (!sprite)
            continue;

        if (!sprite->GetTextureRectangle(textureRect, flipX_, flipY_))
        {
            URHO3D_LOGWARNINGF("AnimatedSprite2D() - UpdateSourceBatchesSpriter : node=%s ... No GetTextureRect !", node_->GetName().CString());
            return;
        }

        // lit or unlit fx
		SetTextureMode(TXM_FX_LIT, spriteKey->fx_ > 0 ? 1U : textureFX_, texmode);

        const Spriter::SpatialInfo& spatialinfo = spriteKey->info_;

        if (!flipX_)
        {
            position.x_ = spatialinfo.x_;
            pivot.x_ = spriteKey->pivotX_;
        }
        else
        {
            position.x_ = -spatialinfo.x_;
            pivot.x_ = 1.0f - spriteKey->pivotX_;
        }
        if (!flipY_)
        {
            position.y_ = spatialinfo.y_;
            pivot.y_ = spriteKey->pivotY_;
        }
        else
        {
            position.y_ = -spatialinfo.y_;
            pivot.y_ = 1.0f - spriteKey->pivotY_;
        }

        angle = spatialinfo.angle_;
        if (flipX_ != flipY_)
            angle = -angle;

        // use the custom hotspot at each time, don't flip again, pivot is already setted
        sprite->GetDrawRectangle(drawRect, pivot);

        ttexture = sprite->GetTexture();
        if (ttexture && ttexture != texture)
        {
            SetTextureMode(TXM_UNIT, GetTextureUnit(material, ttexture), texmode);
            texture = ttexture;
        }

        scale.x_ = spatialinfo.scaleX_;
        scale.y_ = spatialinfo.scaleY_;

        spriteWorldTransform = nodeWorldTransform * Matrix2x3(position * PIXEL_SIZE, angle, scale);
        spriteWorldTransform.Multiply(drawRect.min_, vertex0.position_);
        spriteWorldTransform.Multiply(Vector2(drawRect.min_.x_, drawRect.max_.y_), vertex1.position_);
        spriteWorldTransform.Multiply(drawRect.max_, vertex2.position_);
        spriteWorldTransform.Multiply(Vector2(drawRect.max_.x_, drawRect.min_.y_), vertex3.position_);

        vertex0.uv_ = textureRect.min_;
        vertex1.uv_ = Vector2(textureRect.min_.x_, textureRect.max_.y_);
        vertex2.uv_ = textureRect.max_;
        vertex3.uv_ = Vector2(textureRect.max_.x_, textureRect.min_.y_);

        color.a_ = spriteKey->info_.alpha_ * color_.a_;
        vertex0.color_ = vertex1.color_ = vertex2.color_ = vertex3.color_ = MultColors(spriteKey->color_, color).ToUInt();
        vertex0.texmode_ = vertex1.texmode_ = vertex2.texmode_ = vertex3.texmode_ = texmode;

        vertices.Push(vertex0);
        vertices.Push(vertex1);
        vertices.Push(vertex2);
        vertices.Push(vertex3);
    }
}

void AnimatedSprite2D::UpdateSourceBatchesSpriter_Custom(Vector<SourceBatch2D>* sourceBatchesPtr, int breakZIndex, bool resetBatches)
{
    if (!sourceBatchesPtr)
        return;

    const PODVector<SpriteInfo*>& spriteinfos = AnimatedSprite2D::GetSpriteInfos();
    if (!spriteinfos.Size())
        return;

    Vector<SourceBatch2D>& sourceBatches = *sourceBatchesPtr;

    // Reset firstkey
    if (resetBatches || !sourceBatches.Size() || breakZIndex == RESETFIRSTKEY)
        firstKeyIndex_ = 0;

    // Set the stopkey
    if (breakZIndex > 0)
    {
        if (firstKeyIndex_ >= spritesKeys_.Size()-1)
            return;

        for (size_t i = firstKeyIndex_; i < spritesKeys_.Size(); ++i)
        {
            if (spritesKeys_[i]->zIndex_ > breakZIndex)
            {
                stopKeyIndex_ = i;
                break;
            }
        }
    }
    else
    {
        stopKeyIndex_ = spritesKeys_.Size();
    }

    // Get the material
    Material* material = customMaterial_ ? customMaterial_ : renderer_->GetMaterial(spritesInfos_[0]->sprite_->GetTexture(), blendMode_);
    if (!material)
        return;

    // Reset the batches
    if (resetBatches || !sourceBatches.Size())
    {
        sourceBatches.Resize(1);
        sourceBatches[0].vertices_.Clear();
        sourceBatches[0].drawOrder_ = GetDrawOrder();
        sourceBatches[0].material_ = SharedPtr<Material>(material);
    }

    int iBatch = sourceBatches.Size()-1;
    Material* prevMaterial = sourceBatches[iBatch].material_;

    // Start Loop
    Matrix2x3 nodeWorldTransform(GetNode()->GetWorldPosition2D(), GetNode()->GetWorldRotation2D(), GetNode()->GetWorldScale2D());
    Matrix2x3 spriteWorldTransform;

    Rect drawRect;
    Rect textureRect;

    Color color = MultColors(color_, spriterInstance_->GetEntity()->color_);

    Vertex2D vertex0;
    Vertex2D vertex1;
    Vertex2D vertex2;
    Vertex2D vertex3;

    vertex0.position_.z_ = vertex1.position_.z_ = vertex2.position_.z_ = vertex3.position_.z_ = node_->GetWorldPosition().z_;

    Vector2 position;
    Vector2 scale;
    Vector2 pivot;

    float angle;

    Material* tmaterial;

    Sprite2D* sprite;
    Texture2D* texture = nullptr;
    Texture2D* ttexture = nullptr;
    Spriter::SpriteTimelineKey* spriteKey;

    int textureunit = -1;

    Vector4 texmode;
    SetTextureMode(TXM_FX, textureFX_, texmode);

    for (unsigned i = firstKeyIndex_; i < stopKeyIndex_; i++)
    {
        spriteKey = spritesKeys_[i];
        const SpriteInfo* spriteinfo = spritesInfos_[i];
        sprite = spriteinfo->sprite_;

        if (!sprite->GetTextureRectangle(textureRect, flipX_, flipY_))
            continue;

        ttexture = sprite->GetTexture();
        if (ttexture && texture != ttexture)
        {
            textureunit = GetTextureUnit(material, ttexture);
            // change the material
            tmaterial = customMaterial_ ? customMaterial_ : renderer_->GetMaterial(ttexture, blendMode_);
            if (!tmaterial)
                continue;
            material = tmaterial;

            // get the new texture unit
            textureunit = GetTextureUnit(material, ttexture);
            if (textureunit == -1)
                continue;

            // change the texture mode
            if (GetTextureMode(TXM_UNIT, texmode) != textureunit)
                SetTextureMode(TXM_UNIT, textureunit, texmode);

            // change the texture
            texture = ttexture;
        }

		// lit or unlit fx
		SetTextureMode(TXM_FX_LIT, spriteKey->fx_ > 0 ? 1U : textureFX_, texmode);

        // Add new Batch
        if (material != prevMaterial)
        {
            iBatch++;
            sourceBatches.Resize(iBatch+1);
            sourceBatches[iBatch].vertices_.Clear();
            sourceBatches[iBatch].drawOrder_ = sourceBatches[iBatch-1].drawOrder_+1;
            sourceBatches[iBatch].material_ = SharedPtr<Material>(material);
            prevMaterial = material;
        }

        const Spriter::SpatialInfo& spatialinfo = spriteKey->info_;
        if (spriteinfo->mapinfo_)
        {
            if (!flipX_)
            {
                position.x_ = spatialinfo.x_ + spriteinfo->mapinfo_->instruction_->targetdx_;
                pivot.x_ = spriteKey->pivotX_ + spriteinfo->dPivot_.x_;
            }
            else
            {
                position.x_ = -spatialinfo.x_ - spriteinfo->mapinfo_->instruction_->targetdx_;
                pivot.x_ = 1.0f - spriteKey->pivotX_ - spriteinfo->dPivot_.x_;
            }
            if (!flipY_)
            {
                position.y_ = spatialinfo.y_ + spriteinfo->mapinfo_->instruction_->targetdy_;
                pivot.y_ = spriteKey->pivotY_ + spriteinfo->dPivot_.y_;
            }
            else
            {
                position.y_ = -spatialinfo.y_ - spriteinfo->mapinfo_->instruction_->targetdy_;
                pivot.y_ = 1.0f - spriteKey->pivotY_ - spriteinfo->dPivot_.y_;
            }
            angle = spatialinfo.angle_ + spriteinfo->mapinfo_->instruction_->targetdangle_;
        }
        else
        {
            if (!flipX_)
            {
                position.x_ = spatialinfo.x_;
                pivot.x_ = spriteKey->pivotX_ + spriteinfo->dPivot_.x_;
            }
            else
            {
                position.x_ = -spatialinfo.x_;
                pivot.x_ = 1.0f - spriteKey->pivotX_ - spriteinfo->dPivot_.x_;
            }
            if (!flipY_)
            {
                position.y_ = spatialinfo.y_;
                pivot.y_ = spriteKey->pivotY_ + spriteinfo->dPivot_.y_;
            }
            else
            {
                position.y_ = -spatialinfo.y_;
                pivot.y_ = 1.0f - spriteKey->pivotY_ - spriteinfo->dPivot_.y_;
            }
            angle = spatialinfo.angle_;
        }
        if (flipX_ != flipY_)
            angle = -angle;

        scale.x_ = spatialinfo.scaleX_ * spriteinfo->scale_.x_;
        scale.y_ = spatialinfo.scaleY_ * spriteinfo->scale_.y_;
        if (spriteinfo->mapinfo_)
        {
            scale.x_ *= spriteinfo->mapinfo_->instruction_->targetscalex_;
            scale.y_ *= spriteinfo->mapinfo_->instruction_->targetscaley_;
        }

        // use the custom hotspot at each time, don't flip again, pivot is already setted
        sprite->GetDrawRectangle(drawRect, pivot);

        spriteWorldTransform = nodeWorldTransform * Matrix2x3(position * PIXEL_SIZE, angle, scale);
        spriteWorldTransform.Multiply(drawRect.min_, vertex0.position_);
        spriteWorldTransform.Multiply(Vector2(drawRect.min_.x_, drawRect.max_.y_), vertex1.position_);
        spriteWorldTransform.Multiply(drawRect.max_, vertex2.position_);
        spriteWorldTransform.Multiply(Vector2(drawRect.max_.x_, drawRect.min_.y_), vertex3.position_);

        vertex0.uv_ = textureRect.min_;
        vertex1.uv_ = Vector2(textureRect.min_.x_, textureRect.max_.y_);
        vertex2.uv_ = textureRect.max_;
        vertex3.uv_ = Vector2(textureRect.max_.x_, textureRect.min_.y_);

        // Set Batch
        Vector<Vertex2D>& vertices = sourceBatches[iBatch].vertices_;
        color.a_ = spriteKey->info_.alpha_ * color_.a_;
        vertex0.color_ = vertex1.color_ = vertex2.color_ = vertex3.color_ = MultColors(spriteKey->color_, color).ToUInt();
        vertex0.texmode_ = vertex1.texmode_ = vertex2.texmode_ = vertex3.texmode_ = texmode;

		vertices.Push(vertex0);
        vertices.Push(vertex1);
        vertices.Push(vertex2);
        vertices.Push(vertex3);
    }

    firstKeyIndex_ = stopKeyIndex_;
}

template< typename T > void AnimatedSprite2D::GetVertices(const IntVector2& size, const T& transform, PODVector<float>& verticeData)
{
    const PODVector<Spriter::SpriteTimelineKey* >& spriteKeys = spriterInstance_->GetSpriteKeys();
    if (!spriteKeys.Size())
        UpdateSpriterAnimation(0.0f);

    for (size_t i = 0; i < spriteKeys.Size(); ++i)
    {
        Spriter::SpriteTimelineKey* spriteKey = spriteKeys[i];
        Sprite2D* sprite = animationSet_->GetSpriterFileSprite((spriteKey->folderId_ << 16) + spriteKey->fileId_);

        if (!sprite)
            continue;

        verticeData.Resize(verticeData.Size() + 36);

        const Spriter::SpatialInfo& spatial = spriteKey->info_;

        AddSprite_UI(sprite, size, transform, spatial.x_, spatial.y_, spriteKey->pivotX_, spriteKey->pivotY_, spatial.scaleX_, spatial.scaleY_,
                            spatial.angle_, spatial.alpha_, &verticeData[verticeData.Size() - 36]);
    }
}

template void AnimatedSprite2D::GetVertices(const IntVector2& size, const Matrix3x4& transform, PODVector<float>& verticeData);

void AnimatedSprite2D::AddSprite_UI(Sprite2D* sprite, const IntVector2& size, const Matrix3x4& transform,
                                        float x, float y, float px, float py, float sx, float sy, float angle, float alpha, float* vertices)
{
    static Rect textureRect;
    if (!sprite->GetTextureRectangle(textureRect, flipX_, flipY_))
        return;

    static Rect drawRect;
    static Vector3 position;

    if (flipX_)
    {
        x  = -x;
        px = 1.0f - px;
    }
    if (flipY_)
    {
        y  = -y;
        py = 1.0f - py;
    }
    if (flipX_ != flipY_)
        angle = -angle;

    Matrix3x4 finaltransform = transform * Matrix3x4(Vector3(x, y, 0.f), Quaternion(angle), Vector3(sx, sy, 1.f));

    // use the pivot of the local sprite
    sprite->GetDrawRectangle(drawRect, Vector2(px, py));
    // in screen unit
    drawRect.min_ *= (1.f / PIXEL_SIZE);
    drawRect.max_ *= (1.f / PIXEL_SIZE);

    // use the attribute hotspot from the animatedsprite
    px = hotSpot_.x_ * size.x_;
    py = hotSpot_.y_ * size.y_;

    unsigned ucolor = Color(color_, alpha * color_.a_).ToUInt();

    position = finaltransform * Vector3(drawRect.min_, 0.0f);
    vertices[0] = position.x_ + px;
    vertices[1] = position.y_ + py;
    vertices[2] = 0.f;
    ((unsigned&)vertices[3]) = ucolor;
    vertices[4] = textureRect.min_.x_;
    vertices[5] = textureRect.min_.y_;

    position = finaltransform * Vector3(drawRect.min_.x_, drawRect.max_.y_, 0.0f);
    vertices[6] = position.x_ + px;
    vertices[7] = position.y_ + py;
    vertices[8] = 0.f;
    ((unsigned&)vertices[9]) = ucolor;
    vertices[10] = textureRect.min_.x_;
    vertices[11] = textureRect.max_.y_;

    position = finaltransform * Vector3(drawRect.max_.x_, drawRect.min_.y_, 0.0f);
    vertices[12] = position.x_ + px;
    vertices[13] = position.y_ + py;
    vertices[14] = 0.f;
    ((unsigned&)vertices[15]) = ucolor;
    vertices[16] = textureRect.max_.x_;
    vertices[17] = textureRect.min_.y_;

    position = finaltransform * Vector3(drawRect.max_, 0.0f);
    vertices[18] = position.x_ + px;
    vertices[19] = position.y_ + py;
    vertices[20] = 0.f;
    ((unsigned&)vertices[21]) = ucolor;
    vertices[22] = textureRect.max_.x_;
    vertices[23] = textureRect.max_.y_;

    vertices[24] = vertices[12];
    vertices[25] = vertices[13];
    vertices[26] = vertices[14];
    vertices[27] = vertices[15];
    vertices[28] = vertices[16];
    vertices[29] = vertices[17];

    vertices[30] = vertices[6];
    vertices[31] = vertices[7];
    vertices[32] = vertices[8];
    vertices[33] = vertices[9];
    vertices[34] = vertices[10];
    vertices[35] = vertices[11];
}


void AnimatedSprite2D::Dispose(bool removeNode)
{
#ifdef URHO3D_SPINE
    if (animationState_)
    {
        spAnimationState_dispose(animationState_);
        animationState_ = nullptr;
    }

    if (animationStateData_)
    {
        spAnimationStateData_dispose(animationStateData_);
        animationStateData_ = nullptr;
    }

    if (skeleton_)
    {
        spSkeleton_dispose(skeleton_);
        skeleton_ = nullptr;
    }
#endif
    if (spriterInstance_)
    {
		ClearTriggers(removeNode);

        ResetCharacterMapping();

        spriterInstance_.Reset();
    }

    sourceBatches_.Clear();
    sourceBatches_.Resize(1);

    animationName_.Clear();

    customSourceBatches_ = nullptr;
}

