#pragma once

#include <Urho3D/Container/ArrayPtr.h>
#include <Urho3D/Resource/Resource.h>

#ifdef URHO3D_SPINE
struct spAtlas;
struct spSkeletonData;
struct spAnimationStateData;
#endif

namespace Urho3D
{
    class Sprite2D;
    class SpriteSheet2D;
}

namespace Spriter
{
    struct Entity;
    struct Ref;
    struct SpriterData;
    struct CharacterMap;
}

using namespace Urho3D;

/// Spriter animation set, it includes one or more animations, for more information please refer to http://www.esotericsoftware.com and http://www.brashmonkey.com/spriter.htm.
class AnimationSet2D : public Resource
{
    URHO3D_OBJECT(AnimationSet2D, Resource);

public:
    /// Construct.
    AnimationSet2D(Context* context);
    /// Destruct.
    virtual ~AnimationSet2D();
    /// Register object factory.
    static void RegisterObject(Context* context);

    /// Load resource from stream. May be called from a worker thread. Return true if successful.
    virtual bool BeginLoad(Deserializer& source);
    /// Finish resource loading. Always called from the main thread. Return true if successful.
    virtual bool EndLoad();

    /// Get number of animations.
    unsigned GetNumAnimations() const;
    /// Return animation name.
    const String& GetAnimation(unsigned index) const;
    /// Check has animation.
    bool HasAnimation(const String& animation) const;

    /// Return sprite.
    Sprite2D* GetSprite() const;
    Sprite2D* GetSprite(const String& name) const;

#ifdef URHO3D_SPINE
    Sprite2D* GetSpineSprite() const;
    /// Return spine skeleton data.
    spSkeletonData* GetSkeletonData() const { return skeletonData_; }
#endif

    /// Return spriter data.
    Spriter::SpriterData* GetSpriterData() const { return spriterData_.Get(); }
    /// Return spriter file sprite.
    Sprite2D* GetSpriterFileSprite(int folderId, int fileId) const;
    Sprite2D* GetSpriterFileSprite(unsigned key) const;

    Sprite2D* GetCharacterMapSprite(const Spriter::CharacterMap* characterMap, unsigned index) const;
    void GetCharacterMapSprites(const Spriter::CharacterMap* characterMap, PODVector<Sprite2D*>& sprites);
    void GetSpritesCharacterMapRef(Spriter::CharacterMap* characterMap, ResourceRefList& spriteRefList);

    SpriteSheet2D* GetSpriteSheet() const { return spriteSheet_; }
    const HashMap<unsigned, SharedPtr<Sprite2D> >& GetSpriteMapping() const { return spriterFileSprites_; }

    bool HasSpriteSheet() const { return hasSpriteSheet_; }
    bool IsMultiTextures() const { return mutliTextures_; }

    static String customSpritesheetFile_;

private:
    /// Return sprite by hash.
    Sprite2D* GetSpriterFileSprite(const StringHash& hash) const;
#ifdef URHO3D_SPINE
    /// Begin load spine.
    bool BeginLoadSpine(Deserializer& source);
    /// Finish load spine.
    bool EndLoadSpine();
#endif
    /// Begin load scml.
    bool BeginLoadSpriter(Deserializer& source);
    /// Finish load scml.
    bool EndLoadSpriter();
    /// Dispose all data.
    void Dispose();

    SharedPtr<Sprite2D> sprite_;

#ifdef URHO3D_SPINE
    SharedPtr<Sprite2D> spineSprite_;
    /// Spine json data.
    SharedArrayPtr<char> jsonData_;
    /// Spine skeleton data.
    spSkeletonData* skeletonData_;
    /// Spine atlas.
    spAtlas* atlas_;
#endif

    /// Spriter data.
    UniquePtr<Spriter::SpriterData> spriterData_;
    /// Has sprite sheet.
    bool hasSpriteSheet_;
    /// Sprite sheet file path.
    String spriteSheetFilePath_;
    /// Sprite sheet.
    SharedPtr<SpriteSheet2D> spriteSheet_;
    /// Spriter sprites.
    HashMap<unsigned, SharedPtr<Sprite2D> > spriterFileSprites_;

    bool mutliTextures_;
};


