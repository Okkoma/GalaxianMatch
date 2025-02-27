#include <Urho3D/Urho3D.h>

#include <Urho3D/Core/Context.h>
#include <Urho3D/Core/Profiler.h>

#include <Urho3D/IO/File.h>
#include <Urho3D/IO/FileSystem.h>
#include <Urho3D/IO/Log.h>

#include <Urho3D/Graphics/Graphics.h>
#include <Urho3D/Graphics/Texture2D.h>
#include <Urho3D/Math/AreaAllocator.h>

#include <Urho3D/Resource/Image.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Resource/XMLFile.h>

#ifdef URHO3D_SPINE
#include <spine/spine.h>
#include <spine/extension.h>
#endif

#include "Sprite2D.h"
#include "SpriteSheet2D.h"
#include "SpriterData2D.h"
#include "AnimationSet2D.h"

#ifdef URHO3D_SPINE
Urho3D::Context* urho2dSpineContext_ = nullptr;

void _spAtlasPage_createTexture(spAtlasPage* self, const char* path)
{
    using namespace Urho3D;
    if (!urho2dSpineContext_)
        return;

    ResourceCache* cache = urho2dSpineContext_->GetSubsystem<ResourceCache>();
    Sprite2D* sprite = cache->GetResource<Sprite2D>(path);
    // Add reference
    if (sprite)
        sprite->AddRef();

    self->width = sprite->GetTexture()->GetWidth();
    self->height = sprite->GetTexture()->GetHeight();

    self->rendererObject = sprite;
}

void _spAtlasPage_disposeTexture(spAtlasPage* self)
{
    using namespace Urho3D;
    Sprite2D* sprite = static_cast<Sprite2D*>(self->rendererObject);
    if (sprite)
        sprite->ReleaseRef();

    self->rendererObject = 0;
}

char* _spUtil_readFile(const char* path, int* length)
{
    using namespace Urho3D;

    if (!urho2dSpineContext_)
        return 0;

    ResourceCache* cache = urho2dSpineContext_->GetSubsystem<ResourceCache>();
    SharedPtr<File> file = cache->GetFile(path);
    if (!file)
        return 0;

    unsigned size = file->GetSize();

    char* data = MALLOC(char, size + 1);
    file->Read(data, size);
    data[size] = '\0';

    file.Reset();
    *length = size;

    return data;
}
#endif


String AnimationSet2D::customSpritesheetFile_;

AnimationSet2D::AnimationSet2D(Context* context) :
    Resource(context),
#ifdef URHO3D_SPINE
    skeletonData_(0),
    atlas_(0),
#endif
    hasSpriteSheet_(false),
    mutliTextures_(false)
{
    // Check has custom sprite sheet
    if (!customSpritesheetFile_.Empty())
    {
        if (GetSubsystem<ResourceCache>()->Exists(customSpritesheetFile_))
        {
            spriteSheetFilePath_ = customSpritesheetFile_;
            hasSpriteSheet_ = true;
        }
        else
        {
            spriteSheetFilePath_.Clear();
            hasSpriteSheet_ = false;
        }
    }
}

AnimationSet2D::~AnimationSet2D()
{
    Dispose();
}

void AnimationSet2D::RegisterObject(Context* context)
{
    context->RegisterFactory<AnimationSet2D>();
}

bool AnimationSet2D::BeginLoad(Deserializer& source)
{
    Dispose();

    if (GetName().Empty())
        SetName(source.GetName());

    String extension = GetExtension(source.GetName());
#ifdef URHO3D_SPINE
    if (extension == ".json")
        return BeginLoadSpine(source);
#endif
    if (extension == ".scml")
        return BeginLoadSpriter(source);

    URHO3D_LOGERROR("Unsupport animation set file: " + source.GetName());

    return false;
}

bool AnimationSet2D::EndLoad()
{
#ifdef URHO3D_SPINE
    if (jsonData_)
        return EndLoadSpine();
#endif
    if (spriterData_)
        return EndLoadSpriter();

    return false;
}

unsigned AnimationSet2D::GetNumAnimations() const
{
#ifdef URHO3D_SPINE
    if (skeletonData_)
        return (unsigned)skeletonData_->animationsCount;
#endif
    if (spriterData_ && !spriterData_->entities_.Empty())
        return (unsigned)spriterData_->entities_[0]->animations_.Size();
    return 0;
}

const String& AnimationSet2D::GetAnimation(unsigned index) const
{
    if (index >= GetNumAnimations())
        return String::EMPTY;

#ifdef URHO3D_SPINE
    if (skeletonData_)
        return skeletonData_->animations[index]->name;
#endif
    if (spriterData_ && !spriterData_->entities_.Empty())
        return spriterData_->entities_[0]->animations_[index]->name_;

    return String::EMPTY;
}

bool AnimationSet2D::HasAnimation(const String& animationName) const
{
#ifdef URHO3D_SPINE
    if (skeletonData_)
    {
        for (int i = 0; i < skeletonData_->animationsCount; ++i)
        {
            if (animationName == skeletonData_->animations[i]->name)
                return true;
        }
    }
#endif
    if (spriterData_ && !spriterData_->entities_.Empty())
    {
        const PODVector<Spriter::Animation*>& animations = spriterData_->entities_[0]->animations_;
        for (unsigned i = 0; i < animations.Size(); ++i)
        {
            if (animationName == animations[i]->name_)
                return true;
        }
    }

    return false;
}

#ifdef URHO3D_SPINE
Sprite2D* AnimationSet2D::GetSpineSprite() const
{
    return spineSprite_;
}
#endif

Sprite2D* AnimationSet2D::GetSprite() const
{
#ifdef URHO3D_SPINE
    if (spineSprite_)
        return spineSprite_;
#endif
    return sprite_;
}

Sprite2D* AnimationSet2D::GetSprite(const String& name) const
{
    return spriteSheet_ ? spriteSheet_->GetSprite(name) : nullptr;
}

Sprite2D* AnimationSet2D::GetSpriterFileSprite(int folderId, int fileId) const
{
    if (folderId == -1)
        return nullptr;

    unsigned key = (folderId << 16) + fileId;
    HashMap<unsigned, SharedPtr<Sprite2D> >::ConstIterator i = spriterFileSprites_.Find(key);
    if (i != spriterFileSprites_.End())
        return i->second_;

    return nullptr;
}

Sprite2D* AnimationSet2D::GetSpriterFileSprite(unsigned key) const
{
    HashMap<unsigned, SharedPtr<Sprite2D> >::ConstIterator i = spriterFileSprites_.Find(key);
    if (i != spriterFileSprites_.End())
        return i->second_;

    return nullptr;
}

Sprite2D* AnimationSet2D::GetCharacterMapSprite(const Spriter::CharacterMap* characterMap, unsigned index) const
{
    if (!characterMap)
        return nullptr;

    Spriter::MapInstruction* map = characterMap->maps_[index];
    return GetSpriterFileSprite(map->targetFolder_, map->targetFile_);
}

void AnimationSet2D::GetCharacterMapSprites(const Spriter::CharacterMap* characterMap, PODVector<Sprite2D*>& sprites)
{
    if (!characterMap)
        return;

    const PODVector<Spriter::MapInstruction*>& map = characterMap->maps_;

    sprites.Clear();
    sprites.Resize(map.Size());

    for (unsigned i=0; i< map.Size(); ++i)
        sprites[i] = GetSpriterFileSprite(map[i]->targetFolder_, map[i]->targetFile_);
}

void AnimationSet2D::GetSpritesCharacterMapRef(Spriter::CharacterMap* characterMap, ResourceRefList& spriteRefList)
{

}

#ifdef URHO3D_SPINE
bool AnimationSet2D::BeginLoadSpine(Deserializer& source)
{
    if (GetName().Empty())
        SetName(source.GetName());

    unsigned size = source.GetSize();
    jsonData_ = new char[size + 1];
    source.Read(jsonData_, size);
    jsonData_[size] = '\0';
    SetMemoryUse(size);
    return true;
}

bool AnimationSet2D::EndLoadSpine()
{
    currentAnimationSet = this;

    String atlasFileName = ReplaceExtension(GetName(), ".atlas");
    atlas_ = spAtlas_createFromFile(atlasFileName.CString(), 0);
    if (!atlas_)
    {
        URHO3D_LOGERROR("Create spine atlas failed");
        return false;
    }

    int numAtlasPages = 0;
    spAtlasPage* atlasPage = atlas_->pages;
    while (atlasPage)
    {
        ++numAtlasPages;
        atlasPage = atlasPage->next;
    }

    if (!numAtlasPages)
    {
        URHO3D_LOGERROR("no page loaded in atlas");
        return false;
    }
    if (numAtlasPages > 1)
    {
        URHO3D_LOGERROR("Only one page is supported in Urho3D");
        return false;
    }

    spineSprite_ = static_cast<Sprite2D*>(atlas_->pages->rendererObject);

    spSkeletonJson* skeletonJson = spSkeletonJson_create(atlas_);
    if (!skeletonJson)
    {
        URHO3D_LOGERROR("Create skeleton Json failed");
        return false;
    }

    skeletonJson->scale = 0.01f; // PIXEL_SIZE;
    skeletonData_ = spSkeletonJson_readSkeletonData(skeletonJson, &jsonData_[0]);

    spSkeletonJson_dispose(skeletonJson);
    jsonData_.Reset();

    currentAnimationSet = nullptr;

    return true;
}
#endif

bool AnimationSet2D::BeginLoadSpriter(Deserializer& source)
{
    unsigned dataSize = source.GetSize();
    if (!dataSize && !source.GetName().Empty())
    {
        URHO3D_LOGERROR("Zero sized XML data in " + source.GetName());
        return false;
    }

    SharedArrayPtr<char> buffer(new char[dataSize]);
    if (source.Read(buffer.Get(), dataSize) != dataSize)
        return false;

    spriterData_ = new Spriter::SpriterData();
    if (!spriterData_->Load(buffer.Get(), dataSize))
    {
        URHO3D_LOGERROR("Could not spriter data from " + source.GetName());
        return false;
    }

    // Check has sprite sheet
    String parentPath = GetParentPath(GetName());
    ResourceCache* cache = GetSubsystem<ResourceCache>();

    if (spriteSheetFilePath_.Empty())
    {
        String filename = parentPath + GetFileName(GetName());
        String extension = ".xml";
        hasSpriteSheet_ = cache->Exists(filename + extension);
        if (!hasSpriteSheet_)
        {
            extension = ".sjson";
            hasSpriteSheet_ = cache->Exists(filename + extension);
            if (!hasSpriteSheet_)
            {
                extension = ".plist";
                hasSpriteSheet_ = cache->Exists(filename + extension);
            }
        }

        if (hasSpriteSheet_)
            spriteSheetFilePath_ = filename + extension;
        else
            URHO3D_LOGERRORF("AnimationSet2D : this=%u - Could not find spritesheet files=%s (xml, sjson, plist)", this, filename.CString());
    }

    if (GetAsyncLoadState() == ASYNC_LOADING)
    {
        if (hasSpriteSheet_)
            cache->BackgroundLoadResource<SpriteSheet2D>(spriteSheetFilePath_, true, this);
        else
        {
            for (unsigned i = 0; i < spriterData_->folders_.Size(); ++i)
            {
                Spriter::Folder* folder = spriterData_->folders_[i];
                for (unsigned j = 0; j < folder->files_.Size(); ++j)
                {
                    Spriter::File* file = folder->files_[j];
                    String imagePath = parentPath + file->name_;
                    cache->BackgroundLoadResource<Image>(imagePath, true, this);
                }
            }
        }
    }

    // Note: this probably does not reflect internal data structure size accurately
    SetMemoryUse(dataSize);

    return true;
}

struct SpriterInfoFile
{
    int x;
    int y;
    int texid_;
    Spriter::File* file_;
    SharedPtr<Image> image_;
};

bool AnimationSet2D::EndLoadSpriter()
{
    if (!spriterData_)
        return false;

    ResourceCache* cache = GetSubsystem<ResourceCache>();
    if (hasSpriteSheet_)
    {
        spriteSheet_ = cache->GetResource<SpriteSheet2D>(spriteSheetFilePath_);
        if (!spriteSheet_)
            return false;

        spriterFileSprites_.Clear();
        for (unsigned i = 0; i < spriterData_->folders_.Size(); ++i)
        {
            Spriter::Folder* folder = spriterData_->folders_[i];
            for (unsigned j = 0; j < folder->files_.Size(); ++j)
            {
                Spriter::File* file = folder->files_[j];
                SharedPtr<Sprite2D> sprite(spriteSheet_->GetSprite(GetFileName(file->name_)));

                if (sprite)
                {
                    Vector2 hotSpot(file->pivotX_, file->pivotY_);

                    // If sprite is trimmed, recalculate hot spot
                    const IntVector2& offset = sprite->GetOffset();
                    if (offset != IntVector2::ZERO)
                    {
                        float pivotX = file->width_ * hotSpot.x_;
                        float pivotY = file->height_ * (1.0f - hotSpot.y_);

                        hotSpot.x_ = ((float)offset.x_ + pivotX) / sprite->GetSourceSize().x_;
                        hotSpot.y_ = 1.0f - ((float)offset.y_ + pivotY) / sprite->GetSourceSize().y_;
                    }

                    sprite->SetHotSpot(hotSpot);
                }

                unsigned key = (folder->id_ << 16) + file->id_;
                spriterFileSprites_[key] = sprite;
            }
        }

        if (!sprite_ && !spriterFileSprites_.Empty())
            sprite_ = spriterFileSprites_.Front().second_;
        if (!sprite_)
            sprite_ = spriteSheet_->GetSpriteMapping().Front().second_;

        mutliTextures_ = spriteSheet_->GetTextures().Size() > 1;
    }
    else
    {
        unsigned numTextures = 0U;
        String parentPath = GetParentPath(GetName());

//        URHO3D_LOGINFOF("no SpriteSheet =>");

        for (unsigned folderid = 0; folderid < spriterData_->folders_.Size(); ++folderid)
        {
            Vector<SpriterInfoFile> spriteInfos;
            Spriter::Folder* folder = spriterData_->folders_[folderid];
            const PODVector<Spriter::File*>& files = folder->files_;
            for (unsigned j = 0; j < files.Size(); ++j)
            {
                Spriter::File* file = files[j];
                String imagePath = parentPath + file->name_;
                SharedPtr<Image> image(cache->GetResource<Image>(imagePath));
                if (!image)
                {
                    URHO3D_LOGERROR("Could not load image");
                    continue;
                }
                if (image->IsCompressed())
                {
                    URHO3D_LOGERROR("Compressed image is not support");
                    continue;
                }
                if (image->GetComponents() != 4)
                {
                    URHO3D_LOGERROR("Only support image with 4 components");
                    continue;
                }

                SpriterInfoFile def;
                def.x = -1;
                def.y = -1;
                def.texid_ = -1;
                def.file_ = file;
                def.image_ = image;
                spriteInfos.Push(def);
            }

            if (spriteInfos.Empty())
                continue;

            if (spriteInfos.Size() > 1)
            {
                URHO3D_LOGINFOF("AnimationSet2D() - EndLoadSpriter : create texture ...");

                Vector<IntVector2> texturesSizes;

                // Dispatch Sprites on Textures
                {
                    AreaAllocator allocator(128, 128, 2048, 2048);
                    for (unsigned i = 0; i < spriteInfos.Size(); ++i)
                    {
                        SpriterInfoFile& info = spriteInfos[i];
                        if (!allocator.Allocate(info.image_->GetWidth() + 1, info.image_->GetHeight() + 1, info.x, info.y))
                        {
                            texturesSizes.Push(IntVector2(allocator.GetWidth(), allocator.GetHeight()));
                            allocator.Reset(128, 128, 2048, 2048);
                            if (!allocator.Allocate(info.image_->GetWidth() + 1, info.image_->GetHeight() + 1, info.x, info.y))
                            {
                                URHO3D_LOGERRORF("AnimationSet2D() - EndLoadSpriter : can't allocate texture !");
                                continue;
                            }
                        }
                        info.texid_ = texturesSizes.Size();
                    }

                    // force squared textures
                    int size = Max(allocator.GetWidth(), allocator.GetHeight());
                    texturesSizes.Push(IntVector2(size, size));
                }

                // Allocate Textures
                for (unsigned i = 0; i < texturesSizes.Size(); ++i)
                {
                    IntVector2 size = texturesSizes[i];
                    SharedPtr<Texture2D> texture(new Texture2D(context_));

                    texture->SetMipsToSkip(QUALITY_LOW, 0);
                    texture->SetNumLevels(1);
                    texture->SetSize(size.x_, size.y_, Graphics::GetRGBAFormat());

                    unsigned textureDataSize = size.x_ * size.y_* 4;
                    SharedArrayPtr<unsigned char> textureData(new unsigned char[textureDataSize]);
                    memset(textureData.Get(), 0, textureDataSize);

                    SharedPtr<Image> image(new Image(context_));
                    image->SetSize(size.x_, size.y_, 4);
                    image->Clear(Color(0,0,0,0));
                    for (unsigned j = 0; j < spriteInfos.Size(); ++j)
                    {
                        SpriterInfoFile& info = spriteInfos[j];
                        if (info.texid_ != i)
                            continue;

                        Image* currentimage = info.image_;

                        for (int y = 0; y < currentimage->GetHeight(); ++y)
                        {
                            memcpy(textureData.Get() + ((info.y + y) * texturesSizes[i].x_ + info.x) * 4,
                                currentimage->GetData() + y * currentimage->GetWidth() * 4, currentimage->GetWidth() * 4);
                        }

                        SharedPtr<Sprite2D> sprite(new Sprite2D(context_));
                        sprite->SetName(currentimage->GetName());
                        sprite->SetTexture(texture);
                        IntRect rectangle(info.x, info.y, info.x + currentimage->GetWidth(), info.y + currentimage->GetHeight());
                        sprite->SetRectangle(rectangle);
                        sprite->SetSourceSize(currentimage->GetWidth(), currentimage->GetHeight());
                        sprite->SetHotSpot(Vector2(info.file_->pivotX_, info.file_->pivotY_));
                        unsigned key = (info.file_->folder_->id_ << 16) + info.file_->id_;
                        spriterFileSprites_[key] = sprite;

                        image->SetSubimage(currentimage, rectangle);
                    }

                    texture->SetData(0, 0, 0, texturesSizes[i].x_, texturesSizes[i].y_, textureData.Get());
                    texture->SetName(GetName() + String(folderid) + String(i));

                    image->SetName(texture->GetName());
                    cache->AddManualResource(image);
                    URHO3D_LOGERRORF("AnimationSet2D() - EndLoadSpriter : AddManualResource image=%s", image->GetName().CString());

                    numTextures++;
                }
            }
            else
            {
                SharedPtr<Texture2D> texture(new Texture2D(context_));
                texture->SetMipsToSkip(QUALITY_LOW, 0);
                texture->SetNumLevels(1);

                SpriterInfoFile& info = spriteInfos[0];
                texture->SetData(info.image_, true);
                texture->SetName(info.image_->GetName());

                sprite_ = new Sprite2D(context_);
                sprite_->SetTexture(texture);
                sprite_->SetRectangle(IntRect(info.x, info.y, info.x + info.image_->GetWidth(), info.y + info.image_->GetHeight()));
                sprite_->SetSourceSize(info.image_->GetWidth(), info.image_->GetHeight());
                sprite_->SetHotSpot(Vector2(info.file_->pivotX_, info.file_->pivotY_));

                unsigned key = (info.file_->folder_->id_ << 16) + info.file_->id_;
                spriterFileSprites_[key] = sprite_;

                if (!cache->GetExistingResource<Image>(info.image_->GetName()))
                {
                    cache->AddManualResource(info.image_);
                    URHO3D_LOGERRORF("AnimationSet2D() - EndLoadSpriter : AddManualResource image=%s", info.image_->GetName().CString());
                }

                numTextures++;
            }
        }

        mutliTextures_ = numTextures > 1;
        sprite_ = spriterFileSprites_.Front().second_;
    }

    return true;
}

void AnimationSet2D::Dispose()
{
#ifdef URHO3D_SPINE
    spineSprite_.Reset();
    if (skeletonData_)
    {
        spSkeletonData_dispose(skeletonData_);
        skeletonData_ = 0;
    }

    if (atlas_)
    {
        spAtlas_dispose(atlas_);
        atlas_ = 0;
    }
#endif

    sprite_.Reset();
    spriterData_.Reset();
    spriterFileSprites_.Clear();
    spriteSheet_.Reset();
}

