#pragma once

namespace Urho3D
{
    class Context;
    class Sprite;
    class Texture2D;
}

using namespace Urho3D;


class SplashScreen : public Object
{
	URHO3D_OBJECT(SplashScreen, Object);

public :
	SplashScreen(Context* context, const StringHash& beginTrigger, const StringHash& finishTrigger, const char * fileName, float delay=0.f);
	~SplashScreen();

    void Stop();
    static SplashScreen* Get() { return splashScreen_; }

private :
    void SubscribeToEvents();
	void SetOpenClosePositions();

    void HandleOpenSplash(StringHash eventType, VariantMap& eventData);
    void HandleCloseSplash(StringHash eventType, VariantMap& eventData);
    void HandleVisibility(StringHash eventType, VariantMap& eventData);
	void HandleStop(StringHash eventType, VariantMap& eventData);

	int state_;

    StringHash beginTrigger_;
    StringHash finishTrigger_;

	Sprite* splashUI_top;
	Sprite* splashUI_bottom;
	Texture2D* texture;

	Vector2 openposition_top;
	Vector2 closeposition_top;
	Vector2 openposition_bottom;
	Vector2 closeposition_bottom;
    float delay_, keepDelay_;

    static SplashScreen* splashScreen_;
};
