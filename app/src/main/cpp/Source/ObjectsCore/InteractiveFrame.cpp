#include <Urho3D/Urho3D.h>

#include <Urho3D/Core/Object.h>
#include <Urho3D/Core/Context.h>

#include <Urho3D/IO/Log.h>

#include <Urho3D/Resource/Localization.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Resource/XMLFile.h>

#include <Urho3D/Graphics/Viewport.h>
#include <Urho3D/Graphics/DebugRenderer.h>

#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Scene/SceneEvents.h>
#include <Urho3D/Scene/ObjectAnimation.h>
#include <Urho3D/Scene/ValueAnimation.h>

#include <Urho3D/Urho2D/Sprite2D.h>
#include <Urho3D/Urho2D/StaticSprite2D.h>
#include <Urho3D/Urho2D/AnimatedSprite2D.h>
#include <Urho3D/Urho2D/PhysicsWorld2D.h>
#include <Urho3D/Urho2D/RigidBody2D.h>

#include <Urho3D/Input/Input.h>
#include <Urho3D/Input/InputEvents.h>

#include <Urho3D/UI/UI.h>
#include <Urho3D/UI/UIEvents.h>
#include <Urho3D/UI/Font.h>
#include <Urho3D/UI/Text3D.h>

#include "Game.h"
#include "GameUI.h"
#include "GameStatics.h"
#include "GameHelpers.h"
#include "GameOptions.h"
#include "GameEvents.h"
#include "GameAttributes.h"

#include "MAN_Matches.h"

#include "DelayAction.h"
#include "DelayInformer.h"
#include "TimerRemover.h"

#include "InteractiveFrame.h"



const String INTERACTIVEKEYWORD("Interactive_");
const String ABILITYKEYWORD("Interactive_Ability");
const String CONTENTKEYWORD("Content");
const String FRAMEKEYWORD("Frame");
const String TITLEKEYWORD("Title");

enum InteractiveItemState
{
    SELECTED = 0,
    UNSELECTED,
    NUM_ITERACTIVEITEMSTATE
};

const String InteractiveItemStateStr[NUM_ITERACTIVEITEMSTATE] =
{
    String("selected"), String("unselected")
};

const Vector3 BONUSPOSITION[5] = { Vector3::ZERO, Vector3(-1.f, 0.f, 0.f), Vector3(1.f, 0.f, 0.f), Vector3(-0.5f, -1.f, 0.f), Vector3(0.5f, -1.f, 0.f) };

const float ABILITYPOSITIONGAP = 1.f;

int InteractiveFrame::numRunningInstances_ = 0;
bool InteractiveFrame::allowInputs_ = true;
HashMap<String, Vector<SharedPtr<InteractiveFrame> > > InteractiveFrame::instances_;

InteractiveFrame::InteractiveFrame(Context* context, const String& layoutnodefile) :
    Object(context), exitindex_(0), layer_(10000), state_(IFrame_Cleaned), releaseSelectionContactEnd_(true), breakinput_(false), bonusEnabled_(false),
    behavior_(IB_MESSAGEBOX), selectionMode_(0), clicsOnSelection_(0), autostarttime_(0.f), autostoptime_(0.f), bonusstarttime_(0.5f)
{
    URHO3D_LOGINFOF("InteractiveFrame() - Create from %s ...", layoutnodefile.CString());

    Node* interactiveRoot = GameStatics::rootScene_->GetChild(INTERACTIVES_ROOT);
    if (!interactiveRoot)
        interactiveRoot = GameStatics::rootScene_->CreateChild(INTERACTIVES_ROOT, LOCAL);

    layoutname_ = layoutnodefile;

    Init();

    URHO3D_LOGINFOF("InteractiveFrame() - Create from %s ... OK !", layoutnodefile.CString());
}

InteractiveFrame::~InteractiveFrame()
{
    URHO3D_LOGINFOF("~InteractiveFrame() : %s ... ", framename_.CString());

    Clean();

    URHO3D_LOGINFOF("~InteractiveFrame() : %s ... OK !", framename_.CString());
}


InteractiveFrame* InteractiveFrame::Get(const String& layoutfilename, bool inCleanState)
{
    HashMap<String, Vector<SharedPtr<InteractiveFrame> > >::Iterator it = instances_.Find(layoutfilename);

    if (it == instances_.End())
    {
        // no instance : add a new one
        SharedPtr<InteractiveFrame> frame(new InteractiveFrame(GameStatics::context_, layoutfilename));
        instances_[layoutfilename].Push(frame);
        return frame.Get();
    }

    // don't need an instance in cleanstate, so get the first one
    if (!inCleanState)
        return it->second_.Front().Get();

    // search for an instance in cleanstate
    Vector<SharedPtr<InteractiveFrame> >& frames = it->second_;
    for (Vector<SharedPtr<InteractiveFrame> >::Iterator jt = frames.Begin(); jt != frames.End(); ++jt)
    {
        if ((*jt)->state_ == IFrame_Cleaned)
            return jt->Get();
    }

    // there's no instance in cleanState : add a new one
    frames.Push(SharedPtr<InteractiveFrame>(new InteractiveFrame(GameStatics::context_, layoutfilename)));
    return frames.Back().Get();
}

void InteractiveFrame::FreeAll()
{
    for (HashMap<String, Vector<SharedPtr<InteractiveFrame> > >::Iterator it = instances_.Begin(); it != instances_.End(); ++it)
    for (Vector<SharedPtr<InteractiveFrame> >::Iterator jt = it->second_.Begin(); jt != it->second_.End(); ++jt)
        (*jt)->Clean();
}

void InteractiveFrame::Reset()
{
    instances_.Clear();
}


void InteractiveFrame::Init()
{
    if (!node_)
    {
        node_ = GameStatics::rootScene_->GetChild(INTERACTIVES_ROOT)->CreateChild(String::EMPTY, LOCAL);

        // Load frame node
        if (!GameHelpers::LoadNodeXML(context_, node_, layoutname_, LOCAL))
        {
            URHO3D_LOGINFOF("InteractiveFrame() - Init : Can't load layout=%s ... NOK !", layoutname_.CString());
            return;
        }

        framename_ = node_->GetName();

        // Add initial "interactive" nodes
        interactiveItems_.Clear();

        PODVector<Node*> children;
        node_->GetChildren(children);
        for (int i=0; i < children.Size(); i++)
        {
            Node*& node = children[i];

            if (node->GetName().StartsWith(INTERACTIVEKEYWORD))
            {
                URHO3D_LOGINFOF("InteractiveFrame() - Init : Node[%d]=%s interactive", i, node->GetName().CString());
                interactiveItems_.Push(node);
            }
        }
    }

    if (node_)
        node_->SetEnabledRecursive(false);
}

void InteractiveFrame::Start(bool force, bool instant)
{
    if (state_ != IFrame_Started)
    {
        if (!node_ || force)
            Init();

        PODVector<Node*> children;
        node_->GetChildren(children);

        for (int i=0; i< children.Size(); i++)
        {
            Node*& node = children[i];

            if (node->GetName().StartsWith(INTERACTIVEKEYWORD))
                SetLayering(node, layer_+3);
            else if (node->GetName().StartsWith(CONTENTKEYWORD))
                SetLayering(node, layer_+2);
            else if (node->GetName().StartsWith(TITLEKEYWORD))
                SetLayering(node, layer_+1);
            else
                SetLayering(node, layer_);
        }

		if (autostarttime_)
        	UnsubscribeFromEvent(this, GAME_UIFRAME_ADD);

        node_->SetEnabledRecursive(true);

        ResetAnimations();

        bool interactivesok = IsInteractivesEnabled();
        if (!force)
            if (!interactivesok && !autostoptime_ && behavior_ == IB_BUTTON)
                breakinput_ = true;

        if (behavior_ == IB_ABILITIES)
        {
            SubscribeToEvent(GAME_ABILITYRESETED, URHO3D_HANDLER(InteractiveFrame, OnAbilityReseted));
            SubscribeToEvent(GAME_ABILITYADDED, URHO3D_HANDLER(InteractiveFrame, OnAbilityAdded));
            SubscribeToEvent(GAME_ABILITYREMOVED, URHO3D_HANDLER(InteractiveFrame, OnAbilityRemoved));
        }

        GameHelpers::SetMoveAnimation(node_, positionentrance_, position_, 0.f, instant ? 0.01f : 0.2f);

        if (bonusItems_.Size())
        {
            unsigned maxbonus = Min(bonusItems_.Size(), 5);
            for (unsigned i=0; i < maxbonus; ++i)
                GameHelpers::SetMoveAnimation(bonusItems_[i], positionentrancebonus_, BONUSPOSITION[i], bonusstarttime_ + 0.5f*float(i), 0.2f);

            if (bonusEnabled_)
                GameStatics::AddBonuses(GetBonuses(), this);
        }

        lastSelectedNode_ = 0;
        selectedNode_ = 0;
        selectedId_ = 0;
        clicsOnSelection_ = 0;

        if (breakinput_)
            SubscribeToBreakInputEvents();
        else
            SubscribeToInputEvents();

        if (autostoptime_)
        {
            SubscribeToEvent(this, GAME_UIFRAME_STOP, URHO3D_HANDLER(InteractiveFrame, HandleAutoStop));
            DelayInformer::Get(this, autostoptime_, GAME_UIFRAME_STOP);
        }

        SubscribeToEvent(this, GAME_UIFRAME_REMOVE, URHO3D_HANDLER(InteractiveFrame, HandleClean));

        state_ = IFrame_Started;

        if (behavior_ != IB_ABILITIES)
            numRunningInstances_ = Min(numRunningInstances_+1, 1);

        if (behavior_ == IB_MESSAGEBOX)
            MatchesManager::SetPhysicsEnable(false);

        URHO3D_LOGINFOF("InteractiveFrame() - Start : this=%u %s Started behavior=%d breakinput=%s !", this, framename_.CString(), behavior_, breakinput_?"true":"false");

        SendEvent(GAME_UIFRAME_START);
    }
}

void InteractiveFrame::Stop()
{
    if (state_ == IFrame_Started)
    {
        state_ = IFrame_Stopped;
        numRunningInstances_ = Max(numRunningInstances_-1, 0);

        UnsubscribeFromAllEvents();

        if (node_ && node_->IsEnabled())
        {
            if (nodeBonus_)
                node_->AddChild(nodeBonus_);

            GameHelpers::SetMoveAnimation(node_, position_, positionexit_[exitindex_], 0.f, 0.2f);
        }

        SubscribeToEvent(this, GAME_UIFRAME_REMOVE, URHO3D_HANDLER(InteractiveFrame, HandleClean));
        DelayInformer::Get(this, 0.21f, GAME_UIFRAME_REMOVE);

        if (behavior_ == IB_MESSAGEBOX)
            MatchesManager::SetPhysicsEnable(true);

        bonuses_.Clear();
        bonusItems_.Clear();

        URHO3D_LOGINFOF("InteractiveFrame() - Stop : this=%u name=%s Stopped !", this, framename_.CString());
    }
}

void InteractiveFrame::Clean()
{
    Stop();

    state_ = IFrame_Cleaned;

    UnsubscribeFromAllEvents();

    if (node_)
    {
        node_->Remove();
        node_.Reset();
    }

    if (nodeBonus_)
    {
        nodeBonus_->Remove();
        nodeBonus_.Reset();
    }

    for (unsigned i = 0; i < abilities_.Size(); i++)
        abilities_[i]->Remove();
    abilities_.Clear();

    interactiveItems_.Clear();

    URHO3D_LOGINFOF("InteractiveFrame() - Clean : %s remove nodes from scene !", framename_.CString());
}


void InteractiveFrame::SetScreenPosition(const IntVector2& position, bool instant)
{
    position_ = GameHelpers::ScreenToWorld2D_FixedZoom(position);

    if (instant)
    {
        if (!node_)
            Init();
        node_->SetPosition(position_);
    }
}

void InteractiveFrame::SetScreenPositionEntrance(const IntVector2& position)
{
    positionexit_[0] = positionentrance_ = GameHelpers::ScreenToWorld2D_FixedZoom(position);
}

void InteractiveFrame::SetScreenPositionExit(int index, const IntVector2& position)
{
    positionexit_[index] = GameHelpers::ScreenToWorld2D_FixedZoom(position);
}

void InteractiveFrame::SetScreenPositionEntranceForBonus(const IntVector2& position)
{
    positionentrancebonus_ = GameHelpers::ScreenToWorld2D_FixedZoom(position);
}

void InteractiveFrame::SetSelectionMode(int mode)
{
    selectionMode_ = mode;
}

void InteractiveFrame::SetAutoStart(float time)
{
	if (autostarttime_)
		UnsubscribeFromEvent(this, GAME_UIFRAME_ADD);

    autostarttime_ = time;

    if (autostarttime_)
    {
        state_ = IFrame_Starting;
        SubscribeToEvent(this, GAME_UIFRAME_ADD, URHO3D_HANDLER(InteractiveFrame, HandleAutoStart));
        DelayInformer::Get(this, autostarttime_, GAME_UIFRAME_ADD);
    }
}

void InteractiveFrame::SetAutoStop(float time)
{
    autostoptime_ = time;
}

void InteractiveFrame::SetBreakInput(bool enabled)
{
    breakinput_ = enabled;
}

void InteractiveFrame::SetText(const String& name, const String& text, bool localization)
{
    if (!node_)
        Init();

    Node* node = node_->GetChild(name);
    if (node)
    {
        Text3D* text3d = node->GetComponent<Text3D>();
        if (text3d)
        {
            if (localization)
                text3d->SetText(GetSubsystem<Localization>()->Get(text));
            else
                text3d->SetText(text);
        }
    }
}

void InteractiveFrame::SetReleaseSelectionOnContactEnd(bool state)
{
    releaseSelectionContactEnd_ = state;
}

void InteractiveFrame::SetLayer(int layer)
{
    layer_ = layer;
}

void InteractiveFrame::SetLayering(Node* node, int layer)
{
    Drawable2D* drawable = node->GetDerivedComponent<Drawable2D>();

    if (!drawable)
        return;

    URHO3D_LOGINFOF("InteractiveFrame() - SetLayering : node=%s(%u) layer=%d !", node->GetName().CString(), node->GetID(), layer);

    drawable->SetLayer(layer);
}

void InteractiveFrame::SetBehavior(int behavior)
{
    behavior_ = behavior;

    if (!node_)
        Init();

    if (behavior_ == IB_ABILITIES)
    {
        SubscribeToEvent(GAME_ABILITYADDED, URHO3D_HANDLER(InteractiveFrame, OnAbilityAdded));

        UpdateAbilities();
    }
}

void InteractiveFrame::SetAbilityMoveZone(int dleft, int dright, float dy)
{
    abilityMoveRect_.min_ = Vector2(dleft * ABILITYPOSITIONGAP, -dy);
    abilityMoveRect_.max_ = Vector2(dright * ABILITYPOSITIONGAP, dy);
}

void InteractiveFrame::SetBonusEnabled(bool state)
{
    bonusEnabled_ = state;
}

void InteractiveFrame::SetBonusStart(float time)
{
    bonusstarttime_ = time;
}



void InteractiveFrame::UpdateAbilities()
{
    // Check Static Abilities
    for (unsigned i=0; i < MAXABILITIES; i++)
    {
        if (GameStatics::playerState_->powers_[i].enabled_ && GameStatics::playerState_->powers_[i].qty_ > 0)
            AddAbility(i+1);
    }
}

void InteractiveFrame::FindAbilities()
{
    if (!node_)
        return;

    Node* node = 0;
    unsigned index = 0;
    abilities_.Clear();

    PODVector<Node*> children;
    node_->GetChildren(children);

    if (children.Size())
    {
        // Find Ability Type
        for (unsigned i=0; i < children.Size(); i++)
        {
            if (children[i]->GetName().StartsWith(ABILITYKEYWORD))
            {
                int checkedability = ToInt(children[i]->GetName().Substring(ABILITYKEYWORD.Length()));

//                URHO3D_LOGINFOF("InteractiveFrame() - FindAbility : Find Ability=%s(%d) !", children[i]->GetName().CString(), checkedability);
                int index=0;
                for (unsigned j=0; j < abilities_.Size(); j++)
                {
                    if (checkedability < ToInt(abilities_[j]->GetName().Substring(ABILITYKEYWORD.Length())))
                        break;

                    index++;
                }

                abilities_.Insert(index, children[i]);
            }
        }
    }
}

void InteractiveFrame::AddAbility(int ability, bool autostart)
{
    if (behavior_ != IB_ABILITIES || !ability || !node_)
        return;

    unsigned char& qty = GameStatics::playerState_->powers_[ability-1].qty_;

    URHO3D_LOGINFOF("InteractiveFrame() - AddAbility : ability=%d qty=%u !", ability, qty);

    if (autostart)
        Start(true, true);

    Node* node = 0;
    unsigned index = 0;

    // Find Ability Type
    FindAbilities();

    for (unsigned i=0; i < abilities_.Size(); i++)
    {
        int checkedability = ToInt(abilities_[i]->GetName().Substring(ABILITYKEYWORD.Length()));

        if (checkedability == ability)
        {
            node = abilities_[i];
            break;
        }
        else if (checkedability < ability)
            index++;
    }

    // Create Ability Type
    if (!node)
    {
        URHO3D_LOGINFOF("InteractiveFrame() - AddAbility : Create New Ability %d, insert index=%u!", ability, index);

        // Create node
        node = GOT::GetObject(StringHash("Abilities"))->Clone(LOCAL, true, 0, 0, node_);
        interactiveItems_.Push(node);
        abilities_.Insert(index, node);

        float offsetx = (float)(abilities_.Size()-1) * ABILITYPOSITIONGAP * 0.5f;

        node->SetName(ABILITYKEYWORD + String(ability));
        node->SetVar(GOA::BONUS, ability);
        node->SetPosition(Vector3(index * ABILITYPOSITIONGAP -  offsetx, 0.f, 0.f));

        AnimatedSprite2D* animatedsprite = node->GetComponent<AnimatedSprite2D>();
        animatedsprite->SetEntity(ToString("Ability%d", ability));
        animatedsprite->SetAnimation("unselect");
        animatedsprite->SetLayer(ABILITYLAYER);

        // Move Nodes
        for (unsigned i=0; i < abilities_.Size(); i++)
        {
            Node* abinode = abilities_[i];
            GameHelpers::SetMoveAnimation(abinode, abinode->GetPosition(), Vector3(i * ABILITYPOSITIONGAP - offsetx, 0.f, 0.f), 0.f, 0.2f);
        }
    }

    // Set Qty Text
    if (qty > 1)
    {
        Text3D* qtytxt = node->GetComponent<Text3D>();
//        Text3D* qtytxt = node->GetOrCreateComponent<Text3D>();
        if (qtytxt)
        {
//            URHO3D_LOGINFOF("InteractiveFrame() - AddAbility : ability=%d -> Set Quantity Text Label to %d !", ability, qty);
//            qtytxt->SetFont(GetSubsystem<ResourceCache>()->GetResource<Font>("Fonts/BlueHighway.sdf"), 48);
            qtytxt->SetText(String((int)qty));
        }
    }
}

void InteractiveFrame::RemoveAbility(int ability)
{
    if (behavior_ != IB_ABILITIES || !ability)
        return;

    unsigned char& qty = GameStatics::playerState_->powers_[ability-1].qty_;

    URHO3D_LOGINFOF("InteractiveFrame() - RemoveAbility : ability=%d qty=%u !", ability, qty);

    FindAbilities();

    Node* node = 0;
    unsigned index = 0;

    for (unsigned i=0; i < abilities_.Size(); i++)
    {
        int checkedability = ToInt(abilities_[i]->GetName().Substring(ABILITYKEYWORD.Length()));

        if (checkedability == ability)
        {
            index = i;
            node = abilities_[i];
            break;
        }
    }

    if (node)
    {
        Text3D* qtytxt = node->GetComponent<Text3D>();

        if (!qty)
        {
            float offsetx = (float)(abilities_.Size()-1) * ABILITYPOSITIONGAP * 0.5f;

            if (qtytxt)
                qtytxt->SetText(String::EMPTY);

            abilities_.Erase(index);
            interactiveItems_.Remove(node);

            GameHelpers::SetMoveAnimation(node, node->GetPosition(), Vector3(index * ABILITYPOSITIONGAP - offsetx, -1.f, 0.f), 0.f, 0.2f);
            TimerRemover::Get()->Start(node, 0.25f);

            // Move Nodes
            offsetx -= ABILITYPOSITIONGAP*0.5f;
            for (unsigned i=0; i < abilities_.Size(); i++)
                GameHelpers::SetMoveAnimation(abilities_[i], abilities_[i]->GetPosition(), Vector3(i * ABILITYPOSITIONGAP - offsetx, 0.f, 0.f), 0.f, 0.2f);
        }
        else
        {
            if (qtytxt)
                qtytxt->SetText(qty > 1 ? String((int)qty) : String::EMPTY);
        }
    }
}

void InteractiveFrame::MoveAbilities(const Vector2& offset)
{
    if (abilities_.Size() < 3)
        return;

    float dx = offset.x_;

    if (offset.x_ < 0.f)
    {
        if (abilityMoveRect_.IsInside(abilities_.Back()->GetPosition2D()) == INSIDE)
            return;

        dx = Max(offset.x_, abilityMoveRect_.max_.x_ - abilities_.Back()->GetPosition2D().x_);
    }
    else if (offset.x_ > 0.f)
    {
        if (abilityMoveRect_.IsInside(abilities_.Front()->GetPosition2D()) == INSIDE)
            return;

        dx = Min(offset.x_, abilityMoveRect_.min_.x_ - abilities_.Front()->GetPosition2D().x_);
    }

    if (dx == 0.f)
        return;

    for (unsigned i=0; i < abilities_.Size(); i++)
        GameHelpers::SetMoveAnimation(abilities_[i], abilities_[i]->GetPosition(), abilities_[i]->GetPosition() + Vector3(dx, 0.f, 0.f), 0.f, 0.2f);
}

void InteractiveFrame::AddBonus(const Slot& slot)
{
    URHO3D_LOGINFOF("InteractiveFrame() - AddBonus : Slot(%s) ...", slot.GetString().CString());

    if (!slot.cat_)
        return;

    if (!nodeBonus_)
        nodeBonus_ = GameStatics::rootScene_->GetChild(INTERACTIVES_ROOT)->CreateChild(framename_+String("_Bonus"), LOCAL);

    Node* templatebonus = COT::GetObjectFromCategory(StringHash(slot.cat_), slot.indexcat_);
    if (templatebonus)
    {
        Node* bonusitem = templatebonus->Clone(LOCAL, true, 0, 0, nodeBonus_);
        URHO3D_LOGINFOF("InteractiveFrame() - AddBonus : slotqty=%d !!!", slot.qty_);
        if (slot.qty_ > 1 && bonusitem->GetVar(GOA::BONUS).GetInt() != slot.qty_)
        {
            ResourceCache* cache = context_->GetSubsystem<ResourceCache>();
            Font* font = cache->GetResource<Font>("Fonts/BlueHighway.ttf");
            Text3D* text = bonusitem->CreateComponent<Text3D>();
            text->SetText(String((int)slot.qty_));
            text->SetFont(font, 75);
            text->SetAlignment(HA_LEFT, VA_TOP);
        }
        SetLayering(bonusitem, layer_+1);

        bonusItems_.Push(bonusitem);
        bonuses_.Push(slot);
    }
    else
        URHO3D_LOGERRORF("InteractiveFrame() - AddBonus : templateBonus ERROR on bonus=%d !!!", slot.indexcat_);
}

void InteractiveFrame::ResetAnimations()
{
    if (!node_)
        return;

    Node* content = node_->GetChild("Content");
    if (content)
    {
        AnimatedSprite2D* anim = content->GetComponent<AnimatedSprite2D>();
        if (anim)
            anim->SetAnimation("play");
    }

    for (unsigned i=0; i < interactiveItems_.Size(); i++)
        SetItemState(interactiveItems_[i], UNSELECTED);
}

void InteractiveFrame::SetItemState(Node* node, int state)
{
    AnimatedSprite2D* anim = node->GetComponent<AnimatedSprite2D>();
    if (anim)
        anim->SetAnimation(InteractiveItemStateStr[state]);

    Node* child = node->GetChild(0U);
    Text3D* text = child ? child->GetComponent<Text3D>() : 0;
    if (text)
    {
        if (state == UNSELECTED)
        {
            child->SetScale(Vector3(1.5f, 1.5f, 1.f));
            text->SetColor(Color(1.f,0.3f,0.f, 0.8f));
        }
        else
        {
            child->SetScale(Vector3(1.6f, 1.6f, 1.f));
            text->SetColor(Color::WHITE);
        }
    }
}

bool InteractiveFrame::IsInteractivesEnabled() const
{
    for (unsigned i=0; i < interactiveItems_.Size(); ++i)
        if (interactiveItems_[i]->IsEnabled())
            return true;

    return false;
}


void InteractiveFrame::SubscribeToInputEvents()
{
    if (behavior_ != IB_ABILITIES)
        SubscribeToEvent(E_KEYDOWN, URHO3D_HANDLER(InteractiveFrame, HandleKeyDown));
    SubscribeToEvent(E_KEYUP, URHO3D_HANDLER(InteractiveFrame, HandleInput));

    SubscribeToEvent(E_JOYSTICKBUTTONDOWN, URHO3D_HANDLER(InteractiveFrame, HandleInput));
    SubscribeToEvent(E_JOYSTICKAXISMOVE, URHO3D_HANDLER(InteractiveFrame, HandleInput));
    SubscribeToEvent(E_JOYSTICKHATMOVE, URHO3D_HANDLER(InteractiveFrame, HandleInput));

    SubscribeToEvent(E_TOUCHEND, URHO3D_HANDLER(InteractiveFrame, HandleInput));
    SubscribeToEvent(E_TOUCHBEGIN, URHO3D_HANDLER(InteractiveFrame, HandleInput));
    SubscribeToEvent(E_TOUCHMOVE, URHO3D_HANDLER(InteractiveFrame, HandleInput));

    SubscribeToEvent(E_MOUSEMOVE, URHO3D_HANDLER(InteractiveFrame, HandleInput));
    SubscribeToEvent(E_MOUSEBUTTONDOWN, URHO3D_HANDLER(InteractiveFrame, HandleInput));
}

void InteractiveFrame::SubscribeToBreakInputEvents()
{
    SubscribeToEvent(E_KEYDOWN, URHO3D_HANDLER(InteractiveFrame, HandleBreakInput));
    SubscribeToEvent(E_JOYSTICKBUTTONDOWN, URHO3D_HANDLER(InteractiveFrame, HandleBreakInput));
    SubscribeToEvent(E_TOUCHBEGIN, URHO3D_HANDLER(InteractiveFrame, HandleBreakInput));
    SubscribeToEvent(E_MOUSEBUTTONDOWN, URHO3D_HANDLER(InteractiveFrame, HandleBreakInput));
}


void InteractiveFrame::HandleAutoStart(StringHash eventType, VariantMap& eventData)
{
    Start(false, false);
}

void InteractiveFrame::HandleAutoStop(StringHash eventType, VariantMap& eventData)
{
    Stop();
}

void InteractiveFrame::HandleClean(StringHash eventType, VariantMap& eventData)
{
    Clean();
}

void InteractiveFrame::HandleUpdate(StringHash eventType, VariantMap& eventData)
{

}

void InteractiveFrame::HandleKeyDown(StringHash eventType, VariantMap& eventData)
{
    if (eventData[KeyDown::P_KEY].GetInt() == KEY_ESCAPE)
    {
        URHO3D_LOGINFOF("InteractiveFrame() - HandleKeyDown : this=%u E_MESSAGEACK !", this);

        Stop();

        eventData[MessageACK::P_OK] = false;
        SendEvent(E_MESSAGEACK, eventData);
    }
}

void InteractiveFrame::HandleInput(StringHash eventType, VariantMap& eventData)
{
    if (!allowInputs_)
        return;

    static bool inside = false;
    bool move = false;
    bool launch = false;
    const bool mouseevents = eventType == E_MOUSEMOVE || eventType == E_MOUSEBUTTONDOWN;
    const bool touchevents = eventType == E_TOUCHEND || eventType == E_TOUCHMOVE || eventType == E_TOUCHBEGIN;

    /// Set Mouse & Touch Events
    if (mouseevents || touchevents)
    {
        const bool mbuttonok = GameStatics::input_->GetMouseButtonDown(MOUSEB_LEFT);

        IntVector2 position;

        if (touchevents)
        {
            if (eventType == E_TOUCHEND)
            {
                position.x_ = eventData[TouchEnd::P_X].GetInt();
                position.y_ = eventData[TouchEnd::P_Y].GetInt();
            }
            else if (eventType == E_TOUCHMOVE)
            {
                position.x_ = eventData[TouchMove::P_X].GetInt();
                position.y_ = eventData[TouchMove::P_Y].GetInt();
//                URHO3D_LOGINFOF("InteractiveFrame() - HandleInput : E_TOUCHMOVE pos=%s !", position.ToString().CString());
            }
            else if (eventType == E_TOUCHBEGIN)
            {
                position.x_ = eventData[TouchBegin::P_X].GetInt();
                position.y_ = eventData[TouchBegin::P_Y].GetInt();
    //            URHO3D_LOGINFOF("InteractiveFrame() - HandleInput : E_TOUCHBEGIN pos=%s !", position.ToString().CString());
            }
        }
        else
        {
            position = GameStatics::input_->GetMousePosition();
//            URHO3D_LOGINFOF("InteractiveFrame() - HandleInput : E_MOUSEMOVE pos=%s !", position.ToString().CString());
        }

        const Vector2 wpos = GameHelpers::ScreenToWorld2D(position);

        if (behavior_ == IB_ABILITIES)
        {
            if (eventType == E_TOUCHMOVE)
            {
                if (Abs(wpos.y_ - node_->GetWorldPosition2D().y_) < abilityMoveRect_.max_.y_)
                    MoveAbilities(Vector2(eventData[TouchMove::P_DX].GetInt(), 0.f));
            }
            else if (mbuttonok && eventType == E_MOUSEMOVE)
            {
                if (Abs(wpos.y_ - node_->GetWorldPosition2D().y_) < abilityMoveRect_.max_.y_)
                    MoveAbilities(Vector2(eventData[MouseMove::P_DX].GetInt(), 0.f));
            }
        }

        if (selectionMode_ == SELECTIONONMOVE)
        {
            if (eventType != E_MOUSEBUTTONDOWN)
            {
                RigidBody2D* body = GameStatics::rootScene_->GetComponent<PhysicsWorld2D>()->GetRigidBody(wpos, 8);
                if (body && body->GetNode() && interactiveItems_.Contains(body->GetNode()))
                {
                    if (lastSelectedNode_ != body->GetNode())
                    {
                        lastSelectedNode_ = selectedNode_;
                        selectedNode_ = body->GetNode();
                        move = true;
        //                URHO3D_LOGINFOF("InteractiveFrame() - HandleInput : Frame=%s Inside Interactive=%s !", framename_.CString(), selectedNode_->GetName().CString());
                    }

                    inside = true;
                }
                else if (releaseSelectionContactEnd_)
                {
                    lastSelectedNode_ = selectedNode_;
                    selectedNode_ = 0;
                    inside = false;
                    move = true;
                }
            }
            else
            {
                PODVector<RigidBody2D*> results;
                unsigned i = 0;
                GameStatics::rootScene_->GetComponent<PhysicsWorld2D>()->GetRigidBodies(results, Rect(wpos, wpos), 8);
                for (i=0; i < results.Size(); i++)
                {
                    RigidBody2D* body = results[i];
                    if (body && body->GetNode() && interactiveItems_.Contains(body->GetNode()))
                    {
                        if (selectedNode_ != body->GetNode())
                        {
                            lastSelectedNode_ = selectedNode_;
                            selectedNode_ = body->GetNode();
                            move = true;
                        }

                        inside = true;
                        break;
                    }
                }
                if (i == results.Size())
                {
                    lastSelectedNode_ = selectedNode_;
                    selectedNode_ = 0;
                    inside = false;
                    move = true;
                }
            }
        }
        else if (selectionMode_ == SELECTIONONCLICK)
        {
            if (eventType == E_MOUSEBUTTONDOWN || eventType == E_TOUCHEND || mbuttonok)
            {
                RigidBody2D* body = GameStatics::rootScene_->GetComponent<PhysicsWorld2D>()->GetRigidBody(wpos, 8);
                if (body && body->GetNode() && interactiveItems_.Contains(body->GetNode()))
                {
                    if (selectedNode_ == body->GetNode())
                        clicsOnSelection_++;
                    else
                    {
                        clicsOnSelection_ = 1;
                        lastSelectedNode_ = selectedNode_;
                        selectedNode_ = body->GetNode();
                        move = true;

                        MatchesManager::SelectAbility(selectedNode_->GetVar(GOA::BONUS).GetInt());

                        URHO3D_LOGINFOF("InteractiveFrame() - HandleInput : Frame=%s SelectionOnClick Inside Interactive=%s !", framename_.CString(), selectedNode_->GetName().CString());
                    }

                    inside = true;
                }
                else if (!body)
                {
                    clicsOnSelection_ = 0;
                    lastSelectedNode_ = selectedNode_;
                    selectedNode_ = 0;
                    inside = false;
                    move = true;

                    MatchesManager::SelectAbility(-1);

//                    URHO3D_LOGINFOF("InteractiveFrame() - HandleInput : Frame=%s SelectionOnClick Outside numclick=%d !", framename_.CString(), clicsOnSelection_);
                }
            }
        }
    }

    /// Set KeyBoard/JoyPad Moves in Menu Items
    else if (eventType == E_KEYUP || eventType == E_JOYSTICKAXISMOVE || eventType == E_JOYSTICKHATMOVE)
    {
        int direction=0;

        if (eventType == E_KEYUP)
        {
            int key = eventData[KeyUp::P_KEY].GetInt();
            if (key == KEY_UP || key == KEY_LEFT)
                direction = 1;
            else if (key == KEY_DOWN || key == KEY_RIGHT)
                direction = -1;
            move = (direction != 0);
            launch = (key == KEY_SPACE || key == KEY_RETURN);
        }

        if (eventType == E_JOYSTICKAXISMOVE)
        {
            if (abs(eventData[JoystickAxisMove::P_POSITION].GetFloat()) >= 0.9f)
            {
                direction = eventData[JoystickAxisMove::P_POSITION].GetFloat() > 0.f ? -1 : 1;
                move = true;
            }
        }

        if (eventType == E_JOYSTICKHATMOVE && eventData[JoystickHatMove::P_POSITION].GetInt())
        {
            direction = eventData[JoystickHatMove::P_POSITION].GetInt() & (HAT_DOWN | HAT_RIGHT) ? -1 : 1;
            move = true;
        }

        if (move)
        {
            inside = true;
            lastSelectedId_ = selectedId_;

            if (direction == 1 && selectedId_+1 < interactiveItems_.Size())
                selectedId_++;
            else if (direction == -1 && selectedId_-1 > 0)
                selectedId_--;
            else
                move = false;
        }

        if (move)
        {
            selectedNode_ = selectedId_ < interactiveItems_.Size() ? interactiveItems_[selectedId_] : 0;
            lastSelectedNode_ = lastSelectedId_ < interactiveItems_.Size() ? interactiveItems_[lastSelectedId_] : 0;
        }
    }

    /// Change Animations
    if (move)
    {
        if (lastSelectedNode_ && lastSelectedNode_ != selectedNode_ && lastSelectedNode_->GetComponent<AnimatedSprite2D>())
        {
            SetItemState(lastSelectedNode_, UNSELECTED);
//            URHO3D_LOGINFOF("InteractiveFrame() - HandleInput : Frame=%s unselected=%s !", framename_.CString(), lastSelectedNode_->GetName().CString());
        }

        if (selectedNode_ && selectedNode_->GetComponent<AnimatedSprite2D>())
        {
            SetItemState(selectedNode_, SELECTED);
//            URHO3D_LOGINFOF("InteractiveFrame() - HandleInput : Frame=%s selectedNode=%s animation=%s !",
//                            framename_.CString(), selectedNode_->GetName().CString(), selectedNode_->GetComponent<AnimatedSprite2D>()->GetAnimation().CString());
        }
    }

    /// Check control input for launch
    if (inside)
    {
        if (eventType == E_MOUSEBUTTONDOWN)
            launch = selectionMode_ == SELECTIONONMOVE ? (eventData[MouseButtonDown::P_BUTTON] == MOUSEB_LEFT) : clicsOnSelection_ > 1;
        else if (eventType == E_TOUCHEND)
            launch = true;
        else if (eventType == E_JOYSTICKBUTTONDOWN)
            launch = eventData[JoystickButtonDown::P_BUTTON] == 0;  // PS4controller = X
    }

    /// Launch Selected Item or quit
    if (launch && selectedNode_)
    {
        String action = selectedNode_->GetName().Substring(INTERACTIVEKEYWORD.Length());

        if (action == "OK" || action == "NOK")
        {
            bool ok = (action == "OK");
            exitindex_ = ok ? 1 : 0;
            Stop();

            URHO3D_LOGINFOF("InteractiveFrame() - HandleInput : E_MESSAGEACK !");
            eventData[MessageACK::P_OK] = ok;
            eventData[TextEntry::P_TEXT] = action;
            SendEvent(E_MESSAGEACK, eventData);
        }
        else
        {
            URHO3D_LOGINFOF("InteractiveFrame() - HandleInput : E_MESSAGEACK action=%s !", action.CString());
            eventData[MessageACK::P_OK] = true;
            eventData[TextEntry::P_TEXT] = action;
            SendEvent(E_MESSAGEACK, eventData);
        }
    }
}

void InteractiveFrame::HandleBreakInput(StringHash eventType, VariantMap& eventData)
{
    URHO3D_LOGINFOF("InteractiveFrame() - HandleBreakInput !");

    Stop();

    eventData[MessageACK::P_OK] = true;
    SendEvent(E_MESSAGEACK, eventData);
}


void InteractiveFrame::OnAbilityReseted(StringHash eventType, VariantMap& eventData)
{
    if (selectedNode_)
    {
        selectedNode_->GetComponent<AnimatedSprite2D>()->SetAnimation(String("unselected"));
        selectedNode_ = lastSelectedNode_ = 0;
        clicsOnSelection_ = 0;
    }
}

void InteractiveFrame::OnAbilityAdded(StringHash eventType, VariantMap& eventData)
{
    int power = eventData[Game_AbilityAdded::ABILITY_ID].GetInt();
    AddAbility(power, true);

    if ((power == 0 || power == 1) && Game::Get()->GetCompanion() && Game::Get()->GetCompanion()->IsNewMessage("tuto_poweruse1"))
    {
        Game::Get()->GetCompanion()->AddMessage(STATE_PLAY, true, "tuto_poweruse1", "capi", 1.f, PLAY_ANIMATION, "UI/Companion/animatedcursors.scml", "cursor_arrow_ontop", GetNode()->GetName());
        Game::Get()->GetCompanion()->AddMessage(STATE_PLAY, true, "tuto_poweruse2", "capi", 0.f, PLAY_ANIMATION, "UI/Companion/animatedcursors.scml", "cursor_arrow_ontop", GetNode()->GetName());
        Game::Get()->GetCompanion()->AddMessage(STATE_PLAY, true, "tuto_poweruse3", "capi", 0.f, PLAY_ANIMATION, "UI/Companion/animatedcursors.scml", "cursor_arrow_onbottom",
                                                MatchesManager::GetGrid().GetGridNode()->GetName());
    }
}

void InteractiveFrame::OnAbilityRemoved(StringHash eventType, VariantMap& eventData)
{
    RemoveAbility(eventData[Game_AbilityRemoved::ABILITY_ID].GetInt());
}


void InteractiveFrame::DrawDebugGeometry(DebugRenderer* debug, bool depthTest)
{
    if (debug && node_)
    {
        debug->AddNode(node_, 1.f, false);
        Rect rect;
        rect.min_ = node_->GetWorldTransform2D() * abilityMoveRect_.min_;
        rect.max_ = node_->GetWorldTransform2D() * abilityMoveRect_.max_;
        GameHelpers::DrawDebugRect(rect, debug, depthTest);
    }
}
