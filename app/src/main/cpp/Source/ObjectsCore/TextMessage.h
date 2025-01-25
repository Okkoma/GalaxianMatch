#pragma once

namespace Urho3D
{
    class Node;
    class Text;
    class Text3D;
}

#include "Pool.h"

URHO3D_EVENT(TEXTMESSAGE_EXPIRED, TextMessage_Expired) { }

class TextMessage : public Object
{
    URHO3D_OBJECT(TextMessage, Object);

public :
    static void Reset(unsigned size=0);
    static TextMessage* Get();
    static TextMessage* Get(Node* node, const String& message, const char *fontName, int fontSize,
             float duration, bool autoRemove=true, float delayedStart=0.f, float delayedRemove=0.f);
    static TextMessage* Get(const String& message, const char *fontName, int fontSize,
             float duration, IntVector2 position=IntVector2::ZERO,
             bool autoRemove=true, float delayedStart=0.f, float delayedRemove=0.f);
    static void FreeAll();

    TextMessage();
    TextMessage(Context* context);
    TextMessage(const TextMessage& timer);
    ~TextMessage();

    TextMessage* Set(Node* node, const String& message, const char *fontName, int fontSize,
             float duration, bool autoRemove=true, float delayedStart=0.f, float delayedRemove=0.f);
    TextMessage* Set(const String& message, const char *fontName, int fontSize,
             float duration, IntVector2 position=IntVector2::ZERO,
             bool autoRemove=true, float delayedStart=0.f, float delayedRemove=0.f);
    void SetDuration(float duration);
    void SetColor(const Color& colorTL=Color::WHITE, const Color& colorTR=Color::WHITE, const Color& colorBL=Color::WHITE, const Color& colorBR=Color::WHITE);
    void HandleUpdate1(StringHash eventType, VariantMap& eventData);
    void HandleUpdate2(StringHash eventType, VariantMap& eventData);
    void HandleUpdate1_3D(StringHash eventType, VariantMap& eventData);
    void HandleUpdate2_3D(StringHash eventType, VariantMap& eventData);
    void HandleUpdate3(StringHash eventType, VariantMap& eventData);

    bool Expired() { return (timer_ > expirationTime3_); }

    void Free();

private :
    WeakPtr<Text> text_;
    WeakPtr<Node> node_;
    Text3D* text3D_;

    float velocityY;

    int type_;
    bool autoRemove_;
    float timer_;
	float expirationTime1_,expirationTime2_,expirationTime3_;

	static Pool<TextMessage> pool_;
};
