#pragma once

#include <Urho3D/Core/Context.h>
#include <Urho3D/Graphics/GraphicsEvents.h>

#ifdef ACTIVE_CUSTOM_URHO

#include "Sprite2D.h"
#include "SpriteSheet2D.h"

#include "Drawable2D.h"
#include "Renderer2D.h"
#include "StaticSprite2D.h"
#include "SpriterData2D.h"
#include "SpriterInstance2D.h"
#include "AnimationSet2D.h"
#include "AnimatedSprite2D.h"
#include "ParticleEffect2D.h"
#include "ParticleEmitter2D.h"

#include "Text2D.h"
#include "AnimatedSprite.h"

void RegisterGraphics2DLibrary(Urho3D::Context* context);

#else

#include <Urho3D/Urho2D/Sprite2D.h>
#include <Urho3D/Urho2D/SpriteSheet2D.h>

#include <Urho3D/Urho2D/Drawable2D.h>
#include <Urho3D/Urho2D/Renderer2D.h>
#include <Urho3D/Urho2D/StaticSprite2D.h>
#include <Urho3D/Urho2D/SpriterData2D.h>
#include <Urho3D/Urho2D/SpriterInstance2D.h>
#include <Urho3D/Urho2D/AnimationSet2D.h>
#include <Urho3D/Urho2D/AnimatedSprite2D.h>
#include <Urho3D/Urho2D/ParticleEffect2D.h>
#include <Urho3D/Urho2D/ParticleEmitter2D.h>

#endif
