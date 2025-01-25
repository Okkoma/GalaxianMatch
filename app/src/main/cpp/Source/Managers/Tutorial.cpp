#include <Urho3D/Urho3D.h>

#include <Urho3D/Core/Context.h>

#include <Urho3D/IO/Log.h>

#include <Urho3D/Resource/Resource.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Resource/Localization.h>

#include <Urho3D/Graphics/Graphics.h>
#include <Urho3D/Graphics/DebugRenderer.h>

#include <Urho3D/Urho2D/AnimationSet2D.h>
#include <Urho3D/Urho2D/AnimatedSprite2D.h>

#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Scene/SceneEvents.h>

#include <Urho3D/Input/Input.h>

#include <Urho3D/UI/UI.h>
#include <Urho3D/UI/Font.h>
#include <Urho3D/UI/UIEvents.h>

#include "Game.h"
#include "GameUI.h"
#include "GameAttributes.h"
#include "GameStatics.h"
#include "GameHelpers.h"
#include "GameEvents.h"
#include "TimerRemover.h"
#include "DelayInformer.h"
#include "InteractiveFrame.h"

#include "MAN_Matches.h"

#include "Tutorial.h"


SharedPtr<Tutorial> Tutorial::manager_;


Tutorial::Tutorial(Context* context) :
    Object(context)
{
    manager_ = SharedPtr<Tutorial>(this);

    Init();
}

Tutorial::~Tutorial()
{

}

void Tutorial::RegisterObject(Context* context)
{
    context->RegisterFactory<Tutorial>();

    Reset(context);
}

void Tutorial::Reset(Context* context)
{
    manager_.Reset();

    if (context)
        manager_ = new Tutorial(context);
}

void Tutorial::SetDatas(const Vector<TutorialData*>& datas)
{
    if (manager_)
    {
        manager_->datas_ = datas;
    }
}

void Tutorial::Start()
{
    if (manager_ && !manager_->datas_.Size())
    {
        URHO3D_LOGINFOF("Tutorial() - Start !");

        manager_->SetEnabled(true);
    }
}

void Tutorial::Stop()
{
    if (manager_)
    {
        URHO3D_LOGINFOF("Tutorial() - Stop !");

        manager_->UnsubscribeFromEvents();
        manager_->Init();
    }
}

void Tutorial::Clear()
{
    if (frame_)
        frame_->Clean();

    frame_.Reset();
    datas_.Clear();
    tutorialInfos_.Clear();
}

void Tutorial::Init()
{
    Clear();
    enabled_ = false;
}

void Tutorial::SetEnabled(bool enable)
{
    URHO3D_LOGINFOF("Tutorial() - SetEnabled = %s !", enable ? "true":"false");

    enabled_ = enable;

    if (!enabled_)
    {
        // close the frame if open
        if (frame_)
            frame_->Stop();

        UnsubscribeFromEvent(GAME_NOMATCHSTATE);
        UnsubscribeFromEvents();
    }
    else
    {
        SubscribeToEvent(GAME_NOMATCHSTATE, URHO3D_HANDLER(Tutorial, HandleTutorialStart));
        SubscribeToEvents();
    }
}

void Tutorial::SubscribeToEvents()
{
    if (!GameStatics::rootScene_)// || !manager_->datas_.Size())
        return;

    SubscribeToEvent(TUTORIAL_LAUNCH, URHO3D_HANDLER(Tutorial, HandleTutorialLaunch));
    SubscribeToEvent(TUTORIAL_NEXT, URHO3D_HANDLER(Tutorial, HandleTutorialNext));
    SubscribeToEvent(GAME_POWERADDED, URHO3D_HANDLER(Tutorial, HandleEvents));
}

void Tutorial::UnsubscribeFromEvents()
{
    UnsubscribeFromEvent(TUTORIAL_LAUNCH);
    UnsubscribeFromEvent(TUTORIAL_NEXT);
    UnsubscribeFromEvent(GAME_POWERADDED);
}

extern Color MatchColors[NUMCOLORTYPES];

// The launch of a tutorial is now delayed intentionnally
// because when Tutorial::Start() is executed, the objects on the grid are not always on their final position.
void Tutorial::HandleTutorialLaunch(StringHash eventType, VariantMap& eventData)
{
    if (!enabled_)
        return;

    if (MatchesManager::GetState() != NoMatchState)
        return;

    if (Game::Get()->GetCompanion() && Game::Get()->GetCompanion()->IsVisible())
    {
        URHO3D_LOGINFOF("Tutorial() - Launch ... : Companion visible ! wait");
        return;
    }

    TutorialInfo tinfo = tutorialInfos_.Front();

    if (!tinfo.node_)
    {
        tutorialInfos_.PopFront();
        DelayInformer::Get(this, 0.f, TUTORIAL_NEXT);
        return;
    }

    if (GameStatics::playerState_->powers_[tinfo.powerid_-1].shown_)
    {
        tutorialInfos_.PopFront();
        DelayInformer::Get(this, 0.f, TUTORIAL_NEXT);
        return;
    }

    // Check if the power has an active solution
    Match* m = MatchesManager::GetGrid().GetMatch(tinfo.node_);
    if (m && MatchesManager::GetGrid().IsPower(*m) && !MatchesManager::GetGrid().IsPowerActivable(*m))
    {
        URHO3D_LOGINFOF("Tutorial() - Launch ... power not activable ... skip it (tutorialinfos=%u) !", tutorialInfos_.Size());
        if (tutorialInfos_.Size() > 1)
        {
            tutorialInfos_.PopFront();
            // put on back for later use
            tutorialInfos_.Push(tinfo);

            DelayInformer::Get(this, 0.f, TUTORIAL_NEXT);
        }
        else
        {
            // wait
            DelayInformer::Get(this, 3.f, TUTORIAL_NEXT);
        }
        return;
    }

    URHO3D_LOGINFOF("Tutorial() - Launch ...");

    // Mark tutorial as shown
    GameStatics::playerState_->powers_[tinfo.powerid_-1].shown_ = 1;

    // Add Touch Effect for tutorial activation
    MatchesManager::GetGrid().AddEffectInNode(tinfo.node_, 4, 10.f, 5.f, Color::YELLOW/*MatchColors[tinfo.colorid_]*/);

    if (frame_)
        frame_->Stop();

    // Add Tutorial Frame
    frame_ = GameHelpers::AddInteractiveFrame("UI/Tutorial/GameTutorialFrame.xml");
    frame_->SetBehavior(IB_BUTTON);
    frame_->SetLayer(TUTORIALLAYER);

    // Set the Content
    Node* content = frame_->GetNode()->GetChild("Content");
    String animationname(ToString("Power%d", tinfo.powerid_));
    AnimatedSprite2D* animated = content->GetComponent<AnimatedSprite2D>(LOCAL);
    if (animated->HasAnimation(animationname))
        animated->SetAnimation(animationname);
    else
        animated->SetAnimation("NoPower");

    // Set the position of the frame to prevent the covering of the object 'power'
    IntRect framerect;
    GameHelpers::OrthoWorldToScreen(framerect, frame_->GetNode()->GetChild("Frame")->GetComponent<StaticSprite2D>()->GetWorldBoundingBox2D());
    IntVector2 position, framepos;
    GameHelpers::OrthoWorldToScreen(position, tinfo.node_);
    const int screenw = GameStatics::graphics_->GetWidth();
    const int screenh = GameStatics::graphics_->GetHeight();
    framepos.x_ = screenw/2;
    if (position.y_ < screenh/2)
        framepos.y_ = position.y_ + (int)floor(framerect.Size().y_/1.5f);
    else
        framepos.y_ = position.y_ - (int)floor(framerect.Size().y_/1.5f);
    framepos.y_ = Clamp(framepos.y_, framerect.Size().y_/2, screenh-framerect.Size().y_/2);
    frame_->SetScreenPositionEntrance(IntVector2(position.x_ < screenw/2 ? screenw+framerect.Size().x_/2 : -framerect.Size().x_/2, framepos.y_));
    frame_->SetScreenPosition(framepos);
    content->SetEnabledRecursive(false);

    String tutoActivationMessage = String("tuto_activation")+animationname;
    if (Game::Get()->GetCompanion() && Game::Get()->GetCompanion()->IsNewMessage(tutoActivationMessage))
    {
        URHO3D_LOGINFOF("Tutorial() - Launch : add companion message=%s", tutoActivationMessage.CString());
        Game::Get()->GetCompanion()->PushFrontMessage(STATE_PLAY, false, tutoActivationMessage, "capi", 0.f);
        Game::Get()->GetCompanion()->StartMessage(true, true);
        SubscribeToEvent(MESSAGE_STOP, URHO3D_HANDLER(Tutorial, HandleTutorialFrameStopped));
        frame_->SetBreakInput(false);
    }
    else
    {
        frame_->SetBreakInput(true);
        SubscribeToEvent(frame_, E_MESSAGEACK, URHO3D_HANDLER(Tutorial, HandleTutorialFrameStopped));
    }

    // Start Frame
    frame_->Start(true, false);

    URHO3D_LOGINFOF("Tutorial() - Launch : Add Animation %s pos=%s framepos=%s framerectsize=%s", animationname.CString(), position.ToString().CString(),
                    framepos.ToString().CString(), framerect.Size().ToString().CString());
}

void Tutorial::HandleTutorialNext(StringHash eventType, VariantMap& eventData)
{
    if (!tutorialInfos_.Size())
        return;

//    tutorialInfos_.PopFront();

    // Launch the next tutorial if exists
    if (tutorialInfos_.Size())
        DelayInformer::Get(this, 2.f, TUTORIAL_LAUNCH);
}

void Tutorial::HandleEvents(StringHash eventType, VariantMap& eventData)
{
    if (eventType == GAME_POWERADDED)
    {
        Node* node = static_cast<Node*>(eventData[Game_PowerAdded::NODE].GetPtr());
        if (!node)
            return;

        unsigned property = eventData[Game_PowerAdded::MATCHPROPERTY].GetUInt();
        MatchProperty* mprop = (MatchProperty*)(&property);
        int powerid = mprop->otype_+1;

        // Add Tutorial to Show
        tutorialInfos_.Push(TutorialInfo(node, powerid, mprop->ctype_));

        URHO3D_LOGINFOF("Tutorial() - HandleEvents : GAME_POWERADDED powerid=%d color=%d", powerid, mprop->ctype_);

        if (!enabled_)
            return;
    }
}

void Tutorial::HandleTutorialStart(StringHash eventType, VariantMap& eventData)
{
    if (GameStatics::numRemainObjectives_ <= 0)
        return;

    URHO3D_LOGINFOF("Tutorial() - HandleTutorialStart !");

    DelayInformer::Get(this, 3.f, TUTORIAL_LAUNCH);
}

void Tutorial::HandleTutorialFrameStopped(StringHash eventType, VariantMap& eventData)
{
    if (tutorialInfos_.Size())
    {
        if (eventType == MESSAGE_STOP)
        {
            frame_->Stop();
            UnsubscribeFromEvent(MESSAGE_STOP);
        }
        else
        {
            // Unsubscribe the previous frame
            InteractiveFrame* stoppedframe = static_cast<InteractiveFrame*>(GameStatics::context_->GetEventSender());
            if (stoppedframe)
                UnsubscribeFromEvent(stoppedframe, E_MESSAGEACK);
        }

        DelayInformer::Get(this, 0.f, TUTORIAL_NEXT);
    }
}

//
//void Tutorial::DrawDebugGeometry(DebugRenderer* debug, bool depth) const
//{
//
//}
