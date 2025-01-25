#pragma once

#include <cstdarg>

#include <Urho3D/Graphics/Light.h>

#include "DefsGame.h"

namespace Urho3D
{
class Camera;
class UIElement;
class Sprite2D;
class Font;
class Text;
class MessageBox;
class AnimatedSprite2D;
}

using namespace Urho3D;

class AnimatedSprite;
class TextMessage;
class InteractiveFrame;

enum
{
    GAMELOG_PRELOAD = 1 << 0,
    GAMELOG_MAPPRELOAD = 1 << 1,
    GAMELOG_MAPCREATE = 1 << 2,
    GAMELOG_MAPUNLOAD = 1 << 3,
    GAMELOG_WORLDUPDATE = 1 << 4,
    GAMELOG_WORLDVISIBLE = 1 << 5,
    GAMELOG_PLAYER = 1 << 6,
};



class GameHelpers
{

public:

    static unsigned ToUIntAddr(void* value)
    {
        return (unsigned)(size_t)value;
    }

    /// Log Helpers
    static void SetGameLogFilter(unsigned logfilter);
    static unsigned GetGameLogFilter();
    static void ResetGameLog(Context* context);
    static void SetGameLogEnable(Context* context, unsigned filterbits, bool state);

    /// Serialize Data Helpers
    static bool LoadData(Context* context, const char* fileName, void* data, bool allocate = false);
    static bool SaveData(Context* context, void* data, unsigned size, const char* fileName);
    static char* GetTextFileToBuffer(const char* fileName, size_t& s);

    /// Serialize Scene/Node Helpers
    static bool LoadSceneXML(Context* context, Node* node, CreateMode mode, const String& fileName);
    static bool SaveSceneXML(Context* context, Scene* scene, const char* fileName);
    static bool LoadNodeXML(Context* context, Node* node, const String& fileName, CreateMode mode=LOCAL, bool applyAttr=true);
    static bool SaveNodeXML(Context* context, Node* node, const char* fileName);

    /// Preload Node Resources
    static bool PreloadXMLResourcesFrom(Context* context, const String& fileName);

    /// Node Attributes Helpers
    static void LoadNodeAttributes(Node* node, const NodeAttributes& nodeAttr, bool applyAttr=true);
    static void SaveNodeAttributes(Node* node, NodeAttributes& nodeAttr);
    static const Variant& GetNodeAttributeValue(const StringHash& attr, const NodeAttributes& nodeAttr, unsigned index=0);
    static void CopyAttributes(Node* source, Node* dest, bool apply=true, bool removeUnusedComponents=true);

    /// Scene/Node Remover Helpers
    static bool CleanScene(Scene* scene, const String& stateToClean, int step);
    static void RemoveNodeSafe(Node* node, bool dumpnodes=false);

    /// Spawn Node Helpers
    static Node* SpawnGOtoNode(Context* context, StringHash type, Node* scenenode);
    static Node* SpawnParticleEffectInNode(Context* context, const String& effectName, int layer, Node* node, const Vector2& direction, float fscale=1.f, bool removeTimer=true, float duration = -1.0f, const Color& color=Color::WHITE, CreateMode mode = REPLICATED);
    static void SpawnParticleEffect(Context* context, const String& effectName, int layer, const Vector2& position, float worldAngle = 0.f, float worldScale = 1.f, bool removeTimer=true, float duration = -1.0f, const Color& color=Color::WHITE, CreateMode mode = REPLICATED);
    static void SpawnParticleEffect(Context* context, Node* node, const Vector2& direction, int layer, const String& effectName, bool removeTimer=true, float duration = -1.0f, CreateMode mode = REPLICATED);
    static void AddEffects(Node* root, int index, int num, float x, float y, float delaybetweenspawns, float removedelay, int layer);

    /// Component Helpers
    static Node* AddStaticSprite2D(Node* rootnode, CreateMode mode, const char *nodename, const char *texturename, const Vector3& position, const Vector3& scale, int layer, const char *materialname=0);
    static AnimatedSprite2D* AddAnimatedSprite2D(Node* root, const String& label, const String& spritename, const String& entityname,
                                          const Vector2& position, float rotation, bool reactive=false, const Vector2& reactivecenter=Vector2::ZERO, float reactiveradius=0.f);
    static void AddPlane(const char *nodename, Context* context, Node* node, const char *materialname,const Vector3& pos, const Vector2& scale, CreateMode mode = REPLICATED);
    static void AddLight(Context* context, Node *node, LightType type, const Vector3& relativePosition=Vector3::ZERO,const Vector3& direction=Vector3::FORWARD, const float& fov=60.0f, const float& range=10.0f,
                         float brightness=1.0f, bool pervertex=false, bool colorAnimated=false);
    static void AddAnimation(Node* node, const String& attribute, const Variant& value1, const Variant& value2, float time, WrapMode mode = WM_ONCE);
    static void SetMoveAnimation(Node* node, const Vector3& from, const Vector3& to, float start, float delay);

    /// UI Helpers
    static void ResetCamera();
    static void ApplySizeRatio(int w, int h, IntVector2& size);
    static void ApplySizeRatioMaximized(int w, int h, IntVector2& size);
    static void AddText3DFadeAnim(Node* rootNode, const String& text, Text* originaltext, const Vector3& deltamove, float fadetime, float scalefactor);
    static void AddTextFadeAnim(UIElement* uiroot, const String& text, Text* originaltext, const IntVector2& deltamove, float fadetime, float scalefactor);
    static TextMessage* AddUIMessage(const String& text, bool localize, const String& font, int fontsize, const Color& color, const IntVector2& position, float duration, float delaystart=0.f);
    static MessageBox* AddMessageBox(const String& title, const String& question, bool questionl10, const String& answer1, const String& answer2, Object* subscriber, EventHandler* handler, XMLFile* layoutFile = 0);
    static InteractiveFrame* AddInteractiveFrame(const String& framefile, Object* subscriber=0, EventHandler* handler=0, bool autostart=true);
    static AnimatedSprite* AddAnimatedSpriteUI(UIElement* root, const String& label, const String& spritename, const String& entityname, const Vector2& position, float rotation=0.f);
    static void SetMoveAnimationUI(UIElement* elt, const IntVector2& from, const IntVector2& to, float start, float delay);
    static void SetScaleAnimationUI(UIElement* elt, float from, float to, float mid, float start, float delay);
    static void FindParentWithAttribute(const StringHash& attribute, UIElement* element, UIElement*& parent, Variant& var);
    static void FindParentWhoContainsName(const String& name, UIElement* element, UIElement*& parent);
    static void SetUIScale(UIElement* root, const Vector2& scale);

    /// Helper function to generate a random ID
    static String GetRandomString(unsigned size);

private:
    static void SetScaleChildRecursive(UIElement* elt, const Vector2& scale);

public:
    /// Input Helpers
    static void GetInputPosition(int& x, int& y, UIElement** uielt=0);

    /// Math / Converter Helpers
    static float GetMinAdjustedScale(Vector2 framesize, Vector2 imagesize);
    static float GetMaximizedScaleWithImageRatioKeeped(Vector2 framesize, Vector2 imagesize);
    static Vector2 GetMinAdjustedSize(Vector2 framesize, Vector2 imagesize);
    static float GetMaxAdjustedScale(Vector2 framesize, Vector2 imagesize);
    static Vector2 GetMaxAdjustedSize(Vector2 framesize, Vector2 imagesize);

    static void SetAdjustedToScreen(Node* node, float basescale=1.f, float minscale=1.f, bool maximize=true);
    static void ClampSizeTo(IntVector2& size, int dimension);
    static Vector2 ScreenToWorld2D(int screenx, int screeny);
    static Vector2 ScreenToWorld2D(const IntVector2& screenpoint);
    static Vector2 ScreenToWorld2D_FixedZoom(const IntVector2& screenpoint);
    static void OrthoWorldToScreen(IntVector2& point, const Vector3& position);
    static void OrthoWorldToScreen(IntVector2& point, Node* node);
    static void OrthoWorldToScreen(IntRect& rect, const BoundingBox& box);
    static void SpriterCorrectorScaleFactor(Context* context, const char *filename, const float& scale_factor);
    static void EqualizeValues(PODVector<float>& values, int coeff=1, bool incbleft=true, bool incbright=true);
    template< typename T > static bool FloodFill(Matrix2D<T>& buffer, Stack<unsigned>& stack, const T& patternToFill, const T& fillPattern, const int& xo, const int& yo);
    template< typename T > static bool FloodFill(Matrix2D<T>& buffer, Stack<unsigned>& stack, const T& patternToFillFrom, const T& patternToFillTo, const T& fillPattern, const int& xo, const int& yo);

    /// Audio Helpers
    static void SpawnSound(Node* node, const char* fileName, float gain = 1.f);
    static void SpawnSound(Node* node, int soundid, float gain = 1.f);
    static void SpawnSound3D(Node* node, const char* fileName, float gain = 1.f);
    static void SetMusic(int channel, float gain = 0.7f, int music=-1, bool loop=true);
    static void StopMusic(int channel);
    static void StopMusics();
    static void StopSound(Node* node);
    static void SetSoundVolume(float volume);
    static void SetMusicVolume(float volume);

    /// Dump Helpers
    static void AppendBufferToString(String& string, const char* format, ...);
    static void DumpText(const char* buffer);
    static void DumpMap(int width, int height, const int* data);
    static void DumpObject(Object* ref);
    static void DumpNode(Node* node, int maxrecursivedeepth=1, bool withcomponent=true);
    static void DumpNodeRecursive(Node* node, unsigned currentDeepth, bool withcomponent=true, int deepth=0);
    static void DumpNode(unsigned id, bool withcomponentattr);
    static void DumpComponentTemplates();
    static void DumpVariantMap(const VariantMap& varmap);
    template< typename T > static void DumpData(const T* data, int markpattern, int n,  ...);

    static void DrawDebugRect(const Rect& rect, DebugRenderer* debugRenderer, bool depthTest=false, const Color& color=Color::WHITE);
};
