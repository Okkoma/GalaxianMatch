#include <Urho3D/Urho3D.h>

#include <Urho3D/Core/CoreEvents.h>

#include <Urho3D/IO/Log.h>

#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Graphics/Graphics.h>

#include <Urho3D/Scene/Scene.h>

#include <Urho3D/UI/Font.h>
#include <Urho3D/UI/Text.h>
#include <Urho3D/UI/Text3D.h>
#include <Urho3D/UI/UI.h>
#include <Urho3D/UI/UIElement.h>

#include "GameStatics.h"

#include "GameRand.h"

#include "TextMessage.h"


Pool<TextMessage> TextMessage::pool_;

void TextMessage::Reset(unsigned size)
{
    pool_.Resize(GameStatics::context_, size);
}

TextMessage* TextMessage::Get()
{
    return pool_.Get();
}

TextMessage* TextMessage::Get(Node* node, const String& message, const char *fontName, int fontSize,
             float duration, bool autoRemove, float delayedStart, float delayedRemove)
{
    return pool_.Get()->Set(node, message, fontName, fontSize, duration, autoRemove, delayedStart, delayedRemove);
}

TextMessage* TextMessage::Get(const String& message, const char *fontName, int fontSize,
             float duration, IntVector2 position, bool autoRemove, float delayedStart, float delayedRemove)
{
    return pool_.Get()->Set(message, fontName, fontSize, duration, position, autoRemove, delayedStart, delayedRemove);
}

void TextMessage::FreeAll()
{
    pool_.FreeAll();
}


TextMessage::TextMessage() : Object(0) { }
TextMessage::TextMessage(Context* context) : Object(context) { }
TextMessage::TextMessage(const TextMessage& timer) : Object(timer.GetContext()) { }
TextMessage::~TextMessage() { }

void TextMessage::Free()
{
    UnsubscribeFromEvent(E_UPDATE);

    if (type_== 0)
    {
        if (text_)
            GetSubsystem<UI>()->GetRoot()->RemoveChild(text_);
    }
    else if (node_)
        node_->Remove();

    pool_.Free(this);
}

TextMessage* TextMessage::Set(Node* node, const String& message, const char *fontName, int fontSize, float duration,
                      bool autoRemove, float delayedStart, float delayedRemove)
{
    type_ = 1;

    Font* font = GetSubsystem<ResourceCache>()->GetResource<Font>(fontName);

    Vector3 position = node->GetPosition();
    node_ = node->GetScene()->CreateChild("Text3D");
    if (!node_)
    {
        Free();
        return 0;
    }

    node_->SetPosition(Vector3(position.x_, position.y_+2.f, position.z_-3.f));

    text3D_ = node_->CreateComponent<Text3D>();
    if (!text3D_)
    {
        Free();
        return 0;
    }

    text3D_->SetText(message);
    text3D_->SetColor(C_TOPLEFT , Color(1, 1, 0.25));
    text3D_->SetColor(C_TOPRIGHT, Color(1, 1, 0.25));
    text3D_->SetColor(C_BOTTOMLEFT, Color(1, 0.25, 0.25));
    text3D_->SetColor(C_BOTTOMRIGHT, Color(1, 0.25, 0.25));
    text3D_->SetFont(font, fontSize);
    text3D_->SetAlignment(HA_CENTER, VA_CENTER);
    node_->SetEnabled(false);

    velocityY = GameRand::GetRand(ALLRAND, 2) + 1.f;

    autoRemove_ = autoRemove;
    expirationTime1_ = delayedStart;
    expirationTime2_ = expirationTime1_ + duration;
    expirationTime3_ = expirationTime2_ + delayedRemove;
    timer_ = 0.0f;

    SubscribeToEvent(E_UPDATE, URHO3D_HANDLER(TextMessage, HandleUpdate1_3D));

    return this;
}

TextMessage* TextMessage::Set(const String& message, const char *fontName, int fontSize,
                      float duration, IntVector2 position,  bool autoRemove, float delayedStart, float delayedRemove)
{
    type_ = 0;

    Font* font = GetSubsystem<ResourceCache>()->GetResource<Font>(fontName);
    Graphics* graphics = GetSubsystem<Graphics>();

    text_ = GetSubsystem<UI>()->GetRoot()->CreateChild<Text>();
    if (!text_)
    {
        Free();
        return 0;
    }

    text_->SetText(message);
    text_->SetFont(font, fontSize);
    text_->SetPriority(M_MAX_INT-2);

    if (position.x_ == 0)
        text_->SetHorizontalAlignment(HA_CENTER);
    if (position.y_ == 0)
        text_->SetVerticalAlignment(VA_CENTER);
    if (position.x_ < 0)
        position.x_ += GameStatics::ui_->GetRoot()->GetSize().x_ - 2*fontSize;
    if (position.y_ < 0)
        position.y_ += GameStatics::ui_->GetRoot()->GetSize().y_ - 2*fontSize;

    text_->SetPosition(position);

//    SetColor(Color(1, 1, 0.25), Color(1, 1, 0.25),  Color(1, 0.25, 0.25), Color(1, 0.25, 0.25));

    text_->SetVisible(false);

    autoRemove_ = autoRemove;
    expirationTime1_ = delayedStart;
    expirationTime2_ = expirationTime1_ + duration;
    expirationTime3_ = expirationTime2_ + delayedRemove;
    timer_ = 0.0f;

    SubscribeToEvent(E_UPDATE, URHO3D_HANDLER(TextMessage, HandleUpdate1));

    return this;
}

void TextMessage::SetDuration(float duration)
{
    expirationTime2_ = duration;
    expirationTime3_ = expirationTime2_;
    timer_ = 0.0f;
    text_->SetVisible(true);
    SubscribeToEvent(E_UPDATE, URHO3D_HANDLER(TextMessage, HandleUpdate2));
}

void TextMessage::SetColor(const Color& colorTL, const Color& colorTR, const Color& colorBL, const Color& colorBR)
{
    if (type_ == 0)
    {
        text_->SetColor(C_TOPLEFT , colorTL);
        text_->SetColor(C_TOPRIGHT, colorTR);
        text_->SetColor(C_BOTTOMLEFT, colorBL);
        text_->SetColor(C_BOTTOMRIGHT, colorBR);
    }
    else
    {
        text3D_->SetColor(C_TOPLEFT , colorTL);
        text3D_->SetColor(C_TOPRIGHT, colorTR);
        text3D_->SetColor(C_BOTTOMLEFT, colorBL);
        text3D_->SetColor(C_BOTTOMRIGHT, colorBR);
    }
}

void TextMessage::HandleUpdate1(StringHash eventType, VariantMap& eventData)
{
    timer_ += eventData[Update::P_TIMESTEP].GetFloat();
    if (timer_ > expirationTime1_)
    {
        if (!text_)
        {
            Free();
            URHO3D_LOGERRORF("TextMessage() - handleUpdate1 : no text !");
            return;
        }

        text_->SetVisible(true);

        UnsubscribeFromEvent(E_UPDATE);
        SubscribeToEvent(E_UPDATE, URHO3D_HANDLER(TextMessage, HandleUpdate2));
    }
}

void TextMessage::HandleUpdate1_3D(StringHash eventType, VariantMap& eventData)
{
    timer_ += eventData[Update::P_TIMESTEP].GetFloat();
    if (timer_ > expirationTime1_)
    {
        if (!node_)
        {
            Free();
            URHO3D_LOGERRORF("TextMessage() - handleUpdate1_3D : no text !");
            return;
        }
        node_->SetEnabled(true);

        UnsubscribeFromEvent(E_UPDATE);
        SubscribeToEvent(E_UPDATE, URHO3D_HANDLER(TextMessage, HandleUpdate2_3D));
    }
}

void TextMessage::HandleUpdate2(StringHash eventType, VariantMap& eventData)
{
    timer_ += eventData[Update::P_TIMESTEP].GetFloat();

    if (timer_ > expirationTime2_)
    {
        UnsubscribeFromEvent(E_UPDATE);

        if (!text_)
        {
            Free();
            return;
        }
        text_->SetVisible(false);

        SendEvent(TEXTMESSAGE_EXPIRED);

        if (expirationTime3_ > expirationTime2_)
            SubscribeToEvent(E_UPDATE, URHO3D_HANDLER(TextMessage, HandleUpdate3));
        else
            if (autoRemove_)
                Free();
    }
}

void TextMessage::HandleUpdate2_3D(StringHash eventType, VariantMap& eventData)
{
    float timeStep = eventData[Update::P_TIMESTEP].GetFloat();
    timer_ += timeStep;

    node_->Translate(Vector3(GameRand::GetRand(ALLRAND, 10) < 5 ? -timeStep : timeStep, velocityY * timeStep, 0.f));
    node_->SetScale(node_->GetScale()*1.005f);
    text3D_->SetOpacity(text3D_->GetOpacity()-timeStep);

    if (timer_ > expirationTime2_)
    {
        UnsubscribeFromEvent(E_UPDATE);

        if (!node_)
        {
            Free();
            return;
        }
        node_->SetEnabled(false);

        SendEvent(TEXTMESSAGE_EXPIRED);

        if (expirationTime3_ > expirationTime2_)
            SubscribeToEvent(E_UPDATE, URHO3D_HANDLER(TextMessage, HandleUpdate3));
        else
            if (autoRemove_)
                Free();
    }
}

void TextMessage::HandleUpdate3(StringHash eventType, VariantMap& eventData)
{
    timer_ += eventData[Update::P_TIMESTEP].GetFloat();

    if (timer_ > expirationTime3_)
    {
        UnsubscribeFromEvent(E_UPDATE);
        if (autoRemove_)
            Free();
        else if ((type_== 0 && !text_) || (type_ == 1 && !node_))
            Free();
    }
}
