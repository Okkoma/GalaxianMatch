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

#include "../Precompiled.h"

#include "../Core/Context.h"

#include "../Core/Profiler.h"

#include "../IO/Log.h"
#include "../Resource/ResourceCache.h"
#include "../Scene/Scene.h"
#include "../Scene/SceneEvents.h"
#include "../Urho2D/Renderer2D.h"
#include "../Urho2D/CollisionCircle2D.h"
#include "../Urho2D/CollisionBox2D.h"
#include "../Urho2D/AnimatedSprite2D.h"
#include "../Urho2D/AnimationSet2D.h"
#include "../Urho2D/Sprite2D.h"
#include "../Urho2D/SpriterInstance2D.h"
#include "../Graphics/Material.h"
#include "../Graphics/Texture2D.h"
#include "../DebugNew.h"

#ifdef URHO3D_SPINE
#include <spine/spine.h>
#endif


//#define ANIM_WORLDBOUNDINGBOXUPDATE

static const Urho3D::StringHash SPRITER_SOUND           = Urho3D::StringHash("SPRITER_Sound");
static const Urho3D::StringHash SPRITER_ANIMATION       = Urho3D::StringHash("SPRITER_Animation");
static const Urho3D::StringHash SPRITER_ANIMATIONINSIDE = Urho3D::StringHash("SPRITER_AnimationInside");
static const Urho3D::StringHash SPRITER_ENTITY          = Urho3D::StringHash("SPRITER_Entity");

namespace Urho3D
{

extern const char* URHO2D_CATEGORY;
extern const char* blendModeNames[];

const char* loopModeNames[] =
{
    "Default",
    "ForceLooped",
    "ForceClamped",
    0
};


AnimatedSprite2D::AnimatedSprite2D(Context* context) :
    StaticSprite2D(context),
#ifdef URHO3D_SPINE
    skeleton_(0),
    animationStateData_(0),
    animationState_(0),
#endif
    speed_(1.0f),
    loopMode_(LM_DEFAULT),
    useCharacterMap_(false),
    characterMapDirty_(true),
    renderEnabled_(true)
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
    context->RegisterFactory<AnimatedSprite2D>(URHO2D_CATEGORY);

    URHO3D_COPY_BASE_ATTRIBUTES(StaticSprite2D);
    URHO3D_REMOVE_ATTRIBUTE("Sprite");
    URHO3D_ACCESSOR_ATTRIBUTE("Speed", GetSpeed, SetSpeed, float, 1.0f, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Custom Spritesheet", GetEmptyString, SetCustomSpriteSheetAttr, String, String::EMPTY, AM_DEFAULT);
    URHO3D_MIXED_ACCESSOR_ATTRIBUTE("Animation Set", GetAnimationSetAttr, SetAnimationSetAttr, ResourceRef, ResourceRef(AnimatedSprite2D::GetTypeStatic(), String::EMPTY), AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Entity", GetEntityName, SetEntity, String, String::EMPTY, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Animation", GetAnimation, SetAnimationAttr, String, String::EMPTY, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Applied Character Maps", GetAppliedCharacterMapsAttr, SetAppliedCharacterMapsAttr, VariantVector, Variant::emptyVariantVector, AM_DEFAULT);
    //URHO3D_MIXED_ACCESSOR_ATTRIBUTE("Apply Character Map", GetCharacterMapAttr, SetCharacterMapAttr, String, String::EMPTY, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Apply Character Map", GetEmptyString, SetCharacterMapAttr, String, String::EMPTY, AM_DEFAULT);
    URHO3D_ENUM_ACCESSOR_ATTRIBUTE("Loop Mode", GetLoopMode, SetLoopMode, LoopMode2D, loopModeNames, LM_DEFAULT, AM_DEFAULT);

#ifdef USE_KEYPOOLS
    Spriter::SpriterData::InitKeyPools();
#endif
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

//    URHO3D_LOGINFOF("AnimatedSprite2D() - SetAnimationSet : entity = %s ...", entityName_.CString());

#ifdef URHO3D_SPINE
    if (animationSet_->GetSkeletonData())
    {
        spSkeletonData* skeletonData = animationSet->GetSkeletonData();

        // Create skeleton
        skeleton_ = spSkeleton_create(skeletonData);
        skeleton_->flipX = flipX_;
        skeleton_->flipY = flipY_;

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

    SetSprite(animationSet_->GetSprite());

//    URHO3D_LOGINFOF("AnimatedSprite2D() - SetAnimationSet : entity = %s ... OK !", entityName_.CString());

    // Clear animation name
    animationName_.Clear();
    loopMode_ = LM_DEFAULT;
}

void AnimatedSprite2D::SetEntity(const String& entity)
{
    if (entity == entityName_)
        return;

    drawRectDirty_ = true;

    entityName_ = entity;

//    URHO3D_LOGINFOF("AnimatedSprite2D() - SetEntity : entity = %s", entityName_.CString());

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

    drawRectDirty_ = true;

    entityName_ = entityname;

//    URHO3D_LOGINFOF("AnimatedSprite2D() - SetSpriterEntity : entity = %s", entityName_.CString());

    spriterInstance_->SetEntity(index);
    SetAnimation(GetAnimation());
}

void AnimatedSprite2D::SetAnimation(const String& name, LoopMode2D loopMode)
{
    if (!animationSet_)
        return;

//    URHO3D_LOGINFOF("AnimatedSprite2D() - SetAnimation : %s(%u) current:%s to new:%s ... ",
//                     node_->GetName().CString(), node_->GetID(), animationName_.CString(), name.CString());
    if (!name.Empty())
    {
        if (animationSet_->HasAnimation(name))
            animationName_ = name;
    }

    if (animationName_.Empty() || (!animationName_.Empty() && !animationSet_->HasAnimation(animationName_)))
        animationName_ = GetDefaultAnimation();

    if (animationName_.Empty())
    {
        URHO3D_LOGWARNINGF("AnimatedSprite2D() - SetAnimation : No Animation Name !");
        return;
    }

    loopMode_ = loopMode;

#ifdef URHO3D_SPINE
    if (skeleton_)
        SetSpineAnimation();
#endif
    if (spriterInstance_)
        SetSpriterAnimation();

//    URHO3D_LOGINFOF("AnimatedSprite2D() - SetAnimation : name = %s ... OK !", animationName_.CString());
}

void AnimatedSprite2D::SetRenderEnable(bool enable, int zindex)
{
    if (!enable)
    {
        renderEnabled_ = enable;
        sourceBatches_.Resize(1);
        sourceBatches_[0].vertices_.Clear();
        renderZIndex_ = zindex;
    }
}

void AnimatedSprite2D::SetLoopMode(LoopMode2D loopMode)
{
    loopMode_ = loopMode;
}

void AnimatedSprite2D::SetSpeed(float speed)
{
    speed_ = speed;
    MarkNetworkUpdate();
}

void AnimatedSprite2D::SetCustomSpriteSheetAttr(const String& value)
{
    //if (!value.Empty())
    {
//        URHO3D_LOGERRORF("AnimatedSprite2D() - SetCustomSpriteSheetAttr : %s(%u) spritesheet=%s", node_->GetName().CString(), node_->GetID(), value.CString());
        AnimationSet2D::customSpritesheetFile_ = value;
    }
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

//    URHO3D_LOGINFOF("AnimatedSprite2D() - SetAnimationAttr : %s(%u) name=%s !",
//                     node_->GetName().CString(), node_->GetID(), animationName_.CString());

    SetAnimation(animationName_, loopMode_);
}

void AnimatedSprite2D::CleanDependences()
{
//    URHO3D_LOGINFOF("AnimatedSprite2D() - CleanDependences : node=%s(%u) enabled=%s", node_->GetName().CString(), node_->GetID(), IsEnabledEffective() ? "true" : "false");

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
    if (spriterInstance_)
        return spriterInstance_->GetCurrentTime();

    return 0.f;
}

bool AnimatedSprite2D::HasFinishedAnimation() const
{
    return spriterInstance_->HasFinishedAnimation();
}

Spriter::SpriterInstance* AnimatedSprite2D::GetSpriterInstance() const
{
    return spriterInstance_.Get();
}

int AnimatedSprite2D::GetSpriterAnimationId() const
{
    return spriterInstance_->GetAnimation() ? spriterInstance_->GetAnimation()->id_ : -1;
}

int AnimatedSprite2D::GetSpriterAnimationId(const String& animationName) const
{
    Spriter::Animation* animation = spriterInstance_->GetAnimation(animationName);
    return animation ? animation->id_ : -1;
}


Spriter::Animation* AnimatedSprite2D::GetSpriterAnimation(int index) const
{
    return index == -1 ? spriterInstance_->GetAnimation() : spriterInstance_->GetAnimation(index);
}

Spriter::Animation* AnimatedSprite2D::GetSpriterAnimation(const String& animationName) const
{
    return animationName != String::EMPTY ? spriterInstance_->GetAnimation(animationName) : 0;
}

void AnimatedSprite2D::GetLocalSpritePositions(unsigned spriteindex, Vector2& position, float& angle, Vector2& scale)
{
    const Spriter::SpriteTimelineKey& spritekey = *spritesKeys_[spriteindex];
    const Spriter::SpatialInfo& spatialinfo = spritekey.info_;
    const SpriteInfo* spriteinfo  = spritesInfos_[spriteindex];

    position.x_ = spatialinfo.x_ * PIXEL_SIZE;
    position.y_ = spatialinfo.y_ * PIXEL_SIZE;

    if (flipX_)
        position.x_ = -position.x_;
    if (flipY_)
        position.y_ = -position.y_;

    angle = spatialinfo.angle_;
    if (flipX_ != flipY_)
        angle = -angle;

//    if (spriteinfo->sprite_->GetRotated())
//        angle -= 90;

//    if (spriteinfo->rotate_)
//        angle = (angle+180);

    scale.x_ = spatialinfo.scaleX_ * spriteinfo->scalex_;
    scale.y_ = spatialinfo.scaleY_ * spriteinfo->scaley_;

//    Rect drawRect;
//    spriteinfo->sprite_->GetDrawRectangle(drawRect, Vector2(spritekey.pivotX_, spritekey.pivotY_));
//    position = drawRect.Transformed(Matrix2x3(position, angle, scale)).Center();
}

Sprite2D* AnimatedSprite2D::GetSprite(unsigned spriteindex) const
{
    return spritesInfos_[spriteindex]->sprite_;
}

ResourceRef AnimatedSprite2D::GetAnimationSetAttr() const
{
    return GetResourceRef(animationSet_, AnimationSet2D::GetTypeStatic());
}


/// CHARACTER MAPPING SETTERS

void AnimatedSprite2D::SetAppliedCharacterMapsAttr(const VariantVector& characterMapApplied)
{
    characterMapApplied_.Clear();

    if (characterMapApplied.Empty())
        return;

    for (VariantVector::ConstIterator it=characterMapApplied.Begin(); it != characterMapApplied.End(); ++it)
        ApplyCharacterMap(it->GetStringHash());

    MarkNetworkUpdate();
}

void AnimatedSprite2D::SetCharacterMapAttr(const String& characterMapNames)
{
//    URHO3D_LOGINFOF("AnimatedSprite2D() - SetCharacterMapAttr : %s(%u) names=%s !",
//                     node_->GetName().CString(), node_->GetID(), characterMapNames.CString());

    characterMapApplied_.Clear();

    if (characterMapNames.Empty())
        return;

    bool state = false;
    Vector<String> names = characterMapNames.Split('|', false);

    for (Vector<String>::ConstIterator it=names.Begin(); it != names.End(); ++it)
        state |= ApplyCharacterMap(StringHash(*it));

    MarkNetworkUpdate();

//    URHO3D_LOGINFOF("AnimatedSprite2D() - SetCharacterMapAttr : %s(%u) names=%s !",
//                    node_->GetName().CString(), node_->GetID(), characterMapNames.CString());
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
        Spriter::MapInstruction* map = *it;

        key = (map->folder_ << 16) + map->file_;

        if (map->targetFolder_ == -1)
        {
            spriteMapping_[key].Reset();
        }
        else
        {
            Sprite2D* sprite = animationSet_->GetSpriterFileSprite(map->targetFolder_, map->targetFile_);

            spriteMapping_[key] = SharedPtr<Sprite2D>(sprite);
        }
    }

    if (!IsCharacterMapApplied(characterMap->hashname_))
        characterMapApplied_.Push(characterMap->hashname_);

    characterMaps_.Push(characterMap);

    useCharacterMap_ = true;

    sourceBatchesDirty_ = true;

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
    URHO3D_LOGINFOF("AnimatedSprite2D() - SwapSprites : ...");

    Spriter::CharacterMap* characterMapOrigin = GetCharacterMap(characterMap);

    if (!characterMapOrigin)
    {
        URHO3D_LOGERRORF("AnimatedSprite2D() - SwapSprites : no characterMap origin !");
        return;
    }

    PODVector<Sprite2D*> originalSprites;
    GetMappedSprites(characterMapOrigin, originalSprites);

    if (!originalSprites.Size() || !replacements.Size())
    {
        URHO3D_LOGERRORF("AnimatedSprite2D() - SwapSprites : no spriteslist !");
        return;
    }

    SwapSprites(originalSprites, replacements, keepProportion);

    ApplyCharacterMap(characterMap);

    sourceBatchesDirty_ = true;

    URHO3D_LOGINFOF("AnimatedSprite2D() - SwapSprites : ... OK !");
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
                            node_->GetName().CString(), node_->GetID(), replacement->GetName().CString());
        return;
    }

    swappedSprites_[original] = SharedPtr<Sprite2D>(replacement);

    if (original == replacement)
        return;

    if (replacement)
    {
        const IntRect& orect = original->GetRectangle();
        const IntRect& rrect = replacement->GetRectangle();

        SpriteInfo& info = spriteInfoMapping_[replacement];
        info.sprite_ = replacement;
        info.deltaHotspotx_ = replacement->GetHotSpot().x_ - original->GetHotSpot().x_;
        info.deltaHotspoty_ = replacement->GetHotSpot().y_ - original->GetHotSpot().y_;

        if (!keepRatio)
        {
            info.scalex_ = (float)(orect.right_ - orect.left_) / (float)(rrect.right_ - rrect.left_);
            info.scaley_ = (float)(orect.bottom_ - orect.top_) / (float)(rrect.bottom_ - rrect.top_);
        }
//        else
//        {
//            // get the replacement ratio (width/height)
//            float rratio = (float)(rrect.right_ - rrect.left_) / (float)(rrect.bottom_ - rrect.top_);
//            // keep the min replacement size width and height
//            // width > height => use width for scale
//            if (rratio > 0.f)
//                info.scalex_ = info.scaley_  = (float)(orect.right_ - orect.left_) / (float)(rrect.right_ - rrect.left_);
//            // height > width => use height for scale
//            else
//                info.scaley_ = info.scalex_ = (float)(orect.bottom_ - orect.top_) / (float)(rrect.bottom_ - rrect.top_);
//        }
        else
        {
            info.scalex_ = info.scaley_ = 1.f;
        }

        URHO3D_LOGINFOF("AnimatedSprite2D() - SwapSprite : node=%s(%u) original=%s osize=(%d,%d) replacement=%s rsize=(%d,%d) keepRatio=%s scale=(%f,%f) deltaHotspot=(%f,%f)",
                            node_->GetName().CString(), node_->GetID(),
                            original->GetName().CString(), orect.right_ - orect.left_, orect.bottom_ - orect.top_,
                            replacement->GetName().CString(), rrect.right_ - rrect.left_, rrect.bottom_ - rrect.top_,
                            keepRatio ? "true": "false", info.scalex_, info.scaley_, info.deltaHotspotx_, info.deltaHotspoty_);
    }
}

void AnimatedSprite2D::SwapSprites(const PODVector<Sprite2D*>& originals, const PODVector<Sprite2D*>& replacements, bool keepRatio)
{
    int size = Min((int)originals.Size(), (int)replacements.Size());

    URHO3D_LOGINFOF("AnimatedSprite2D() - SwapSprites : node=%s(%u) originalListSize=%u replacementListSize=%u keepRatio=%s ...",
                    node_->GetName().CString(), node_->GetID(), originals.Size(), replacements.Size(), keepRatio ? "true":"false");
    if (!size)
        return;

    for (int i = 0; i < size; ++i)
    {
        URHO3D_LOGINFOF(" => original=%s(ptr=%u) replacement=%s(ptr=%u)",
                        originals[i] ? originals[i]->GetName().CString() : "", originals[i],
                        replacements[i] ? replacements[i]->GetName().CString() : "", replacements[i]);
        SwapSprite(originals[i], replacements[i], keepRatio);
    }

    URHO3D_LOGINFOF("AnimatedSprite2D() - SwapSprites : node=%s(%u) ... OK !", node_->GetName().CString(), node_->GetID());
}

void AnimatedSprite2D::UnSwapSprite(Sprite2D* original)
{
    if (!original)
        return;

    swappedSprites_.Erase(original);
}

void AnimatedSprite2D::ResetCharacterMapping()
{
    characterMaps_.Clear();
    characterMapApplied_.Clear();

    spriteMapping_.Clear();
    spritesInfos_.Clear();

    UnSwapAllSprites();

    characterMapDirty_ = false;
    useCharacterMap_ = false;
}


/// CHARACTER MAPPING GETTERS

/*
String AnimatedSprite2D::GetCharacterMapAttr() const
{
    String names;
    for (PODVector<Spriter::CharacterMap* >::ConstIterator it=characterMaps_.End()-1; it==characterMaps_.Begin(); --it)
    {
        const String& name = (*it)->name_;
        if (!names.Contains(name))
        {
            if (names.Empty())
                names.Append('|');
            names.Append(name);
        }
    }
    return names;
}
*/

const VariantVector& AnimatedSprite2D::GetAppliedCharacterMapsAttr() const
{
    return characterMapApplied_;
}

bool AnimatedSprite2D::HasCharacterMapping() const
{
    return spriterInstance_ ? spriterInstance_->GetEntity()->characterMaps_.Size() != 0 : false;
}

bool AnimatedSprite2D::HasCharacterMap(const StringHash& hashname) const
{
    return GetCharacterMap(hashname) != 0;
}

bool AnimatedSprite2D::HasCharacterMap(const String& name) const
{
    return GetCharacterMap(name) != 0;
}

Spriter::CharacterMap* AnimatedSprite2D::GetCharacterMap(const StringHash& characterMap) const
{
    if (!spriterInstance_)
    {
        URHO3D_LOGWARNINGF("AnimatedSprite2D() - GetCharacterMap : %s(%u) no spriterinstance !", node_->GetName().CString(), node_->GetID());
        return 0;
    }

    Spriter::Entity* entity = spriterInstance_->GetEntity();

    const PODVector<Spriter::CharacterMap*>& characterMaps = entity->characterMaps_;
    for (PODVector<Spriter::CharacterMap*>::ConstIterator it=characterMaps.Begin(); it != characterMaps.End(); ++it)
    {
        if ((*it)->hashname_ == characterMap)
            return *it;
    }
//    URHO3D_LOGWARNINGF("AnimatedSprite2D() - GetCharacterMap : no characterMap hashname=%u", characterMap.Value());
    return 0;
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
//        if (map->targetFolder_ == -1)
//            continue;
//        sprites.Push(animationSet_->GetSpriterFileSprite(map->targetFolder_, map->targetFile_));

        sprites.Push(map->targetFolder_ == -1 ? 0 : animationSet_->GetSpriterFileSprite(map->targetFolder_, map->targetFile_));
    }
}

Sprite2D* AnimatedSprite2D::GetMappedSprite(unsigned key) const
{
    HashMap<unsigned, SharedPtr<Sprite2D> >::ConstIterator it = spriteMapping_.Find(key);
    return it != spriteMapping_.End() ? it->second_.Get() : animationSet_->GetSpriterFileSprite(key);
}

Sprite2D* AnimatedSprite2D::GetMappedSprite(int folderid, int fileid) const
{
    return GetMappedSprite((folderid << 16) + fileid);
}

Sprite2D* AnimatedSprite2D::GetSwappedSprite(Sprite2D* original) const
{
    if (!original)
        return 0;

    HashMap<Sprite2D*, SharedPtr<Sprite2D> >::ConstIterator it = swappedSprites_.Find(original);
    return it != swappedSprites_.End() ? it->second_.Get() : original;
}

SpriteInfo* AnimatedSprite2D::GetSpriteInfo(Sprite2D* sprite)
{
    SpriteInfo& info = spriteInfoMapping_[sprite];
    if (!info.sprite_)
    {
        info.sprite_ = sprite;
        info.scalex_ = 1.f;
        info.scaley_ = 1.f;
        info.deltaHotspotx_ = 0.f;
        info.deltaHotspoty_ = 0.f;
    }
    return &info;
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
//    URHO3D_LOGINFOF("AnimatedSprite2D() - OnSetEnabled : node=%s(%u) enabled=%s",
//             node_->GetName().CString(), node_->GetID(), IsEnabledEffective() ? "true" : "false");

    Drawable2D::OnSetEnabled();

    bool enabled = IsEnabledEffective();

    Scene* scene = GetScene();
    if (scene)
    {
        if (enabled)
        {
            SetTriggers();
            UpdateAnimation(0.f);
            SubscribeToEvent(scene, E_SCENEPOSTUPDATE, URHO3D_HANDLER(AnimatedSprite2D, HandleScenePostUpdate));
        }
        else
        {
            UnsubscribeFromEvent(scene, E_SCENEPOSTUPDATE);
            if (spriterInstance_) spriterInstance_->ResetCurrentTime();
            HideTriggers();
        }
    }
}

void AnimatedSprite2D::OnDrawOrderChanged()
{
}

void AnimatedSprite2D::OnSceneSet(Scene* scene)
{
//    URHO3D_LOGINFOF("AnimatedSprite2D() - OnSceneSet : node=%s(%u) scene=%u enabled=%s",
//             node_->GetName().CString(), node_->GetID(), scene, IsEnabledEffective() ? "true" : "false");

    StaticSprite2D::OnSceneSet(scene);

    if (scene)
    {
        if (scene == node_)
            URHO3D_LOGWARNING(GetTypeName() + " should not be created to the root scene node");

        if (IsEnabledEffective())
        {
            SubscribeToEvent(scene, E_SCENEPOSTUPDATE, URHO3D_HANDLER(AnimatedSprite2D, HandleScenePostUpdate));
        }
    }
    else
    {
        UnsubscribeFromEvent(E_SCENEPOSTUPDATE);
        //HideTriggers();
    }
}

void AnimatedSprite2D::HandleScenePostUpdate(StringHash eventType, VariantMap& eventData)
{
    if (speed_)
        UpdateAnimation(eventData[ScenePostUpdate::P_TIMESTEP].GetFloat());
}


/// UPDATERS

void AnimatedSprite2D::SetTime(float time)
{
    if (spriterInstance_ && spriterInstance_->GetAnimation())
        spriterInstance_->SetTime(time);
}

void AnimatedSprite2D::UpdateAnimation(float timeStep)
{
    if (!timeStep)
        visibility_ = true;

    if (renderer_ && !renderer_->IsDrawableVisible(this))
    {
        if (visibility_)
        {
            ClearSourceBatches();
            visibility_ = false;
//            URHO3D_LOGINFOF("%s No Visible fwbox=%s !", node_->GetName().CString(), GetWorldBoundingBox().ToString().CString());
        }
        return;
    }

    URHO3D_PROFILE(AnimatedSprite2D_Update);

#ifdef URHO3D_SPINE
    if (skeleton_ && animationState_)
        UpdateSpineAnimation(timeStep);
#endif
    if (spriterInstance_ && spriterInstance_->GetAnimation())
    {
        if (!visibility_)
        {
            spriterInstance_->ResetCurrentTime();
            timeStep = 0.f;
        }

        UpdateSpriterAnimation(timeStep);
    }

    if (!visibility_)
    {
        visibility_ = true;
//        URHO3D_LOGINFOF("%s Visible !", node_->GetName().CString());
    }
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

    skeleton_->flipX = flipX_;
    skeleton_->flipY = flipY_;

    spSkeleton_update(skeleton_, timeStep);
    spAnimationState_update(animationState_, timeStep);
    spAnimationState_apply(animationState_, skeleton_);
    spSkeleton_updateWorldTransform(skeleton_);

    sourceBatchesDirty_ = true;
    worldBoundingBoxDirty_ = true;
}

void AnimatedSprite2D::UpdateSourceBatchesSpine()
{
    const Matrix3x4& worldTransform = GetNode()->GetWorldTransform();

    SourceBatch2D& sourceBatch = sourceBatches_[0];
    sourceBatches_[0].vertices_.Clear();

    const int SLOT_VERTEX_COUNT_MAX = 1024;
    float slotVertices[SLOT_VERTEX_COUNT_MAX];

    for (int i = 0; i < skeleton_->slotsCount; ++i)
    {
        spSlot* slot = skeleton_->drawOrder[i];
        spAttachment* attachment = slot->attachment;
        if (!attachment)
            continue;

        unsigned color = Color(color_.r_ * slot->r,
            color_.g_ * slot->g,
            color_.b_ * slot->b,
            color_.a_ * slot->a).ToUInt();

        if (attachment->type == SP_ATTACHMENT_REGION)
        {
            spRegionAttachment* region = (spRegionAttachment*)attachment;
            spRegionAttachment_computeWorldVertices(region, slot->bone, slotVertices);

            Vertex2D vertices[4];
            vertices[0].position_ = worldTransform * Vector3(slotVertices[SP_VERTEX_X1], slotVertices[SP_VERTEX_Y1]);
            vertices[1].position_ = worldTransform * Vector3(slotVertices[SP_VERTEX_X2], slotVertices[SP_VERTEX_Y2]);
            vertices[2].position_ = worldTransform * Vector3(slotVertices[SP_VERTEX_X3], slotVertices[SP_VERTEX_Y3]);
            vertices[3].position_ = worldTransform * Vector3(slotVertices[SP_VERTEX_X4], slotVertices[SP_VERTEX_Y4]);

            vertices[0].color_ = color;
            vertices[1].color_ = color;
            vertices[2].color_ = color;
            vertices[3].color_ = color;

            vertices[0].uv_ = Vector2(region->uvs[SP_VERTEX_X1], region->uvs[SP_VERTEX_Y1]);
            vertices[1].uv_ = Vector2(region->uvs[SP_VERTEX_X2], region->uvs[SP_VERTEX_Y2]);
            vertices[2].uv_ = Vector2(region->uvs[SP_VERTEX_X3], region->uvs[SP_VERTEX_Y3]);
            vertices[3].uv_ = Vector2(region->uvs[SP_VERTEX_X4], region->uvs[SP_VERTEX_Y4]);

            sourceBatches_[0].vertices_.Push(vertices[0]);
            sourceBatches_[0].vertices_.Push(vertices[1]);
            sourceBatches_[0].vertices_.Push(vertices[2]);
            sourceBatches_[0].vertices_.Push(vertices[3]);
        }
        else if (attachment->type == SP_ATTACHMENT_MESH)
        {
            spMeshAttachment* mesh = (spMeshAttachment*)attachment;
            if (mesh->verticesCount > SLOT_VERTEX_COUNT_MAX)
                continue;

            spMeshAttachment_computeWorldVertices(mesh, slot, slotVertices);

            Vertex2D vertex;
            vertex.color_ = color;
            for (int j = 0; j < mesh->trianglesCount; ++j)
            {
                int index = mesh->triangles[j] << 1;
                vertex.position_ = worldTransform * Vector3(slotVertices[index], slotVertices[index + 1]);
                vertex.uv_ = Vector2(mesh->uvs[index], mesh->uvs[index + 1]);

                sourceBatches_[0].vertices_.Push(vertex);
                // Add padding vertex
                if (j % 3 == 2)
                    sourceBatches_[0].vertices_.Push(vertex);
            }
        }
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
    }
}
#endif

void AnimatedSprite2D::SetSpriterAnimation(int index, LoopMode2D loopMode)
{
    if (index == -1)
    {
        if (!spriterInstance_->SetAnimation(animationName_, (Spriter::LoopMode)loopMode_))
        {
//            URHO3D_LOGWARNINGF("AnimatedSprite2D() - SetSpriterAnimation : %s(%u) - Set animation failed ! animationName = %s ...",
//                               node_->GetName().CString(), node_->GetID(), animationName_.CString());
            return;
        }
    }
    else
    {
        if (!spriterInstance_->SetAnimation(index, (Spriter::LoopMode)loopMode))
        {
//            URHO3D_LOGWARNINGF("AnimatedSprite2D() - SetSpriterAnimation : %s(%u) - Set animation failed ! index = %d ...",
//                               node_->GetName().CString(), node_->GetID(), index);
            return;
        }
        animationName_ = spriterInstance_->GetAnimation()->name_;
    }

    HideTriggers();

    MarkNetworkUpdate();
}

void AnimatedSprite2D::SetTriggers()
{
    // Get the pre-declared TrigAttack triggers from the child nodes
    const Vector<SharedPtr<Node> >& children = node_->GetChildren();
    for (Vector<SharedPtr<Node> >::ConstIterator it = children.Begin(); it != children.End(); ++it)
    {
        Node* node = it->Get();
        if (node->GetName().StartsWith("Trig_Attack"))
        {
            if (!triggerNodes_.Contains(node))
                triggerNodes_.Push(node);
            node->SetEnabled(false);
        }
    }
}

void AnimatedSprite2D::HideTriggers()
{
    activedEventTriggers_.Clear();

    if (!triggerNodes_.Size())
        return;

//    URHO3D_LOGINFOF("AnimatedSprite2D() - HideTriggers : %s(%u) triggerNodes_=%u...", node_->GetName().CString(), node_->GetID(), triggerNodes_.Size());

    // Inactive Trigger Nodes
    for (PODVector<Node* >::ConstIterator it = triggerNodes_.Begin();it!=triggerNodes_.End();++it)
        (*it)->SetEnabled(false);

//    URHO3D_LOGINFOF("AnimatedSprite2D() - HideTriggers : %s(%u) ... OK !", node_->GetName().CString(), node_->GetID());
}

void AnimatedSprite2D::ClearTriggers(bool removeNode)
{
    if (removeNode)
    {
        for (PODVector<Node* >::ConstIterator it = triggerNodes_.Begin();it!=triggerNodes_.End();++it)
            if ((*it)->IsTemporary())
                (*it)->Remove();
    }

    activedEventTriggers_.Clear();
    triggerNodes_.Clear();
    renderNodes_.Clear();
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

    Matrix2x3 worldtransform = node_->GetWorldTransform2D() * Matrix2x3(Vector2(centerx, centery), 0.f, Vector2(spatialinfo.scaleX_, spatialinfo.scaleY_));

    position = worldtransform.Translation();
}

void AnimatedSprite2D::UpdateTriggers()
{
    // Update Event Triggers
    const HashMap<Spriter::Timeline*, Spriter::SpriteTimelineKey* >& eventTriggers = spriterInstance_->GetEventTriggers();
    if (eventTriggers.Size())
    {
        StringHash triggerEventName;
        StringHash triggerEvent;
        Vector<String> args;
        Spriter::Timeline* timeline;
        Spriter::SpriteTimelineKey* key;
        Node* triggerNode;

        for (HashMap<Spriter::Timeline*, Spriter::SpriteTimelineKey* >::ConstIterator it=eventTriggers.Begin(); it!=eventTriggers.End() ; ++it)
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
                        triggerNode->isPoolNode_ = node_->isPoolNode_;
                        triggerNode->SetChangeModeEnable(false);
                        triggerNodes_.Push(triggerNode);
                        renderNodes_.Push(triggerNode);
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
/// FOR FROMBONES
//                    if (args.Size() > 1)
//                    {
//                        key = it->second_;
//                        Vector<String> params = args[1].Split(',');
//
//                        triggerInfo_.type_ = StringHash(params.Size() > 0 ? params[0] : args[1]);
//                        triggerInfo_.zindex_ = key->zIndex_;
//                        LocalToWorld(key, triggerInfo_.position_, triggerInfo_.rotation_);
//                        triggerInfo_.datas_ = params.Size() > 1 ? params[1] : String::EMPTY;
//
////                        URHO3D_LOGWARNINGF("AnimatedSprite2D() - UpdateTriggers : Set Initial Event=%s(%u) position=%s nodepos=%s...",
////                                       timeline->name_.CString(), triggerEvent.Value(), triggerInfo_.position_.ToString().CString(), node_->GetWorldPosition2D().ToString().CString());
//                    }
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

//                        URHO3D_LOGWARNINGF("AnimatedSprite2D() - UpdateTriggers : Set Initial Event=%s(%u) position=%s nodepos=%s numparams=%u ...",
//                                       timeline->name_.CString(), triggerEvent.Value(), triggerInfo_.position_.ToString().CString(), node_->GetWorldPosition2D().ToString().CString(), params.Size());
                    }

					node_->SendEvent(triggerEvent);
                }

//                URHO3D_LOGWARNINGF("AnimatedSprite2D() - UpdateTriggers : Set Initial Event=%s(%u) got=%u data=%u ...",
//                                       timeline->name_.CString(), triggerEvent.Value(), paramEvent[SPRITER_Event::TYPE].GetStringHash().Value(), paramEvent[SPRITER_Event::DATAS].GetVoidPtr());
            }
        }
    }
    else
        activedEventTriggers_.Clear();

    // Update Node Triggers
    const HashMap<String, Spriter::BoneTimelineKey* >& nodeTriggers = spriterInstance_->GetNodeTriggers();
    if (nodeTriggers.Size())
    {
        float centerx, centery, angle;
        String nodeName;
        Node* triggerNode;

        for (HashMap<String, Spriter::BoneTimelineKey* >::ConstIterator it=nodeTriggers.Begin(); it!=nodeTriggers.End() ; ++it)
        {
            nodeName = it->first_;

            triggerNode = node_->GetChild(nodeName);
            if (!triggerNode)
                continue;

            StaticSprite2D* drawable = triggerNode->GetDerivedComponent<StaticSprite2D>();
            if (!drawable)
                continue;

            const Spriter::SpatialInfo& info =  it->second_->info_;
            centerx = info.x_ * PIXEL_SIZE;
            centery = info.y_ * PIXEL_SIZE;

            // y orientation on x
            angle = info.angle_;
            if (flipX_ != flipY_)
                angle = -angle;

            if (flipX_)
                centerx = -centerx;

            if (flipY_)
                centery = -centery;

            triggerNode->SetPosition2D(centerx, centery);
            triggerNode->SetRotation2D(angle);
            drawable->SetFlip(flipX_, flipY_);
//            URHO3D_LOGWARNINGF("AnimatedSprite2D() - UpdateTriggers : NodeTrigger node=%s(%u) Update position x=%f y=%f ...", nodeName.CString(), triggerNode, centerx, centery);
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
                physicNode->isPoolNode_ = node_->isPoolNode_;
                physicNode->SetChangeModeEnable(false);

                triggerNodes_.Push(physicNode);

                if (isAbox)
                {
                    collisionBox = physicNode->CreateComponent<CollisionBox2D>(LOCAL);
                    collisionBox->SetChangeModeEnable(false);
                    collisionBox->SetTrigger(false);
                }
                else
                {
                    collisionCircle = physicNode->CreateComponent<CollisionCircle2D>(LOCAL);
                    collisionCircle->SetChangeModeEnable(false);
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

//            URHO3D_LOGINFOF("AnimatedSprite2D() - UpdateTriggers : PhysicTrigger node=%s(%u) isTriggerAttack=%s center=%s ...",
//                               timeline->name_.CString(), triggerNode, isTriggerAttack ? "true" : "false", center.ToString().CString());
        }
    }

    for (PODVector<Node* >::ConstIterator it = triggerNodes_.Begin(); it != triggerNodes_.End(); ++it)
    {
        if (!updatedPhysicNodes_.Contains(*it))
            (*it)->SetEnabled(false);
    }
}

void AnimatedSprite2D::UpdateSpriterAnimation(float timeStep)
{
    if (spriterInstance_->Update(timeStep * speed_))
    {
        UpdateTriggers();

        sourceBatchesDirty_ = true;
    }
}

const PODVector<SpriteInfo*>& AnimatedSprite2D::GetSpriteInfos()
{
    spritesKeys_.Clear();
    spritesInfos_.Clear();

    const PODVector<Spriter::SpriteTimelineKey* >& spriteKeys = spriterInstance_->GetSpriteKeys();
    if (!spriteKeys.Size())
    {
        UpdateSpriterAnimation(0.0f);
//        URHO3D_LOGERRORF("AnimatedSprite2D() - CreateSpriteInfos : node=%s ... No Spriter Keys !", node_->GetName().CString());
    }
    else
    {
        Sprite2D* sprite;
        Spriter::SpriteTimelineKey* spriteKey;

        // Get Sprite Keys only
        for (size_t i = 0; i < spriteKeys.Size(); ++i)
        {
            spriteKey = spriteKeys[i];
            sprite = GetMappedSprite(spriteKey->folderId_, spriteKey->fileId_);

            if (!sprite)
                continue;

            sprite = GetSwappedSprite(sprite);

            if (!sprite)
                continue;

            spritesKeys_.Push(spriteKey);
            spritesInfos_.Push(GetSpriteInfo(sprite));
        }

        if (!spritesInfos_.Size())
            URHO3D_LOGWARNINGF("AnimatedSprite2D() - CreateSpriteInfos : node=%s ... No Spriter Keys !", node_->GetName().CString());
    }

    return spritesInfos_;
}

bool AnimatedSprite2D::UpdateDrawRectangle()
{
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

    for (size_t i = 0; i < spriteKeys.Size(); ++i)
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

        Matrix2x3 localTransform(position * PIXEL_SIZE, angle, scale);
        sprite->GetDrawRectangle(drawRect, pivot);
        drawRect_.Merge(drawRect.Transformed(localTransform));
    }

    drawRectDirty_ = false;
    return true;
}


void AnimatedSprite2D::UpdateSourceBatches()
{
//    URHO3D_LOGINFOF("AnimatedSprite2D() - UpdateSourceBatches : node=%s(%u) ...", node_->GetName().CString(), node_->GetID());

    if (!renderEnabled_ || !visibility_)
    {
        sourceBatchesDirty_ = false;
//        URHO3D_LOGWARNINGF("AnimatedSprite2D() - UpdateSourceBatches : node=%s(%u) ... No Render - No Visibility !",
//                             node_->GetName().CString(), node_->GetID());
        return;
    }

    URHO3D_PROFILE(AnimatedSprite2D_Batch);

#ifdef URHO3D_SPINE
    if (skeleton_ && animationState_)
        UpdateSourceBatchesSpine();
#endif
    if (spriterInstance_ && spriterInstance_->GetAnimation())
    {
    #ifdef ANIM_WORLDBOUNDINGBOXUPDATE
        worldBoundingBoxDirty_ = drawRectDirty_ = true;
    #endif

        if (!UpdateDrawRectangle())
            return;

        if (renderer_->orthographicMode_)
        {
            if (renderNodes_.Size())
                UpdateSourceBatchesSpriter_RenderNodesOrthographic();
            else if (useCharacterMap_ || animationSet_->IsMultiTextures())
                UpdateSourceBatchesSpriter_MultiMaterials(GetNode()->GetWorldTransform2D(), sourceBatches_);
            else
                UpdateSourceBatchesSpriter_OneMaterial(GetNode()->GetWorldTransform2D(), sourceBatches_);
        }
        else
        {
            if (renderNodes_.Size())
                UpdateSourceBatchesSpriter_RenderNodes();
            else if (useCharacterMap_ || animationSet_->IsMultiTextures())
                UpdateSourceBatchesSpriter_MultiMaterials(GetNode()->GetWorldTransform(),sourceBatches_);
            else
                UpdateSourceBatchesSpriter_OneMaterial(GetNode()->GetWorldTransform(), sourceBatches_);
        }

        visibility_ = true;
    }
    else
        URHO3D_LOGWARNINGF("AnimatedSprite2D() - UpdateSourceBatches : node=%s(%u) enabled=%s no spriterInstance or no animation !",
                                    node_->GetName().CString(), node_->GetID(), IsEnabledEffective() ? "true" : "false");

	sourceBatchesDirty_ = false;
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

template< typename T > void AnimatedSprite2D::UpdateSourceBatchesSpriter_OneMaterial(const T& nodeWorldTransform, Vector<SourceBatch2D>& sourceBatches, bool resetBatches)
{
    const PODVector<Spriter::SpriteTimelineKey* >& spriteKeys = spriterInstance_->GetSpriteKeys();
    if (!spriteKeys.Size())
        UpdateSpriterAnimation(0.0f);

    if (!spriteKeys.Size())
        return;

    int iBatch = resetBatches ? 0 : sourceBatches.Size();

    sourceBatches.Resize(iBatch+1);
    sourceBatches[iBatch].vertices_.Clear();
    sourceBatches[iBatch].material_ = sourceBatches_[0].material_;

    assert(sourceBatches[iBatch].material_);

    sourceBatches[iBatch].drawOrder_ = iBatch > 0 ? sourceBatches[iBatch-1].drawOrder_ + renderZIndex_ : GetDrawOrder() + spriteKeys[0]->zIndex_;

    Vector<Vertex2D>& vertices = sourceBatches[iBatch].vertices_;
    for (size_t i = 0; i < spriteKeys.Size(); ++i)
    {
        Spriter::SpriteTimelineKey* spriteKey = spriteKeys[i];
        Sprite2D* sprite = animationSet_->GetSpriterFileSprite((spriteKey->folderId_ << 16) + spriteKey->fileId_);

        if (!sprite)
            continue;

        vertices.Resize(vertices.Size() + 4);

        const Spriter::SpatialInfo& spatial = spriteKey->info_;

        AddSprite(sprite, nodeWorldTransform, spatial.x_, spatial.y_, spriteKey->pivotX_, spriteKey->pivotY_, spatial.scaleX_, spatial.scaleY_,
                            spatial.angle_, spatial.alpha_, &vertices[vertices.Size()-4].position_.x_);
    }
}

template< typename T > void AnimatedSprite2D::UpdateSourceBatchesSpriter_MultiMaterials(const T& nodeWorldTransform, Vector<SourceBatch2D>& sourceBatches, int breakZIndex, bool resetBatches)
{
    const PODVector<SpriteInfo*>& spriteinfos = AnimatedSprite2D::GetSpriteInfos();
    if (!spriteinfos.Size())
        return;

    Material* material = renderer_->GetMaterial(spritesInfos_[0]->sprite_->GetTexture(), blendMode_);
    if (!material)
        return;

    int drawOrder = GetDrawOrder();
    if (resetBatches)
    {
        firstKeyIndex_ = 0;
        sourceBatches.Resize(1);
        sourceBatches[0].vertices_.Clear();
        sourceBatches[0].drawOrder_ = drawOrder + spritesKeys_[0]->zIndex_;
        sourceBatches[0].material_ = SharedPtr<Material>(material);
    }

    stopKeyIndex_ = spritesKeys_.Size();
    if (breakZIndex > 0)
    {
        if (firstKeyIndex_ == stopKeyIndex_)
            return;

        for (size_t i = firstKeyIndex_; i < stopKeyIndex_; ++i)
            if (spritesKeys_[i]->zIndex_ > breakZIndex)
            {
                stopKeyIndex_ = i;
                break;
            }
    }

    int iBatch = sourceBatches.Size()-1;
    Material* prevMaterial = resetBatches ? material : sourceBatches[iBatch].material_;
    Material* tmaterial;
    Texture2D *texture=0, *ttexture=0;
    Vector<Vertex2D>* vertices = &sourceBatches[iBatch].vertices_;

    for (size_t i = firstKeyIndex_; i < stopKeyIndex_; ++i)
    {
        const SpriteInfo* spriteinfo = spritesInfos_[i];
        Spriter::SpriteTimelineKey* spriteKey = spritesKeys_[i];

        ttexture = spriteinfo->sprite_->GetTexture();
        if (ttexture && texture != ttexture)
        {
            texture = ttexture;
            tmaterial = renderer_->GetMaterial(texture, blendMode_);

            if (!tmaterial)
                continue;

            material = tmaterial;
        }

        if (material != prevMaterial)
        {
            // Add new Batch
            iBatch++;
            sourceBatches.Resize(iBatch+1);
            sourceBatches[iBatch].vertices_.Clear();
            sourceBatches[iBatch].drawOrder_ = drawOrder + spriteKey->zIndex_;
            sourceBatches[iBatch].material_ = SharedPtr<Material>(material);
            prevMaterial = material;
            vertices = &sourceBatches[iBatch].vertices_;
        }

        vertices->Resize(vertices->Size() + 4);

        const Spriter::SpatialInfo& spatial = spriteKey->info_;

        AddSprite(spriteinfo->sprite_, nodeWorldTransform, spatial.x_, spatial.y_, spriteKey->pivotX_ + spriteinfo->deltaHotspotx_, spriteKey->pivotY_ + spriteinfo->deltaHotspoty_,
                  spatial.scaleX_ * spriteinfo->scalex_, spatial.scaleY_ * spriteinfo->scaley_, spatial.angle_, spatial.alpha_, &vertices->At(vertices->Size()-4).position_.x_);
    }

    firstKeyIndex_ = stopKeyIndex_;
}

template void AnimatedSprite2D::UpdateSourceBatchesSpriter_OneMaterial(const Matrix3x4& nodeWorldTransform, Vector<SourceBatch2D>& sourceBatches, bool resetBatches);
template void AnimatedSprite2D::UpdateSourceBatchesSpriter_OneMaterial(const Matrix2x3& nodeWorldTransform, Vector<SourceBatch2D>& sourceBatches, bool resetBatches);
template void AnimatedSprite2D::UpdateSourceBatchesSpriter_MultiMaterials(const Matrix3x4& nodeWorldTransform, Vector<SourceBatch2D>& sourceBatches, int breakZIndex, bool resetBatches);
template void AnimatedSprite2D::UpdateSourceBatchesSpriter_MultiMaterials(const Matrix2x3& nodeWorldTransform, Vector<SourceBatch2D>& sourceBatches, int breakZIndex, bool resetBatches);
template void AnimatedSprite2D::GetVertices(const IntVector2& size, const Matrix3x4& transform, PODVector<float>& verticeData);

void AnimatedSprite2D::UpdateSourceBatchesSpriter_RenderNodes()
{
    bool hasRendered = false;

    for (unsigned i=0; i < renderNodes_.Size(); i++)
    {
        AnimatedSprite2D* animToRender = renderNodes_[i]->GetComponent<AnimatedSprite2D>();
        if (animToRender)
        {
			if (!animToRender->GetSpriterInstance())
				continue;

            UpdateSourceBatchesSpriter_MultiMaterials(GetNode()->GetWorldTransform(), sourceBatches_, animToRender->renderZIndex_, i == 0);
            animToRender->UpdateSourceBatchesSpriter_OneMaterial(animToRender->GetNode()->GetWorldTransform(), sourceBatches_, false);
            hasRendered = true;
        }
    }

    UpdateSourceBatchesSpriter_MultiMaterials(GetNode()->GetWorldTransform(), sourceBatches_, -1, !hasRendered);
}

void AnimatedSprite2D::UpdateSourceBatchesSpriter_RenderNodesOrthographic()
{
    bool hasRendered = false;

    for (unsigned i=0; i < renderNodes_.Size(); i++)
    {
        AnimatedSprite2D* animToRender = renderNodes_[i]->GetComponent<AnimatedSprite2D>();
        if (animToRender)
        {
			if (!animToRender->GetSpriterInstance())
				continue;

            UpdateSourceBatchesSpriter_MultiMaterials(GetNode()->GetWorldTransform2D(), sourceBatches_, animToRender->renderZIndex_, i == 0);
            animToRender->UpdateSourceBatchesSpriter_OneMaterial(animToRender->GetNode()->GetWorldTransform2D(), sourceBatches_, false);
            hasRendered = true;
        }
    }

    UpdateSourceBatchesSpriter_MultiMaterials(GetNode()->GetWorldTransform2D(), sourceBatches_, -1, !hasRendered);
}

// Rotation 90° Orthographic
static Matrix2x3 sRotatedMatrixOrtho_(-4.37114e-08f, -1.f, 0.f, 1.f, -4.37114e-08f, 0.f);
// Rotation 90°
static Matrix3x4 sRotatedMatrix_(5.96046e-08f, -1.f, 0.f, 0.f, 1.f, 5.96046e-08f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f);

inline void AnimatedSprite2D::AddSprite(Sprite2D* sprite, const Matrix3x4& transform,
                                        float x, float y, float px, float py, float sx, float sy, float angle, float alpha, float* vertices)
{
    static Rect textureRect;
    if (!sprite->GetTextureRectangle(textureRect, flipX_, flipY_))
        return;

    static Matrix3x4 finaltransform, localTransform;
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

    localTransform.Set(Vector3(x, y, 0.f) * PIXEL_SIZE, Quaternion(angle), Vector3(sx, sy, 1.f));

    if (sprite->GetRotated())
    {
        // set the translation part
        sRotatedMatrix_.m03_ = -px * (float)sprite->GetSourceSize().x_ * PIXEL_SIZE;
        sRotatedMatrix_.m13_ = (1.f - py) * (float)sprite->GetSourceSize().y_ * PIXEL_SIZE;
        localTransform = localTransform * sRotatedMatrix_;
    }

    finaltransform = transform * localTransform;

    // use the custom hotspot at each time, don't flip again, pivot is already setted
    sprite->GetDrawRectangle(drawRect, Vector2(px, py));

    unsigned ucolor = Color(color_, alpha * color_.a_).ToUInt();

    position = finaltransform * Vector3(drawRect.min_, 0.0f);
    vertices[0] = position.x_;
    vertices[1] = position.y_;
    vertices[2] = 0.f;
    ((unsigned&)vertices[3]) = ucolor;
    vertices[4] = textureRect.min_.x_;
    vertices[5] = textureRect.min_.y_;

    position = finaltransform * Vector3(drawRect.min_.x_, drawRect.max_.y_, 0.0f);
    vertices[6] = position.x_;
    vertices[7] = position.y_;
    vertices[8] = 0.f;
    ((unsigned&)vertices[9]) = ucolor;
    vertices[10] = textureRect.min_.x_;
    vertices[11] = textureRect.max_.y_;

    position = finaltransform * Vector3(drawRect.max_, 0.0f);
    vertices[12] = position.x_;
    vertices[13] = position.y_;
    vertices[14] = 0.f;
    ((unsigned&)vertices[15]) = ucolor;
    vertices[16] = textureRect.max_.x_;
    vertices[17] = textureRect.max_.y_;

    position = finaltransform * Vector3(drawRect.max_.x_, drawRect.min_.y_, 0.0f);
    vertices[18] = position.x_;
    vertices[19] = position.y_;
    vertices[20] = 0.f;
    ((unsigned&)vertices[21]) = ucolor;
    vertices[22] = textureRect.max_.x_;
    vertices[23] = textureRect.min_.y_;
}

inline void AnimatedSprite2D::AddSprite(Sprite2D* sprite, const Matrix2x3& transform,
                                        float x, float y, float px, float py, float sx, float sy, float angle, float alpha, float* vertices)
{
    static Rect textureRect;
    if (!sprite->GetTextureRectangle(textureRect, flipX_, flipY_))
        return;

    static Matrix2x3 finaltransform, localTransform;
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

    localTransform.Set(Vector2(x, y) * PIXEL_SIZE, angle, Vector2(sx, sy));

    if (sprite->GetRotated())
    {
        // set the translation part
        sRotatedMatrixOrtho_.m02_ = -px * (float)sprite->GetSourceSize().x_ * PIXEL_SIZE;
        sRotatedMatrixOrtho_.m12_ = (1.f - py) * (float)sprite->GetSourceSize().y_ * PIXEL_SIZE;
        localTransform = localTransform * sRotatedMatrixOrtho_;
    }

    finaltransform = transform * localTransform;

    // use the custom hotspot at each time, don't flip again, pivot is already setted
    sprite->GetDrawRectangle(drawRect, Vector2(px, py));

    unsigned ucolor = Color(color_, alpha * color_.a_).ToUInt();

    position = finaltransform * drawRect.min_;
    vertices[0] = position.x_;
    vertices[1] = position.y_;
    vertices[2] = 0.f;
    ((unsigned&)vertices[3]) = ucolor;
    vertices[4] = textureRect.min_.x_;
    vertices[5] = textureRect.min_.y_;

    position = finaltransform * Vector2(drawRect.min_.x_, drawRect.max_.y_);
    vertices[6] = position.x_;
    vertices[7] = position.y_;
    vertices[8] = 0.f;
    ((unsigned&)vertices[9]) = ucolor;
    vertices[10] = textureRect.min_.x_;
    vertices[11] = textureRect.max_.y_;

    position = finaltransform * drawRect.max_;
    vertices[12] = position.x_;
    vertices[13] = position.y_;
    vertices[14] = 0.f;
    ((unsigned&)vertices[15]) = ucolor;
    vertices[16] = textureRect.max_.x_;
    vertices[17] = textureRect.max_.y_;

    position = finaltransform * Vector2(drawRect.max_.x_, drawRect.min_.y_);
    vertices[18] = position.x_;
    vertices[19] = position.y_;
    vertices[20] = 0.f;
    ((unsigned&)vertices[21]) = ucolor;
    vertices[22] = textureRect.max_.x_;
    vertices[23] = textureRect.min_.y_;
}

inline void AnimatedSprite2D::AddSprite_UI(Sprite2D* sprite, const IntVector2& size, const Matrix3x4& transform,
                                        float x, float y, float px, float py, float sx, float sy, float angle, float alpha, float* vertices)
{
    static Rect textureRect;
    if (!sprite->GetTextureRectangle(textureRect, flipX_, flipY_))
        return;

    static Matrix3x4 finaltransform, localTransform;
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

    localTransform.Set(Vector3(x, y, 0.f), Quaternion(angle), Vector3(sx, sy, 1.f));
    finaltransform = transform * localTransform;

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
        animationState_ = 0;
    }

    if (animationStateData_)
    {
        spAnimationStateData_dispose(animationStateData_);
        animationStateData_ = 0;
    }

    if (skeleton_)
    {
        spSkeleton_dispose(skeleton_);
        skeleton_ = 0;
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

    //entityName_.Clear();
    animationName_.Clear();
}


}
