#include <Urho3D/Urho3D.h>

#include "Graphics2D.h"


void RegisterGraphics2DLibrary(Urho3D::Context* context)
{
    Urho3D::Sprite2D::RegisterObject(context);
    Urho3D::SpriteSheet2D::RegisterObject(context);

    Renderer2D::RegisterObject(context);

    Drawable2D::RegisterObject(context);
    StaticSprite2D::RegisterObject(context);

    AnimationSet2D::RegisterObject(context);
    AnimatedSprite2D::RegisterObject(context);

    ParticleEffect2D::RegisterObject(context);
    ParticleEmitter2D::RegisterObject(context);

    Text2D::RegisterObject(context);
    
    AnimatedSprite::RegisterObject(context);
}

