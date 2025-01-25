#pragma once


namespace Urho3D
{
    class Object;
    class UIElement;
    class Sprite;
    class Text;
}

class AnimatedSprite;
class DelayAction;
class DelayInformer;


#include <Urho3D/UI/Menu.h>

#include "GameHelpers.h"

using namespace Urho3D;

const float DefaultCompanionActivationDelay = 3.f;

struct Message
{
    Message() : hashname_(StringHash::ZERO) { }
    Message(unsigned activationGameState, bool persistent, const String& name, StringHash hashname, const String& companion, float activation=DefaultCompanionActivationDelay);
    Message(unsigned activationGameState, bool persistent, const String& name, StringHash hashname, const String& companion, float delay, const String& animfilename,
            const String& entityname, Animatable* parent, const Vector2& position);
    Message(unsigned activationGameState, bool persistent, const String& name, StringHash hashname, const String& companion, float delay,
            int actiontype, const String& animfilename, const String& entityname, const String& parentname, const Vector2& position);
    ~Message();

    Message(const Message& m) : name_(m.name_), hashname_(m.hashname_), companion_(m.companion_), activationDelay_(m.activationDelay_), persistent_(m.persistent_),
                                activationGameState_(m.activationGameState_), delayActions_(m.delayActions_) { }

    bool operator !=(const Message& rhs) const { return name_ != rhs.name_; }

    void AddEventAction(DelayAction* action);

    void Clear();
    void Start();
    void Stop();

    bool Exists() const { return !name_.Empty(); }
    bool HasRunningActions() const;

    String name_;
    StringHash hashname_;
    String companion_;
    float activationDelay_;
    bool persistent_;
    unsigned activationGameState_;
    PODVector<DelayAction* > delayActions_;
};

class UIMenu : public Menu
{
    URHO3D_OBJECT(UIMenu, Menu);

public:

    UIMenu(Context* context) : Menu(context) { }
    ~UIMenu() { }

    static void RegisterObject(Context* context);

    virtual void OnShowPopup();
    virtual void OnHidePopup();
};

class UIDialog : public Object
{
    URHO3D_OBJECT(UIDialog, Object);

public:

	UIDialog(Context* context);
	~UIDialog();

	void CleanMessages();
    void Clear(bool clean=false);
    void Set(UIElement* root, const String& layout);

    Message* AddMessage(unsigned activationGameState, bool persistent, const String& name, const String& companion=String::EMPTY, float activation=DefaultCompanionActivationDelay);
    Message* AddMessage(unsigned activationGameState, bool persistent, const String& name, const String& companion, float delay, const String& animfilename,
                        const String& entityname, Animatable* parent, const Vector2& position=Vector2::ZERO);
    Message* AddMessage(unsigned activationGameState, bool persistent, const String& name, const String& companion, float delay,
                        int actiontype, const String& animfilename, const String& entityname, const String& parentname, const Vector2& position=Vector2::ZERO);
    Message* PushFrontMessage(unsigned activationGameState, bool persistent, const String& name, const String& companion=String::EMPTY, float activation=DefaultCompanionActivationDelay);
    Message* PushFrontMessage(unsigned activationGameState, bool persistent, const String& name, const String& companion, float delay, const String& animfilename,
                              const String& entityname, Animatable* parent, const Vector2& position=Vector2::ZERO);
    void SetMessage(const String& name);
    void SetArchiveViewedMessages(bool enable);
    void SetLoopMessages(bool enable);
    void SetCustomContent(const String& content, bool show);
    void SetCustomMessage(const String& name, const String& content, bool show);

    UIElement* GetFrame() const;
	const IntVector2& GetFrameSize() const;

	float GetCurrentMessageDelay() const;
    float GetNextMessageDelay() const;
    bool ContainsMessage(StringHash hashname) const;
    bool HasMessagesToShow() const { return imessage_ < messages_.Size(); }
    bool IsStarted() const { return started_; }
    bool IsVisible() const;
    bool IsBusy() const;
    bool IsNewMessage(const String& name) const;

	void StartMessage(bool animated, bool incmessage=true);
	void StopMessage(bool animated);

	void Resize();

	void Dump() const;

private:
    bool HasArchivedMessage(StringHash hashname) const;
    void PushToArchivedMessages(StringHash hashname);

    void SetScaleYAnimation(Sprite* sprite, bool appear, float delay);
    void SetMoveXAnimation(Sprite* sprite, bool appear, float delay);

    void Show();
    void Hide();
    void UpdateMessage();
    void Update();

    void HandleShow(StringHash eventType, VariantMap& eventData);
    void HandleHide(StringHash eventType, VariantMap& eventData);
    void HandleUpdate(StringHash eventType, VariantMap& eventData);

    String custommessage_, custommessagecontent_;
    List<Message> messages_;

    Message currentmessage_;
    int imessage_;
    bool messageDirty_;

    WeakPtr<UIElement> panel_;
    WeakPtr<Sprite> frame_;
    WeakPtr<Sprite> rect_;
    WeakPtr<Text> title_;
    WeakPtr<Text> content_;
    WeakPtr<AnimatedSprite> companion_;
    Vector2 panelOriginSize_;

    Vector2 framePosNormalized_;
    Vector2 frameSizeNormalized_;
    Vector<int> textFontSize_;
    bool started_, archiveMessagesEnable_, loopMessagesEnable_;

    DelayInformer* informer_;
};
