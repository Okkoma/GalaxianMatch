#include <Urho3D/Urho3D.h>

#include <Urho3D/IO/Log.h>

#include <Urho3D/Graphics/Graphics.h>
#include <Urho3D/Graphics/GraphicsEvents.h>

#include <Urho3D/Resource/Localization.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Resource/XMLFile.h>

#include <Urho3D/Scene/ObjectAnimation.h>
#include <Urho3D/Scene/ValueAnimation.h>

#include <Urho3D/Input/Input.h>

#include <Urho3D/UI/UI.h>
#include <Urho3D/UI/UIEvents.h>
#include <Urho3D/UI/Font.h>
#include <Urho3D/UI/Button.h>
#include <Urho3D/UI/Text.h>
#include <Urho3D/UI/Sprite.h>

#include "AnimatedSprite.h"
#include "DelayInformer.h"
#include "DelayAction.h"

#include "GameOptions.h"
#include "GameAttributes.h"
#include "GameEvents.h"
#include "GameStateManager.h"
#include "GameStatics.h"
#include "GameHelpers.h"

#include "GameUI.h"


namespace Urho3D
{
    extern const char* UI_CATEGORY;
}


extern int UISIZE[NUMUIELEMENTSIZE];



void UIMenu::RegisterObject(Context* context) { context->RegisterFactory<UIMenu>(Urho3D::UI_CATEGORY); }
void UIMenu::OnShowPopup() { SendEvent(UIMENU_SHOWCONTENT); }
void UIMenu::OnHidePopup() { SendEvent(UIMENU_HIDECONTENT); }


Message::Message(unsigned activationGameState, bool persistent, const String& name, StringHash hashname, const String& companion, float activation) :
    name_(name),
    hashname_(hashname),
    companion_(companion),
    activationDelay_(activation),
    persistent_(persistent),
    activationGameState_(activationGameState)
{ }

Message::Message(unsigned activationGameState, bool persistent, const String& name, StringHash hashname, const String& companion, float delay,
                 const String& animfilename, const String& entityname, Animatable* parent, const Vector2& position) :
    name_(name),
    hashname_(hashname),
    companion_(companion),
    activationDelay_(DefaultCompanionActivationDelay),
    persistent_(persistent),
    activationGameState_(activationGameState)
{
    delayActions_.Push(DelayAction::Get(MESSAGE_START, MESSAGE_STOP, delay, animfilename, entityname, parent, position, 0.f));
}

Message::Message(unsigned activationGameState, bool persistent, const String& name, StringHash hashname, const String& companion, float delay,
                 int actiontype, const String& animfilename, const String& entityname, const String& parentname, const Vector2& position) :
    name_(name),
    hashname_(hashname),
    companion_(companion),
    activationDelay_(DefaultCompanionActivationDelay),
    persistent_(persistent),
    activationGameState_(activationGameState)
{
    delayActions_.Push(DelayAction::Get(MESSAGE_START, MESSAGE_STOP, delay, actiontype, animfilename, entityname, parentname, position, 0.f));
}

Message::~Message()
{
    Clear();
}

void Message::AddEventAction(DelayAction* action)
{
    delayActions_.Push(action);
}

void Message::Clear()
{
    for (PODVector<DelayAction* >::Iterator it = delayActions_.Begin(); it != delayActions_.End(); ++it)
        (*it)->Free();

    name_.Clear();
    delayActions_.Clear();
}

void Message::Start()
{
    for (PODVector<DelayAction* >::Iterator it = delayActions_.Begin(); it != delayActions_.End(); ++it)
        (*it)->Start();
}

void Message::Stop()
{
    for (PODVector<DelayAction* >::Iterator it = delayActions_.Begin(); it != delayActions_.End(); ++it)
        (*it)->Stop();
}

bool Message::HasRunningActions() const
{
    for (PODVector<DelayAction* >::ConstIterator it = delayActions_.Begin(); it != delayActions_.End(); ++it)
        if ((*it)->IsRunning())
            return true;

    return false;
}



UIDialog::UIDialog(Context* context) :
    Object(context),
    started_(false),
    archiveMessagesEnable_(false),
    loopMessagesEnable_(false),
    informer_(0)
{
    Clear();
}

UIDialog::~UIDialog()
{
    if (panel_)
        panel_->Remove();
}

void UIDialog::CleanMessages()
{
    List<Message>::Iterator it = messages_.Begin();
    while (it != messages_.End())
    {
        if (!it->persistent_)
            it = messages_.Erase(it);
        else
            it++;
    }
}

void UIDialog::Clear(bool clean)
{
    Hide();

    imessage_ = 0;
    if (clean)
        CleanMessages();
    else
        messages_.Clear();
    messageDirty_ = false;

    if (informer_ && !informer_->Expired())
        informer_->Free();
}

void UIDialog::Set(UIElement* root, const String& layout)
{
    if (!GameStatics::ui_)
        return;

    SharedPtr<UIElement> uielement = GameStatics::ui_->LoadLayout(GetSubsystem<ResourceCache>()->GetResource<XMLFile>(layout));
    if (uielement)
    {
        if (panel_)
            panel_->Remove();
        panel_.Reset();

        panel_ = uielement.Get();
        root->AddChild(panel_);

        panel_->SetVisible(false);

        frame_ = panel_->GetChildStaticCast<Sprite>(String("Frame"), true);
        rect_ = frame_->GetChildStaticCast<Sprite>(String("Rect"), true);
        companion_ = frame_->GetChildStaticCast<AnimatedSprite>(String("Companion"), true);

        panelOriginSize_ = Vector2((float)panel_->GetSize().x_, (float)panel_->GetSize().y_);
        framePosNormalized_.x_ = frame_->GetPosition().x_ / (float)panel_->GetSize().x_;
        framePosNormalized_.y_ = frame_->GetPosition().y_ / (float)panel_->GetSize().y_;
        frameSizeNormalized_.x_ = frame_->GetSize().x_ / (float)panel_->GetSize().x_;
        frameSizeNormalized_.y_ = frame_->GetSize().y_ / (float)panel_->GetSize().y_;

        Sprite* frame = rect_ ? rect_ : frame_;
        title_ = frame->GetChildStaticCast<Text>(String("Name"), true);
        content_ = frame->GetChildStaticCast<Text>(String("Content"), true);

        const Vector<SharedPtr<UIElement> >& children = rect_ ? rect_->GetChildren() : frame->GetChildren();
        for (Vector<SharedPtr<UIElement> >::ConstIterator it = children.Begin(); it != children.End(); ++it)
        {
            if ((*it)->IsInstanceOf<Text>())
                textFontSize_.Push(DynamicCast<Text>(*it)->GetFontSize());
        }

        Resize();
    }
}


const IntVector2& UIDialog::GetFrameSize() const
{
    return frame_ ? frame_->GetSize() : IntVector2::ZERO;
}

UIElement* UIDialog::GetFrame() const
{
    return frame_.Get();
}

float UIDialog::GetCurrentMessageDelay() const
{
    if (imessage_ != -1 && messages_.Size())
    {
        if (imessage_ < messages_.Size())
            return messages_.GetConstIteratorAt(imessage_)->activationDelay_;
    }

    return DefaultCompanionActivationDelay;
}

float UIDialog::GetNextMessageDelay() const
{
    if (imessage_ != -1 && messages_.Size())
    {
        if (imessage_+1 < messages_.Size())
            return messages_.GetConstIteratorAt(imessage_+1)->activationDelay_;
        else if (loopMessagesEnable_ && imessage_+1 >= messages_.Size())
            return messages_.Front().activationDelay_;
    }

    return DefaultCompanionActivationDelay;
}

bool UIDialog::IsBusy() const
{
    return imessage_ != -1 && currentmessage_.Exists() && currentmessage_.HasRunningActions();
}

bool UIDialog::IsVisible() const
{
    return frame_ && frame_->IsVisibleEffective();
}

bool UIDialog::IsNewMessage(const String& name) const
{
    return !HasArchivedMessage(StringHash(name));
}

bool UIDialog::ContainsMessage(StringHash hashname) const
{
    for (List<Message>::ConstIterator it = messages_.Begin(); it != messages_.End(); ++it)
    {
        if (it->hashname_ == hashname)
            return true;
    }

    if (!archiveMessagesEnable_)
        return false;

    return HasArchivedMessage(hashname);
}

bool UIDialog::HasArchivedMessage(StringHash hashname) const
{
    const unsigned value = hashname.Value();
    const unsigned* archivedmessages = GameStatics::playerState_->archivedmessages_;
    const unsigned size = archivedmessages[0];
    for (unsigned i = 1; i <= size; i++)
        if (value == archivedmessages[i])
            return true;

    return false;
}

void UIDialog::PushToArchivedMessages(StringHash hashname)
{
    unsigned* archivedmessages = GameStatics::playerState_->archivedmessages_;
    archivedmessages[0]++;
    if (archivedmessages[0] > NBMAXMESSAGES)
    {
        URHO3D_LOGERRORF("UIDialog() - PushToArchivedMessages : nbMaxMessages Overwritten ! ");
        archivedmessages[0] = NBMAXMESSAGES;
    }

    archivedmessages[archivedmessages[0]] = hashname.Value();
}

Message* UIDialog::AddMessage(unsigned activationGameState, bool persistent, const String& name, const String& companion, float activationdelay)
{
    StringHash hashname(name);
    if (!ContainsMessage(hashname))
    {
        messages_.Push(Message(activationGameState, persistent, name, hashname, companion, activationdelay));
        messageDirty_ = true;
        return &messages_.Back();
    }
    return 0;
}

Message* UIDialog::AddMessage(unsigned activationGameState, bool persistent, const String& name, const String& companion, float delay,
                              const String& animfilename, const String& entityname, Animatable* parent, const Vector2& position)
{
    StringHash hashname(name);
    if (!ContainsMessage(hashname))
    {
        messages_.Push(Message(activationGameState, persistent, name, hashname, companion, delay, animfilename, entityname, parent, position));
        messageDirty_ = true;
        return &messages_.Back();
    }
    return 0;
}

Message* UIDialog::AddMessage(unsigned activationGameState, bool persistent, const String& name, const String& companion, float delay,
                              int actiontype, const String& animfilename, const String& entityname, const String& parentname, const Vector2& position)
{
    StringHash hashname(name);
    if (!ContainsMessage(hashname))
    {
        messages_.Push(Message(activationGameState, persistent, name, hashname, companion, delay, actiontype, animfilename, entityname, parentname, position));
        messageDirty_ = true;
        return &messages_.Back();
    }
    return 0;
}

Message* UIDialog::PushFrontMessage(unsigned activationGameState, bool persistent, const String& name, const String& companion, float activationdelay)
{
    StringHash hashname(name);
    if (!ContainsMessage(hashname))
    {
        imessage_ = 0;
        messages_.PushFront(Message(activationGameState, persistent, name, hashname, companion, activationdelay));
        messageDirty_ = true;
        return &messages_.Front();
    }
    return 0;
}

Message* UIDialog::PushFrontMessage(unsigned activationGameState, bool persistent, const String& name, const String& companion, float delay,
                                    const String& animfilename, const String& entityname, Animatable* parent, const Vector2& position)
{
    StringHash hashname(name);
    if (!ContainsMessage(hashname))
    {
        imessage_ = 0;
        messages_.PushFront(Message(activationGameState, persistent, name, hashname, companion, delay, animfilename, entityname, parent, position));
        messageDirty_ = true;
        return &messages_.Front();
    }
    return 0;
}

void UIDialog::SetMessage(const String& name)
{
    imessage_ = 0;
    messages_.Clear();
    messages_.Push(Message(0, false, name, StringHash(name), String::EMPTY));

    messageDirty_ = true;
}

void UIDialog::SetArchiveViewedMessages(bool enable)
{
    archiveMessagesEnable_ = enable;
}

void UIDialog::SetCustomContent(const String& content, bool show)
{
    imessage_ = -1;
    custommessagecontent_ = content;
    messageDirty_ = true;

    if (show)
        StartMessage(true);
}

void UIDialog::SetCustomMessage(const String& name, const String& content, bool show)
{
    imessage_ = -1;
    custommessage_ = name;
    custommessagecontent_ = content;
    messageDirty_ = true;

    if (show)
        StartMessage(true);
}

void UIDialog::SetScaleYAnimation(Sprite* sprite, bool appear, float delay)
{
    sprite->RemoveObjectAnimation();

    Vector2 scalestart(1.f, appear ? 0.f : 1.f);
    Vector2 scaleend(1.f, appear ? 1.f : 0.f);

    SharedPtr<ObjectAnimation> objectAnimation(new ObjectAnimation(sprite->GetContext()));
    SharedPtr<ValueAnimation> scaleAnimation(new ValueAnimation(sprite->GetContext()));
    scaleAnimation->SetKeyFrame(0.f, scalestart);
    scaleAnimation->SetKeyFrame(delay, scaleend);
    objectAnimation->AddAttributeAnimation("Scale", scaleAnimation, WM_ONCE);
    sprite->SetObjectAnimation(objectAnimation);

    sprite->SetAnimationEnabled(true);
}

void UIDialog::SetMoveXAnimation(Sprite* sprite, bool appear, float delay)
{
    sprite->RemoveObjectAnimation();

    Vector2 positionStart(appear ? sprite->GetSize().x_ * 0.5f : -sprite->GetSize().x_, sprite->GetPosition().y_);
    Vector2 positionEnd(appear ? -sprite->GetSize().x_ : sprite->GetSize().x_ * 0.5f, sprite->GetPosition().y_);

    SharedPtr<ObjectAnimation> objectAnimation(new ObjectAnimation(sprite->GetContext()));
    SharedPtr<ValueAnimation> moveAnimation(new ValueAnimation(sprite->GetContext()));
    moveAnimation->SetKeyFrame(0.f, sprite->GetPosition());
    moveAnimation->SetKeyFrame(delay, positionEnd);
    objectAnimation->AddAttributeAnimation("Position", moveAnimation, WM_ONCE);
    sprite->SetObjectAnimation(objectAnimation);

    sprite->SetAnimationEnabled(true);
}

void UIDialog::StartMessage(bool animated, bool incmessage)
{
    if (messages_.Empty())
        return;

    if (started_)
        StopMessage(false);

    if (imessage_ != -1)
    {
        if (incmessage && currentmessage_.Exists())
            currentmessage_.Clear();

        if (!currentmessage_.Exists())
        {
            List<Message>::Iterator it = messages_.GetIteratorAt(imessage_);
            while (it != messages_.End())
            {
                // find an activable message
                if (!it->activationGameState_ || it->activationGameState_ == GameStateManager::Get()->GetActiveState()->GetStateHash())
                {
                    messageDirty_ = true;
                    currentmessage_ = *it;

                    if (archiveMessagesEnable_)
                        PushToArchivedMessages(currentmessage_.hashname_);

                    if (incmessage)
                    {
                        if (!it->persistent_ || archiveMessagesEnable_)
                        {
                            it = messages_.Erase(it);
                            if (it == messages_.End())
                                imessage_ = messages_.Size();
                        }
                        else if (imessage_ < messages_.Size())
                            imessage_ = imessage_ + 1;
                    }

                    if (loopMessagesEnable_ && imessage_ >= messages_.Size())
                        imessage_ = 0;

                    break;
                }
                else if (incmessage)
                {
                    it++;
                    imessage_++;
                }
                else
                    break;
            }
        }

        if (!currentmessage_.Exists())
            return;
    }

    started_ = true;

    if (animated)
    {
        panel_->SetVisible(true);

        UpdateMessage();

        if (title_)
            title_->SetVisible(true);

        if (content_)
            content_->SetVisible(false);

        if (rect_)
            SetScaleYAnimation(rect_, true, 0.25f);
        else if (frame_)
            SetScaleYAnimation(frame_, true, 0.25f);
        if (companion_)
            SetMoveXAnimation(companion_, true, 0.25f);

        SubscribeToEvent(this, UIDIALOG_SHOWCONTENT, URHO3D_HANDLER(UIDialog, HandleShow));

        if (informer_ && !informer_->Expired())
            informer_->Free();
        informer_ = DelayInformer::Get(this, 0.3f, UIDIALOG_SHOWCONTENT);
    }
    else
    {
        Show();
    }
}

void UIDialog::StopMessage(bool animated)
{
    if (!started_)
        return;

    started_ = false;

    if (animated)
    {
        if (title_)
            title_->SetVisible(false);

        if (content_)
            content_->SetVisible(false);

        if (rect_)
            SetScaleYAnimation(rect_, false, 0.25f);
        else if (frame_)
            SetScaleYAnimation(frame_, false, 0.25f);
        if (companion_)
            SetMoveXAnimation(companion_, false, 0.25f);

        SubscribeToEvent(this, UIDIALOG_HIDECONTENT, URHO3D_HANDLER(UIDialog, HandleHide));

        if (informer_ && !informer_->Expired())
            informer_->Free();
        informer_ = DelayInformer::Get(this, 0.3f, UIDIALOG_HIDECONTENT);
    }
    else
    {
        Hide();
    }
}

void UIDialog::UpdateMessage()
{
    if (messageDirty_ && (title_ || content_))
    {
        if (imessage_ == -1 && !custommessage_.Empty())
        {
            if (title_)
                title_->SetText(custommessage_);
            if (content_)
                content_->SetText(custommessagecontent_);
            messageDirty_ = false;
        }
        else if (currentmessage_.Exists())
        {
            URHO3D_LOGINFOF("UIDialog() - UpdateMessage : imessage=%d currentmessage=%s ...", imessage_, currentmessage_.name_.CString());
            Localization* l10n = context_->GetSubsystem<Localization>();
            if (title_)
                title_->SetText(l10n->Get(currentmessage_.name_));
            if (content_)
                content_->SetText(l10n->Get(currentmessage_.name_ + "_c"));
            if (companion_ && !currentmessage_.companion_.Empty())
            {
                companion_->SetEntity(currentmessage_.companion_);
                companion_->SetAnimation("idle");
            }

            messageDirty_ = false;
        }
    }
}

void UIDialog::Update()
{
    ;
}

void UIDialog::Resize()
{
    const bool needRestart = started_;

    if (needRestart)
        StopMessage(false);

    // Apply uiscale on frame
    if (panel_)
        panel_->SetSize(GameStatics::ui_->GetRoot()->GetSize());

    if (frame_)
    {
        frame_->SetPosition(panel_->GetSize().x_ * framePosNormalized_.x_, panel_->GetSize().y_ * framePosNormalized_.y_);
        frame_->SetSize(panel_->GetSize().x_ * frameSizeNormalized_.x_, panel_->GetSize().y_ * frameSizeNormalized_.y_);
        frame_->SetHotSpot(frame_->GetSize()/2);

        UIElement* framebtn = frame_->GetChild(String("FrameBtn"));
        if (framebtn)
            framebtn->SetSize(frame_->GetSize());

        if (rect_)
        {
            rect_->SetSize(frame_->GetSize());
            rect_->SetHotSpot(frame_->GetSize()/2);
        }

        // Apply uiscale on fonts
        Sprite* frame = rect_ ? rect_ : frame_;
        const Vector<SharedPtr<UIElement> >& children = frame->GetChildren();
        for (Vector<SharedPtr<UIElement> >::ConstIterator it = children.Begin(); it != children.End(); ++it)
        {
            if ((*it)->IsInstanceOf<Text>())
                DynamicCast<Text>(*it)->SetFontSize((*it)->GetName() == String("Name") ? UISIZE[FONTSIZE_SMALL] : UISIZE[FONTSIZE_NORMAL]);
        }
    }

    if (needRestart)
        StartMessage(false, false);
}

void UIDialog::Show()
{
    if (panel_)
        panel_->SetVisible(true);

    UpdateMessage();

    if (title_)
        title_->SetVisible(true);

    if (content_)
    {
        content_->SetSize(frame_->GetSize().x_ * 0.85f, frame_->GetSize().y_ * 0.9f);
        content_->SetVisible(true);
    }

    if (currentmessage_.Exists())
        currentmessage_.Start();

    SendEvent(MESSAGE_START);
}

void UIDialog::Hide()
{
    SendEvent(MESSAGE_STOP);

    if (panel_)
        panel_->SetVisible(false);

    if (imessage_ != -1 && currentmessage_.Exists())
        currentmessage_.Stop();

    UnsubscribeFromAllEvents();
}

void UIDialog::HandleShow(StringHash eventType, VariantMap& eventData)
{
    Show();
}

void UIDialog::HandleHide(StringHash eventType, VariantMap& eventData)
{
    Hide();
}

void UIDialog::HandleUpdate(StringHash eventType, VariantMap& eventData)
{
    Update();
}

void UIDialog::Dump() const
{
    URHO3D_LOGINFOF("UIDialog() - Dump()");
    unsigned i = 0;
    for (List<Message>::ConstIterator it = messages_.Begin(); it != messages_.End(); ++it, ++i)
        URHO3D_LOGINFOF("messages[%d] = %s", i, it->name_.CString());
}
