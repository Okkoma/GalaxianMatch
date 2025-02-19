#include <Urho3D/Urho3D.h>

#include "Graphics2D.h"


void RegisterGraphics2DLibrary(Urho3D::Context* context)
{
    Urho3D::Text2D::RegisterObject(context);
}
